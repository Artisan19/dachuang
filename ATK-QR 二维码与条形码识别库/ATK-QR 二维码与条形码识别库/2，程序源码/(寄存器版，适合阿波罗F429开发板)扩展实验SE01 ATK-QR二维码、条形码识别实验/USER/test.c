#include "sys.h"
#include "delay.h" 
#include "led.h"  
#include "usart.h" 
#include "lcd.h" 
#include "ltdc.h"   
#include "sdram.h"    
#include "key.h" 
#include "malloc.h" 
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"  
#include "text.h"	   
#include "ov5640.h" 
#include "dcmi.h"  
#include "pcf8574.h"
#include "atk_qrdecode.h"
//ALIENTEK 阿波罗STM32F429开发板 扩展实验SE01
//二维码/条形码识别实验 - 寄存器版
//技术支持：www.openedv.com
//广州市星翼电子科技有限公司 

u16 qr_image_width;						//输入识别图像的宽度（长度=宽度）
u8 	readok=0;							//采集完一帧数据标识
u32 *dcmi_line_buf[2];					//摄像头采用一行一行读取,定义行缓存  
u16 *rgb_data_buf;						//RGB565帧缓存buf 
u16 dcmi_curline=0;						//摄像头输出数据,当前行编号						

//摄像头数据DMA接收完成中断回调函数
void qr_dcmi_rx_callback(void)
{  
	u32 *pbuf;
	u16 i;
	pbuf=(u32*)(rgb_data_buf+dcmi_curline*qr_image_width);//将rgb_data_buf地址偏移赋值给pbuf
	
	if(DMA2_Stream1->CR&(1<<19))//DMA使用buf1,读取buf0
	{ 
		for(i=0;i<qr_image_width/2;i++)
		{
			pbuf[i]=dcmi_line_buf[0][i];
		} 
	}else 										//DMA使用buf0,读取buf1
	{
		for(i=0;i<qr_image_width/2;i++)
		{
			pbuf[i]=dcmi_line_buf[1][i];
		} 
	} 
	dcmi_curline++;
}

//显示图像
void qr_show_image(u16 xoff,u16 yoff,u16 width,u16 height,u16 *imagebuf)
{
	u16 linecnt=yoff;
	
	if(lcdltdc.pwidth!=0)//RGB屏
	{
		for(linecnt=0;linecnt<height;linecnt++)
		{
			LTDC_Color_Fill(xoff,linecnt+yoff,xoff+width-1,linecnt+yoff,imagebuf+linecnt*width);//RGB屏,DM2D填充 
		}
		
	}else LCD_Color_Fill(xoff,yoff,xoff+width-1,yoff+height-1,imagebuf);	//MCU屏,直接显示
}

//imagewidth:<=240;大于240时,是240的整数倍
//imagebuf:RGB图像数据缓冲区
void qr_decode(u16 imagewidth,u16 *imagebuf)
{
	static u8 bartype=0; 
	u8 *bmp;
	u8 *result=NULL;
	u16 Color;
	u16 i,j;	
	u16 qr_img_width=0;						//输入识别器的图像宽度,最大不超过240!
	u8 qr_img_scale=0;						//压缩比例因子
	
	if(imagewidth>240)
	{
		if(imagewidth%240)return ;	//不是240的倍数,直接退出
		qr_img_width=240;
		qr_img_scale=imagewidth/qr_img_width;
	}else
	{
		qr_img_width=imagewidth;
		qr_img_scale=1;
	}  
	result=mymalloc(SRAMIN,1536);//申请识别结果存放内存
	bmp=mymalloc(SRAMCCM,qr_img_width*qr_img_width);//CCM管理内存为60K，这里最大可申请240*240=56K 
	mymemset(bmp,0,qr_img_width*qr_img_width);
	if(lcdltdc.pwidth==0)//MCU屏,无需镜像
	{ 
		for(i=0;i<qr_img_width;i++)		
		{
			for(j=0;j<qr_img_width;j++)		//将RGB565图片转成灰度
			{	
				Color=*(imagebuf+((i*imagewidth)+j)*qr_img_scale); //按照qr_img_scale压缩成240*240
				*(bmp+i*qr_img_width+j)=(((Color&0xF800)>> 8)*76+((Color&0x7E0)>>3)*150+((Color&0x001F)<<3)*30)>>8;
			}		
		}
	}else	//RGB屏,需要镜像
	{
		for(i=0;i<qr_img_width;i++)		
		{
			for(j=0;j<qr_img_width;j++)		//将RGB565图片转成灰度
			{	
				Color=*(imagebuf+((i*imagewidth)+qr_img_width-j-1)*qr_img_scale);//按照qr_img_scale压缩成240*240
				*(bmp+i*qr_img_width+j)=(((Color&0xF800)>> 8)*76+((Color&0x7E0)>>3)*150+((Color&0x001F)<<3)*30)>>8;
			}		
		}		
	}
	
	atk_qr_decode(qr_img_width,qr_img_width,bmp,bartype,result);//识别灰度图片（注意：单次耗时约0.2S）
	
	if(result[0]==0)//没有识别出来
	{
		bartype++;
		if(bartype>=5)bartype=0; 
	}
	else if(result[0]!=0)//识别出来了，显示结果
	{	
		PCF8574_WriteBit(BEEP_IO,0);//打开蜂鸣器
		delay_ms(100);
		PCF8574_WriteBit(BEEP_IO,1);
		POINT_COLOR=BLUE; 
		LCD_Fill(0,(lcddev.height+qr_image_width)/2+20,lcddev.width,lcddev.height,BLACK);
		Show_Str(0,(lcddev.height+qr_image_width)/2+20,lcddev.width,
								(lcddev.height-qr_image_width)/2-20,(u8*)result,16,0							
						);//LCD显示识别结果
		printf("\r\nresult:\r\n%s\r\n",result);//串口打印识别结果 		
	}
	myfree(SRAMCCM,bmp);		//释放灰度图bmp内存
	myfree(SRAMIN,result);	//释放识别结果	
}  

int main(void)
{     
	float fac;							 
 	u8 key;						   
	u8 i;	
	
 	Stm32_Clock_Init(360,25,2,8);	//设置时钟,180Mhz
	delay_init(180);			//初始化延时函数 
	uart_init(90,115200);	//初始化串口波特率为115200 
  LED_Init();						//初始化与LED连接的硬件接口
	SDRAM_Init();					//初始化SDRAM 
	LCD_Init();						//初始化LCD
	KEY_Init();						//初始化按键
	PCF8574_Init();				//初始化PCF8574
	OV5640_Init();				//初始化OV5640 
	W25QXX_Init();				//初始化W25Q256
 	my_mem_init(SRAMIN);	//初始化内部内存池
	my_mem_init(SRAMEX);	//初始化外部内存池
	my_mem_init(SRAMCCM);	//初始化CCM内存池    
	POINT_COLOR=RED; 
	LCD_Clear(BLACK); 	
	while(font_init()) 		//检查字库
	{	    
		LCD_ShowString(30,50,200,16,16,(u8*)"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//清除显示	     
		delay_ms(200);				  
	}  	 
 	Show_Str_Mid(0,0,(u8*)"阿波罗STM32F4/F7开发板",16,lcddev.width);	 			    	 
	Show_Str_Mid(0,20,(u8*)"二维码/条形码识别实验",16,lcddev.width);	
	while(OV5640_Init())//初始化OV5640
	{
		Show_Str(30,190,240,16,(u8*)"OV5640 错误!",16,0);
		delay_ms(200);
	    LCD_Fill(30,190,239,206,WHITE);
		delay_ms(200);
	}	
	//自动对焦初始化
	OV5640_RGB565_Mode();		//RGB565模式 
	OV5640_Focus_Init(); 
	OV5640_Light_Mode(0);		//自动模式
	OV5640_Color_Saturation(3);	//色彩饱和度0
	OV5640_Brightness(4);		//亮度0
	OV5640_Contrast(3);			//对比度0
	OV5640_Sharpness(33);		//自动锐度
	OV5640_Focus_Constant();//启动持续对焦
	DCMI_Init();						//DCMI配置 

	qr_image_width=lcddev.width;
	if(qr_image_width>480)qr_image_width=480;//这里qr_image_width设置为240的倍数
	if(qr_image_width==320)qr_image_width=240;
	Show_Str(0,(lcddev.height+qr_image_width)/2+4,240,16,(u8*)"识别结果：",16,1);
	
	dcmi_line_buf[0]=mymalloc(SRAMIN,qr_image_width*2);						//为行缓存接收申请内存	
	dcmi_line_buf[1]=mymalloc(SRAMIN,qr_image_width*2);						//为行缓存接收申请内存
	rgb_data_buf=mymalloc(SRAMEX,qr_image_width*qr_image_width*2);//为rgb帧缓存申请内存
	
	dcmi_rx_callback=qr_dcmi_rx_callback;//DMA数据接收中断回调函数
	DCMI_DMA_Init((u32)dcmi_line_buf[0],(u32)dcmi_line_buf[1],qr_image_width/2,1,1);//DCMI DMA配置  
 
	fac=800/qr_image_width;	//得到比例因子
	OV5640_OutSize_Set((1280-fac*qr_image_width)/2,(800-fac*qr_image_width)/2,qr_image_width,qr_image_width); 
 	DCMI_Start(); 					//启动传输	 
	 
	printf("SRAM IN:%d\r\n",my_mem_perused(SRAMIN));
	printf("SRAM EX:%d\r\n",my_mem_perused(SRAMEX));
	printf("SRAM CCM:%d\r\n",my_mem_perused(SRAMCCM)); 
	
	atk_qr_init();//为算法申请内存
	
	printf("1SRAM IN:%d\r\n",my_mem_perused(SRAMIN));
	printf("1SRAM EX:%d\r\n",my_mem_perused(SRAMEX));
	printf("1SRAM CCM:%d\r\n",my_mem_perused(SRAMCCM)); 
	 while(1)
	{	
		key=KEY_Scan(0);//不支持连按
		if(key)
		{ 
			OV5640_Focus_Single();  //按KEY0、KEY1、KEYUP手动单次自动对焦
			if(key==KEY2_PRES)break;//按KEY2结束识别
		} 
		if(readok==1)			//采集到了一帧图像
		{		
			readok=0;
			qr_show_image((lcddev.width-qr_image_width)/2,(lcddev.height-qr_image_width)/2,qr_image_width,qr_image_width,rgb_data_buf);
			qr_decode(qr_image_width,rgb_data_buf);
		}
		i++;
		if(i==20)//DS0闪烁.
		{
			i=0;
			LED0=!LED0;
 		}
	}
	atk_qr_destroy();//释放算法内存
	printf("3SRAM IN:%d\r\n",my_mem_perused(SRAMIN));
	printf("3SRAM EX:%d\r\n",my_mem_perused(SRAMEX));
	printf("3SRAM CCM:%d\r\n",my_mem_perused(SRAMCCM)); 
	while(1)
	{
		LED0=!LED0;
		delay_ms(200);
	}
}






