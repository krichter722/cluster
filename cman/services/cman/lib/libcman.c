/*
 * Provides a cman API using the corosync executive
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <limits.h>

#include <corosync/corotypes.h>
#include <corosync/coroipc.h>
#include <corosync/mar_gen.h>
#include <corosync/ipc_gen.h>
#include <corosync/cfg.h>
#include <corosync/confdb.h>
#include <corosync/votequorum.h>
#include <corosync/ipc_cman.h>

#include "ccs.h"
#include "libcman.h"

#define CMAN_MAGIC 0x434d414e

#define CMAN_SHUTDOWN_ANYWAY   1
#define CMAN_SHUTDOWN_REMOVED  2

struct cman_inst {
	int magic;
	void *ipc_ctx;
	int finalize;
	void *privdata;
	cman_datacallback_t data_callback;
	cman_callback_t notify_callback;
	pthread_mutex_t response_mutex;
	pthread_mutex_t dispatch_mutex;

	int node_count;
	votequorum_node_t * node_list;
	int node_list_size;

	corosync_cfg_handle_t cfg_handle;
	votequorum_handle_t cmq_handle;
};

static void cfg_shutdown_callback(
	corosync_cfg_handle_t handle,
	corosync_cfg_shutdown_flags_t flags);

static void votequorum_notification_callback(
        votequorum_handle_t handle,
	uint64_t context,
        uint32_t quorate,
        uint32_t node_list_entries,
        votequorum_node_t node_list[]);

static votequorum_callbacks_t cmq_callbacks =
{
	.votequorum_notify_fn = votequorum_notification_callback,
};

static corosync_cfg_callbacks_t cfg_callbacks =
{
	.corosync_cfg_state_track_callback = NULL,
	.corosync_cfg_shutdown_callback = cfg_shutdown_callback,
};


#define VALIDATE_HANDLE(h) do {if (!(h) || (h)->magic != CMAN_MAGIC) {errno = EINVAL; return -1;}} while (0)

static struct cman_inst *admin_inst;

static void cfg_shutdown_callback(
	corosync_cfg_handle_t handle,
	corosync_cfg_shutdown_flags_t flags)
{
	int cman_flags = 0;

	if (!admin_inst)
		return;

	if (flags == COROSYNC_CFG_SHUTDOWN_FLAG_REGARDLESS)
		cman_flags = CMAN_SHUTDOWN_ANYWAY;

	if (admin_inst->notify_callback)
		admin_inst->notify_callback((void *)admin_inst, admin_inst->privdata, CMAN_REASON_TRY_SHUTDOWN, cman_flags);

}

static void votequorum_notification_callback(
        votequorum_handle_t handle,
	uint64_t context,
        uint32_t quorate,
        uint32_t node_list_entries,
        votequorum_node_t node_list[])
{
	struct cman_inst *cman_inst;

	votequorum_context_get(handle, (void **)&cman_inst);

	/* Save information for synchronous queries */
	cman_inst->node_count = node_list_entries;
	if (cman_inst->node_list_size < node_list_entries) {
		if (cman_inst->node_list)
			free(cman_inst->node_list);

		cman_inst->node_list = malloc(sizeof(votequorum_node_t) * node_list_entries * 2);
		if (cman_inst->node_list) {
			memcpy(cman_inst->node_list, node_list, sizeof(votequorum_node_t) * node_list_entries);
			cman_inst->node_list_size = node_list_entries;
		}
	}

	if (context && cman_inst->notify_callback)
		cman_inst->notify_callback((void*)cman_inst, cman_inst->privdata, CMAN_REASON_STATECHANGE, quorate);
}

static int votequorum_check_and_start(struct cman_inst *cman_inst)
{
	if (!cman_inst->cmq_handle) {
		if (votequorum_initialize(&cman_inst->cmq_handle, &cmq_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
		votequorum_context_set(cman_inst->cmq_handle, (void*)cman_inst);
	}
	return 0;
}

static int refresh_node_list(struct cman_inst *cman_inst)
{
	int error;

	if (votequorum_check_and_start(cman_inst))
		return -1;

	votequorum_trackstart(cman_inst->cmq_handle, 0, CS_TRACK_CURRENT);

	error = votequorum_dispatch(cman_inst->cmq_handle, CS_DISPATCH_ONE);
	return (error==CS_OK?0:-1);
}

cman_handle_t cman_init (
	void *privdata)
{
	cs_error_t error;
	struct cman_inst *cman_inst;

	cman_inst = malloc(sizeof(struct cman_inst));
	if (!cman_inst)
		return NULL;

	memset(cman_inst, 0, sizeof(struct cman_inst));
	error = cslib_service_connect (CMAN_SERVICE, &cman_inst->ipc_ctx);
	if (error != CS_OK) {
		goto error;
	}

	cman_inst->privdata = privdata;
	cman_inst->magic = CMAN_MAGIC;
	pthread_mutex_init (&cman_inst->response_mutex, NULL);
	pthread_mutex_init (&cman_inst->dispatch_mutex, NULL);

	return (void *)cman_inst;

error:
	free(cman_inst);
	errno = ENOMEM;
	return NULL;
}

cman_handle_t cman_admin_init (
	void *privdata)
{
	if (admin_inst) {
		errno = EBUSY;
		return NULL;
	}

	admin_inst = cman_init(privdata);
	return admin_inst;
}


int cman_finish (
	cman_handle_t handle)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (cman_inst->cmq_handle) {
		votequorum_finalize(cman_inst->cmq_handle);
		cman_inst->cmq_handle = 0;
	}
	if (cman_inst->cfg_handle) {
		corosync_cfg_finalize(cman_inst->cfg_handle);
		cman_inst->cfg_handle = 0;
	}

	if (handle == admin_inst)
		admin_inst = NULL;

	pthread_mutex_lock (&cman_inst->response_mutex);

	/*
	 * Another thread has already started finalizing
	 */
	if (cman_inst->finalize) {
		pthread_mutex_unlock (&cman_inst->response_mutex);
		errno = EINVAL;
		return -1;
	}

	cman_inst->finalize = 1;

	pthread_mutex_unlock (&cman_inst->response_mutex);

	/*
	 * Disconnect from the server
	 */
	cslib_service_disconnect (&cman_inst->ipc_ctx);

	return 0;
}

/* These next four calls are the only ones that are specific to cman in the release. Everything else
 * uses standard corosync or 'ccs' libraries.
 * If you really want to do inter-node communications then CPG might be more appropriate to
 * your needs. These functions are here partly to provide an API compatibility, but mainly
 * to provide wire-protocol compatibility with older versions.
 */
int cman_start_recv_data (
	cman_handle_t handle,
	cman_datacallback_t callback,
	uint8_t port)
{
	int error;
	struct cman_inst *cman_inst;
	struct iovec iov[2];
	struct req_lib_cman_bind req_lib_cman_bind;
	mar_res_header_t res;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	pthread_mutex_lock (&cman_inst->response_mutex);

	req_lib_cman_bind.header.size = sizeof (struct req_lib_cman_bind);
	req_lib_cman_bind.header.id = MESSAGE_REQ_CMAN_BIND;
	req_lib_cman_bind.port = port;

	iov[0].iov_base = (char *)&req_lib_cman_bind;
	iov[0].iov_len = sizeof (struct req_lib_cman_bind);

        error = cslib_msg_send_reply_receive (cman_inst->ipc_ctx,
					      iov, 1,
					      &res, sizeof (mar_res_header_t));

	pthread_mutex_unlock (&cman_inst->response_mutex);

	if (error != CS_OK) {
		goto error_exit;
	}

	errno = error = res.error;

error_exit:
	return (error==CS_OK?0:-1);
}

int cman_end_recv_data (
	cman_handle_t handle)
{
	int error;
	struct cman_inst *cman_inst;
	struct iovec iov[2];
	mar_req_header_t req;
	mar_res_header_t res;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	pthread_mutex_lock (&cman_inst->response_mutex);

	req.size = sizeof (mar_req_header_t);
	req.id = MESSAGE_REQ_CMAN_UNBIND;

	iov[0].iov_base = (char *)&req;
	iov[0].iov_len = sizeof (mar_req_header_t);

        error = cslib_msg_send_reply_receive (cman_inst->ipc_ctx,
					      iov, 1,
					      &res, sizeof (mar_res_header_t));

	pthread_mutex_unlock (&cman_inst->response_mutex);

	if (error != CS_OK) {
		goto error_exit;
	}

	errno = error = res.error;

error_exit:

	return (error?-1:0);
}

int cman_send_data(cman_handle_t handle, const void *message, int len, int flags, uint8_t port, int nodeid)
{
	int error;
	struct cman_inst *cman_inst;
	struct iovec iov[2];
	char buf[len+sizeof(struct req_lib_cman_sendmsg)];
	struct req_lib_cman_sendmsg *req_lib_cman_sendmsg = (struct req_lib_cman_sendmsg *)buf;
	mar_res_header_t res;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	pthread_mutex_lock (&cman_inst->response_mutex);

	req_lib_cman_sendmsg->header.size = sizeof (mar_req_header_t);
	req_lib_cman_sendmsg->header.id = MESSAGE_REQ_CMAN_SENDMSG;
	req_lib_cman_sendmsg->to_port = port;
	req_lib_cman_sendmsg->to_node = nodeid;
	req_lib_cman_sendmsg->msglen = len;
	memcpy(req_lib_cman_sendmsg->message, message, len);

	iov[0].iov_base = buf;
	iov[0].iov_len = len+sizeof(struct req_lib_cman_sendmsg);

        error = cslib_msg_send_reply_receive (cman_inst->ipc_ctx,
					      iov, 1,
					      &res, sizeof (mar_res_header_t));

	pthread_mutex_unlock (&cman_inst->response_mutex);

	if (error != CS_OK) {
		goto error_exit;
	}

	errno = error = res.error;

error_exit:

	return (error?-1:0);
}

int cman_is_listening (
	cman_handle_t handle,
	int nodeid,
	uint8_t port)
{
	int error;
	struct cman_inst *cman_inst;
	struct iovec iov[2];
	struct res_lib_cman_is_listening res_lib_cman_is_listening;
	struct req_lib_cman_is_listening req_lib_cman_is_listening;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	pthread_mutex_lock (&cman_inst->response_mutex);

	req_lib_cman_is_listening.header.size = sizeof (struct req_lib_cman_is_listening);
	req_lib_cman_is_listening.header.id = MESSAGE_REQ_CMAN_IS_LISTENING;
	req_lib_cman_is_listening.nodeid = nodeid;
	req_lib_cman_is_listening.port = port;

	iov[0].iov_base = (char *)&req_lib_cman_is_listening;
	iov[0].iov_len = sizeof (struct req_lib_cman_is_listening);

        error = cslib_msg_send_reply_receive (cman_inst->ipc_ctx,
					      iov, 1,
					      &res_lib_cman_is_listening, sizeof (struct res_lib_cman_is_listening));

	pthread_mutex_unlock (&cman_inst->response_mutex);

	if (error != CS_OK) {
		goto error_exit;
	}

	errno = error = res_lib_cman_is_listening.header.error;

error_exit:

	return (error?-1:0);
}

/* This call is now handled by cfg */
int cman_get_node_addrs (
	cman_handle_t handle,
	int nodeid,
	int max_addrs,
	int *num_addrs,
	struct cman_node_address *addrs)
{
	int error;
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (!cman_inst->cfg_handle) {
		if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
	}

	error = corosync_cfg_get_node_addrs(cman_inst->cfg_handle, nodeid, max_addrs, num_addrs, (corosync_cfg_node_address_t *)addrs);

	return (error==CS_OK?0:-1);
}

/*
 * An example of how we would query the quorum service.
 * In fact we can use the lower-level quorum service if quorate all we
 * needed to know - it provides the quorum state regardless of which
 * quorum provider is loaded.
 * Users of libcman typically are nosy and want to know all sorts of
 * other things.
 */
int cman_is_quorate(cman_handle_t handle)
{
	struct cman_inst *cman_inst;
	int quorate = -1;
	struct votequorum_info info;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	if (votequorum_getinfo(cman_inst->cmq_handle, 0, &info) != CS_OK)
		errno = EINVAL;
	else
		quorate = ((info.flags & VOTEQUORUM_INFO_FLAG_QUORATE) != 0);

	return quorate;
}

/* This call is now handled by cfg */
int cman_shutdown(cman_handle_t handle, int flags)
{
	struct cman_inst *cman_inst;
	int error;
	corosync_cfg_shutdown_flags_t cfg_flags = 0;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (!cman_inst->cfg_handle) {
		if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
	}

	if (flags && CMAN_LEAVEFLAG_REMOVED) {
		if (votequorum_check_and_start(cman_inst))
			return -1;

		votequorum_leaving(cman_inst->cmq_handle);
	}

	if (flags == CMAN_SHUTDOWN_ANYWAY)
		cfg_flags = COROSYNC_CFG_SHUTDOWN_FLAG_REGARDLESS;

	error = corosync_cfg_try_shutdown(cman_inst->cfg_handle, cfg_flags);

	/* ERR_LIBRARY happens because corosync shuts down while we are connected */
	if (error == CS_ERR_LIBRARY || error == CS_OK)
		error = 0;

	return error;
}
/*
 * This call is now mostly handled by cfg.
 * However if we want to do a "leave remove" then we need to tell
 * votequorum first.
 */
int cman_leave_cluster(cman_handle_t handle, int flags)
{
	struct cman_inst *cman_inst;
	int error;
	corosync_cfg_shutdown_flags_t cfg_flags = 0;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (!cman_inst->cfg_handle) {
		if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
	}

	/* Tell votequorum to reduce quorum when we go */
	if (flags && CMAN_LEAVEFLAG_REMOVED) {
		if (votequorum_check_and_start(cman_inst))
			return -1;

		votequorum_leaving(cman_inst->cmq_handle);
	}


	cfg_flags = COROSYNC_CFG_SHUTDOWN_FLAG_IMMEDIATE;

	error = corosync_cfg_try_shutdown(cman_inst->cfg_handle, cfg_flags);
	/* ERR_LIBRARY happens because corosync shuts down while we are connected */
	if (error == CS_ERR_LIBRARY || error == CS_OK)
		error = 0;

	return error;
}

/* This call is now handled by cfg */
int cman_replyto_shutdown(cman_handle_t handle, int flags)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (!cman_inst->cfg_handle) {
		if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
	}

	error = corosync_cfg_replyto_shutdown(cman_inst->cfg_handle, flags);

	return error;
}

/* This call is now handled by cfg */
int cman_kill_node(cman_handle_t handle, int nodeid)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (!cman_inst->cfg_handle) {
		if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
			errno = ENOMEM;
			return -1;
		}
	}

	error = corosync_cfg_kill_node(cman_inst->cfg_handle, nodeid, "Killed by cman_tool");

	return (error==CS_OK?0:-1);
}

/* This call is handled by votequorum */
int cman_set_votes(cman_handle_t handle, int votes, int nodeid)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_setvotes(cman_inst->cmq_handle, nodeid, votes);

	return (error==CS_OK?0:-1);
}

/* This call is handled by votequorum */
int cman_set_expected_votes(cman_handle_t handle, int expected)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_setexpected(cman_inst->cmq_handle, expected);

	return (error==CS_OK?0:-1);
}

int cman_get_fd (
	cman_handle_t handle)
{
	struct cman_inst *cman_inst;
	int fd;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	fd = cslib_fd_get (cman_inst->ipc_ctx);

	return fd;
}

int cman_getprivdata(
	cman_handle_t handle,
	void **context)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	*context = cman_inst->privdata;

	return (CS_OK);
}

int cman_setprivdata(
	cman_handle_t handle,
	void *context)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	cman_inst->privdata = context;

	return (CS_OK);
}

/* This call is handled by votequorum */
int cman_register_quorum_device(cman_handle_t handle, char *name, int votes)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_qdisk_register(cman_inst->cmq_handle, name, votes);

	return error;
}

/* This call is handled by votequorum */
int cman_unregister_quorum_device(cman_handle_t handle)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_qdisk_unregister(cman_inst->cmq_handle);

	return error;
}

/* This call is handled by votequorum */
int cman_poll_quorum_device(cman_handle_t handle, int isavailable)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_qdisk_poll(cman_inst->cmq_handle, 1);

	return error;
}

/* This call is handled by votequorum */
int cman_get_quorum_device(cman_handle_t handle, struct cman_qdev_info *info)
{
	struct cman_inst *cman_inst;
	int error;
	struct votequorum_qdisk_info qinfo;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_qdisk_getinfo(cman_inst->cmq_handle, &qinfo);

	if (!error) {
		info->qi_state = qinfo.state;
		info->qi_votes = qinfo.votes;
		strcpy(info->qi_name, qinfo.name);
	}

	return error;
}

/* This call is handled by votequorum */
int cman_set_dirty(cman_handle_t handle)
{
	struct cman_inst *cman_inst;
	int error;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	error = votequorum_setstate(cman_inst->cmq_handle);

	return error;
}


struct res_overlay {
	mar_res_header_t header __attribute__((aligned(8)));
	char data[512000];
};


int cman_dispatch (
	cman_handle_t handle,
	int dispatch_types)
{
	int timeout = -1;
	cs_error_t error = CS_OK;
	int cont = 1; /* always continue do loop except when set to 0 */
	int dispatch_avail;
	struct cman_inst *cman_inst;
	struct res_overlay dispatch_data;
	struct res_lib_cman_sendmsg *res_lib_cman_sendmsg;

	if (dispatch_types != CS_DISPATCH_ONE &&
		dispatch_types != CS_DISPATCH_ALL &&
		dispatch_types != CS_DISPATCH_BLOCKING) {

		return (CS_ERR_INVALID_PARAM);
	}

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	/*
	 * Timeout instantly for SA_DISPATCH_ONE or CS_DISPATCH_ALL and
	 * wait indefinately for CS_DISPATCH_BLOCKING
	 */
	if (dispatch_types == CS_DISPATCH_ALL) {
		timeout = 0;
	}

	do {
		pthread_mutex_lock (&cman_inst->dispatch_mutex);

		dispatch_avail = cslib_dispatch_recv (cman_inst->ipc_ctx,
			(void *)&dispatch_data, timeout);

		pthread_mutex_unlock (&cman_inst->dispatch_mutex);

		if (error != CS_OK) {
			goto error_put;
		}

		if (dispatch_avail == 0 && dispatch_types == CMAN_DISPATCH_ALL) {
			pthread_mutex_unlock (&cman_inst->dispatch_mutex);
			break; /* exit do while cont is 1 loop */
		} else
		if (dispatch_avail == 0) {
			pthread_mutex_unlock (&cman_inst->dispatch_mutex);
			continue; /* next poll */
		}
		if (dispatch_avail == -1) {
			if (cman_inst->finalize == 1) {
				error = CS_OK;
			} else {
				error = CS_ERR_LIBRARY;
			}
			goto error_put;
		}
		pthread_mutex_unlock (&cman_inst->dispatch_mutex);

		/*
		 * Dispatch incoming message
		 */
		switch (dispatch_data.header.id) {

		case MESSAGE_RES_CMAN_SENDMSG:
			if (cman_inst->data_callback == NULL) {
				continue;
			}
			res_lib_cman_sendmsg = (struct res_lib_cman_sendmsg *)&dispatch_data;

			cman_inst->data_callback ( handle,
						   cman_inst->privdata,
						   res_lib_cman_sendmsg->message,
						   res_lib_cman_sendmsg->msglen,
						   res_lib_cman_sendmsg->from_port,
						   res_lib_cman_sendmsg->from_node);
			break;

		default:
			error = CS_ERR_LIBRARY;
			goto error_put;
			break;
		}

		/*
		 * Determine if more messages should be processed
		 * */
		switch (dispatch_types) {
		case CS_DISPATCH_ONE:
			cont = 0;
			break;
		case CS_DISPATCH_ALL:
			break;
		case CS_DISPATCH_BLOCKING:
			break;
		}
	} while (cont);

	goto error_put;

error_put:
	return (error);
}

/*
 * This call expects to get a listing of all nodes known to the
 * system so we query ccs rather than corosync, as some nodes
 * might not be up yet
 */
int cman_get_node_count(cman_handle_t handle)
{
	struct cman_inst *cman_inst;
	int ccs_handle;
	int num_nodes = 0;
	char path[PATH_MAX];
	char *value;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	/* Returns the number of nodes known to ccs, not cman! */
	ccs_handle = ccs_connect();

	sprintf(path, "/cluster/clusternodes/clusternode[%d]/@name", num_nodes+1);

	while (!ccs_get(ccs_handle, path, &value))
	{
		num_nodes++;
		sprintf(path, "/cluster/clusternodes/clusternode[%d]/@name", num_nodes+1);
	};

	ccs_disconnect(ccs_handle);
	return num_nodes;
}

int cman_is_active(cman_handle_t handle)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	/* If we have connected, then 'cman' is active */
	return 1;
}

/*
 * Here we just read values from ccs
 */
int cman_get_cluster(cman_handle_t handle, cman_cluster_t *clinfo)
{
	struct cman_inst *cman_inst;
	int ccs_handle;
	char *value;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	ccs_handle = ccs_connect();
	if (!ccs_get(ccs_handle, "/cluster/@name", &value)) {
		strcpy(clinfo->ci_name, value);
		free(value);
	}
	if (!ccs_get(ccs_handle, "/cluster/cman/@cluster_id", &value)) {
		clinfo->ci_number = atoi(value);
		free(value);
	}
	else {
		clinfo->ci_number = 0;
	}
	clinfo->ci_generation = 0; // CC: TODO ???

	ccs_disconnect(ccs_handle);

	return 0;
}

/*
 * libccs doesn't do writes yet so we need to use confdb to
 * change the config version.
 * This will signal votequorum to reload the configuration 'file'
 */
int cman_set_version(cman_handle_t handle, const cman_version_t *version)
{
	struct cman_inst *cman_inst;
	confdb_handle_t confdb_handle;
	unsigned int ccs_handle;
	char *value;
	char error[256];
	int ret = 0;
	int cur_version=0;
	confdb_callbacks_t callbacks = {
        	.confdb_key_change_notify_fn = NULL,
        	.confdb_object_create_change_notify_fn = NULL,
        	.confdb_object_delete_change_notify_fn = NULL
	};


	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	ccs_handle = ccs_connect();
	if (!ccs_get(ccs_handle, "/cluster/@config_version", &value)) {
		cur_version = atoi(value);
		free(value);
	}
	ccs_disconnect(ccs_handle);

	if (cur_version && cur_version >= version->cv_config) {
		errno = EINVAL;
		return -1;
	}

	if (confdb_initialize(&confdb_handle, &callbacks) != CS_OK) {
		errno = EINVAL;
		return -1;
	}

	if (confdb_reload(confdb_handle, 0, error) != CS_OK) {
		ret = EINVAL;
	}

	confdb_finalize(confdb_handle);
	return ret;
}


/* This mainly just retreives values from ccs */
int cman_get_version(cman_handle_t handle, cman_version_t *version)
{
	struct cman_inst *cman_inst;
	int ccs_handle;
	char *value;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	ccs_handle = ccs_connect();
	if (!ccs_get(ccs_handle, "/cluster/@config_version", &value)) {
		version->cv_config = atoi(value);
		free(value);
	}

	/* These are cman_tool versions now ;-) */
	version->cv_major = 7;
	version->cv_minor = 0;
	version->cv_patch = 1;
	ccs_disconnect(ccs_handle);

	return 0;
}



static char *node_name(corosync_cfg_node_address_t *addr)
{
	static char name[256];

	if (getnameinfo((struct sockaddr *)addr->address, addr->address_length, name, sizeof(name), NULL, 0, NI_NAMEREQD))
		return NULL;
	else
		return name;
}

/*
 * This is a slightly complicated mix of ccs and votequorum queries.
 * votequorum only knows about active nodes and does not hold node names.
 * so once we have a list of active nodes we fill in the names
 * and also the nodes that have never been seen by corosync.
 */
int cman_get_nodes(cman_handle_t handle, int maxnodes, int *retnodes, cman_node_t *nodes)
{
	struct cman_inst *cman_inst;
	int ccs_handle;
	char *value;
	int ret;
	int i;
	int num_nodes = 0;
	char path[PATH_MAX];
	int noconfig_flag=0;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	refresh_node_list(cman_inst);
	ccs_handle = ccs_connect();

	if (!ccs_get(ccs_handle, "/cluster/@no_config", &value)) {
		noconfig_flag = atoi(value);
		free(value);
	}

	/* If we don't have a config file we will have to look up node names */
	if (noconfig_flag) {
		int max_addrs = 4;
		corosync_cfg_node_address_t addrs[max_addrs];
		int num_addrs;
		char *name = NULL;
		int error;

		if (!cman_inst->cfg_handle) {
			if (corosync_cfg_initialize(&cman_inst->cfg_handle, &cfg_callbacks) != CS_OK) {
				errno = ENOMEM;
				return -1;
			}
		}

		for (i=0; i < cman_inst->node_count; i++) {
			nodes[i].cn_nodeid = cman_inst->node_list[i].nodeid;
			name = NULL;

			error = corosync_cfg_get_node_addrs(cman_inst->cfg_handle, nodes[i].cn_nodeid, max_addrs, &num_addrs, addrs);
			if (error == CS_OK) {
				name = node_name(&addrs[0]);
			}
			if (name) {
				sprintf(nodes[i].cn_name, "%s", name);
			}
			else {
				sprintf(nodes[i].cn_name, "Node-%x", nodes[i].cn_nodeid);
			}

			nodes[i].cn_member = (cman_inst->node_list[i].state == NODESTATE_MEMBER);
		}
	}
	else {

		/* We DO have a config file, reconcile in-memory with configuration */
		do {
			sprintf(path, "/cluster/clusternodes/clusternode[%d]/@name", num_nodes+1);
			ret = ccs_get(ccs_handle, path, &value);
			if (!ret) {
				strcpy(nodes[num_nodes].cn_name, value);
				free(value);
			}

			sprintf(path, "/cluster/clusternodes/clusternode[%d]/@nodeid", num_nodes+1);
			ret = ccs_get(ccs_handle, path, &value);
			if (!ret) {
				nodes[num_nodes].cn_nodeid = atoi(value);
				free(value);
			}

			/* Reconcile with active nodes list. */
			for (i=0; i < cman_inst->node_count; i++) {
				if (cman_inst->node_list[i].nodeid == nodes[num_nodes].cn_nodeid) {
					nodes[num_nodes].cn_member = (cman_inst->node_list[i].state == NODESTATE_MEMBER);
				}
			}

			num_nodes++;
		} while (ret == 0 && num_nodes < maxnodes);
	}

	*retnodes = num_nodes-1;
	if (cman_inst->node_count > *retnodes)
		*retnodes = cman_inst->node_count;
	ccs_disconnect(ccs_handle);
	return 0;
}

int cman_get_disallowed_nodes(cman_handle_t handle, int maxnodes, int *retnodes, cman_node_t *nodes)
{
	struct cman_inst *cman_inst;
	int i;
	int num_nodes = 0;
	int ccs_handle;
	char *value;
	int ret;
	char path[PATH_MAX];

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	refresh_node_list(cman_inst);
	ccs_handle = ccs_connect();

	for (i=0; i < cman_inst->node_count; i++) {
		if (cman_inst->node_list[i].state == NODESTATE_DISALLOWED) {
			nodes[num_nodes].cn_nodeid = cman_inst->node_list[i].nodeid;

			/* Find the name: */
			sprintf(path, "/cluster/clusternodes/clusternode[@nodeid=\"%d\"]/@name", cman_inst->node_list[i].nodeid);
			ret = ccs_get(ccs_handle, path, &value);
			if (!ret) {
				strcpy(nodes[num_nodes].cn_name, value);
				free(value);
			}
			else {
				sprintf(nodes[i].cn_name, "Node-%x", nodes[i].cn_nodeid);
			}
		}
	}
	*retnodes = num_nodes;
	ccs_disconnect(ccs_handle);
	return 0;

}

int cman_get_node(cman_handle_t handle, int nodeid, cman_node_t *node)
{
	struct cman_inst *cman_inst;
	struct votequorum_info qinfo;
	int i;
	int ccs_handle;
	int ret = 0;
	char *value;
	char path[PATH_MAX];

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	refresh_node_list(cman_inst);
	ccs_handle = ccs_connect();

	if (node->cn_name[0] == '\0') {
		/* Query by node ID */
		if (nodeid == CMAN_NODEID_US) {
			if (votequorum_getinfo(cman_inst->cmq_handle, 0, &qinfo) != CS_OK) {
				return -1;
			}
			nodeid = node->cn_nodeid = qinfo.node_id;
		}

		sprintf(path, "/cluster/clusternodes/clusternode[@nodeid=\"%d\"]/@name", nodeid);
		ret = ccs_get(ccs_handle, path, &value);
		if (!ret) {
			strcpy(node->cn_name, value);
			free(value);
		}
	}
	else {
		/* Query by node name */
		sprintf(path, "/cluster/clusternodes/clusternode[@name=\"%s\"]/@nodeid", node->cn_name);
		ret = ccs_get(ccs_handle, path, &value);
		if (!ret) {
			node->cn_nodeid = atoi(value);
			free(value);
		}
	}

	/* Fill in state */
	node->cn_member = 3; /* Not in cluster */
	for (i=0; i < cman_inst->node_count; i++) {
		if (cman_inst->node_list[i].nodeid == node->cn_nodeid) {
			if (cman_inst->node_list[i].state == NODESTATE_MEMBER)
				node->cn_member = 2;
		}
	}

	return 0;
}

int cman_start_notification(cman_handle_t handle, cman_callback_t callback)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	if (votequorum_check_and_start(cman_inst))
		return -1;

	cman_inst->notify_callback = callback;

	if (votequorum_trackstart(cman_inst->cmq_handle, (uint64_t)(long)handle, CS_TRACK_CURRENT) != CS_OK)
		return -1;

	return 0;
}

int cman_stop_notification(cman_handle_t handle)
{
	struct cman_inst *cman_inst;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	votequorum_trackstop(cman_inst->cmq_handle);
	cman_inst->notify_callback = NULL;

	return 0;
}


/* This is not complete ... but that's my problem, not yours */
int cman_get_extra_info(cman_handle_t handle, cman_extra_info_t *info, int maxlen)
{
	struct cman_inst *cman_inst;
	unsigned int ccs_handle;
	char *value;
	struct votequorum_info qinfo;

	cman_inst = (struct cman_inst *)handle;
	VALIDATE_HANDLE(cman_inst);

	refresh_node_list(cman_inst);

	if (votequorum_getinfo(cman_inst->cmq_handle, 0, &qinfo) != CS_OK) {
		return -1;
	}

	memset(info, 0, sizeof(cman_extra_info_t));
	info->ei_flags = qinfo.flags;
	info->ei_node_votes = qinfo.node_votes;
	info->ei_total_votes = qinfo.total_votes;
	info->ei_expected_votes = qinfo.node_expected_votes;
	info->ei_quorum = qinfo.quorum;
	info->ei_members = cman_inst->node_count;
	info->ei_node_state = 2;
	info->ei_num_addresses = 1;

	ccs_handle = ccs_connect();
	if (!ccs_get(ccs_handle, "/totem/interface/@mcastaddr", &value)) {
		strcpy(info->ei_addresses, value);
		free(value);
	}

	if (!ccs_get(ccs_handle, "/cluster/@no_config", &value)) {
		free(value);
		info->ei_flags |= CMAN_EXTRA_FLAG_NOCONFIG;
	}


	ccs_disconnect(ccs_handle);

	return 0;
}

