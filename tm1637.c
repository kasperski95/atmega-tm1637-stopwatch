/**
 * Copyright (c) 2017-2018, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 *
 * This is ATtiny13/25/45/85 library for 4-Digit LED Display based on TM1637 chip.
 *
 * Features:
 * - display raw segments
 * - display digits
 * - display colon
 * - display on/off
 * - brightness control
 *
 * References:
 * - library: https://github.com/lpodkalicki/attiny-tm1637-library
 * - documentation: https://github.com/lpodkalicki/attiny-tm1637-library/README.md
 * - TM1637 datasheet: https://github.com/lpodkalicki/attiny-tm1637-library/blob/master/docs/TM1637_V2.4_EN.pdf
 *
 * ============================================================================================================
 *
 * modified by Arkadiusz Kasprzyk. Tested on ATmega8A.
 *
 * Features:
 * - display character [a-z][0-9]{-=', _}
 * - display first 4 characters of c-string
 * - display integer from -999 to 9999
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "tm1637.h"

#define	TM1637_DIO_HIGH()		(PORTC |= _BV(TM1637_DIO_PIN))
#define	TM1637_DIO_LOW()		(PORTC &= ~_BV(TM1637_DIO_PIN))
#define	TM1637_DIO_OUTPUT()		(DDRC |= _BV(TM1637_DIO_PIN))
#define	TM1637_DIO_INPUT()		(DDRC &= ~_BV(TM1637_DIO_PIN))
#define	TM1637_DIO_READ() 		(((PINC & _BV(TM1637_DIO_PIN)) > 0) ? 1 : 0)
#define	TM1637_CLK_HIGH()		(PORTC |= _BV(TM1637_CLK_PIN))
#define	TM1637_CLK_LOW()		(PORTC &= ~_BV(TM1637_CLK_PIN))

static void TM1637_send_config(const uint8_t enable, const uint8_t brightness);
static void TM1637_send_command(const uint8_t value);
static void TM1637_start(void);
static void TM1637_stop(void);
static uint8_t TM1637_write_byte(uint8_t value);

static uint8_t _config = TM1637_SET_DISPLAY_ON | TM1637_BRIGHTNESS_MAX;
static uint8_t _segments = 0xff;
PROGMEM const uint8_t _chars2segments[] =
{
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F, // 9
	0b01110111,  // A
	0b01111100,  // b
	0b00111001,  // C
	0b01011110,  // d
	0b01111001,  // E
	0b01110001,  // F
	0b01101111,  // 9
	0b01110100,  // h
	0b00010000,  // i
	0b00001110,  // J
	0b01110110,  // H
	0b00111000,  // L
	0b01010100,  // n
	0b01010100,  // n
	0b01011100,  // o
	0b01110011,  // P
	0b01100111,  // q
	0b01010000,  // r
	0b01101101,  // 5
	0b01111000,  // t
	0b00111110,  // U
	0b00011100,  // u (v)
	0b00011100,  // u (w)
	0b01110110,  // H (x)
	0b01100110,  // 4 (y)
	0b01011011,  // 2 (z)
	0b01000000,  // -
	0b01001000,  // =
	0b00000010,  // '
	0b00000100,  // ,
	0b00001000,  // _
	0b00000000   // ' '
};

void
TM1637_init(const uint8_t enable, const uint8_t brightness)
{
	DDRC |= (_BV(TM1637_DIO_PIN)|_BV(TM1637_CLK_PIN));
	PORTC &= ~(_BV(TM1637_DIO_PIN)|_BV(TM1637_CLK_PIN));
	TM1637_display_colon(0);
	TM1637_send_config(enable, brightness);
}

void
TM1637_display_integer(int32_t number) {
    uint8_t buffer_size = 4;
    char buffer[] = {' ',' ',' ',' ','\0'};
    uint8_t i, bNegative = 0;

    if (number > 9999) {
        TM1637_display_c_str("E999");
    } else if (number < -999) {
        TM1637_display_c_str("E-99");
    } else {
        // make number positive
        if (number < 0) {
            bNegative = 1;
            number *= -1;
        }

        // convert to c-string
        i = buffer_size - 1;
        if (number > 0) {
            for (; number > 0; --i) {
                // convert to digit code in ASCII table
                buffer[i] = (number % 10) + '0';
                number /= 10;
            }
            if (bNegative) buffer[i] = '-';
        } else
            buffer[i] = '0';

        TM1637_display_c_str(buffer);
    }
}

void
TM1637_display_c_str(const char c_str[]) {
    uint8_t i = 0;
    uint8_t strlen = TM1637_POSITION_MAX;
    for (; i < TM1637_POSITION_MAX; ++i) {
        if (c_str[i] == '\0') strlen = i;

        if (i < strlen)TM1637_display_char(i, c_str[i]);
        else TM1637_display_char(i, ' ');
    }
}

void
TM1637_display_char(const uint8_t position, const uint8_t c)
{
	uint8_t segments;
	int16_t offset = 0;
	if (c >= '0' && c <= '9') offset = -'0';
	if (c >= 'a' && c <= 'z') offset = -'a' + 10;
	if (c >= 'A' && c <= 'Z') offset = -'A' + 10;

	switch (c) {
        case '-': offset = -'-' + 36; break;
        case '=': offset = -'=' + 37; break;
        case '\'': offset = -'\'' + 38; break;
        case ',': offset = -',' + 39; break;
        case '_': offset = -'_' + 40; break;
        case ' ': offset = -' ' + 41; break;
	}

	segments = pgm_read_byte_near((uint8_t *)&_chars2segments + c + offset);

	// display colon, if is enabled
	if (position == 0x01) {
		segments = segments | (_segments & 0x80);
		_segments = segments;
	}

	TM1637_display_segments(position, segments);
}

void
TM1637_display_colon(const uint8_t value)
{
	if (value) {
		_segments |= 0x80;
	} else {
		_segments &= ~0x80;
	}
	TM1637_display_segments(0x01, _segments);
}

void
TM1637_clear(void)
{
	uint8_t i;

	for (i = 0; i < TM1637_POSITION_MAX; ++i) {
		TM1637_display_segments(i, 0x00);
	}
}

void
TM1637_display_segments(const uint8_t position, const uint8_t segments)
{
	TM1637_send_command(TM1637_CMD_SET_DATA | TM1637_SET_DATA_F_ADDR);
	TM1637_start();
	TM1637_write_byte(TM1637_CMD_SET_ADDR | (position & (TM1637_POSITION_MAX - 1)));
	TM1637_write_byte(segments);
	TM1637_stop();
}

void
TM1637_enable(const uint8_t value)
{
	TM1637_send_config(value, _config & TM1637_BRIGHTNESS_MAX);
}

void
TM1637_set_brightness(const uint8_t value)
{
	TM1637_send_config(_config & TM1637_SET_DISPLAY_ON,
		value & TM1637_BRIGHTNESS_MAX);
}

void
TM1637_send_config(const uint8_t enable, const uint8_t brightness)
{

	_config = (enable ? TM1637_SET_DISPLAY_ON : TM1637_SET_DISPLAY_OFF) |
		(brightness > TM1637_BRIGHTNESS_MAX ? TM1637_BRIGHTNESS_MAX : brightness);

	TM1637_send_command(TM1637_CMD_SET_DSIPLAY | _config);
}

void
TM1637_send_command(const uint8_t value)
{
	TM1637_start();
	TM1637_write_byte(value);
	TM1637_stop();
}

void
TM1637_start(void)
{

	TM1637_DIO_HIGH();
	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);
	TM1637_DIO_LOW();
}

void
TM1637_stop(void)
{

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_HIGH();
}

uint8_t
TM1637_write_byte(uint8_t value)
{
	uint8_t i, ack;

	for (i = 0; i < 8; ++i, value >>= 1) {
		TM1637_CLK_LOW();
		_delay_us(TM1637_DELAY_US);

		if (value & 0x01) {
			TM1637_DIO_HIGH();
		} else {
			TM1637_DIO_LOW();
		}

		TM1637_CLK_HIGH();
		_delay_us(TM1637_DELAY_US);
	}

	TM1637_CLK_LOW();
	TM1637_DIO_INPUT();
	TM1637_DIO_HIGH();
	_delay_us(TM1637_DELAY_US);

	ack = TM1637_DIO_READ();
	if (ack) {
		TM1637_DIO_OUTPUT();
		TM1637_DIO_LOW();
	}
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_OUTPUT();

	return ack;
}
