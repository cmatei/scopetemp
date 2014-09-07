#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "scopetemp.h"

#define REQUEST_READ  0xC0
#define REQUEST_WRITE 0x40

/* ScopeTemp commands */
#define ST_REQ_TEMPS            1
#define ST_REQ_GUIDE            2
#define ST_REQ_FANS             3


static libusb_device_handle *handle = NULL;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static void initialize_usb()
{
	static int usb_initialized = 0;

	if (!usb_initialized)
		libusb_init (NULL);

	usb_initialized = 1;
}

int st_disconnect()
{
	if (handle)
		libusb_close(handle);
	handle = NULL;

	return 0;
}

/* calling this multiple times will reconnect */
int st_connect()
{
	libusb_device **devices;
	libusb_device *dev;
	struct libusb_device_descriptor desc;
	int i, n;
	uint32_t devID;
	static char manufacturer[32];
	static char product[32];

	initialize_usb();

	st_disconnect();

	n = libusb_get_device_list (NULL, &devices);

	for (i = 0; i < n; i++) {
		dev = devices[i];
		if (libusb_get_device_descriptor(dev, &desc) < 0)
			continue;


		/* voti.nl USB VID/PID for vendor class devices */
		devID = (desc.idVendor << 16) + desc.idProduct;
		if (devID != 0x16C005DC)
			continue;

		if (libusb_open(dev, &handle) < 0)
			continue;

		if ((libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char *) manufacturer, 32) < 0) ||
		    (libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char *) product, 32) < 0)) {
			libusb_close(handle);
			continue;
		}

		if (strcmp(manufacturer, DEV_MANUFACTURER) || strcmp(product, DEV_PRODUCT)) {
			libusb_close(handle);
			continue;
		}


		libusb_free_device_list (devices, 1);

		/* found it, keep the handle open */

		return 0;
	}

	libusb_free_device_list (devices, 1);
	handle = NULL;

	return 1;
}

static double temp_value(uint8_t *temps)
{
	return (((int8_t) temps[1] << 8) + (temps[0] & 0xFE)) / 2.0 - 0.25 + (temps[3] - temps[2]) / (1.0 * temps[3]);
}

int st_get_temperatures(double *temps, int n)
{
	uint8_t buffer[4];
	int i, r = 0;

	if (!handle)
		return 1;

	for (i = 0; i < MIN(4, n); i++) {
		memset(buffer, 0, 4);

		if (libusb_control_transfer(handle, REQUEST_READ, ST_REQ_TEMPS, i, 0, buffer, 4, 0) != 4)
			r = 1;

		temps[i] = temp_value(buffer);
	}

	return r;
}

int st_set_fans (int fan1, int fan2)
{
	if (!handle)
		return 1;

	libusb_control_transfer(handle, REQUEST_WRITE, ST_REQ_FANS,
				(fan1 ? 0x01 : 0x00) | (fan2 ? 0x02 : 0x00),
				0,
				NULL, 0, 0);

	return 0;
}

int st_set_guiding (int n, int s, int w, int e)
{
	uint8_t val = 0;
/* connected to PORTB

#define GUIDE_RA_PLUS   _BV(3)
#define GUIDE_RA_MINUS  _BV(5)
#define GUIDE_DEC_PLUS  _BV(1)
#define GUIDE_DEC_MINUS _BV(4)
*/

	if (!handle)
		return 1;

	val |= n ? (1 << 1) : 0; // dec+
	val |= s ? (1 << 4) : 0; // dec-
	val |= w ? (1 << 3) : 0; // ra+
	val |= e ? (1 << 5) : 0; // ra-

	libusb_control_transfer(handle, REQUEST_WRITE, ST_REQ_GUIDE, val, 0, NULL, 0, 0);

	return 0;
}
