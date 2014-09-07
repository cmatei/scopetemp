#include <stdio.h>
#include <string.h>

#include "libindiclient/indi.h"
#include "common_indi.h"
#include "thermal_indi.h"

#include <glib-object.h>


static void thermal_temp_change_cb (struct indi_prop_t *iprop, void *data)
{
	struct thermal_t *thermal = data;

	if (!thermal->ready)
		return;

	INDI_exec_callbacks (INDI_COMMON (thermal), THERMAL_CALLBACK_TEMPS);
}

static void thermal_fan_change_cb (struct indi_prop_t *iprop, void *data)
{
	struct thermal_t *thermal = data;

	if (!thermal->ready)
		return;

	INDI_exec_callbacks (INDI_COMMON (thermal), THERMAL_CALLBACK_FANS);
}

static void thermal_check_state(struct thermal_t *thermal)
{
	if (thermal->is_connected && thermal->temp_prop && thermal->fan_prop) {
		thermal->ready = 1;
		d4_printf("Thermal is ready\n");
		INDI_exec_callbacks (INDI_COMMON (thermal), THERMAL_CALLBACK_READY);
	}
}

static void thermal_connect(struct indi_prop_t *iprop, void *callback_data)
{
	struct thermal_t *thermal = (struct thermal_t *) callback_data;

	if (!strcmp(iprop->name, "TEMPERATURE")) {
		thermal->temp_prop = iprop;
		indi_prop_add_cb (iprop, (IndiPropCB) thermal_temp_change_cb, thermal);
	}
	else if (!strcmp(iprop->name, "FANS")) {
		thermal->fan_prop = iprop;
		indi_prop_add_cb (iprop, (IndiPropCB) thermal_fan_change_cb, thermal);
	}
	else if (!strcmp(iprop->name, "GUIDER")) {
		thermal->guide_prop = iprop;
	}
	else
		INDI_try_dev_connect (iprop, INDI_COMMON (thermal), NULL);

	thermal_check_state (thermal);
}



void thermal_get_temperatures (struct thermal_t *thermal, float *t1, float *t2, float *t3, float *t4)
{
	struct indi_elem_t *elem = NULL;

	*t1 = *t2 = *t3 = *t4 = 0.0;

	if (!thermal->ready || !thermal->temp_prop)
		return;

	if ((elem = indi_find_elem (thermal->temp_prop, "T1")) != NULL)
		*t1 = elem->value.num.value;

	if ((elem = indi_find_elem (thermal->temp_prop, "T2")) != NULL)
		*t2 = elem->value.num.value;

	if ((elem = indi_find_elem (thermal->temp_prop, "T3")) != NULL)
		*t3 = elem->value.num.value;

	if ((elem = indi_find_elem (thermal->temp_prop, "T4")) != NULL)
		*t4 = elem->value.num.value;
}

void thermal_get_fans (struct thermal_t *thermal, int *fan1, int *fan2)
{
	struct indi_elem_t *elem = NULL;

	*fan1 = *fan2 = 0;

	if (!thermal->ready || !thermal->fan_prop)
		return;

	if ((elem = indi_find_elem (thermal->fan_prop, "FAN1")) != NULL)
		*fan1 = elem->value.set;

	if ((elem = indi_find_elem (thermal->fan_prop, "FAN2")) != NULL)
		*fan2 = elem->value.set;
}

void thermal_set_fans (struct thermal_t *thermal, int fan1, int fan2)
{
	int send = 0;

	if (!thermal->ready || !thermal->fan_prop)
		return;

	if (INDI_update_elem_if_changed (thermal->fan_prop, "FAN1", fan1))
		send = 1;

	if (INDI_update_elem_if_changed (thermal->fan_prop, "FAN2", fan2))
		send = 1;

	if (send)
		indi_send (thermal->fan_prop, NULL);
}

void thermal_set_guiding (struct thermal_t *thermal, int dir)
{
	if (!thermal->ready || !thermal->guide_prop)
		return;

	if (INDI_update_elem_if_changed (thermal->guide_prop, "ACTION", dir))
		indi_send (thermal->guide_prop, NULL);
}


void thermal_set_ready_callback (void *window, void *func, void *data)
{
	struct thermal_t *thermal;

	if ((thermal = (struct thermal_t *) g_object_get_data (G_OBJECT (window), "thermal")) == NULL)
		return;

	INDI_set_callback(INDI_COMMON (thermal), THERMAL_CALLBACK_READY, func, data);
}


struct thermal_t *thermal_find (void *window)
{
	struct thermal_t *thermal;
	struct indi_t *indi;

	thermal = (struct thermal_t *) g_object_get_data (G_OBJECT (window), "thermal");
	if (thermal) {
		if (thermal->ready) {
			d4_printf("Found scope thermal monitoring\n");
			return thermal;
		}

		return NULL;
	}

	if (! (indi = INDI_get_indi (window)))
		return NULL;

	thermal = g_new0 (struct thermal_t, 1);
	INDI_common_init (INDI_COMMON (thermal), "thermal", thermal_check_state, THERMAL_CALLBACK_MAX);

	indi_device_add_cb (indi, indi_scopetemp_device, (IndiDevCB) thermal_connect, thermal);

	g_object_set_data (G_OBJECT (window), "thermal", thermal);

	if (thermal->ready)
		return thermal;

	return NULL;
}

