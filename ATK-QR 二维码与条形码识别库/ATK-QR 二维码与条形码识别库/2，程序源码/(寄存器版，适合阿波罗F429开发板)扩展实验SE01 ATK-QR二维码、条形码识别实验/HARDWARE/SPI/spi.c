#include "spi.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板 
//SPI 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/12/28
//版本：V1.1
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
//********************************************************************************
//升级说明
//V1.1 20151230
//新增对SPI2的应用代码
////////////////////////////////////////////////////////////////////////////////// 	 


//以下是SPI模块的初始化代码，配置成主机模式 						  
//SPI口初始化
//这里针是对SPI5的初始化
void SPI5_Init(void)
{	 
	u16 tempreg=0;
	RCC->AHB1ENR|=1<<5;    	//使能PORTF时钟	   
	RCC->APB2ENR|=1<<20;   	//SPI5时钟使能 
	GPIO_Set(GPIOF,7<<7,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);	//PF7~9复用功能输出	
  	GPIO_AF_Set(GPIOF,7,5);	//PF7,AF5
 	GPIO_AF_Set(GPIOF,8,5);	//PF8,AF5
 	GPIO_AF_Set(GPIOF,9,5);	//PF9,AF5 

	//这里只针对SPI口初始化
	RCC->APB2RSTR|=1<<20;	//复位SPI5
	RCC->APB2RSTR&=~(1<<20);//停止复位SPI5
	tempreg|=0<<10;			//全双工模式	
	tempreg|=1<<9;			//软件nss管理
	tempreg|=1<<8;			 
	tempreg|=1<<2;			//SPI主机  
	tempreg|=0<<11;			//8位数据格式	
	tempreg|=1<<1;			//空闲模式下SCK为1 CPOL=1 
	tempreg|=1<<0;			//数据采样从第2个时间边沿开始,CPHA=1  
 	//对SPI5属于APB2的外设.时钟频率最大为96Mhz频率.
	tempreg|=7<<3;			//Fsck=Fpclk1/256
	tempreg|=0<<7;			//MSB First  
	tempreg|=1<<6;			//SPI启动 
	SPI5->CR1=tempreg; 		//设置CR1  
	SPI5->I2SCFGR&=~(1<<11);//选择SPI模式
	SPI5_ReadWriteByte(0xff);//启动传输		 
}   
//SPI5速度设置函数
//SpeedSet:0~7
//SPI速度=fAPB2/2^(SpeedSet+1)
//fAPB2时钟一般为96Mhz
void SPI5_SetSpeed(u8 SpeedSet)
{
	SpeedSet&=0X07;			//限制范围
	SPI5->CR1&=0XFFC7; 
	SPI5->CR1|=SpeedSet<<3;	//设置SPI5速度  
	SPI5->CR1|=1<<6; 		//SPI设备使能	  
} 
//SPI5 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI5_ReadWriteByte(u8 TxData)
{		 			 
	while((SPI5->SR&1<<1)==0);		//等待发送区空 
	SPI5->DR=TxData;	 	  		//发送一个byte  
	while((SPI5->SR&1<<0)==0);		//等待接收完一个byte  
 	return SPI5->DR;          		//返回收到的数据				    
}

//以下是SPI模块的初始化代码，配置成主机模式 						  
//SPI口初始化
//这里针是对SPI2的初始化
void SPI2_Init(void)
{	 
	u16 tempreg=0;
	RCC->AHB1ENR|=1<<1;		//使能PORTB时钟 
	RCC->APB1ENR|=1<<14;   	//SPI2时钟使能 
	GPIO_Set(GPIOB,7<<13,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);	//PB13~15复用功能输出	
  	GPIO_AF_Set(GPIOB,13,5);//PB13,AF5
 	GPIO_AF_Set(GPIOB,14,5);//PB14,AF5
 	GPIO_AF_Set(GPIOB,15,5);//PB15,AF5 

	//这里只针对SPI口初始化
	RCC->APB1RSTR|=1<<14;	//复位SPI2
	RCC->APB1RSTR&=~(1<<14);//停止复位SPI2
	tempreg|=0<<10;			//全双工模式	
	tempreg|=1<<9;			//软件nss管理
	tempreg|=1<<8;			 
	tempreg|=1<<2;			//SPI主机  
	tempreg|=0<<11;			//8位数据格式	
	tempreg|=1<<1;			//空闲模式下SCK为1 CPOL=1 
	tempreg|=1<<0;			//数据采样从第2个时间边沿开始,CPHA=1  
 	//对SPI2属于APB1的外设.时钟频率最大为48Mhz频率.
	tempreg|=7<<3;			//Fsck=Fpclk1/256
	tempreg|=0<<7;			//MSB First  
	tempreg|=1<<6;			//SPI启动 
	SPI2->CR1=tempreg; 		//设置CR1  
	SPI2->I2SCFGR&=~(1<<11);//选择SPI模式
	SPI2_ReadWriteByte(0xff);//启动传输		 
}   
//SPI2速度设置函数
//SpeedSet:0~7
//SPI速度=fAPB1/2^(SpeedSet+1)
//fAPB2时钟一般为45Mhz
void SPI2_SetSpeed(u8 SpeedSet)
{
	SpeedSet&=0X07;			//限制范围
	SPI2->CR1&=0XFFC7; 
	SPI2->CR1|=SpeedSet<<3;	//设置SPI2速度  
	SPI2->CR1|=1<<6; 		//SPI设备使能	  
} 
//SPI2 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI2_ReadWriteByte(u8 TxData)
{		 			 
	while((SPI2->SR&1<<1)==0);		//等待发送区空 
	SPI2->DR=TxData;	 	  		//发送一个byte  
	while((SPI2->SR&1<<0)==0);		//等待接收完一个byte  
 	return SPI2->DR;          		//返回收到的数据				    
}






