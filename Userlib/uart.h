/*
 *	Функции приема/отправки данных по uart
 *	Используется timer2 для определения конца пакета по приему
 *		- не должен быть использован в программе
 *	UART_BUF_SIZE должен быть задан в macros.h, там же операции с битами
 *
 */ 

#include <stdbool.h>
#include "macros.h"
#include <avr/io.h>
#include <stdint.h>





#ifndef UART_H_
#define UART_H_

	struct UartBufSt {					//структура работы с УАРТ на уровне отделенном от работы с регистрами
		uint8_t Buf[UART_BUF_SIZE];		//буффер куда приходят, откуда уходят данные
		uint8_t	N;						//количество переданных, принятых данных
		uint8_t NeedSend;				//количество байт, которыйе необходимо передать из Buf,
										//число отличное от нуля, переводит в режим передачи функцией UARTStartByRec()
		bool EndMes;					//признак конца пакета для модбаса
	};

	enum EvenParityENUM
	{	none,
		even,
		odd
	};
	//перечисление допустимых будрейтов для 16М осцилятора
	//подумать как сделать для отсальных в будущем
	enum Baud16M
	{	BR9600	= 9600,
		BR14400 = 14400,
		BR19200 = 19200,
		BR28800 = 28800,
		BR38400	= 38400,
		BR57600 = 57600,
		BR76800	= 76800,
		BR115200 = 115200
	};

	struct UARTSettingsStruc				//структура настройки UART
	{	enum Baud16M baudrate;				//из перечисления
		uint8_t CharacterSise;				//количество бит в кадре данных (пока пишу только для 8)
		enum EvenParityENUM	EvenParity;		//проверка на четность
		uint8_t	StopBits;
	};
	

	void UARTInit(struct UARTSettingsStruc Settings); 

	//прием байта из аппаратного буфера УАРТ
	void UARTRec(struct UartBufSt *UartBuf, uint8_t UDRbuf);

	//передача следующего байта из буфера (если он есть) в аппаратный буфер 	
	void UARTNextByTrans(struct UartBufSt *UartBuf);

	//инициирует отправку байтов из буфера, последующие байты отправляются функцией UARTNextByTrans()	
	void UARTStartByRec(struct UartBufSt *UartBuf);	

	//должна вызываться в обработчике прерывания таймера 2 по сравнению для определения конца пакета	
	void UARTendMB(struct UartBufSt *UartBuf);		

#endif	//UART_H_