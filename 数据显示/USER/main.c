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
const u16 ADCx[12]={1,2,5,10,20,50,100,200,500,1000,2000,5000};//ʱ�����걶�ʣ�us��
u16 DACin[256];
u8 KEY_VALUE,CONTEST,MEASURE,TYPE;

int main(void)
{
    u8 key,mode1,mode2;
	u8 *p;
    u8 SendBuff1[200],SendBuff2[200]; //�������ݻ�����
	u16 aK,bK,cK,dK,eK=0,i,j,temp;
     u16 t,k,cY1=0,cY2=0;
     float freq=0,cycle=0,phase=0,scope=0,freqout=0,scopeout=0;//����
	 p=callbuf;	
    u32 ECG_Value1,ECG_Value2;
    
    Cache_Enable();                 //��L1-Cache
    MPU_Memory_Protection();        //������ش洢����
    HAL_Init();				        //��ʼ��HAL��
    Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
    delay_init(216);                //��ʱ��ʼ��
	uart_init(115200);		        //���ڳ�ʼ��
    LED_Init();                     //��ʼ��LED
    KEY_Init();                     //��ʼ������
    SDRAM_Init();                   //��ʼ��SDRAM
    LCD_Init();                     //��ʼ��LCD
    Beep_Init();                    //��������ʼ��
    MYDMA_Config(DMA2_Stream5,DMA_CHANNEL_4);//��ʼ��DMA
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
    HAL_UART_Receive_DMA(&UART1_Handler,SendBuff1,200);//����DMA����
    while(1)
    {
      //  key_check();  
         //ʹ������1��������
         if(__HAL_DMA_GET_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5))//�ȴ�DMA2_Steam7�������
        {
            __HAL_DMA_CLEAR_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5);//���DMA2_Steam7������ɱ�־
            HAL_UART_DMAStop(&UART1_Handler);      //��������Ժ�رմ���DMA
            HAL_UART_Receive_DMA(&UART1_Handler,SendBuff2,200);//����DMA����
        }
        //��������
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
        //ʹ������2��������
       if(__HAL_DMA_GET_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5))//�ȴ�DMA2_Steam7�������
        {
            __HAL_DMA_CLEAR_FLAG(&UART1RxDMA_Handler,DMA_FLAG_TCIF1_5);//���DMA2_Steam7������ɱ�־
            HAL_UART_DMAStop(&UART1_Handler);      //��������Ժ�رմ���DMA
            HAL_UART_Receive_DMA(&UART1_Handler,SendBuff1,200);//����DMA����
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


