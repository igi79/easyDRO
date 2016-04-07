#define F_CPU 8000000UL


#include <avr/io.h>
#include <compat/deprecated.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usiTwiSlave.h"

#define chb(port,bit) ((port) & (1<<(bit)))

volatile uint8_t portbhistory = 0;
volatile unsigned int ms = 0;
volatile uint16_t lom, him;
volatile uint8_t np = 1;
volatile long int f=0;

ISR(TIMER0_COMPA_vect)
{
	ms++;
}

ISR (PCINT0_vect)
{
	
	uint8_t changedbits;

	changedbits = PINB ^ portbhistory;
	portbhistory = PINB;

	if(changedbits & (1 << PB4) && chb(PINB,PB4))
	{
		cli();		
		if(ms > 20){
			uint16_t m1=0;
			uint16_t m2=0;
			uint8_t bit = 0, positive = 1;
			for(bit = 0; bit<16; bit++){
				while(chb(PINB,PB4));
				if(chb(PINB,PB3))m1 |= (1 << bit);
				while(!chb(PINB,PB4));
			}
			for(bit = 0; bit < 12; bit++){
				while(chb(PINB,PB4));
				if(chb(PINB,PB3))m2 |= (1 << bit);
				while(!chb(PINB,PB4));
			}
			/*
			while(chb(PINB,PB4));
			if(chb(PINB,PB3))positive = 0;
			np = positive;
			*/	
			lom = m1;
			him = m2;
			
			
			//np = chb(m3,5) ? -1 : 1;
		}
		ms = 0;
		sei();
	}
}

void init(void){
	
	DDRB |= _BV(DDB1);
	const uint8_t slaveAddress = 0x16;
	usiTwiSlaveInit(slaveAddress);
	
	cbi(PORTB,PB4); //disable pull-up resistor
	cbi(DDRB,DDB4); //input
	cbi(PORTB,PB4);

	cbi(PORTB,PB3); //disable pull-up resistor
	cbi(DDRB,DDB3); //input
	cbi(PORTB,PB3);	
	
	sbi(GIMSK,PCIE); // Turn on Pin Change interrupt
	sbi(PCMSK,PCINT4);
	
	TCCR0A = 0b0000010;
	TCCR0B = 0b0000101;
	OCR0A =  78;
	sbi(TIMSK, OCIE0A);
	
	sei();
}

#if defined(FULLDECODE)
int bcd2dec(uint16_t bcd)
{
	int dec=0;
	int mult;
	uint8_t i;
   for (mult=1; bcd && i<4; bcd=bcd>>4,mult*=10,i++)
   {
	  uint8_t d=0; 
	  d = (bcd & 0x0f); 
	 //d = bcd;
	  //d d= 9;
	  if(d>=0 && d<10)dec += d * mult;
   }
	return dec;
}
#endif

int main(void)
{
	init();

	while(1)
	{
		if(usiTwiDataInReceiveBuffer())
		{
			uint8_t value;
			uint8_t temp = usiTwiReceiveByte();
			/*
			if(temp == 'b'){
				uint8_t *ptr, i;
				ptr = (uint8_t *)&lom;
				for (i = 0; i < 2; usiTwiTransmitByte(*(ptr + i)), i++);
				ptr = (uint8_t *)&him;
				for (i = 0; i < 2; usiTwiTransmitByte(*(ptr + i)), i++);				
			}
			*/
			#if defined(FULLDECODE)
			
			cli();
			f = (int)bcd2dec(lom);
			if(f<=9999)f += 10000 * (int)bcd2dec(him);
			if(!np)f *= -1;		
			sei();
			uint8_t *ptr, i;
			ptr = (uint8_t *)&f;
			for (i = 0; i < 4; usiTwiTransmitByte(*(ptr + i)), i++);
			
			#else
			
			uint8_t *ptr, i;
			ptr = (uint8_t *)&lom;
			for (i = 0; i < 2; usiTwiTransmitByte(*(ptr + i)), i++);
			ptr = (uint8_t *)&him;
			for (i = 0; i < 2; usiTwiTransmitByte(*(ptr + i)), i++);
			
			#endif
		}
	}
	
	return 1;
}


