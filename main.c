#define F_CPU 1000000UL

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "tm1637.h"


int main(void) {
	TM1637_init(1/*enable*/, 0/*brightness*/);
	TM1637_display_colon(0);

    TM1637_display_integer(-314);
    uint8_t i = 0;

	while (1) {
		TM1637_display_integer(i++);
	}
}
