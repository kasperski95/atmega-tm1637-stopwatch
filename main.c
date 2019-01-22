#include <stdint.h>
#include <avr/io.h>

#define F_CPU 1000000UL

#include <util/delay.h>
#include "tm1637.h"

int
main(void)
{
	uint8_t n, k = 0;

	/* setup */
	TM1637_init(1/*enable*/, 1/*brightness*/);
    TM1637_display_digit(0,8);
    TM1637_display_digit(1,8);
    TM1637_display_digit(2,8);
    TM1637_display_digit(3,8);

	/* loop */
	while (1) {

		TM1637_display_colon(1);
		_delay_ms(1000);
		TM1637_display_colon(0);
		_delay_ms(1000);

		/*for (n = 0; n < TM1637_POSITION_MAX; ++n) {
			TM1637_display_digit(n, (k + n) % 0x10);
		}
		TM1637_display_colon(1);
		_delay_ms(200);
		TM1637_display_colon(0);
		_delay_ms(200);
		k++;
		*/
	}
}
