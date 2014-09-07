
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <gtk/gtk.h>

#include "libindiclient/indi.h"
#include "common_indi.h"
#include "thermal_indi.h"

char *indi_hostname;
int   indi_port;
char *indi_scopetemp_device;

/* main window */
static GtkWidget *window;

/* the temps */
static float t1 = 0.0, t2 = 0.0, t3 = 0.0, t4 = 0.0;

/* the fans */
static int fan1 = 0, fan2 = 0;

static GtkWidget *gui_setup ()
{
	GtkWidget *mwin;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *t1, *t2, *t3, *t4;
	GtkWidget *f1, *f2;
	GtkWidget *l;

	mwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request (mwin, 400, 200);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add (GTK_CONTAINER (mwin), vbox);

	t1 = gtk_label_new("0.0");
	g_object_set_data (G_OBJECT (mwin), "t1", t1);
	gtk_box_pack_start (GTK_BOX (vbox), t1, FALSE, FALSE, 3);

	t2 = gtk_label_new("0.0");
	g_object_set_data (G_OBJECT (mwin), "t2", t2);
	gtk_box_pack_start (GTK_BOX (vbox), t2, FALSE, FALSE, 3);

	t3 = gtk_label_new("0.0");
	g_object_set_data (G_OBJECT (mwin), "t3", t3);
	gtk_box_pack_start (GTK_BOX (vbox), t3, FALSE, FALSE, 3);

	t4 = gtk_label_new("0.0");
	g_object_set_data (G_OBJECT (mwin), "t4", t4);
	gtk_box_pack_start (GTK_BOX (vbox), t4, FALSE, FALSE, 3);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	/* FAN 1 */
	l = gtk_label_new ("Fan #1");
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, FALSE, 3);

	f1 = gtk_switch_new ();
	g_object_set_data (G_OBJECT (mwin), "f1", f1);
	gtk_box_pack_start (GTK_BOX (hbox), f1, TRUE, FALSE, 3);


	/* FAN 2 */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	l = gtk_label_new ("Fan #2");
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, FALSE, 3);

	f2 = gtk_switch_new ();
	g_object_set_data (G_OBJECT (mwin), "f2", f2);
	gtk_box_pack_start (GTK_BOX (hbox), f2, TRUE, FALSE, 3);


	g_signal_connect (G_OBJECT (mwin), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all (mwin);

	return mwin;
}

static void temp_to_label (char *label, float val)
{
	GtkWidget *l;
	static char buffer[32];

	snprintf(buffer, 32, "%.3f", val);

	l = g_object_get_data (G_OBJECT (window), label);
	g_return_if_fail (l != NULL);

	gtk_label_set_text (GTK_LABEL (l), buffer);
}

int update_temps (struct thermal_t *thermal)
{
	FILE *fout;
	thermal_get_temperatures (thermal, &t1, &t2, &t3, &t4);

	temp_to_label("t1", t1);
	temp_to_label("t2", t2);
	temp_to_label("t3", t3);
	temp_to_label("t4", t4);

	if ((fout = fopen("scopetemp.log", "a")) == NULL)
		return 1;

	fprintf(fout, "%lu %.3f %.3f %.3f %.3f %d %d\n",
		time(NULL),
		t1, t2, t3, t4,
		fan1, fan2);

	fclose(fout);

	return 1;
}

static void update_switch(char *label, gboolean val)
{
	GtkWidget *sw;

	sw = g_object_get_data (G_OBJECT (window), label);
	g_return_if_fail (sw != NULL);

	gtk_switch_set_active (GTK_SWITCH (sw), val);
}

int update_fans (struct thermal_t *thermal)
{
	thermal_get_fans (thermal, &fan1, &fan2);

	update_switch ("f1", fan1);
	update_switch ("f2", fan2);

	return 1;
}

void scopetemp_ready(void *data)
{
	struct thermal_t *thermal = g_object_get_data (G_OBJECT (window), "thermal");

	g_return_if_fail (thermal != NULL);

	INDI_set_callback((struct INDI_common_t *) thermal, THERMAL_CALLBACK_TEMPS, update_temps, thermal);
	INDI_set_callback((struct INDI_common_t *) thermal, THERMAL_CALLBACK_FANS, update_fans, thermal);
}


int main(int argc, char **argv)
{
	struct thermal_t *thermal;
	int i;

	indi_hostname = strdup("localhost");
	indi_port = 7624;
	indi_scopetemp_device = strdup("ScopeTemp");

	gtk_init (&argc, &argv);

	window = gui_setup ();
	thermal = thermal_find(window);
	thermal_set_ready_callback (window, scopetemp_ready, NULL);

	gtk_main ();

	return 0;
}
