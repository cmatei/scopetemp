
bin_PROGRAMS = guithermal logthermal indithermal

guithermal_SOURCES	= guithermal.c \
			\
			common_indi.c common_indi.h \
			thermal_indi.c thermal_indi.h \
			\
			libindiclient/lilxml.h libindiclient/lilxml.c \
			libindiclient/base64.h libindiclient/base64.c \
			libindiclient/indi.h libindiclient/indi.c \
			libindiclient/indigui.h libindiclient/gtk/indigui.c \
			libindiclient/indi_io.h libindiclient/gtk/indi_io.c \
			libindiclient/indi_config.h libindiclient/gtk/indi_config.c \
			libindiclient/indi_list.h libindiclient/gtk/indi_list.c \
			libindiclient/gtk/indisave.h libindiclient/gtk/indisave.c \
			libindiclient/gtk/gtkled.h libindiclient/gtk/gtkled.c


indithermal_SOURCES	= indithermal.c scopetemp.c
indithermal_CPPFLAGS    = @LIBUSB_CFLAGS@ @INDI_CFLAGS@
indithermal_LDADD       = @LIBUSB_LIBS@ @INDI_LIBS@ -lindidriver -lz -lm

guithermal_CPPFLAGS	= @GTK_CFLAGS@
guithermal_LDADD	= @GTK_LIBS@ -lz -lm

logthermal_SOURCES	= logthermal.c \
			\
			common_indi.c common_indi.h \
			thermal_indi.c thermal_indi.h \
			\
			libindiclient/lilxml.h libindiclient/lilxml.c \
			libindiclient/base64.h libindiclient/base64.c \
			libindiclient/indi.h libindiclient/indi.c \
			libindiclient/indigui.h libindiclient/gtk/indigui.c \
			libindiclient/indi_io.h libindiclient/gtk/indi_io.c \
			libindiclient/indi_config.h libindiclient/gtk/indi_config.c \
			libindiclient/indi_list.h libindiclient/gtk/indi_list.c \
			libindiclient/gtk/indisave.h libindiclient/gtk/indisave.c \
			libindiclient/gtk/gtkled.h libindiclient/gtk/gtkled.c
logthermal_CPPFLAGS	= @GTK_CFLAGS@
logthermal_LDADD	= @GTK_LIBS@ -lz -lm


CLEANFILES = *~

