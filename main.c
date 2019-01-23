#define F_CPU 2000000UL

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tm1637.h"



//--------------------------------------------------------------------
// DECLARATIONS

void init_external_interrupts();
void init_timer_16_ctc(uint16_t interval_ms, uint16_t prescaler);

//--------------------------------------------------------------------
// GLOBALS

volatile uint16_t INTERVAL_MS_COUNTER = 0;
volatile uint8_t B_UPDATE_DISPLAY = 0;


//--------------------------------------------------------------------

int main(void) {
    // turn on auxiliary LED
    DDRB |= (1<<PB0);
    PORTB = 1;

    // INIT
    TM1637_init(1 /*enable*/, 0 /*brightness*/);
    init_external_interrupts();
    init_timer_16_ctc(10 /*interval in ms*/, 64 /* prescaler */);
	sei(); // turn on interrupts

	// LOOP
	while (1) {
        if (B_UPDATE_DISPLAY) {
            TM1637_display_integer(INTERVAL_MS_COUNTER);
            B_UPDATE_DISPLAY = 0;
        }

	}
}



//--------------------------------------------------------------------
// INTERRUPT HANDLERS

ISR(INT0_vect) {
    INTERVAL_MS_COUNTER = 0;
}

ISR(INT1_vect) {
    INTERVAL_MS_COUNTER = 10;
}


ISR(TIMER1_COMPA_vect) {
    INTERVAL_MS_COUNTER++;
    B_UPDATE_DISPLAY = 1;
}



//--------------------------------------------------------------------
// INITS

void init_external_interrupts() {
    MCUCR = 0;

    // INT0
    DDRD  |= (0<<PD2);
    PORTD |= (1<<PD2);   // enable pull-up resistor
    GICR  |= (1<<INT0);  // enable interrupt from INT0
	MCUCR |= (1<<ISC01); // on falling edge

	// INT1
    DDRD  |= (0<<PD3);
    PORTD |= (1<<PD3);
    GICR  |= (1<<INT1);
	MCUCR |= (1<<ISC11);
}


void init_timer_16_ctc(uint16_t interval_ms, uint16_t prescaler) {
	uint8_t prescaler_config = 0;
	switch (prescaler) {
        case 1:     prescaler_config = (0<<CS12)|(0<<CS11)|(1<<CS10); break;
        case 8:     prescaler_config = (0<<CS12)|(1<<CS11)|(0<<CS10); break;
        case 64:    prescaler_config = (0<<CS12)|(1<<CS11)|(1<<CS10); break;
        case 256:   prescaler_config = (1<<CS12)|(0<<CS11)|(0<<CS10); break;
        case 1024:  prescaler_config = (1<<CS12)|(0<<CS11)|(1<<CS10); break;
	}

	TCCR1A = (1<<COM1A1);
	TCCR1B = (1<<WGM12)| prescaler_config;
	OCR1A = (F_CPU / prescaler) / interval_ms - 1;
	TIMSK |= (1<<OCIE1A);
}



