#ifndef __THERMAL_INDI_H
#define __THERMAL_INDI_H

#include "libindiclient/indi.h"

#include <glib.h>

#include "common_indi.h"

enum THERMAL_GUIDE_DIRECTIONS {
	THERMAL_GUIDE_NONE = 0,
	THERMAL_GUIDE_N,
	THERMAL_GUIDE_S,
	THERMAL_GUIDE_E,
	THERMAL_GUIDE_W,
};

enum THERMAL_CALLBACKS {
	THERMAL_CALLBACK_READY = 0,
	THERMAL_CALLBACK_TEMPS,
	THERMAL_CALLBACK_FANS,
	THERMAL_CALLBACK_GUIDER,

	THERMAL_CALLBACK_MAX
};

struct thermal_t {
	//This must be fist in the structure
	COMMON_INDI_VARS

	struct indi_prop_t *temp_prop;
	struct indi_prop_t *fan_prop;
	struct indi_prop_t *guide_prop;
};

struct thermal_t *thermal_find(void *window);

void thermal_set_ready_callback(void *window, void *func, void *data);


#endif
