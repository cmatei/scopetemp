
AC_PREREQ(2.60)

AC_INIT([thermal], [0.1])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/guithermal.c])

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AM_PROG_CC_C_O

AM_PATH_GTK_3_0(3.2.0, , AC_MSG_ERROR(Cannot find GTK: is pkg-config in path?))

PKG_CHECK_MODULES([LIBUSB], libusb-1.0, , AC_MSG_ERROR(Cannot find libusb-1.0))
PKG_CHECK_MODULES([INDI], libindi, , AC_MSG_ERROR(Cannot find INDI libraries))

AC_CONFIG_FILES([
        Makefile
        src/Makefile
])
AC_OUTPUT

