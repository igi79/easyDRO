#define	F_CPU			8000000
#define chb(port,bit) ((port) & (1<<(bit)))

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usiTwiSlave.h"
#include <compat/deprecated.h>

volatile unsigned long ms = 0;

ISR(TIMER0_COMPA_vect)
{
	cli();
	ms++;
	sei();
}

void start_sw(void){
	cli();
	ms = 0;
	sbi(TIMSK, OCIE0A);
	sei();
}

unsigned int stop_sw(void){
	cli();
	cbi(TIMSK, OCIE0A);
	sei();
	return ms;	
}

int main(void)
{
	/*
	TCCR0A = 0b0000010;
	TCCR0B = 0b0000101;
	OCR0A =  78;
	//sbi(TIMSK, OCIE0A);		// enable interrupt on TMR0 compare match A
*/
		
	sei(); // enable interupts
	
	sbi(DDRB,DDB1);
	cbi(PORTB,PB1);
	
	cbi(PORTB,PB4); //enable pull-up resistor
	cbi(DDRB,DDB4); //input
	cbi(PORTB,PB4);

	cbi(PORTB,PB3); //enable pull-up resistor
	cbi(DDRB,DDB3); //input
	//cbi(PORTB,PB3);
	
	float x = 9999;	

	const uint8_t slaveAddress = 0x14;
	usiTwiSlaveInit(slaveAddress);
	
	void sendfloat(float v)
	{
		uint8_t *ptr, i;
		ptr = (uint8_t *)&v;
		for (i = 0; i < sizeof(float); usiTwiTransmitByte(*(ptr + i)), i++);
	}
	
	sendfloat(x);
	
	for(;;)
	{	
		/*
		if(usiTwiDataInReceiveBuffer())
		{
			
			usiClearReceiveBuffer();
			
			//while (chb(PINB,PB4) == true);
			//start_sw();
			//while (chb(PINB,PB4) != true);
			//sei();
			
			x = (float)ms;
			ms = 0;
			
			sei();
			//usiClearTransmitBuffer();
			continue;
		}
		*/		
		
		uint8_t state;
		//x = chb(PINB,PB4);
			
		//x = chb(PINB,PB4) ? 100 : 200;
		
		//_delay_ms(3);
		
		if(chb(PINB,PB4))PORTB ^= _BV(PB1);
		

		
		ms++;
	}
}