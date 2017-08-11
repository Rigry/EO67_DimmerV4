/*
 * Dimmer_v4.c
 */ 

#define START_COUNT	TCCR1B|=(1<<CS11); TIMSK|=(1<<OCIE1A);

/*остановка таймера, запрет прерываний*/
#define STOP_COUNT 	TCCR1B&=~(1<<CS11);	TIMSK&=~(1<<OCIE1A);
#define START_ADC 	ADCSRA|=(1<<ADSC);							/*запуск АЦП*/
#define INCR_STEP 10											/*шаг нарастания*/

#include <macros.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h> 
#include <stdbool.h>
#include "MBSlave.h"
#include "eeprom.h"

/*переменные*/
uint16_t T_work;											/*результат АЦП*/
uint16_t T_work_incr; 									/*время включения порта*/
uint16_t ADCOut=0;										/*буферная переменная, определяет элемент массива*/
uint16_t ModbusAdr;										/*адрес MB*/
struct MBRegSt stMBRegs;
volatile struct EEPROMst stEEPROM;
struct UartBufSt stUART;

/*флаги прерываний*/
bool IntFlagMain;
bool IntFlagRecUART;
bool IntFlagTransUART;
bool IntFlagEndMB;
bool IntFlagZero;

uint8_t UDRbuf;

enum MBInRegE {	
	uartsetset 	= 0,	//когда все нули 9600-8-N-1
	mbadrset	= 1,	//адрес (от 1 - 255)
	psswrd		= 2,	//пароль для изменения следующего регистра
	DevNset		= 3,	//установка заводского номера устройства
	DutyRatio	= 4,	//скважность управляющего импульса
	};
	enum MBOutRegE {
	Dev			= 0,	//код типа устройства
	DevN		= 1,	//заводской номер устройства
	uartset 	= 2,
	mbadr 		= 3,
	//opertime	= 4,	//время работы
	};

int main(void)
{
	//порты светодиода на выход
	RLED_OUT();
	GLED_OUT();
		
	//обнуляем UART и Modbus
	stUART.NeedSend = 0;
	stUART.N = 0;
	stUART.EndMes = false;
	for(uint8_t i=0; i< QTY_IN_REG; i++){
		stMBRegs.RegIn[i] = 0;
		stMBRegs.RegInMaxVal[i] = 0;
		stMBRegs.RegInMinVal[i] = 0;
	}
	
	//заполнение мин/макс значений регистров модбаса
	 
	stMBRegs.RegInMaxVal[uartsetset] = 0b00111111;
	stMBRegs.RegInMinVal[mbadrset] = 1;
	stMBRegs.RegInMaxVal[mbadrset] = 255;
	stMBRegs.RegInMaxVal[DutyRatio] = 255;
		
	ReadEEPROM(&stEEPROM);
	if (stEEPROM.mbadr==0xFFFF){
		stEEPROM.mbadr=1;
		stEEPROM.DevN=0;
		stEEPROM.uartset=0;
//		for (uint16_t i=0;i<QTY_OF_BYTES;i++){
//			stEEPROM.opertime[i]=0;
//		}
	}
	
	//заполнение выходных регистров Modbus
	stMBRegs.RegOut[Dev]=DEVUNIQNUMBER;
	stMBRegs.RegOut[DevN]=stEEPROM.DevN;
	stMBRegs.RegOut[uartset]=stEEPROM.uartset;
	stMBRegs.RegOut[mbadr]=stEEPROM.mbadr;
	stMBRegs.RegIn[uartsetset] = stMBRegs.RegOut[uartset];
	
	_delay_ms(500);

	//инициализация UART
	struct UARTSettingsStruc UARTSet;
	if (BitIsSet(FABIN_PORT, FABIN_PIN)) {
		ModbusAdr=1;
		UARTSet.baudrate = BR9600;
		UARTSet.CharacterSise = 8;
		UARTSet.EvenParity = none;
		UARTSet.StopBits = 1;
		
	} else {
		
		ModbusAdr=stEEPROM.mbadr;
		UARTSet.CharacterSise = 8;
		if (BitIsClear(stEEPROM.uartset, 0)) {
			UARTSet.EvenParity = none;
		} else if (BitIsSet(stEEPROM.uartset, 1)) {
			UARTSet.EvenParity = even;
		} else {
			UARTSet.EvenParity = odd;
		}

		UARTSet.StopBits = BitIsSet(stEEPROM.uartset, 2) ? 2 : 1;
		uint8_t tmp8 = (stEEPROM.uartset >> 3 & 0b111);
		switch (tmp8) {
			case 0b000:
				UARTSet.baudrate = BR9600;
				break;
			case 0b001:
				UARTSet.baudrate = BR14400;
				break;
			case 0b010:
				UARTSet.baudrate = BR19200;
				break;
			case 0b011:
				UARTSet.baudrate = BR28800;
				break;
			case 0b100:
				UARTSet.baudrate = BR38400;
				break;
			case 0b101:
				UARTSet.baudrate = BR57600;
				break;
			case 0b110:
				UARTSet.baudrate = BR76800;
				break;
			case 0b111:
				UARTSet.baudrate = BR115200;
				break;
			default:
				UARTSet.baudrate = BR9600;
				break;
		}
		if (UARTSet.baudrate == BR19200) {
			GLED_ON();
		}
	}
	UARTInit(UARTSet);


			
	wdt_enable(WDTO_1S);
	TCCR0|=(1<<CS02)|(1<<CS00); TIMSK|=(1<<TOIE0);
	/*включаем внешнее прерывание INT0 по заднему фронту*/
	MCUCR=MCUCR|(1<<ISC01); /*задний фронт INT0*/
	GICR=GICR|(1<<INT0);	/*разрешение прерывания*/
	/*инициализация порта PB1 на выход*/
	DDRB|=(1<<PB1);
	/*инициализация таймера вся в прерываниях*/
	/*инициализация АЦП*/
	/*от внутреннего источника 2,56В, выравнивание по левому краю*/
	ADMUX|=(1<<REFS1)|(1<<REFS0)|(1<<ADLAR);
	/*разрешение работы АЦП, разрешение прерывания, частота F_CPU/16*/
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2);
	/*инициализация спящего режима*/ 
	/*	set_sleep_mode(SLEEP_MODE_IDLE); /*режим сна*/
	/*	sleep_enable();		/*разрешаем сон*/
	sei();				/*разрешаем прерывания*/
	    
	while(1) {
		/*спящий режим*/
        /*sleep_cpu()*/
		if (IntFlagMain){
			IntFlagMain=false;
						
			//Обновление значений еепром по команде модбас

			if (stMBRegs.RegIn[uartsetset] != stMBRegs.RegOut[uartset]) {
				stEEPROM.uartset = stMBRegs.RegIn[uartsetset];
				stMBRegs.RegOut[uartset] = stEEPROM.uartset;
				stMBRegs.RegIn[uartsetset] = stMBRegs.RegOut[uartset];
			} else if (stMBRegs.RegIn[mbadrset] != 0) {
				stEEPROM.mbadr = stMBRegs.RegIn[mbadrset];
				stMBRegs.RegOut[mbadr] = stEEPROM.mbadr;
				stMBRegs.RegIn[mbadrset] = 0;
			} else if (stMBRegs.RegIn[psswrd] == PASSWORD) {
				stMBRegs.RegIn[psswrd] = 0;
				if (stMBRegs.RegIn[DevNset] != 0) {
					stEEPROM.DevN = stMBRegs.RegIn[DevNset];
					stMBRegs.RegOut[DevN] = stEEPROM.DevN;
					stMBRegs.RegIn[DevNset] = 0;					
				}
			}
			UpdEEPROM(&stEEPROM);
		}
		if (IntFlagRecUART) {
			IntFlagRecUART = false;
			UARTRec(&stUART, UDRbuf);
		}
		
		if (IntFlagTransUART) {
			IntFlagTransUART = false;
			UARTNextByTrans(&stUART);
			if (stUART.NeedSend==0){
				ClearBit(PORTD,PD7);
			}
		}	

		
		if (IntFlagEndMB) {
			UARTendMB(&stUART);
			MBSlave(&stUART,&stMBRegs,ModbusAdr);
			if (stUART.NeedSend!=0) {
				UARTStartByRec(&stUART);
				SetBit(PORTD,PD7);
			}
			IntFlagEndMB = false;
		}

		if (IntFlagZero) {
			_delay_us(350);
			if ((PIND&(1<<PD2))==0)
			{
				if (T_work!=0)
				{
					/*включаем выход*/
					PORTB|=(1<<PB1);
					/*запускаем таймер*/
					/*запись в регистры сравнения*/
					OCR1AH=T_work_incr/256;
					OCR1AL=T_work_incr%256;
					START_COUNT;
				} 
				/*запуск АЦП*/
				START_ADC;
			}
		
			if (stMBRegs.RegIn[DutyRatio]>0){
				T_work=stMBRegs.RegIn[DutyRatio]*37;
			} else if (ADCOut>NOISE_LEVEL) {
				T_work=ADCOut*37;
			} else {
				T_work=0;
			}
			
			if (T_work_incr < T_work) {
				T_work_incr = T_work_incr + INCR_STEP;
			} else if (T_work_incr > T_work) {
				T_work_incr = T_work;
			}
			IntFlagZero=false;
		}
    }//while(1) 
}//int main(void)


ISR(INT0_vect)
{
	IntFlagZero=true;	
}


/*прерывание таймера*/
ISR(TIMER1_COMPA_vect)
{
	/*выключаем выход*/
	PORTB&=~(1<<PB1);
	/*останавливаем таймер*/
	STOP_COUNT;
	/*обнуляем регистр счета*/
	TCNT1=0;
}

/*прерывание таймера*/
ISR(TIMER0_OVF_vect)
{
	IntFlagMain=true;
	__asm__ __volatile__ ("wdr");
}

/*прерывание АЦП*/
ISR(ADC_vect)
{
	/*запоминаем значение с АЦП в буферную переменную*/
	ADCOut=ADCH;
}
ISR(USART_RXC_vect)
{
	IntFlagRecUART = true;
	UDRbuf = UDR;
}
ISR(USART_TXC_vect) 
{
	IntFlagTransUART = true;
}
ISR(TIMER2_COMP_vect) 
{
	IntFlagEndMB = true;
}
ISR(__vector_default)
{
	
}