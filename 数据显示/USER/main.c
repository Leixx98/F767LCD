#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "w25qxx.h"
#include "nand.h"  
#include "mpu.h"
#include "sdmmc_sdcard.h"
#include "malloc.h"
#include "ff.h"
#include "exfuns.h"
#include "fontupd.h"
#include "text.h"
#include "string.h"
#include "touch.h"
#include "keyboard.h"
#include "beep.h"
#include "pcf8574.h"
#include "dma.h"
/************************************************
 ALIENTEK 阿波罗STM32F7开发板 实验43
 汉字显示实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/
//extern u16 dacval;
extern const u8 Ref_Buffer[2048];
extern const uint16_t Sine12bit2[256];
extern u8 callbuf[33]; 
u16 LCDout1[450];//数据输出缓冲器1
u16 LCDout2[450];//数据输出缓冲器2
u16 ADCin[4096];//数据采集缓冲器
u16 ADCin1[2048];//数据采集缓冲器
u16 *ADC_addr=ADCin;
u16 aX=4,bX=128,aY=5,bY=0,cY=180,cYmax=359,cYmin=0;//X为水平控制，Y为垂直控制,a为幅度控制（放大系数），b为相位控制（1：左移，0：右移）,c是幅度平均值
u16 aZ=1,bZ=10,aT=180,bT=1;//aZ为H加减控制，bZK加减,aT触发电平
u16 LCDout_sta=0,LCD_data=0;
u8 LCDnew_sta=0,LCDpin_sta=0,LCD_n=0,Tlock_sta=0,aT_sta=0;//LCD刷新标志，信号显示选择，信号更新选择
u8 LCDDCAC_sta=0,LCDDIS_sta=0,LCDone=0,LCD_run=0; //0=DC显示，1=AC显示//单次//0=执行，1=停止
u8 time_1s_bz=0,TIMd=0,Volt=3,Ymax=36,Ymid=18,Ymin=0;
u8 UpdatTrue,DAC_new=1;
const u16 ADCx[12]={1,2,5,10,20,50,100,200,500,1000,2000,5000};//时间坐标倍率（us）
u16 DACin[256];
u8 KEY_VALUE,CONTEST,MEASURE,TYPE;

int main(void)
{
    u8 key,mode1,mode2;
	u8 *p;
    u8 SendBuff1[200],SendBuff2[200]; //发送数据缓冲区
	u16 aK,bK,cK,dK,eK=0,i,j,temp;
     u16 t,k,cY1=0,cY2=0;
     float freq=0,cycle=0,phase=0,scope=0,freqout=0,scopeout=0;//参数
	 p=callbuf;	
    u32 ECG_Value1,ECG_Value2;
    
    Cache_Enable();                 //打开L1-Cache
    MPU_Memory_Protection();        //保护相关存储区域
    HAL_Init();				        //初始化HAL库
    Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
    delay_init(216);                //延时初始化
	uart_init(115200);		        //串口初始化
    LED_Init();                     //初始化LED
    KEY_Init();                     //初始化按键
    SDRAM_Init();                   //初始化SDRAM
    LCD_Init();                     //初始化LCD
    Beep_Init();                    //蜂鸣器初始化
    MYDMA_Config(DMA2_Stream5,DMA_CHANNEL_4);//初始化DMA
	W25QXX_Init();				    //初始化W25Q256
    tp_dev.init();				    //触摸屏初始化 
    my_mem_init(SRAMIN);            //初始化内部内存池
    my_mem_init(SRAMEX);            //初始化外部SDRAM内存池
    my_mem_init(SRAMDTCM);          //初始化内部DTCM内存池
    exfuns_init();		            //为fatfs相关变量申请内存  
    f_mount(fs[0],"0:",1);          //挂载SD卡 
  	f_mount(fs[1],"1:",1);          //挂载SPI FLASH.
    f_mount(fs[2],"2:",1);          //挂在NAND FLASH
	while(font_init()) 		        //检查字库
	{
  UPD:    
		LCD_Clear(WHITE);		   	//清屏
		POINT_COLOR=RED;			//设置字体为红色	   	   	  
		LCD_ShowString(30,50,200,16,16,"Apollo STM32F4/F7");
		while(SD_Init())			//检测SD卡
		{
		  	LCD_ShowString(30,70,200,16,16,"SD Card Failed!");
		  	delay_ms(200);
		  	LCD_Fill(30,70,200+30,70+16,WHITE);
		   	delay_ms(200);		    
		 }								 						    
		LCD_ShowString(30,70,200,16,16,"SD Card OK");
		LCD_ShowString(30,90,200,16,16,"Font Updating...");
		key=update_font(20,110,16,"0:");//更新字库
		while(key)//更新失败		
		{		
			 LCD_ShowString(30,110,200,16,16,"Font Update Failed!");
			 delay_ms(200);
			 LCD_Fill(20,110,200+20,110+16,WHITE);
			 delay_ms(200);		       
		} 		  
		LCD_ShowString(30,110,200,16,16,"Font Update Success!   ");
		delay_ms(1500);	
		LCD_Clear(WHITE);//清屏	       
	} 
	Keyboard_Init();
    HAL_UART_Receive_DMA(&UART1_Handler,SendBuff1,200);//开启DMA传输
    while(1)
    {
      //  key_check();  
         //使用数组1接收数据
         if(__HAL_DMA_GET_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5))//等待DMA2_Steam7传输完成
        {
            __HAL_DMA_CLEAR_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5);//清除DMA2_Steam7传输完成标志
            HAL_UART_DMAStop(&UART1_Handler);      //传输完成以后关闭串口DMA
            HAL_UART_Receive_DMA(&UART1_Handler,SendBuff2,200);//开启DMA传输
        }
        //处理数据
        while(j<200)
        {
            if(SendBuff1[j++]==0x11)
            {
                temp=j;
                if(SendBuff1[temp+7]==0x01)
                {
                    ECG_Value1=(u32)(SendBuff1[temp+1]<<16) | (u32)(SendBuff1[temp+2]<<8) | (u32)(SendBuff1[temp+3]);
                    ECG_Value2=(u32)(SendBuff1[temp+4]<<16) | (u32)(SendBuff1[temp+5]<<8) | (u32)(SendBuff1[temp+6]);
                }
            }
        }
        j=0;
        //使用数组2接收数据
       if(__HAL_DMA_GET_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5))//等待DMA2_Steam7传输完成
        {
            __HAL_DMA_CLEAR_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5);//清除DMA2_Steam7传输完成标志
            HAL_UART_DMAStop(&UART1_Handler);      //传输完成以后关闭串口DMA
            HAL_UART_Receive_DMA(&UART1_Handler,SendBuff1,200);//开启DMA传输
        }
        
        while(j<200)
        {
            if(SendBuff2[j++]==0x11)
            {
                temp=j;
                if(SendBuff2[temp+7]==0x01)
                {
                    ECG_Value1=(u32)(SendBuff2[temp+1]<<16) | (u32)(SendBuff2[temp+2]<<8) | (u32)(SendBuff2[temp+3]);
                    ECG_Value2=(u32)(SendBuff2[temp+4]<<16) | (u32)(SendBuff2[temp+5]<<8) | (u32)(SendBuff2[temp+6]);
                }
            }
        }
    }
}


