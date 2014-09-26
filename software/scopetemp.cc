/* scopetemp.cc -- ScopeTemp INDI driver */

#include <cstring>
#include <cmath>
#include <memory>

#include "scopetemp.h"

static ScopeTemp *scopeTemp = new ScopeTemp();

void ISGetProperties(const char *dev)
{
	scopeTemp->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
	scopeTemp->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
	scopeTemp->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
	scopeTemp->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
        INDI_UNUSED(dev);
        INDI_UNUSED(name);
        INDI_UNUSED(sizes);
        INDI_UNUSED(blobsizes);
        INDI_UNUSED(blobs);
        INDI_UNUSED(formats);
        INDI_UNUSED(names);
        INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
	scopeTemp->ISSnoopDevice(root);
}




ScopeTemp::ScopeTemp()
{
	usb_handle = NULL;
	_timerNS = _timerEW = 0;
	_guideN = _guideS = _guideE = _guideW = 0;
	_timerTemp = 0;
}

ScopeTemp::~ScopeTemp()
{
	Disconnect();
}


bool ScopeTemp::getTemperature(int id, double *temp)
{
	uint8_t buffer[4];

	if (!usb_handle || libusb_control_transfer(usb_handle, ST_READ, ST_REQUEST_TEMP, id, 0, buffer, 4, 0) != 4)
		return false;

	*temp = (((int8_t) buffer[1] << 8) + (buffer[0] & 0xFE)) / 2.0 - 0.25 + (buffer[3] - buffer[2]) / (1.0 * buffer[3]);

	return true;
}

bool ScopeTemp::setPWM(int pwm1, int pwm2)
{
	if (!usb_handle || libusb_control_transfer(usb_handle, ST_WRITE, ST_REQUEST_PWM, pwm1, pwm2, NULL, 0, 0) != 0)
		return false;

	return true;
}

bool ScopeTemp::setGuiding(int n, int s, int w, int e)
{
	uint8_t val = 0;

	val |= n ? (1 << 1) : 0; // dec+
	val |= s ? (1 << 4) : 0; // dec-
	val |= w ? (1 << 3) : 0; // ra+
	val |= e ? (1 << 5) : 0; // ra-

	if (!usb_handle || libusb_control_transfer(usb_handle, ST_WRITE, ST_REQUEST_GUIDE, val, 0, NULL, 0, 0) != 0)
		return false;

	return true;
}

bool ScopeTemp::Connect()
{
	libusb_device **devices, *dev;
	libusb_device_descriptor desc;
	libusb_device_handle *handle;
	int i, n;
	uint32_t devID;
	char manufacturer[32], product[32];

	if (usb_handle)
		return true;

	libusb_init(NULL);

	n = libusb_get_device_list(NULL, &devices);
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

		if (strcmp(manufacturer, ST_MANUFACTURER) || strcmp(product, ST_PRODUCT)) {
			libusb_close(handle);
			continue;
		}

		/* found it, keep open */
		usb_handle = handle;
		break;
	}

	libusb_free_device_list(devices, 1);

	return usb_handle ? true : false;
}

bool ScopeTemp::Disconnect()
{
	if (usb_handle)
		libusb_close(usb_handle);

	usb_handle = NULL;

	libusb_exit(NULL);

	return true;
}

bool ScopeTemp::initProperties()
{
	INDI::DefaultDevice::initProperties();

	IUFillNumber(&TempN[0], "T1", "T1 (C)", "%5.2f", -55., 125., 0., 0.);
	IUFillNumber(&TempN[1], "T2", "T2 (C)", "%5.2f", -55., 125., 0., 0.);
	IUFillNumber(&TempN[2], "T3", "T3 (C)", "%5.2f", -55., 125., 0., 0.);
	IUFillNumber(&TempN[3], "T4", "T4 (C)", "%5.2f", -55., 125., 0., 0.);
	IUFillNumberVector(&TempNP, TempN, 4, getDeviceName(), "TEMPERATURE", "Temperatures", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

	IUFillNumber(&PWMN[0], "PWM1", "PWM1 (%)", "%.f", 0., 100., 1., 0.);
	IUFillNumber(&PWMN[1], "PWM2", "PWM2 (%)", "%.f", 0., 100., 1., 0.);
	IUFillNumberVector(&PWMNP, PWMN, 2, getDeviceName(), "PWM", "PWM Control", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

	IUFillSwitch(&MoveNSS[0], "MOTION_NORTH", "Guide N", ISS_OFF);
	IUFillSwitch(&MoveNSS[1], "MOTION_SOUTH", "Guide S", ISS_OFF);
	IUFillSwitchVector(&MoveNSSP, MoveNSS, 2, getDeviceName(), "TELESCOPE_MOTION_NS", "DEC", GUIDE_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

	IUFillSwitch(&MoveEWS[0], "MOTION_WEST", "Guide W", ISS_OFF);
	IUFillSwitch(&MoveEWS[1], "MOTION_EAST", "Guide E", ISS_OFF);
	IUFillSwitchVector(&MoveEWSP, MoveEWS, 2, getDeviceName(), "TELESCOPE_MOTION_WE", "RA", GUIDE_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

	IUFillNumber(&TimedMoveNSN[0], "TIMED_GUIDE_N", "Timed Guide N", "%.1f", -60000., 60000., 0., 0.);
	IUFillNumber(&TimedMoveNSN[1], "TIMED_GUIDE_S", "Timed Guide S", "%.1f", -60000., 60000., 0., 0.);
	IUFillNumberVector(&TimedMoveNSNP, TimedMoveNSN, 2, getDeviceName(), "TELESCOPE_TIMED_GUIDE_NS", "Timed DEC Guiding", GUIDE_TAB, IP_RW, 60, IPS_IDLE);

	IUFillNumber(&TimedMoveEWN[0], "TIMED_GUIDE_W", "Timed Guide W", "%.1f", -60000., 60000., 0., 0.);
	IUFillNumber(&TimedMoveEWN[1], "TIMED_GUIDE_E", "Timed Guide E", "%.1f", -60000., 60000., 0., 0.);
	IUFillNumberVector(&TimedMoveEWNP, TimedMoveEWN, 2, getDeviceName(), "TELESCOPE_TIMED_GUIDE_EW", "Timed RA Guiding", GUIDE_TAB, IP_RW, 60, IPS_IDLE);

	return true;
}

bool ScopeTemp::updateProperties()
{
	INDI::DefaultDevice::updateProperties();

	if (isConnected()) {
		defineNumber(&TempNP);
		defineNumber(&PWMNP);
		defineSwitch(&MoveNSSP);
		defineSwitch(&MoveEWSP);
		defineNumber(&TimedMoveNSNP);
		defineNumber(&TimedMoveEWNP);

		if (!_timerTemp)
			_timerTemp = IEAddTimer(ST_TEMP_POLL_INTERVAL, (void (*)(void *)) pollTemperature, this);
	} else {
		deleteProperty(TempNP.name);
		deleteProperty(PWMNP.name);
		deleteProperty(MoveNSSP.name);
		deleteProperty(MoveEWSP.name);
		deleteProperty(TimedMoveNSNP.name);
		deleteProperty(TimedMoveEWNP.name);

		if (_timerTemp) {
			IERmTimer(_timerTemp);
			_timerTemp = 0;
		}
	}

	return true;
}

bool ScopeTemp::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
	int pwm1, pwm2;
	double duration;
	int dir;

	if (!strcmp(dev, getDeviceName())) {
		if (!strcmp(name, PWMNP.name)) {
			IUUpdateNumber(&PWMNP, values, names, n);

			pwm1 = (PWMN[0].value / 100.0) * 65535;
			pwm2 = (PWMN[1].value / 100.0) * 65535;
			setPWM(pwm1, pwm2);

			PWMNP.s = IPS_OK;
			IDSetNumber(&PWMNP, NULL);

			return true;
		}

		if (!strcmp(name, TimedMoveNSNP.name)) {
			TimedMoveNSN[0].value = 0.0;
			TimedMoveNSN[1].value = 0.0;

			IUUpdateNumber(&TimedMoveNSNP, values, names, n);

			if (TimedMoveNSN[0].value != 0.0) {
				duration = TimedMoveNSN[0].value;
				dir = 0;
			} else {
				duration = TimedMoveNSN[1].value;
				dir = 1;
			}

			guide_NS(duration, dir);

			TimedMoveNSN[0].value = 0.0;
			TimedMoveNSN[1].value = 0.0;
			TimedMoveNSNP.s = IPS_OK;
			IDSetNumber(&TimedMoveNSNP, NULL);

			return true;
		}

		if (!strcmp(name, TimedMoveEWNP.name)) {
			TimedMoveEWN[0].value = 0.0;
			TimedMoveEWN[1].value = 0.0;

			IUUpdateNumber(&TimedMoveEWNP, values, names, n);

			if (TimedMoveEWN[0].value != 0.0) {
				duration = TimedMoveEWN[0].value;
				dir = 0;
			} else {
				duration = TimedMoveEWN[1].value;
				dir = 1;
			}

			guide_EW(duration, dir);

			TimedMoveEWN[0].value = 0.0;
			TimedMoveEWN[1].value = 0.0;
			TimedMoveEWNP.s = IPS_OK;
			IDSetNumber(&TimedMoveEWNP, NULL);

			return true;
		}

	}
	return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool ScopeTemp::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
	if (!strcmp(dev, getDeviceName())) {
		if (!strcmp(name, MoveNSSP.name)) {
			MoveNSS[0].s = MoveNSS[1].s = ISS_OFF;

			/* no contradictory moves */
			if ((n > 1) && (states[0] == ISS_ON) && (states[1] == ISS_ON)) {
				states[0] = states[1] = ISS_OFF;
			}

			IUUpdateSwitch(&MoveNSSP, states, names, n);

			/* stop timers on manual moves */
			guide_NS(0, 0);
			guide_EW(0, 0);

			setGuiding((MoveNSS[0].s == ISS_ON), (MoveNSS[1].s == ISS_ON),
				   (MoveEWS[0].s == ISS_ON), (MoveEWS[1].s == ISS_ON));

			MoveNSSP.s = IPS_OK;
			IDSetSwitch(&MoveNSSP, NULL);

			return true;
		}
		if (!strcmp(name, MoveEWSP.name)) {
			MoveEWS[0].s = MoveEWS[1].s = ISS_OFF;

			/* no contradictory moves */
			if ((n > 1) && (states[0] == ISS_ON) && (states[1] == ISS_ON)) {
				states[0] = states[1] = ISS_OFF;
			}

			IUUpdateSwitch(&MoveEWSP, states, names, n);

			/* stop timers on manual moves */
			guide_NS(0, 0);
			guide_EW(0, 0);

			setGuiding((MoveNSS[0].s == ISS_ON), (MoveNSS[1].s == ISS_ON),
				   (MoveEWS[0].s == ISS_ON), (MoveEWS[1].s == ISS_ON));

			MoveEWSP.s = IPS_OK;
			IDSetSwitch(&MoveEWSP, NULL);

			return true;
		}

	}
	return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

void ScopeTemp::stop_NS(ScopeTemp *dev)
{
	dev->_guideN = dev->_guideS = 0;
	dev->setGuiding(dev->_guideN, dev->_guideS, dev->_guideE, dev->_guideW);
}

bool ScopeTemp::guide_NS(double duration, int dir)
{
	if (_timerNS) {
		IERmTimer(_timerNS);
		_timerNS = 0;
	}

	_guideN = _guideS = 0;
	if (duration <= 0.0) {
		setGuiding(_guideN, _guideS, _guideE, _guideW);
		return true;
	}

	_guideN = !dir;
	_guideS = dir;

	setGuiding(_guideN, _guideS, _guideE, _guideW);
	_timerNS = IEAddTimer(floor(duration), (void (*)(void *)) stop_NS, this);

	return true;
}

void ScopeTemp::stop_EW(ScopeTemp *dev)
{
	dev->_guideE = dev->_guideW = 0;
	dev->setGuiding(dev->_guideN, dev->_guideS, dev->_guideE, dev->_guideW);
}

bool ScopeTemp::guide_EW(double duration, int dir)
{
	if (_timerEW) {
		IERmTimer(_timerEW);
		_timerEW = 0;
	}

	_guideW = _guideE = 0;
	if (duration <= 0.0) {
		setGuiding(_guideN, _guideS, _guideE, _guideW);
		return true;
	}

	_guideW = !dir;
	_guideE = dir;

	setGuiding(_guideN, _guideS, _guideE, _guideW);
	_timerEW = IEAddTimer(floor(duration), (void (*)(void *)) stop_EW, this);

	return true;
}

void ScopeTemp::pollTemperature(ScopeTemp *dev)
{
	int i;

	for (i = 0; i < 4; i++) {
		dev->getTemperature(i, &dev->TempN[i].value);
	}
	IDSetNumber(&dev->TempNP, NULL);

	dev->_timerTemp = IEAddTimer(ST_TEMP_POLL_INTERVAL, (void (*)(void *)) pollTemperature, dev);
}
