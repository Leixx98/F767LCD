#ifndef __SIM900A_H__
#define __SIM900A_H__	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	   
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F103������ 
//ATK-SIM900A GSM/GPRSģ������	  
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/4/12
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//******************************************************************************** 
//��
/////////////////////////////////////////////////////////////////////////////////// 	
  
#define swap16(x) (x&0XFF)<<8|(x&0XFF00)>>8		//�ߵ��ֽڽ����궨��

//void RTC_LCD(void);       //ʵʱʱ����ʾ

u8 sim900a_chr2hex(u8 chr);
u8 sim900a_hex2chr(u8 hex);
void Keyboard_Init(void);
u8 Keyboard_GetValue(u16 x,u16 y);
u8 hexXchr(u16 hex);
void key_check(void);
#endif

