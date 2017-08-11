/*
 * macros.h
 *
 * Created: 08.03.2017 7:11:01
 *  Author: dvk
 */ 
#include <stdint.h>

#ifndef MACROS_H_
#define MACROS_H_

	/**
	 * для удобства битовых операций
	 */
	#define	SetBit(reg, bit)		reg |= (1<<bit)
	#define	ClearBit(reg, bit)		reg &= (~(1<<bit))
	#define	InvBit(reg, bit)        reg ^= (1<<bit)
	#define	BitIsSet(reg, bit)      ((reg & (1<<bit)) != 0)
	#define	BitIsClear(reg, bit)    ((reg & (1<<bit)) == 0)	

	#define DEVUNIQNUMBER	2		//для этой прошивки и этого устройства, согласно таблице кодов устройств ЭО-

	#define PASSWORD		0xC2B8	//для смены заводского номера

	#define F_CPU	14745600UL

	#define CHANQTY	8

	#define QTY_IN_REG	13
	#define QTY_OUT_REG	5

	#define MAINWORKCICLE_PRESCALER	1024 // 0 8 64 256 1024

	#define UART_BUF_SIZE 100

	#define QTY_OF_BYTES 255		//количество байт памяти для записи времени наработки
	#define NOISE_LEVEL 25			//граница шума на входе управления

	#define RTS_DDR 	DDRD
	#define RTS_PORT 	PORTD
	#define RTS_PIN 	PD3
	
	//пин включения заводских настроек
	#define FABIN_PORT	PIND
	#define FABIN_PIN	PD5

	//пин зеленого индикатора
	#define GLED_DDR	DDRD
	#define GLED_PORT	PORTD
	#define GLED_PIN	PD7

	//пин красного индикатора
	#define RLED_DDR	DDRD
	#define RLED_PORT	PORTD
	#define RLED_PIN	PD6

	#define GLED_OUT()	SetBit(GLED_DDR, GLED_PIN)
	#define GLED_ON()	SetBit(GLED_PORT, GLED_PIN)
	#define GLED_OFF()	ClearBit(GLED_PORT, GLED_PIN)	
	#define RLED_OUT()	SetBit(RLED_DDR, RLED_PIN)
	#define RLED_ON()	SetBit(RLED_PORT, RLED_PIN)
	#define RLED_OFF()	ClearBit(RLED_PORT, RLED_PIN)
	
	/**
	 * структура данных, хранящейся в еепром
	 */
	struct EEPROMst{
		volatile uint16_t 	DevN;		//зав номер устройства
		volatile uint16_t 	uartset;
		volatile uint16_t 	mbadr;
	//	uint8_t		opertime[QTY_OF_BYTES];
	};

#endif /* MACROS_H_ */
