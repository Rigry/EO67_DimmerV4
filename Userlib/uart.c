/*
 * ATMEGA8UART.c
 *
 * Created: 16.02.2017 15:19:49
 *  Author: dvk
 */ 

#include "uart.h"
//#include <avr/delay.h>

#define GO_TRANSMIT SetBit(RTS_PORT,RTS_PIN);	//переход в режим передачи
#define BACK_RECIEVE ClearBit(RTS_PORT,RTS_PIN);	//переход в режим приёма

void UARTInit(struct UARTSettingsStruc Settings)
{	
	uint32_t i;
	i = F_CPU / 16 / Settings.baudrate - 1;
	UBRRL = i;
	UBRRH = i>>8;
	uint8_t temp = 0;
	SetBit(temp,URSEL);
	if (Settings.CharacterSise == 6) {
		SetBit(temp,UCSZ0);
	}	
	if (Settings.CharacterSise == 7) {
		SetBit(temp,UCSZ1);
	}		
	if (Settings.CharacterSise == 8) {
		SetBit(temp,UCSZ0);
		SetBit(temp,UCSZ1);	
	}
	if (Settings.CharacterSise == 9) {
		SetBit(temp,UCSZ0);
		SetBit(temp,UCSZ1);
		SetBit(UCSRB,UCSZ2);
	}
	if (Settings.EvenParity == even) {
		SetBit(temp,UPM1);
	} else if (Settings.EvenParity == odd) {
		SetBit(temp,UPM0);
		SetBit(temp,UPM1);
	}
	if (Settings.StopBits == 2)	{
		SetBit(temp,USBS);
	}		
	UCSRC=temp;
	SetBit(UCSRB,TXEN);		//разрешение на отправку
	SetBit(UCSRB,RXEN);		//разрешение на приём
//	SetBit(UCSRB,TXCIE);	//разрешение прерывания при отправке
	SetBit(UCSRB,RXCIE);	//разрешение прерывания на прием
	SetBit(RTS_DDR,RTS_PIN);	//на выход
	
	//настройки таймера 2 на признак конца пакета модбас
	//3,5 слова. Слово это 10 бит => 35/baudrate (секунд)
	//35/9600=3645мкс
	//35/76800=455мкс
	//регистр считает до 255
	//на самой маленькой скорости он должен выдать близкое к этому число
	//3645мкс*16Мгц/255=228, делитель должен быть больше (ближайший делитель 256)
	uint32_t iTmp;
	iTmp = 35 * F_CPU / Settings.baudrate / 255;	//38400 - 57
	if (iTmp > 256){
		SetBit(TCCR2,CS20);
		SetBit(TCCR2,CS21);
		SetBit(TCCR2,CS22);	//делитель 1024	
		iTmp = 1024;	
	} else if (iTmp > 128) {
		SetBit(TCCR2,CS21);
		SetBit(TCCR2,CS22);	//делитель 256	(случай 9600 16М)
		iTmp = 256;		
	} else if (iTmp > 64) {
		SetBit(TCCR2,CS20);
		SetBit(TCCR2,CS22);	//делитель 128
		iTmp = 128;				
	} else if (iTmp > 32) {
		SetBit(TCCR2,CS22);	//делитель 64	//38400
		iTmp = 64;
	} else if (iTmp > 8) {
		SetBit(TCCR2,CS20);
		SetBit(TCCR2,CS21);	//делитель 32	(случай 76800 16М)
		iTmp = 32;	
	} else {
		SetBit(TCCR2,CS21);	//делитель 8
		iTmp = 8;
	}
	//расчет регистра сравнения
	//35*16М/9600/256+1=228	
	//35*16М/115200/32+1=152
	iTmp= 35 * F_CPU / Settings.baudrate / iTmp + 1;		//38400 - 228	912мкс

	OCR2 = iTmp;				//регистр сравнения 
//	SetBit(TIMSK,OCIE2);	//разрешение прерывания по сравнению (прерывание разрешается в других функциях)
	return;
}//void UARTInit(struct UARTSettingsStruc Settings){	
	
void UARTRec(struct UartBufSt *UartBuf, uint8_t UDRbuf)
{	//прием
	TCNT2 = 0;	//сброс таймера определения конца пакета
	if (UartBuf->EndMes) {
		UartBuf->EndMes=false;
		UartBuf->N=0;
	}
	if (UartBuf->N >= UART_BUF_SIZE) {
		UartBuf->N = 0;
	}	 
	UartBuf->Buf[UartBuf->N] = UDRbuf;
	UartBuf->N++;
	if (UartBuf->N == 1) {
		SetBit(TIFR,OCF2);		//это сброс флага сравнения, иначе сразу же выполнится прерывание
		SetBit(TIMSK,OCIE2);	//разрешение прерывания по сравнению
	}
	return;
}
	
void UARTNextByTrans(struct UartBufSt *UartBuf)
{	//отправка
	if (UartBuf->NeedSend != 0) {
		UDR = UartBuf->Buf[UartBuf->N];
		UartBuf->N++;
		UartBuf->NeedSend--;
	} else {
		UartBuf->N = 0;
		UartBuf->EndMes = false;
		ClearBit(RTS_PORT,RTS_PIN);
		ClearBit(UCSRB,TXCIE);	//запрет прерывания при отправке
	}
	return;
}
	
void UARTStartByRec(struct UartBufSt *UartBuf)
{
	if (UartBuf->NeedSend != 0) {
		SetBit(RTS_PORT,RTS_PIN);	//переход в режим передачи
		UartBuf->N = 1;
		UartBuf->NeedSend--;
		SetBit(UCSRB,TXCIE);	//разрешение прерывания при отправке
		UDR = UartBuf->Buf[0];	//отправляем первый байт
	}
	return;
}
	
void UARTendMB(struct UartBufSt *UartBuf)
{
	TCNT2 = 0;
	ClearBit(TIMSK,OCIE2);	//запретить прерывания таймера
	UartBuf->EndMes = true;
	return;	
}
	


