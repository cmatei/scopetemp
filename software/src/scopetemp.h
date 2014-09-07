#ifndef __SCOPETEMP_H
#define __SCOPETEMP_H

#define DEV_MANUFACTURER "mconovici@gmail.com"
#define DEV_PRODUCT "ScopeTemp"

extern int st_connect();
extern int st_disconnect();

extern int st_get_temperatures(double *temps, int n);
extern int st_set_fans (int fan1, int fan2);
extern int st_set_guiding (int n, int s, int w, int e);

#endif
