
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include "requests.h"
#include "crc8.h"

/* RA+  ... PD3
   RA-  ... PD5
   DEC+ ... PD6
   DEC- ... PD4 */

#define GUIDE_RA_PLUS   _BV(3)
#define GUIDE_RA_MINUS  _BV(5)
#define GUIDE_DEC_PLUS  _BV(1)
#define GUIDE_DEC_MINUS _BV(4)

#define GUIDE_MASK (GUIDE_RA_PLUS | GUIDE_RA_MINUS | GUIDE_DEC_PLUS | GUIDE_DEC_MINUS)

/* DS1820 commands */
#define DS1820_SKIP_ROM        0xCC
#define DS1820_CONVERT_T       0x44
#define DS1820_READ_SCRATCHPAD 0xBE

#define T_DDR  DDRB
#define T_PIN  PINB
#define T_PORT PORTB

/* FAN1 PB7, FAN2 PD6 */
#define FAN1_BIT _BV(7)
#define FAN2_BIT _BV(6)

enum {
	IDLE,
	CONVERTING,
	READING,
};

struct ds1820 {
	uint8_t pin;
	uint8_t state;

	/* LSB, MSB, COUNT_REMAIN, COUNT_PER_C */
	/* FIXME: the 2 byte padding in this struct gives me much shorter code
	   (202 bytes at some point!) */
	uint8_t data[6];
};

/* T1..T4 are on PB5..PB2 */
struct ds1820 sensors[4] = {
	{ .pin = _BV(5), .state = IDLE },
	{ .pin = _BV(4), .state = IDLE },
	{ .pin = _BV(3), .state = IDLE },
	{ .pin = _BV(2), .state = IDLE },
};

struct ds1820 *sensor;

/* FIXME: can't disable interrupts, usb will be upset. Trust the CRC */
uint8_t ds1820_reset()
{
	uint8_t err;
	uint8_t pin = sensor->pin;

	T_PORT &= ~pin;
	T_DDR |= pin;
	_delay_us (480);

	//cli();
	T_DDR &= ~pin;
	_delay_us (66);
	err = T_PIN & pin;
	//sei();

	_delay_us( 480 - 66);
	if ((T_PIN & pin) == 0)
		err = 1;

	return err;
}

uint8_t ds1820_bitio(uint8_t b)
{
	uint8_t pin = sensor->pin;

	//cli();
	T_DDR |= pin;
	_delay_us(1);

	if (b)
		T_DDR &= ~pin;

	_delay_us(15 - 1);
	if ((T_PIN & pin) == 0)
		b = 0;

	_delay_us(60 - 15);

	T_DDR &= ~pin;
	//sei();
	return b;
}

uint8_t ds1820_write (uint8_t val)
{
	uint8_t i = 8, j;
	do {
		j = ds1820_bitio (val & 1);
		val >>= 1;
		if (j)
			val |= 0x80;
	} while (--i);

	return val;
}

uint8_t ds1820_read ()
{
	return ds1820_write (0xFF);
}

static uint8_t ds1820_read_scratchpad()
{
	static uint8_t scratchpad[9];
	int i;

	if (ds1820_reset())
		return 1;

	ds1820_write (DS1820_SKIP_ROM);
	ds1820_write (DS1820_READ_SCRATCHPAD);

	for (i = 0; i < 9; i++)
		scratchpad[i] = ds1820_read();

	if (crc8(scratchpad, 9))
		return 0;

	sensor->data[0] = scratchpad[0];
	sensor->data[1] = scratchpad[1];
	sensor->data[2] = scratchpad[6];
	sensor->data[3] = scratchpad[7];

	return 1;
}

static uint8_t ds1820_convert()
{
	if (ds1820_reset())
		return 1;

	ds1820_write (DS1820_SKIP_ROM);
	ds1820_write (DS1820_CONVERT_T);

	return 1;
}


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	uint8_t val = rq->wValue.bytes[0];

	switch (rq->bRequest) {
	case THERMAL_RQ_TEMPS:
		usbMsgPtr = sensors[val & 0x03].data;
		return 4;

	case THERMAL_RQ_GUIDE:
		PORTD = (PORTD & ~GUIDE_MASK) | (val & GUIDE_MASK);
		return 0;

	case THERMAL_RQ_FANS:
		/* FAN1 */
		PORTB &= ~FAN1_BIT;
		PORTB |= (val & 0x01) ? FAN1_BIT : 0;

		/* FAN2 */
		PORTD &= ~FAN2_BIT;
		PORTD |= (val & 0x02) ? FAN2_BIT : 0;
		break;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */


int main(void)
{
	uint8_t   i;

	/* set DDRD for guiding and FAN2 */
	DDRD  |= GUIDE_MASK | FAN2_BIT;
	PORTD &= ~(GUIDE_MASK | FAN2_BIT);

	/* FAN1 is PB7 */
	DDRB  |=  FAN1_BIT;
	PORTB &= ~FAN1_BIT;

	/* RESET status: all port bits are inputs without pull-up.
	 * That's the way we need D+ and D-. Therefore we don't need any
	 * additional hardware initialization.
	 */
	usbInit();
	usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */

	for (i = 0; i < 150; i++)
		_delay_ms(2);

	usbDeviceConnect();

	sei();

	i = 0;
	for (;;) {                /* main event loop */
		usbPoll();

		/* do the state machine for each temp sensor */
		sensor = &sensors[i];

		switch (sensor->state) {
		case IDLE:
			if (ds1820_convert())
				sensor->state = CONVERTING;
			break;

		case CONVERTING:
			if (ds1820_read(sensor->pin) == 0xFF)
				sensor->state = READING;
			break;

		case READING:
			if (ds1820_read_scratchpad()) {
				sensor->state = IDLE;
			}
			break;
		}

		i++;
		i &= 0x03;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
