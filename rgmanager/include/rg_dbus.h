#ifdef DBUS
#ifndef _RGM_DBUS_H
#define _RGM_DBUS_H

int rgm_dbus_init(void);
int rgm_dbus_release(void);
int rgm_dbus_notify(const char *svcname,
    		    const char *svcstatus,
    		    const char *svcflags,
    		    const char *svcowner,
    		    const char *svclast);

#endif
#endif
