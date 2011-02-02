/* DBus notifications */
#include <stdint.h>
#include <rg_dbus.h>
#include <errno.h>

#ifdef DBUS

#include <stdio.h>
#include <stdint.h>
#include <resgroup.h>
#include <poll.h>
#include <dbus/dbus.h>
#include <liblogthread.h>
#include <members.h>


#define DBUS_RGM_NAME	"com.redhat.cluster.rgmanager"
#define DBUS_RGM_IFACE	"com.redhat.cluster.rgmanager"
#define DBUS_RGM_PATH	"/com/redhat/cluster/rgmanager"

static DBusConnection *db = NULL;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_t th = 0;
static char _err[512];
static int err_set = 0;
#endif

/* Set this to the desired value prior to calling rgm_dbus_init() */
int rgm_dbus_notify = RGM_DBUS_DEFAULT;


int 
rgm_dbus_init(void)
#ifdef DBUS
{
	DBusConnection *dbc = NULL;
	DBusError err;

	if (!rgm_dbus_notify)
		return 0;

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
#else
{
	errno = ENOSYS;
	return -1;
}
#endif


int
rgm_dbus_release(void)
#ifdef DBUS
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
#else
{
	return 0;
}
#endif


#ifdef DBUS
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


static int
_rgm_dbus_notify(const char *svcname,
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


int32_t
rgm_dbus_update(char *key, uint64_t view, void *data, uint32_t size)
{
	char flags[64];
	rg_state_t *st;
	cluster_member_list_t *m = NULL;
	const char *owner;
	const char *last;
	int ret = 0;

	if (!rgm_dbus_notify)
		goto out_free;
	if (view == 1)
		goto out_free;
	if (size != (sizeof(*st)))
		goto out_free;

	st = (rg_state_t *)data;
	swab_rg_state_t(st);

	/* Don't send transitional states */
	if (st->rs_state == RG_STATE_STARTING ||
	    st->rs_state == RG_STATE_STOPPING)
		goto out_free;

	m = member_list();
	if (!m)
		goto out_free;

	owner = memb_id_to_name(m, st->rs_owner);
	last = memb_id_to_name(m, st->rs_last_owner);

	if (!owner)
		owner = "(none)";
	if (!last)
		last = "(none)";

	rg_flags_str(flags, sizeof(flags), st->rs_flags, (char *)" ");
	if (flags[0] == 0)
		snprintf(flags, sizeof(flags), "(none)");

	ret = _rgm_dbus_notify(st->rs_name,
			       rg_state_str(st->rs_state),
			       (char *)flags, owner, last);

	if (ret < 0) {
		logt_print(LOG_ERR, "Error sending update for %s; "
			   "notifications disabled\n", key);
		rgm_dbus_release();
		rgm_dbus_notify = 0;
	}

out_free:
	if (m)
		free_member_list(m);
	free(data);
	return 0;
}
#endif
