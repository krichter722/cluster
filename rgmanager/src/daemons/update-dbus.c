/* DBus notifications */
#include <stdio.h>
#include <stdint.h>
#include <resgroup.h>
#include <poll.h>
#include <dbus/dbus.h>
#include <rg_dbus.h>
#ifdef DBUS

#define DBUS_RGM_NAME	"com.redhat.cluster.rgmanager"
#define DBUS_RGM_IFACE	"com.redhat.cluster.rgmanager"
#define DBUS_RGM_PATH	"/com/redhat/cluster/rgmanager"

static DBusConnection *db = NULL;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_t th = 0;
static char _err[512];
static int err_set = 0;

int 
rgm_dbus_init(void)
{
	DBusConnection *dbc = NULL;
	DBusError err;

	dbus_error_init(&err);

	dbc = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (!dbc) {
		snprintf(_err, sizeof(_err),
			 "dbus_bus_get: %s", err.message);
		err_set = 1;
		dbus_error_free(&err);
		return -1;
	}

	dbus_connection_set_exit_on_disconnect(dbc, FALSE);

	db = dbc;
	return 0;
}


int
rgm_dbus_release(void)
{
	pthread_t t;

	if (!db)
		return 0;

	/* tell thread to exit - not sure how to tell dbus
	 * to wake up, so just have it poll XXX */
	if (th) {
		t = th;
		th = 0;
		pthread_join(t, NULL);
	}

	dbus_connection_unref(db);
	db = NULL;

	return 0;
}


static void *
_dbus_auto_flush(void *arg)
{
	/* DBus connection functions are thread safe */
	dbus_connection_ref(db);

	while (dbus_connection_read_write(db, 500)) {
		if (!th)
			break;	
	}

	dbus_connection_unref(db);

	th = 0;
	return NULL;
}


int
rgm_dbus_notify(const char *svcname,
		const char *svcstatus,
		const char *svcflags,
		const char *svcowner,
		const char *svclast)
{
	DBusMessage *msg = NULL;
	int ret = -1;

	if (err_set) {
		fprintf(stderr, "%s\n", _err);
		err_set = 0;
	}

	if (!db) {
		goto out_free;
	}

	pthread_mutex_lock(&mu);

	if (dbus_connection_get_is_connected(db) != TRUE) {
		err_set = 1;
		snprintf(_err, sizeof(_err), "DBus connection lost");
		rgm_dbus_release();
		goto out_unlock;
	}

	if (!th) {
		/* start auto-flush thread if needed */
		pthread_create(&th, NULL, _dbus_auto_flush, NULL);
	}

	if (!(msg = dbus_message_new_signal(DBUS_RGM_PATH,
	      				    DBUS_RGM_IFACE,
	      				    "ServiceStateChange"))) {
		goto out_unlock;
	}

	if (!dbus_message_append_args(msg,
	 			      DBUS_TYPE_STRING, &svcname,
	 			      DBUS_TYPE_STRING, &svcstatus,
	 			      DBUS_TYPE_STRING, &svcflags,
 				      DBUS_TYPE_STRING, &svcowner,
 				      DBUS_TYPE_STRING, &svclast,
	    			      DBUS_TYPE_INVALID)) {
		goto out_unlock;
	}

	dbus_connection_send(db, msg, NULL);
	ret = 0;

out_unlock:
	pthread_mutex_unlock(&mu);
	if (msg)
		dbus_message_unref(msg);
out_free:
	return ret;
}
#endif
