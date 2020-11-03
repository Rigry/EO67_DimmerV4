#include "eeprom.h"

void ReadEEPROM(volatile struct EEPROMst* st)
{
	uint16_t QtyBytes = sizeof(struct EEPROMst);
	uint8_t* ar = (uint8_t*)st;

	for (uint16_t adr = 0; adr < QtyBytes; adr++) {
		ar[adr] = EEPROM_read(adr+1);		//ячейку с адресом 0 не используем
		EEPROMbuf[adr] = ar[adr];
		if (adr > (MAX_EEPROM_ADR - 1)) {	
			return;
		}
	}
}

void UpdEEPROM(volatile struct EEPROMst *st)
{
	static const uint16_t QtyBytes = sizeof(struct EEPROMst);
	volatile static uint16_t Adr = 0;
	uint8_t* ar = (uint8_t*)st;	
	
	if ( BitIsClear(EECR, EEWE) ) {		//если запись доступна
		if (EEPROMbuf[Adr] != ar[Adr]) {
			EEPROMbuf[Adr] = ar[Adr];
			EEAR = Adr + 1;				//адрес (ячейку с адресом 0 не используем)
			EEDR = EEPROMbuf[Adr];		//данные

			//эта часть не работала в сишном варианте с оптимизацией -O0
			//поэтому сделал вставку
			asm	volatile(	"cli\n"
							"sbi %0, 2\n"	
							"sbi %0, 1\n"
							"sei\n"	
							:: "I" (_SFR_IO_ADDR(EECR))		
			);
			//а это сишный вариант
/*			cli();						//запрещаем прерывания
			SetBit(EECR, EEMWE);		//последовательность запускает запись
			SetBit(EECR, EEWE);
			sei();	*/					//разрешаем прерывания
		}

		Adr++;
		if ( (Adr >= QtyBytes) || (Adr > (MAX_EEPROM_ADR - 1)) ) {
			Adr = 0;
		}
	}
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE)) {
	}
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from data register */
	return EEDR;
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE))
	;
	/* Set up address and data registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMWE */
	EECR |= (1<<EEMWE);
	/* Start eeprom write by setting EEWE */
	EECR |= (1<<EEWE);
}