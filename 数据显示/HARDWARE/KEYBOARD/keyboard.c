#include "keyboard.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"   	 
#include "key.h"	 	 	 	 	 
#include "lcd.h" 
#include "w25qxx.h" 	 
#include "touch.h" 	 
#include "malloc.h"
#include "string.h"    
#include "text.h"	
#include "string.h"
#include "ff.h"
#include "beep.h"
#include "pcf8574.h"


//ATK-SIM900A 各项测试(拨号测试、短信测试、GPRS测试)共用代码
extern const u8 sn[256];
extern u16 LCDout1[450];//数据输出缓冲器1
extern u16 LCDout2[450];//数据输出缓冲器2
extern u16 ADCin[4096];//数据采集缓冲器
extern const u16 ADCx[12];
extern u16 *ADC_addr;
extern u8 KEY_VALUE,CONTEST,MEASURE,TYPE;
//u8 cmode=0;	//模式
u8 callbuf[33]; 
u8 pohnenumlen=0;	//号码长度,最大15个数 
//u8 oldmode=0xFF;
extern u16 aX,bX,aY,bY,cY,cYmax,cYmin;//X为水平控制，Y为垂直控制,a为幅度控制（放大系数），b为相位控制（1：左移，0：右移）,c是幅度平均值
extern u16 aZ,bZ,aT,bT;//aZ为H加减控制，bZK加减,
extern u16 LCDout_sta,LCD_data;
extern u8 LCDnew_sta,LCDpin_sta,LCD_n,Tlock_sta,aT_sta;//LCD刷新标志，信号显示选择，信号更新选择
extern u8 LCDDCAC_sta,LCDDIS_sta,LCDone,LCD_run,TIMd; //0=DC显示，1=AC显示//单次//0=执行，1=停止
extern u8 DAC_new;
u8 key_value[6]={"0"};
u8 chr_value[6]={"0"};


//将1个字符转换为16进制数字
//chr:字符,0~9/A~F/a~F
//返回值:chr对应的16进制数值
u8 sim900a_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}
//将1个16进制数字转换为字符
//hex:16进制数字,0~15;
//返回值:字符
u8 sim900a_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}

//把hex转换成多为ASCII字符串，返回串长度
u8 hexXchr(u16 hex)
{
	u16 res;
	u8 *p;
	p=chr_value;
	if(hex>=1000)
	{
		res=hex/1000;
	  p[0]=sim900a_hex2chr(res);
		res=(hex%1000)/100;
		p[1]=sim900a_hex2chr(res);	
		res=((hex%1000)%100)/10;
		p[2]=sim900a_hex2chr(res);	
		res=((hex%1000)%100)%10;
		p[3]=sim900a_hex2chr(res);
		p[4]=0;
    return 4;		
	}
	else if(hex>=100)
	{
		res=hex/100;
		p[0]=sim900a_hex2chr(res);
		res=(hex%100)/10;
		p[1]=sim900a_hex2chr(res);
 		res=(hex%100)%10;
		p[2]=sim900a_hex2chr(res);
		p[3]=0;
    return 3;		
	}
	else if(hex>=10)
	{
		res=hex/10;
		p[0]=sim900a_hex2chr(res);
 		res=hex%10;
		p[1]=sim900a_hex2chr(res);
		p[2]=0;
		return 2;
	}
	else 
		{
		p[0]=sim900a_hex2chr(hex);
		p[1]=0;
		return 1;
		}	
}

void Keyboard_Init()
{
    
    u8 *p;
	p=callbuf;
    
    
    LCD_Clear(GRAY);                 //清屏
    POINT_COLOR=LGRAY;	  
  	LCD_DrawRectangle(20,20,100,100); //文字框
    LCD_DrawRectangle(21,21,101,101); //文字框
    
  	LCD_DrawRectangle(20,160,100,240); //文字框
    LCD_DrawRectangle(21,161,101,241); //文字框

  	LCD_DrawRectangle(120,20,200,100); //文字框
    LCD_DrawRectangle(121,21,201,101); //文字框    
    
  	LCD_DrawRectangle(120,160,200,240); //文字框
    LCD_DrawRectangle(121,161,201,241); //文字框    
    
    LCD_DrawRectangle(20,350,200,430); //数据框
    LCD_DrawRectangle(21,351,201,431); //数据框1

   
    //命令框
    LCD_DrawRectangle(20,480,200,560);
    LCD_DrawRectangle(21,481,201,561);
    //图形框
    LCD_Fill(222,22,998,578,LGRAY); //图形背景色
    
/*填字*/
    POINT_COLOR=LGRAYBLUE;   
    Show_Str(30,300,200,380,"串口接收量",32,1);	
    Show_Str(30,435,200,480,"状态",32,1);	
    Show_Str(30,40,100,115,"连接",32,1);				    	 
    Show_Str(130,40,200,115,"测量",32,1);	
    Show_Str(35,185,245,235,"EGC",32,1);	
    Show_Str(135,185,245,235,"PPG",32,1);	    

    
}

//按键状态设置
//根据键盘坐标填色和发声
//key:键值（0~32）
//sta:状态，0，松开；1，按下；
void sim900a_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	if(keyx>32)return;
	if(sta)
	{
//	在按键区填绿色;
			if(keyx==1){LCD_Fill(22,22,98,98,LGRAY);Show_Str(30,40,100,115,"连接",32,1);		}//
			else if(keyx==2){LCD_Fill(122,22,198,98,LGRAY);Show_Str(130,40,200,115,"测量",32,1);}//
            else if(keyx==3){LCD_Fill(22,162,98,238,LGRAY);Show_Str(35,185,245,235,"EGC",32,1);}//
            else if(keyx==4){LCD_Fill(122,162,198,238,LGRAY);Show_Str(135,185,245,235,"PPG",32,1);}//
//			else if(keyx==2){LCD_Fill(182,12,223,38,GREEN);Show_Str(190,11,215,25,"Auto",16,1);Show_Str(190,28,215,39,"set",16,1);}//
//			else if(keyx==3){LCD_Fill(232,12,268,38,GREEN);Show_Str(235,11,260,25,"run",16,1);Show_Str(235,28,260,39,"stop",16,1);}//
//			else if(keyx==4){;}
//			else if(keyx==5){;}
//			else if(keyx==6){;}
//			else if(keyx==7){;}
//			else if(keyx==8){LCD_Fill(82,72,123,98,GREEN);Show_Str(88,80,115,90,"频+",16,1);}//
//			else if(keyx==9){LCD_Fill(132,72,173,98,GREEN);Show_Str(140,80,170,90,"上移",16,1);}//
//			else if(keyx==10){LCD_Fill(182,72,223,98,GREEN);Show_Str(190,80,220,90,"左移",16,1);}//
//			else if(keyx==11){LCD_Fill(232,72,268,98,GREEN);Show_Str(235,80,260,90,"升高",16,1);}//
//			else if(keyx==12){LCD_Fill(83,103,122,127,GREEN);Show_Str(88,110,115,120,"频-",16,1);}//
//			else if(keyx==13){LCD_Fill(133,103,172,127,GREEN);Show_Str(140,110,170,120,"下移",16,1);}//
//			else if(keyx==14){LCD_Fill(183,103,222,127,GREEN);Show_Str(190,110,220,120,"右移",16,1);}//
//			else if(keyx==15){LCD_Fill(233,103,267,127,GREEN);Show_Str(235,110,260,120,"降低",16,1);}//
//			else if(keyx==16){LCD_Fill(83,133,122,157,GREEN);Show_Str(88,140,115,150,"幅+",16,1);}//
//			else if(keyx==17){LCD_Fill(133,133,172,157,GREEN);Show_Str(140,140,170,150,"Y移0",16,1);}//
//			else if(keyx==18){LCD_Fill(183,133,222,157,GREEN);Show_Str(190,140,220,150,"X移0",16,1);}//
//			else if(keyx==19){LCD_Fill(233,133,267,157,GREEN);Show_Str(235,140,260,150,"锁定",16,0);}//
//			else if(keyx==20){LCD_Fill(83,163,122,187,GREEN);Show_Str(88,170,115,180,"幅-",16,1);}//
//			else if(keyx==21){LCD_Fill(133,163,172,187,GREEN);Show_Str(140,170,170,180,"Y归0",16,1);}//
//			else if(keyx==22){LCD_Fill(183,163,222,187,GREEN);Show_Str(190,170,220,180,"X归0",16,1);}//
//			else if(keyx==23){LCD_Fill(233,163,267,187,GREEN);Show_Str(235,170,260,180,"方法",16,1);}//
//			else if(keyx==24){LCD_Fill(83,193,122,217,GREEN); Show_Str(88,200,115,210,"信选",16,1);}//
//			else if(keyx==25){LCD_Fill(133,193,172,217,GREEN);Show_Str(140,200,170,210,"放大",16,1);}//
//			else if(keyx==26){LCD_Fill(183,193,222,217,GREEN);Show_Str(190,200,220,210,"扩张",16,1);}//
//			else if(keyx==27){LCD_Fill(233,193,267,217,GREEN);Show_Str(235,200,260,210,"归位",16,1);}//
//			else if(keyx==28){LCD_Fill(83,223,122,247,GREEN); Show_Str(88,230,115,240,"显选",16,1);}//
//			else if(keyx==29){LCD_Fill(133,223,172,247,GREEN);Show_Str(140,230,170,240,"缩小",16,1);}//
//			else if(keyx==30){LCD_Fill(183,223,222,247,GREEN);Show_Str(190,230,220,240,"压缩",16,1);}//
//			
//			else {LCD_Fill(233,223,267,247,GREEN);Show_Str(235,230,260,240,"退格",16,1);}//
		    PCF8574_WriteBit(BEEP_IO,0);	//控制蜂鸣器		  
			delay_ms(30);//延时30ms
			LED0_Toggle;  
			PCF8574_WriteBit(BEEP_IO,1);	//控制蜂鸣器
			//消除绿色
			if(keyx==1){LCD_Fill(22,22,98,98,GRAY);Show_Str(30,40,100,115,"连接",32,1);		}//
			else if(keyx==2){LCD_Fill(122,22,198,98,GRAY);Show_Str(130,40,200,115,"测量",32,1);}//
            else if(keyx==3){LCD_Fill(22,162,98,238,GRAY);Show_Str(35,185,245,235,"EGC",32,1);}//
            else if(keyx==4){LCD_Fill(122,162,198,238,GRAY);Show_Str(135,185,245,235,"PPG",32,1);}//
//			else if(keyx==2){LCD_Fill(182,12,223,38,WHITE);Show_Str(190,11,215,25,"Auto",16,1);Show_Str(190,28,215,39,"set",16,1);}//
//			else if(keyx==3){LCD_Fill(232,12,268,38,WHITE);Show_Str(235,11,260,25,"run",16,1);Show_Str(235,28,260,39,"stop",16,1);}//
//			else if(keyx==4){;}
//			else if(keyx==5){;}
//			else if(keyx==6){;}
//			else if(keyx==7){;}
//			else if(keyx==8){LCD_Fill(82,72,123,98,WHITE);Show_Str(88,80,115,90,"频+",16,1);}//
//			else if(keyx==9){LCD_Fill(132,72,173,98,WHITE);Show_Str(140,80,170,90,"上移",16,1);}//
//			else if(keyx==10){LCD_Fill(182,72,223,98,WHITE);Show_Str(190,80,220,90,"左移",16,1);}//
//			else if(keyx==11){LCD_Fill(232,72,268,98,WHITE);Show_Str(235,80,260,90,"升高",16,1);}//
//			else if(keyx==12){LCD_Fill(83,103,122,127,WHITE);Show_Str(88,110,115,120,"频-",16,1);}//
//			else if(keyx==13){LCD_Fill(133,103,172,127,WHITE);Show_Str(140,110,170,120,"下移",16,1);}//
//			else if(keyx==14){LCD_Fill(183,103,222,127,WHITE);Show_Str(190,110,220,120,"右移",16,1);}//
//			else if(keyx==15){LCD_Fill(233,103,267,127,WHITE);Show_Str(235,110,260,120,"降低",16,1);}//
//			else if(keyx==16){LCD_Fill(83,133,122,157,WHITE);Show_Str(88,140,115,150,"幅+",16,1);}//
//			else if(keyx==17){LCD_Fill(133,133,172,157,WHITE);Show_Str(140,140,170,150,"Y移0",16,1);}//
//			else if(keyx==18){LCD_Fill(183,133,222,157,WHITE);Show_Str(190,140,220,150,"X移0",16,1);}//
//			else if(keyx==19){LCD_Fill(233,133,267,157,WHITE);Show_Str(235,140,260,150,"锁定",16,0);}//
//			else if(keyx==20){LCD_Fill(83,163,122,187,WHITE);Show_Str(88,170,115,180,"幅-",16,1);}//
//			else if(keyx==21){LCD_Fill(133,163,172,187,WHITE);Show_Str(140,170,170,180,"Y归0",16,1);}//
//			else if(keyx==22){LCD_Fill(183,163,222,187,WHITE);Show_Str(190,170,220,180,"X归0",16,1);}//
//			else if(keyx==23){LCD_Fill(233,163,267,187,WHITE);Show_Str(235,170,260,180,"方法",16,1);}//
//			else if(keyx==24){LCD_Fill(83,193,122,217,WHITE); Show_Str(88,200,115,210,"信选",16,1);}//
//			else if(keyx==25){LCD_Fill(133,193,172,217,WHITE);Show_Str(140,200,170,210,"放大",16,1);}//
//			else if(keyx==26){LCD_Fill(183,193,222,217,WHITE);Show_Str(190,200,220,210,"扩张",16,1);}//
//			else if(keyx==27){LCD_Fill(233,193,267,217,WHITE);Show_Str(235,200,260,210,"归位",16,1);}//
//			else if(keyx==28){LCD_Fill(83,223,122,247,WHITE); Show_Str(88,230,115,240,"显选",16,1);}//
//			else if(keyx==29){LCD_Fill(133,223,172,247,WHITE);Show_Str(140,230,170,240,"缩小",16,1);}//
//			else if(keyx==30){LCD_Fill(183,223,222,247,WHITE);Show_Str(190,230,220,240,"压缩",16,1);}//
//			
//			else {LCD_Fill(233,223,267,247,WHITE);Show_Str(235,230,260,240,"退格",16,1);}//
	
	}	  
		 		 
}

//得到触摸屏的输入
//x,y:键盘坐标
//返回值：按键键值（1~15有效；0,无效）
u8 Keyboard_GetValue(u16 x,u16 y)//(80,10)
{
	u16 i,j;
	static u8 key_x=0;//0,没有任何按键按下；1~15，1~15号按键按下
	u8 key=0;
	tp_dev.scan(0); //GT9147芯片		 
	if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
	{	
//		for(i=0;i<8;i++)
//		{
//			for(j=0;j<4;j++)//a<x<b&&c<y<d,有效按键，编号送入key
//			{
//			 	if(tp_dev.x[0]<(x+j*50+45)&&tp_dev.x[0]>(x+j*50)&&tp_dev.y[0]<(y+i*30+30)&&tp_dev.y[0]>(y+i*30))
//				{	
//					key=i*4+j+1;//按键编号：总数位i*j=7*4=23	 
//					break;	 		   
//				}
//			}
//			if(key)
//			{	   
//				if(key_x==key)key=0;//
//				else 
//				{
//					sim900a_key_staset(x,y,key_x-1,0);
//					key_x=key;
//					sim900a_key_staset(x,y,key_x-1,1);
//				}
//				break;
//			}
//		}  
        if(tp_dev.x[0]<95&&tp_dev.x[0]>25&&tp_dev.y[0]<95&&tp_dev.y[0]>25)
            key=1;
        else if(tp_dev.x[0]<195&&tp_dev.x[0]>125&&tp_dev.y[0]<95&&tp_dev.y[0]>25)
            key=2;
        else if(tp_dev.x[0]<95&&tp_dev.x[0]>25&&tp_dev.y[0]<235&&tp_dev.y[0]>165)
            key=3;
        else if(tp_dev.x[0]<195&&tp_dev.x[0]>125&&tp_dev.y[0]<235&&tp_dev.y[0]>165)
            key=4;
        
        if(key)
        {
			sim900a_key_staset(x,y,key,1);
            while(tp_dev.sta&TP_PRES_DOWN)
                tp_dev.scan(0); //GT9147芯片	
        }
	}
//    else if(key_x) 
//	{
//		sim900a_key_staset(x,y,key_x-1,0);
//		key_x=0;
//	} 
	return key; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//拨号测试部分代码
void key_check(void)
{
	u8 *p1,*p2,*p3,i;
	u8 res;
    static u8 key1_temp,key2_temp,key3_temp,key4_temp;
    u8 a=0,b=0x19,c=0,d=0,e=0;
    u8  f=0,g=0,h=0,m=0xca,j=0xc8;
	p1=callbuf;
	p2=chr_value;
	p3=key_value;
	KEY_VALUE=Keyboard_GetValue(80,10);//(80,10)
    
    if(KEY_VALUE==1)
    {
        key1_temp=~key1_temp;              //变量取反
        LCD_Fill(22,482,198,558,GRAY);
        if(key1_temp)
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"连接成功！",32,1);
        }
        else 
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"取消连接！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
    }
    
    else if(KEY_VALUE==2)
    {
        key2_temp=~key2_temp;
        LCD_Fill(22,482,198,558,GRAY);
        if(key2_temp)
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"选择类型！",32,1);
        }
        else
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"取消测量！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
    }
    
    else if(KEY_VALUE==3) 
    {
        key3_temp=~key3_temp;
        LCD_Fill(22,482,198,558,GRAY);
        if(key3_temp)
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"开始传输ECG！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
        else
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"停止传输ECG！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
    }
    
    else if(KEY_VALUE==4)
   {
       key4_temp=~key4_temp;
        LCD_Fill(22,482,198,558,GRAY);
        if(key4_temp)
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"开始传输PPG！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
        else
        {
            POINT_COLOR=LGRAYBLUE; 
            Show_Str(25,490,200,558,"停止传输PPG！",32,1);
            printf("%c%c%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h,m,j);
        }
    }
//		if(key) 
//		{ 
//			res=hexXchr(key);
//			for(i=0;i<res+2;i++)
//			key_value[i]=chr_value[i];
//			POINT_COLOR=BLUE;
//			LCD_Fill(15,57,15+25,57+10,WHITE); 
//			sprintf((char*)p1,"KEY=%s",p3);
//			Show_Str(15,57,70,70,p1,12,0);  
//			if(key==1){;}//help/信号刷新选择0
//				
//			else if(key==2)//single
//				{
//				LCDone=1;
//				POINT_COLOR=BLUE;
//				Show_Str(140,28,165,39,"leOK",16,0);
//				LCD_run=1;
//				Show_Str(235,28,260,39,"stop",16,0);
//				POINT_COLOR=RED;
//				Show_Str(235,11,260,25,"run",16,0);
//				}
//			else if(key==3){;}//Autoset
//			else if(key==4)//run/stop
//				{
//				POINT_COLOR=RED;
//				if(LCD_run!=0)LCD_run=0;
//					else LCD_run=1;
//				if(LCD_run==0)
//					{POINT_COLOR=BLUE;
//					Show_Str(235,11,260,25,"run",16,0);
//					POINT_COLOR=RED;
//					Show_Str(235,28,260,39,"stop",16,0);
//					POINT_COLOR=RED;
//					Show_Str(140,28,165,39,"leNO",16,0);
//					LCDone=0;//运行就没有1次
////          UpdatTrue=0;
//			//		TIM_Cmd(TIM3,ENABLE);//DMA用，重新采样
//					}
//					else  
//						{POINT_COLOR=RED;//
//						Show_Str(235,11,260,25,"run",16,0);
//						POINT_COLOR=BLUE;
//						Show_Str(235,28,260,39,"stop",16,0);
//						Show_Str(140,28,165,39,"leOK",16,0);
//						LCDone=1;//停止=单次
//						}
//				}
//			else if(key==5){;}//空
//			else if(key==6){;}//空
//			else if(key==7){;}//空
//			else if(key==8){;}//空
//			else if(key==9)//F1(频H+100)
//				{
//				POINT_COLOR=BLACK;
//				if(bZ>=1)bZ=bZ-1;
//				res=hexXchr(bZ);
//			//	TIM2_Init(bZ*50+49,0);  //DAC输出信号频率：72M/（（arr+1）（psc+1）*256）=56.25Khz					
//				LCD_Fill(90,95,90+23,95+10,WHITE); 
//				sprintf((char*)p1,"bZ=%s",p2);
//				Show_Str(90,95,110,70,p1,12,0);    //信号频率调节
//				}
//			else if(key==10)//垂直上移
//				{
//				POINT_COLOR=BLACK;
//				if(bY<=360)bY=bY+10;
//				res=hexXchr(bY);
//				LCD_Fill(140,95,140+23,95+10,WHITE); 
//				sprintf((char*)p1,"bY=%s",p2);
//				Show_Str(140,95,160,70,p1,12,0);   //垂直移位
//				}
//			else if(key==11)//水平左移
//				{
//				POINT_COLOR=BLACK;
//				if(bX<=512)bX=bX+10;
//				res=hexXchr(bX);
//				LCD_Fill(185,95,185+23,95+10,WHITE); 
//				sprintf((char*)p1,"bX=%s",p2);
//				Show_Str(185,95,210,70,p1,12,0);
//				}
//			else if(key==12)//门限升高
//			{	
//				POINT_COLOR=BLACK;
//				if(aT<=360)aT=aT+10;
//				res=hexXchr(aT);
//				LCD_Fill(232,95,232+23,95+10,WHITE); 
//				sprintf((char*)p1,"aT=%s",p2);
//				Show_Str(232,95,260,70,p1,12,0); 
//			}
//			else if(key==13)//F2(频H-100)
//				{
//				POINT_COLOR=BLACK;
//				if(bZ<=100)bZ=bZ+1;
//				res=hexXchr(bZ);
//			//	TIM2_Init(bZ*50+49,0);  //DAC输出信号频率：72M/（（arr+1）（psc+1）*256）=56.25Khz					
//				LCD_Fill(90,95,90+23,95+10,WHITE); 
//				sprintf((char*)p1,"bZ=%s",p2);
//				Show_Str(90,95,110,70,p1,12,0);    //信号频率调节
//				}//				
//			else if(key==14)//垂直下移
//				{
//				POINT_COLOR=BLACK;
//				if(bY>=10)bY=bY-10;
//				res=hexXchr(bY);
//				LCD_Fill(140,95,140+25,95+10,WHITE); 
//				sprintf((char*)p1,"bY=%s",p2);
//				Show_Str(140,95,160,70,p1,12,0);   //垂直移位
//				}//
//			else if(key==15)//水平右移
//				{
//				POINT_COLOR=BLACK;
//				if(bX>=10)bX=bX-10;
//				res=hexXchr(bX);
//				LCD_Fill(185,95,185+25,95+10,WHITE); 
//				sprintf((char*)p1,"bX=%s",p2);
//				Show_Str(185,95,210,70,p1,12,0);	
//				}//
//			else if(key==16)//aT降低
//				{
//				POINT_COLOR=BLACK;
//				if(aT>=10)aT=aT-10;
//				if(aT<=9)aT=0;
//				res=hexXchr(aT);
//				LCD_Fill(232,95,232+25,95+10,WHITE); 
//				sprintf((char*)p1,"aT=%s",p2);
//				Show_Str(232,95,260,70,p1,12,0);	
//				}//
//			else if(key==17)//F3信号幅度控制+
//				{
//				if(aZ>=2)aZ=aZ-1;
//				DAC_new=1;
//				POINT_COLOR=BLACK;
//				res=hexXchr(20-aZ);
//				LCD_Fill(90,155,90+25,155+10,WHITE); 
//				sprintf((char*)p1,"aZ=%s",p2);
//				Show_Str(90,155,110,180,p1,12,0);
//				}
//			else if(key==18)//Y移0
//				{
//				POINT_COLOR=BLACK;
//				bY=0;
//				res=hexXchr(bY);
//				LCD_Fill(140,95,140+25,95+10,WHITE); 
//				sprintf((char*)p1,"bY=%s",p2);
//				Show_Str(140,95,160,70,p1,12,0);   //垂直移位
//				}
//			else if(key==19)//X移0
//				{
//				POINT_COLOR=BLACK;
//				bX=128;
//				res=hexXchr(bX);
//				LCD_Fill(185,95,185+25,95+10,WHITE); 
//				sprintf((char*)p1,"bX=%s",p2);
//				Show_Str(185,95,210,70,p1,12,0);
//				}
//			else if(key==20)//锁定
//				{
//					Tlock_sta=!Tlock_sta;
//				POINT_COLOR=RED;
//				if(Tlock_sta==0)Show_Str(235,140,260,150,"锁定",16,0);//Show_Str(151,96+5*45,200,24,"信收",24,0);
//				  else 	Show_Str(235,140,260,150,"失锁",16,0);//Show_Str(151,96+5*45,200,24,"信发",24,0);					
//				}
//			else if(key==21)//F4信号幅度控制-
//				{
//				POINT_COLOR=BLACK;
//				if(aZ<=20)aZ=aZ+1;
//				DAC_new=1;
//				res=hexXchr(20-aZ);
//				LCD_Fill(90,155,90+25,155+10,WHITE); 
//				sprintf((char*)p1,"aZ=%s",p2);
//				Show_Str(90,155,110,180,p1,12,0);	
//				}//
//			else if(key==22)//Y归0
//				{
//				POINT_COLOR=BLACK;
//				aY=5;
//				res=hexXchr(aY);
//				LCD_Fill(140,215,140+25,215+10,WHITE); 
//				sprintf((char*)p1,"aY=%s",p2);
//				Show_Str(140,215,100,160,p1,12,0);
//				}
//			else if(key==23)//X归0
//				{
//				POINT_COLOR=BLACK;
//				aX=4;
////				TIM3_Int_Init(72*ADCx[aX]-1,0); 
//			//	TIM3_Int_Init(72*aX-1,0);	
//					res=hexXchr(aX);
//				LCD_Fill(190,215,190+25,215+10,WHITE); 
//				sprintf((char*)p1,"aX=%s",p2);
//				Show_Str(190,215,100,160,p1,12,0);
//				}
//			else if(key==24)//0=上升触发。1=下降触发
//			  {
//				aT_sta=!aT_sta;
//				POINT_COLOR=RED;
//				if(aT_sta==0)Show_Str(235,170,260,180,"上升",16,0);
//				  else 	Show_Str(235,170,260,180,"下降",16,0);
//			  }
//			else if(key==25)//信发/信收
//				{
//				LCDpin_sta=!LCDpin_sta;
//				POINT_COLOR=RED;			
//				if(LCDpin_sta==0)Show_Str(88,200,115,210,"信收",16,0);
//				  else 	Show_Str(88,200,115,210,"信发",16,0);
//				}
//			else if(key==26)//垂直放大
//				{
//				POINT_COLOR=BLACK;
//				if(aY<=9)aY=aY+1;
//					else if(10<=aY<=100)aY=aY+10;
//				res=hexXchr(aY);
//				LCD_Fill(140,215,140+25,215+10,WHITE); 
//				sprintf((char*)p1,"aY=%s",p2);
//				Show_Str(140,215,100,160,p1,12,0);
//				}
//			else if(key==27)//水平扩张
//				{
//				POINT_COLOR=BLACK;
//				if(aX>=2)aX=aX-1;
////				TIM3_Int_Init(72*ADCx[aX]-1,0);		
//	//			TIM3_Int_Init(72*aX-1,0);						
//				res=hexXchr(aX);
//				LCD_Fill(190,215,190+25,215+10,WHITE); 
//				sprintf((char*)p1,"aX=%s",p2);
//				Show_Str(190,215,100,160,p1,12,0);
//				}
//			else if(key==28)//显示或加粗显示
//				{
//				LCDDIS_sta=!LCDDIS_sta;
//				POINT_COLOR=RED;
//				if(LCDDIS_sta==0)Show_Str(235,200,260,210,"显示",16,0);
//				else Show_Str(235,200,260,210,"粗显",16,0);
//				}
//			else if(key==29)//F6清屏幕
//				{
//				LCD_Fill(17,422,463,778,YELLOW); 
//				LCD_n=!LCD_n;
//				POINT_COLOR=RED;
//				if(LCD_n==0)Show_Str(88,230,115,240,"刷新",16,0); 
//			    else  Show_Str(88,230,115,240,"保留",16,0); 
//				}
//				else if(key==30)//垂直缩小
//				{
//				POINT_COLOR=BLACK;
//				if(2<=aY<=10)aY=aY-1;
//					else if(10<=aY<=100)aY=aY-10;
//				res=hexXchr(aY);
//				LCD_Fill(140,215,140+25,215+10,WHITE); 
//				sprintf((char*)p1,"aY=%s",p2);
//				Show_Str(140,215,100,160,p1,12,0);
//				}//
//			else if(key==31)//水平压缩
//				{
//				POINT_COLOR=BLACK;
//				if(aX<=11)aX=aX+1; 
////				TIM3_Int_Init(72*ADCx[aX]-1,0);
//		//		TIM3_Int_Init(72*aX-1,0);						
//				res=hexXchr(aX);
//				LCD_Fill(190,215,190+25,215+10,WHITE); 
//				sprintf((char*)p1,"aX=%s",p2);
//				Show_Str(190,215,100,160,p1,12,0);	
//				}//			
//			else//直流交流显示切换
//				{
//					LCDDCAC_sta=!LCDDCAC_sta;
//					POINT_COLOR=RED;
//					if(LCDDCAC_sta==0)Show_Str(235,230,260,240,"DC显",16,0);
//					else Show_Str(235,230,260,240,"AC显",16,0);
//				} 
//		}
}