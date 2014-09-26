#ifndef __SCOPETEMP_H
#define __SCOPETEMP_H

#include <libusb-1.0/libusb.h>

#include <indidevapi.h>
#include <defaultdevice.h>


#define ST_MANUFACTURER "mconovici@gmail.com"
#define ST_PRODUCT "ScopeTemp"

#define ST_DEVICE ST_PRODUCT

class ScopeTemp : public INDI::DefaultDevice {

	static const int ST_READ  = 0xC0;
	static const int ST_WRITE = 0x40;

	static const int ST_REQUEST_TEMP  = 1;
	static const int ST_REQUEST_GUIDE = 2;
	static const int ST_REQUEST_PWM   = 3;

	static const int ST_TEMP_POLL_INTERVAL = 10000; // milisec

public:

	ScopeTemp();
	~ScopeTemp();

	bool getTemperature(int id, double *temp);
	bool setPWM(int pwm1, int pwm2);
	bool setGuiding(int n, int s, int w, int e);


	bool Connect();
	bool Disconnect();

	const char *getDefaultName() { return ST_DEVICE; }

	bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
	bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

	bool initProperties();
	bool updateProperties();

private:
	libusb_device_handle *usb_handle;

	int _timerNS;
	int _timerEW;
	int _guideN, _guideS, _guideE, _guideW;

	static void stop_NS(ScopeTemp *dev);
	bool guide_NS(double duration, int dir);

	static void stop_EW(ScopeTemp *dev);
	bool guide_EW(double duration, int dir);

	int _timerTemp;
	static void pollTemperature(ScopeTemp *dev);

	INumber TempN[4];
	INumberVectorProperty TempNP;

	INumber PWMN[2];
	INumberVectorProperty PWMNP;

	ISwitch MoveNSS[2];
	ISwitchVectorProperty MoveNSSP;

	ISwitch MoveEWS[2];
	ISwitchVectorProperty MoveEWSP;

	INumber TimedMoveNSN[2];
	INumberVectorProperty TimedMoveNSNP;

	INumber TimedMoveEWN[2];
	INumberVectorProperty TimedMoveEWNP;
};

#endif
