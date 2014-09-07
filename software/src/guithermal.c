
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

GtkWidget *window, *graph, *status;

static cairo_pattern_t *grey, *col1, *col2, *col3, *col4;

static inline double clamp_double (double v, double min, double max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;

	return v;
}


#define TEMP_MIN -30.0
#define TEMP_MAX 40.0
#define NUM_SAMPLES (4 * 60)
#define NUM_TEMPS 4

static float samples[NUM_TEMPS][NUM_SAMPLES];


#define PAD_X 10.0
#define PAD_Y 10.0

static inline double temp_y(double h, double t, double min, double max)
{
	t = clamp_double(t, min, max);

	return PAD_Y + (h - 2 * PAD_Y) - (t - min) / (max - min) * (h - 2 * PAD_Y);
}

static inline double temp_x(double w, int i, int n)
{
	return PAD_X + ((double) i) / n * (w - 2 * PAD_X);
}


static void draw_set (cairo_t *cr, double w, double h, int idx, cairo_pattern_t *pattern)
{
	int i;

	cairo_set_source (cr, pattern);
	cairo_move_to (cr, temp_x(w, 0, NUM_SAMPLES), temp_y(h, samples[idx][0], TEMP_MIN, TEMP_MAX));

	for (i = 1; i < NUM_SAMPLES; i++) {
		cairo_line_to (cr, temp_x(w, i, NUM_SAMPLES), temp_y(h, samples[idx][i], TEMP_MIN, TEMP_MAX));
	}
	cairo_stroke (cr);

}
static gboolean graph_draw_cb(GtkWidget *widget, cairo_t *cr)
{

	double w = (double) gtk_widget_get_allocated_width (widget);
	double h = (double) gtk_widget_get_allocated_height (widget);

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);


	cairo_set_line_width (cr, 1.0);

	cairo_set_source (cr, grey);
	cairo_move_to (cr, temp_x (w, 0, NUM_SAMPLES), temp_y(h, 0.0, TEMP_MIN, TEMP_MAX));
	cairo_line_to (cr, temp_x (w, NUM_SAMPLES, NUM_SAMPLES), temp_y (h, 0.0, TEMP_MIN, TEMP_MAX));
	cairo_stroke (cr);

	draw_set (cr, w, h, 0, col1);
	draw_set (cr, w, h, 1, col2);
	draw_set (cr, w, h, 2, col3);
	draw_set (cr, w, h, 3, col4);

	printf("draw\n");

	//draw_graph_axes(cr, w, h);

	return TRUE;
}


static gboolean graph_clicked_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	printf("clicked\n");
	return TRUE;
}

static void act_indi_config()
{
}

static void act_indi_connect()
{
}


static GtkActionEntry thermal_actions[] = {
	/* INDI */
	{ "indi-menu", NULL, "_INDI" },
	{ "indi-config",          NULL, "Co_nfiguration",          "<control>N", NULL, G_CALLBACK (act_indi_config) },
	{ "indi-connect",         NULL, "C_onnect",                "<control>O", NULL, G_CALLBACK (act_indi_connect) },
	{ "user-quit",            NULL, "_Quit",                   "<control>Q", NULL, G_CALLBACK (gtk_main_quit) },

};

static char *thermal_ui =
	"<menubar name='thermal-menubar'>"
	"<menu name='indi' action='indi-menu'>"
	"  <menuitem name='Configuration'  action='indi-config'/>"
	"  <menuitem name='Connect' action='indi-connect'/>"
	"  <separator name='separator3'/>"
	"  <menuitem name='Quit'         action='user-quit'/>"
	"</menu>"
        "</menubar>";

static GtkWidget *get_main_menu_bar(GtkWidget *window)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;

	action_group = gtk_action_group_new ("ThermalActions");
	gtk_action_group_add_actions (action_group, thermal_actions,
				      G_N_ELEMENTS (thermal_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, thermal_ui, strlen(thermal_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	ret = gtk_ui_manager_get_widget (ui, "/thermal-menubar");

        /* Make sure that the accelerators work */
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;
}

static GtkWidget *gui_setup()
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *menubar;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_object_ref_sink(window);

	gtk_window_set_title (GTK_WINDOW (window), "Telescope Temperature");

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

	menubar = get_main_menu_bar(window);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

	graph = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), graph, 1, 1, 0);

	status = gtk_label_new ("Not init.");
	g_object_ref (status);
	gtk_misc_set_padding (GTK_MISC (status), 3, 3);
	gtk_misc_set_alignment (GTK_MISC (status), 0, 0.5);
	g_object_set_data_full (G_OBJECT (window), "status", status,
				(GDestroyNotify) g_object_unref);
	gtk_box_pack_start(GTK_BOX(vbox), status, 0, 0, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);


	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);

	g_signal_connect(G_OBJECT(graph), "draw",
			 G_CALLBACK(graph_draw_cb), window);

	g_signal_connect(G_OBJECT(graph), "button_press_event",
			 G_CALLBACK(graph_clicked_cb), window);

	gtk_widget_set_events(graph, GDK_BUTTON_PRESS_MASK);

	g_object_set_data(G_OBJECT(window), "graph", graph);

  	gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);

	gtk_widget_show_all (window);

	return window;
}


int main(int argc, char **argv)
{
	struct thermal_t *thermal;
	int i;

	indi_hostname = strdup("localhost");
	indi_port = 7624;
	indi_scopetemp_device = strdup("ScopeTemp");

	gtk_init (&argc, &argv);

	grey = cairo_pattern_create_rgba (0.5, 0.5, 0.5, 0.8);
	col1 = cairo_pattern_create_rgba (1.0, 0.0, 0.0, 0.8);
	col2 = cairo_pattern_create_rgba (0.0, 1.0, 0.0, 0.8);
	col3 = cairo_pattern_create_rgba (0.0, 0.0, 1.0, 0.8);
	col4 = cairo_pattern_create_rgba (1.0, 1.0, 0.0, 0.8);

#if 0
	for (i = 0; i < NUM_SAMPLES; i++) {
		samples[0][i] = -70.0/NUM_SAMPLES * i + 40;

#define ERR 3.0
		samples[1][i] = samples[0][i] - ERR + drand48() * (2 * ERR);
		samples[2][i] = samples[0][i] - ERR + drand48() * (2 * ERR);
		samples[3][i] = samples[0][i] - ERR + drand48() * (2 * ERR);

	}
#endif

	window = gui_setup ();

	thermal = thermal_find(window);

	gtk_main ();

	return 0;
}
