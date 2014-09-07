
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>

#include <indidevapi.h>
#include <eventloop.h>
#include <indicom.h>

#include "scopetemp.h"

#define DEVICE            "ScopeTemp"

#define MAIN_GROUP        "Main Control"
#define TEMP_GROUP        "Temperature"
#define GUIDE_GROUP       "Guiding"
#define TIMED_GUIDE_GROUP "Pulse Guiding"

#define TEMP_POLL_INTERVAL (10000) // in milisec

#define DEV_PRODUCT "ScopeTemp"
#define DEV_MANUFACTURER "mconovici@gmail.com"

#define REQ_WRITE 0x40
#define REQ_READ  0xC0

#define THERMAL_RQ_TEMPS            1
#define THERMAL_RQ_GUIDE            2
#define THERMAL_RQ_FANS             3

/*********************************************
 Property: Connection
*********************************************/
static ISwitch ConnectS[] = {
	{ "CONNECT",    "Connect",    ISS_OFF, 0, 0 },
	{ "DISCONNECT", "Disconnect", ISS_ON,  0, 0 },
};

static ISwitchVectorProperty ConnectSP = {
	DEVICE, "CONNECTION", "Connection", MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE, ConnectS, NARRAY(ConnectS), "", 0
};

/*********************************************
 Property: Temperatures
*********************************************/

static INumber TemperatureN[] = {
	{ "T1", "T1", "%6.3f", -50.0, 50.0, 0.0, 0.0, 0, 0, 0 },
	{ "T2", "T2", "%6.3f", -50.0, 50.0, 0.0, 0.0, 0, 0, 0 },
	{ "T3", "T3", "%6.3f", -50.0, 50.0, 0.0, 0.0, 0, 0, 0 },
	{ "T4", "T4", "%6.3f", -50.0, 50.0, 0.0, 0.0, 0, 0, 0 },
};

static INumberVectorProperty TemperatureNP = {
	DEVICE, "TEMPERATURE", "Temperatures", TEMP_GROUP, IP_RO, 0, IPS_IDLE, TemperatureN, NARRAY(TemperatureN), "", 0
};

/*********************************************
 Property: Fans
*********************************************/

static ISwitch FanS[] = {
	{ "FAN1", "Fan 1", ISS_OFF, 0, 0 },
	{ "FAN2", "Fan 2", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty FanSP = {
	DEVICE, "FANS", "Fans", TEMP_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE, FanS, NARRAY(FanS), "", 0
};


/*********************************************
 Property: Guiding
*********************************************/

static ISwitch MoveNSS[] = {
	{ "MOTION_NORTH", "Guide N", ISS_OFF, 0, 0 },
	{ "MOTION_SOUTH", "Guide S", ISS_OFF, 0, 0 }
};

static ISwitchVectorProperty MoveNSSP = {
	DEVICE, "TELESCOPE_MOTION_NS", "DEC", GUIDE_GROUP, IP_RW, ISR_ATMOST1, 0, IPS_IDLE, MoveNSS, NARRAY(MoveNSS), "", 0
};

static ISwitch MoveWES[] = {
	{ "MOTION_WEST", "Guide W", ISS_OFF, 0, 0 },
	{ "MOTION_EAST", "Guide E", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty MoveWESP = {
	DEVICE, "TELESCOPE_MOTION_WE", "RA", GUIDE_GROUP, IP_RW, ISR_ATMOST1, 0, IPS_IDLE, MoveWES, NARRAY(MoveWES), "", 0
};

static INumber TimedMoveNSN[] = {
	{ "TIMED_GUIDE_N", "Timed Guide N", "%.1f", -60000.0, 60000.0, 0.0, 0.0, 0, 0, 0 },
	{ "TIMED_GUIDE_S", "Timed Guide S", "%.1f", -60000.0, 60000.0, 0.0, 0.0, 0, 0, 0 },
};

static INumberVectorProperty TimedMoveNSNP = {
	DEVICE, "TELESCOPE_TIMED_GUIDE_NS", "Timed DEC Guiding", TIMED_GUIDE_GROUP, IP_RW, 0, IPS_IDLE, TimedMoveNSN, NARRAY(TimedMoveNSN), "", 0
};

static INumber TimedMoveWEN[] = {
	{ "TIMED_GUIDE_W", "Timed Guide W", "%.1f", -60000.0, 60000.0, 0.0, 0.0, 0, 0, 0 },
	{ "TIMED_GUIDE_E", "Timed Guide E", "%.1f", -60000.0, 60000.0, 0.0, 0.0, 0, 0, 0 },
};

static INumberVectorProperty TimedMoveWENP = {
	DEVICE, "TELESCOPE_TIMED_GUIDE_WE", "Timed RA Guiding", TIMED_GUIDE_GROUP, IP_RW, 0, IPS_IDLE, TimedMoveWEN, NARRAY(TimedMoveWEN), "", 0
};

/* temperature readout timer */
static int TID_temp = 0;

/* timed guiding timers and state */
static int TID_NS = 0, TID_WE = 0;
static int guide_N = 0, guide_S = 0, guide_E = 0, guide_W = 0;

static void stopguide_NS (void *dummy)
{
	guide_N = guide_S = 0;
	st_set_guiding (guide_N, guide_S, guide_E, guide_W);
}

static void stopguide_WE (void *dummy)
{
	guide_E = guide_W = 0;
	st_set_guiding (guide_N, guide_S, guide_E, guide_W);
}

static void pulseguide_NS(double duration, int dir)
{
	/* remove existing timer if present */
	if (TID_NS) {
		IERmTimer (TID_NS);
		TID_NS = 0;
	}

	guide_N = guide_S = 0;
	if (duration <= 0.0) {
		/* disable current NS output */
		st_set_guiding (guide_N, guide_S, guide_E, guide_W);
		return;
	}

	guide_N = !dir;
	guide_S = dir;

	st_set_guiding (guide_N, guide_S, guide_E, guide_W);

	TID_NS = IEAddTimer (floor(duration), stopguide_NS, NULL);
}

static void pulseguide_WE(double duration, int dir)
{
	/* remove existing timer if present */
	if (TID_WE) {
		IERmTimer (TID_WE);
		TID_WE = 0;
	}

	guide_W = guide_E = 0;
	if (duration <= 0.0) {
		/* disable current WE output */
		st_set_guiding (guide_N, guide_S, guide_E, guide_W);
		return;
	}

	guide_W = !dir;
	guide_E = dir;

	st_set_guiding (guide_N, guide_S, guide_E, guide_W);

	TID_WE = IEAddTimer (floor(duration), stopguide_WE, NULL);
}

static void reset_all_properties ()
{
	ConnectSP.s     = IPS_IDLE;
	TemperatureNP.s = IPS_IDLE;
	FanSP.s         = IPS_IDLE;
	MoveNSSP.s      = IPS_IDLE;
	MoveWESP.s      = IPS_IDLE;
	TimedMoveNSNP.s = IPS_IDLE;
	TimedMoveWENP.s = IPS_IDLE;

	IDSetSwitch (&ConnectSP, NULL);
	IDSetNumber (&TemperatureNP, NULL);
	IDSetSwitch (&FanSP, NULL);
	IDSetSwitch (&MoveWESP, NULL);
	IDSetSwitch (&MoveNSSP, NULL);
	IDSetNumber (&TimedMoveWENP, NULL);
	IDSetNumber (&TimedMoveNSNP, NULL);
}

static void poll_temperature()
{
	int i;
	double temps[4];

	st_get_temperatures(temps, 4);

	for (i = 0; i < 4; i++)
		TemperatureN[i].value = temps[i];

	IDSetNumber(&TemperatureNP, NULL);

	TID_temp = IEAddTimer(TEMP_POLL_INTERVAL, poll_temperature, NULL);
}

void ISGetProperties (const char *dev)
{
	IDDefSwitch (&ConnectSP, NULL);
}

void ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
}

void ISSnoopDevice (XMLEle *root)
{
}


void ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	int dir = 0;
	double duration = 0.0;

	if (dev && strcmp(dev, DEVICE))
		return;

	if (ConnectS[0].s != ISS_ON) {
		IDMessage(DEVICE, "ScopeTemp: please connect before issuing commands.");
		reset_all_properties ();
		return;
	}

	if (!strcmp(name, TimedMoveNSNP.name)) {
		TimedMoveNSN[0].value = 0.0;
		TimedMoveNSN[1].value = 0.0;

		if (IUUpdateNumber(&TimedMoveNSNP, values, names, n) < 0)
			return;

		if (TimedMoveNSN[0].value != 0.0) {
			duration = TimedMoveNSN[0].value;
			dir = 0;
		} else {
			duration = TimedMoveNSN[1].value;
			dir = 1;
		}

		pulseguide_NS (duration, dir);

		TimedMoveNSN[0].value = 0.0;
		TimedMoveNSN[1].value = 0.0;
		TimedMoveNSNP.s = IPS_OK;
		IDSetNumber (&TimedMoveNSNP, NULL);

		return;
	}

	if (!strcmp(name, TimedMoveWENP.name)) {
		TimedMoveWEN[0].value = 0.0;
		TimedMoveWEN[1].value = 0.0;

		if (IUUpdateNumber(&TimedMoveWENP, values, names, n) < 0)
			return;

		if (TimedMoveWEN[0].value != 0.0) {
			duration = TimedMoveWEN[0].value;
			dir = 0;
		} else {
			duration = TimedMoveWEN[1].value;
			dir = 1;
		}

		pulseguide_WE (duration, dir);

		TimedMoveWEN[0].value = 0.0;
		TimedMoveWEN[1].value = 0.0;
		TimedMoveWENP.s = IPS_OK;
		IDSetNumber (&TimedMoveWENP, NULL);

		return;
	}

}

void ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	int r;

	if (dev && strcmp(dev, DEVICE))
		return;

	if (!strcmp(name, ConnectSP.name)) {
		if (IUUpdateSwitch (&ConnectSP, states, names, n) < 0)
			return;

		if (ConnectS[0].s == ISS_ON) {
			if (!st_connect()) {
				ConnectSP.s = IPS_OK;
				IDSetSwitch(&ConnectSP, "ScopeTemp online.");

				IDDefNumber (&TemperatureNP, NULL);
				IDDefSwitch (&FanSP, NULL);
				IDDefSwitch (&MoveWESP, NULL);
				IDDefSwitch (&MoveNSSP, NULL);
				IDDefNumber (&TimedMoveWENP, NULL);
				IDDefNumber (&TimedMoveNSNP, NULL);

				/* don't add a new timer on every connect :-) */
				if (TID_temp == 0)
					poll_temperature();
			}

			return;
		} else {
			if (TID_temp)
				IERmTimer(TID_temp);
			TID_temp = 0;

			reset_all_properties ();
			IDSetSwitch (&ConnectSP, "ScopeTemp offline.");
			return;
		}

		return;
	}

	if (ConnectS[0].s != ISS_ON) {
		IDMessage(DEVICE, "ScopeTemp: please connect before issuing commands.");
		reset_all_properties ();
		return;
	}

	if (!strcmp(name, FanSP.name)) {
		if (IUUpdateSwitch (&FanSP, states, names, n) < 0)
			return;

		st_set_fans((FanS[0].s == ISS_ON), (FanS[1].s == ISS_ON));

		FanSP.s = IPS_OK;
		IDSetSwitch (&FanSP, "Fan 1 is %s, Fan 2 is %s\n",
			     (FanS[0].s == ISS_ON) ? "on" : "off",
			     (FanS[1].s == ISS_ON) ? "on" : "off");
		return;
	}

	if (!strcmp(name, MoveNSSP.name)) {

		MoveNSS[0].s = ISS_OFF;
		MoveNSS[1].s = ISS_OFF;

		/* disallow contradictory moves */
		if ((n > 1) && (states[0] == ISS_ON) && (states[1] == ISS_ON)) {
			states[0] = ISS_OFF;
			states[1] = ISS_OFF;
		}

		if (IUUpdateSwitch (&MoveNSSP, states, names, n) < 0)
			return;

		/* stop timers if moving manually */
		pulseguide_NS (0, 0);
		pulseguide_WE (0, 0);

		st_set_guiding ((MoveNSS[0].s == ISS_ON), (MoveNSS[1].s == ISS_ON),
				(MoveWES[0].s == ISS_ON), (MoveWES[1].s == ISS_ON));

		MoveNSSP.s = IPS_OK;
		IDSetSwitch (&MoveNSSP, NULL);
		return;
	}

	if (!strcmp(name, MoveWESP.name)) {
		MoveWES[0].s = ISS_OFF;
		MoveWES[1].s = ISS_OFF;

		/* disallow contradictory moves */
		if ((n > 1) && (states[0] == ISS_ON) && (states[1] == ISS_ON)) {
			states[0] = ISS_OFF;
			states[1] = ISS_OFF;
		}

		if (IUUpdateSwitch (&MoveWESP, states, names, n) < 0)
			return;

		/* stop timers if moving manually */
		pulseguide_NS (0, 0);
		pulseguide_WE (0, 0);

		st_set_guiding ((MoveNSS[0].s == ISS_ON), (MoveNSS[1].s == ISS_ON),
				(MoveWES[0].s == ISS_ON), (MoveWES[1].s == ISS_ON));

		MoveWESP.s = IPS_OK;
		IDSetSwitch (&MoveWESP, NULL);
		return;
	}

}


