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
#include "timer.h"
/************************************************
 ALIENTEK ������STM32F7������ ʵ��43
 ������ʾʵ��-HAL�⺯����
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/
//extern u16 dacval;
extern const u8 Ref_Buffer[2048];
extern const uint16_t Sine12bit2[256];
extern u8 callbuf[33]; 
u8 SendBuff[4500];//�������ݻ�����
u16 LCDout1[450];//�������������1
u16 LCDout2[450];//�������������2
u16 ADCin[4096];//���ݲɼ�������
u16 ADCin1[2048];//���ݲɼ�������
u16 *ADC_addr=ADCin;
u16 aX=4,bX=128,aY=5,bY=0,cY=180,cYmax=359,cYmin=0;//XΪˮƽ���ƣ�YΪ��ֱ����,aΪ���ȿ��ƣ��Ŵ�ϵ������bΪ��λ���ƣ�1�����ƣ�0�����ƣ�,c�Ƿ���ƽ��ֵ
u16 aZ=1,bZ=10,aT=180,bT=1;//aZΪH�Ӽ����ƣ�bZK�Ӽ�,aT������ƽ
u16 LCDout_sta=0,LCD_data=0;
u8 LCDnew_sta=0,LCDpin_sta=0,LCD_n=0,Tlock_sta=0,aT_sta=0;//LCDˢ�±�־���ź���ʾѡ���źŸ���ѡ��
u8 LCDDCAC_sta=0,LCDDIS_sta=0,LCDone=0,LCD_run=0; //0=DC��ʾ��1=AC��ʾ//����//0=ִ�У�1=ֹͣ
u8 time_1s_bz=0,TIMd=0,Volt=3,Ymax=36,Ymid=18,Ymin=0;
u8 UpdatTrue,DAC_new=1;
u8 Show_Flag;
const u16 ADCx[12]={1,2,5,10,20,50,100,200,500,1000,2000,5000};//ʱ�����걶�ʣ�us��
u16 DACin[256];
u8 KEY_VALUE,CONTEST,MEASURE,TYPE;

int main(void)
{
    u8 key,mode1,mode2;
	u8 *p;
	u16 aK,bK,cK,dK,eK=0,i,j,temp,xPoint=223;
     u16 t,k,cY1=0,cY2=0;
    u32 ECG_Value1,ECG_Value2;
    int ECG_Show1,ECG_Show2;
     float freq=0,cycle=0,phase=0,scope=0,freqout=0,scopeout=0;//����
	 p=callbuf;	
    
    Cache_Enable();                 //��L1-Cache
    MPU_Memory_Protection();        //������ش洢����
    HAL_Init();				        //��ʼ��HAL��
    Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
    delay_init(216);                //��ʱ��ʼ��
	uart_init(115200);		        //���ڳ�ʼ��
    HAL_UART_Receive_IT(&UART1_Handler, (u8*)SendBuff, 400);
    LED_Init();                     //��ʼ��LED
    KEY_Init();                     //��ʼ������
    SDRAM_Init();                   //��ʼ��SDRAM
    LCD_Init();                     //��ʼ��LCD
    Beep_Init();                    //��������ʼ��
    TIM3_Init(500-1,10800-1);      //��ʱ��3��ʼ������ʱ��ʱ��Ϊ108M����Ƶϵ��Ϊ10800-1��100ms
	W25QXX_Init();				    //��ʼ��W25Q256
    tp_dev.init();				    //��������ʼ�� 
    my_mem_init(SRAMIN);            //��ʼ���ڲ��ڴ��
    my_mem_init(SRAMEX);            //��ʼ���ⲿSDRAM�ڴ��
    my_mem_init(SRAMDTCM);          //��ʼ���ڲ�DTCM�ڴ��
    exfuns_init();		            //Ϊfatfs��ر��������ڴ�  
    f_mount(fs[0],"0:",1);          //����SD�� 
  	f_mount(fs[1],"1:",1);          //����SPI FLASH.
    f_mount(fs[2],"2:",1);          //����NAND FLASH
	while(font_init()) 		        //����ֿ�
	{
  UPD:    
		LCD_Clear(WHITE);		   	//����
		POINT_COLOR=RED;			//��������Ϊ��ɫ	   	   	  
		LCD_ShowString(30,50,200,16,16,"Apollo STM32F4/F7");
		while(SD_Init())			//���SD��
		{
		  	LCD_ShowString(30,70,200,16,16,"SD Card Failed!");
		  	delay_ms(200);
		  	LCD_Fill(30,70,200+30,70+16,WHITE);
		   	delay_ms(200);		    
		 }								 						    
		LCD_ShowString(30,70,200,16,16,"SD Card OK");
		LCD_ShowString(30,90,200,16,16,"Font Updating...");
		key=update_font(20,110,16,"0:");//�����ֿ�
		while(key)//����ʧ��		
		{		
			 LCD_ShowString(30,110,200,16,16,"Font Update Failed!");
			 delay_ms(200);
			 LCD_Fill(20,110,200+20,110+16,WHITE);
			 delay_ms(200);		       
		} 		  
		LCD_ShowString(30,110,200,16,16,"Font Update Success!   ");
		delay_ms(1500);	
		LCD_Clear(WHITE);//����	       
	} 
	Keyboard_Init();
    HAL_UART_Receive_IT(&UART1_Handler,(u8*)SendBuff,4000);//����DMA����
    while(1)
    {
      //  key_check();  
         //ʹ������1��������
        //��������
        if(Show_Flag == 1)
        {
            while(j<4000)
            {
                 j++;
                if(SendBuff[j]==0x11)
                {
                        temp=j;
                        ECG_Value1=(u32)(SendBuff[temp+1]<<16) | (u32)(SendBuff[temp+2]<<8) | (u32)(SendBuff[temp+3]);
                        ECG_Show1=(u8)(ECG_Value1/5000);
                        ECG_Value2=(u32)(SendBuff[temp+4]<<16) | (u32)(SendBuff[temp+5]<<8) | (u32)(SendBuff[temp+6]);
                        ECG_Show2=(u8)(ECG_Value2/5000);                    
                        POINT_COLOR=LGRAY;
                        LCD_DrawLine(xPoint,23,xPoint,577);
                        POINT_COLOR=BLACK;
                        LCD_DrawPoint(xPoint,ECG_Show1);     
                        xPoint++;  
                        if(xPoint==1000) xPoint=223;                    
                        POINT_COLOR=LGRAY;
                        LCD_DrawLine(xPoint,23,xPoint,577);
                        POINT_COLOR=BLACK;
                        LCD_DrawPoint(xPoint,ECG_Show2);
                        xPoint++;  
                        if(xPoint==1000) xPoint=223; 
                }
            }
            j=0;
            Show_Flag = 0;
//            __HAL_USART_ENABLE_IT(&UART1_Handler, USART_IT_TC); 
        }
        


    }
}


