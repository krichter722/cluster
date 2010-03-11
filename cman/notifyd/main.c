#include "clusterautoconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <corosync/cfg.h>
#include <corosync/quorum.h>
#include <corosync/confdb.h>
#include "liblogthread.h"
#include "ccs.h"

#include "copyright.cf"

int debug = 0;
int daemonize = 1;
int daemon_quit = 0;
int rr = 0;


#define LOCKFILE_NAME	CLUSTERVARRUN "/cmannotifyd.pid"

#define OPTION_STRING "hdfVr"

#ifndef MAX_ARGS
#define MAX_ARGS	128
#endif

static corosync_cfg_handle_t cfg_handle;
static quorum_handle_t quorum_handle;
static confdb_handle_t confdb_handle;

static void corosync_cfg_shutdown_callback (
        corosync_cfg_handle_t cfg_handle,
        corosync_cfg_shutdown_flags_t flags);

static void quorum_notification_callback (
        quorum_handle_t handle,
        uint32_t quorate,
        uint64_t ring_seq,
        uint32_t view_list_entries,
        uint32_t *view_list);

static void confdb_object_create_callback (
        confdb_handle_t handle,
        hdb_handle_t parent_object_handle,
        hdb_handle_t object_handle,
        const void *name_pt,
        size_t name_len);

static corosync_cfg_callbacks_t cfg_callbacks =
{
	.corosync_cfg_state_track_callback = NULL,
	.corosync_cfg_shutdown_callback = corosync_cfg_shutdown_callback
};

static quorum_callbacks_t quorum_callbacks =
{
	.quorum_notify_fn = quorum_notification_callback
};

static confdb_callbacks_t confdb_callbacks =
{
	.confdb_object_create_change_notify_fn = confdb_object_create_callback
};



static void print_usage(void)
{
	printf("Usage:\n\n");
	printf("cmannotifyd [options]\n\n");
	printf("Options:\n\n");
	printf("  -f        Do not fork in background\n");
	printf("  -d        Enable debugging output\n");
	printf("  -r        Run Real Time priority\n");
	printf("  -h        This help\n");
	printf("  -V        Print program version information\n");
	return;
}

static void read_arguments(int argc, char **argv)
{
	int cont = 1;
	int optchar;

	while (cont) {
		optchar = getopt(argc, argv, OPTION_STRING);

		switch (optchar) {

		case 'd':
			debug = 1;
			break;

		case 'f':
			daemonize = 0;
			break;

		case 'r':
			rr = 1;
			break;

		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			break;

		case 'V':
			printf("cmannotifyd %s (built %s %s)\n%s\n",
			       PACKAGE_VERSION, __DATE__, __TIME__,
			       REDHAT_COPYRIGHT);
			exit(EXIT_SUCCESS);
			break;

		case EOF:
			cont = 0;
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", optchar);
			print_usage();
			exit(EXIT_FAILURE);
			break;

		}

	}

	if (getenv("CMANNOTIFYD_DEBUG"))
		debug = 1;

}

static void remove_lockfile(void)
{
	unlink(LOCKFILE_NAME);
}

static void lockfile(void)
{
	int fd, error;
	struct flock lock;
	char buf[128];

	memset(buf, 0, 128);

	fd = open(LOCKFILE_NAME, O_CREAT | O_WRONLY,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		fprintf(stderr, "cannot open/create lock file %s\n",
			LOCKFILE_NAME);
		exit(EXIT_FAILURE);
	}

	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	error = fcntl(fd, F_SETLK, &lock);
	if (error) {
		fprintf(stderr, "cmannotifyd is already running\n");
		exit(EXIT_FAILURE);
	}

	error = ftruncate(fd, 0);
	if (error) {
		fprintf(stderr, "cannot clear lock file %s\n", LOCKFILE_NAME);
		exit(EXIT_FAILURE);
	}

	sprintf(buf, "%d\n", getpid());

	error = write(fd, buf, strlen(buf));
	if (error <= 0) {
		fprintf(stderr, "cannot write lock file %s\n", LOCKFILE_NAME);
		exit(EXIT_FAILURE);
	}

	atexit(remove_lockfile);
}

static void sigterm_handler(int sig)
{
	daemon_quit = 1;
}

static void set_oom_adj(int val)
{
	FILE *fp;

	fp = fopen("/proc/self/oom_adj", "w");
	if (!fp)
		return;

	fprintf(fp, "%i", val);
	fclose(fp);
}

static void set_scheduler(void)
{
	struct sched_param sched_param;
	int rv;

	rv = sched_get_priority_max(SCHED_RR);
	if (rv != -1) {
		sched_param.sched_priority = rv;
		rv = sched_setscheduler(0, SCHED_RR, &sched_param);
		if (rv == -1)
			logt_print(LOG_WARNING,
				   "could not set SCHED_RR priority %d err %d",
				   sched_param.sched_priority, errno);
	} else {
		logt_print(LOG_WARNING,
			   "could not get maximum scheduler priority err %d",
			   errno);
	}
}

static void init_logging(int reconf)
{
	int ccs_handle;
	int mode = LOG_MODE_OUTPUT_FILE | LOG_MODE_OUTPUT_SYSLOG;
	int syslog_facility = SYSLOGFACILITY;
	int syslog_priority = SYSLOGLEVEL;
	char logfile[PATH_MAX];
	int logfile_priority = SYSLOGLEVEL;

	memset(logfile, 0, PATH_MAX);
	sprintf(logfile, LOGDIR "/cmannotifyd.log");

	ccs_handle = ccs_connect();
	if (ccs_handle > 0) {
		ccs_read_logging(ccs_handle, "cmannotifyd", &debug, &mode,
				 &syslog_facility, &syslog_priority, &logfile_priority, logfile);
		ccs_disconnect(ccs_handle);
	}

	if (!daemonize)
		mode |= LOG_MODE_OUTPUT_STDERR;

	if (!reconf)
		logt_init("cmannotifyd", mode, syslog_facility, syslog_priority, logfile_priority, logfile);
	else
		logt_conf("cmannotifyd", mode, syslog_facility, syslog_priority, logfile_priority, logfile);
}

static void dispatch_notification(const char *str, int *quorum)
{
	char *envp[MAX_ARGS];
	char *argv[MAX_ARGS];
	int envptr = 0;
	int argvptr = 0;
	char scratch[PATH_MAX];
	pid_t notify_pid;
	int pidstatus;
	int err = 0;

	if (!str)
		return;

	/* pass notification type */
	snprintf(scratch, sizeof(scratch), "CMAN_NOTIFICATION=%s", str);
	envp[envptr++] = strdup(scratch);

	if (quorum) {
		snprintf(scratch, sizeof(scratch), "CMAN_NOTIFICATION_QUORUM=%d", *quorum);
		envp[envptr++] = strdup(scratch);
	}

	if (debug)
		envp[envptr++] = strdup("CMAN_NOTIFICATION_DEBUG=1");

	envp[envptr--] = NULL;

	argv[argvptr++] = strdup("cman_notify");

	argv[argvptr--] = NULL;

	switch ( (notify_pid = fork()) )
	{
		case -1:
			/* unable to fork */
			err = 1;
			goto out;
			break;

		case 0: /* child */
			execve(SBINDIR "/cman_notify", argv, envp);
			/* unable to execute cman_notify */
			err = 1;
			goto out;
			break;

		default: /* parent */
			waitpid(notify_pid, &pidstatus, 0);
			break;
	}

out:
	while(envptr >= 0) {
		if (envp[envptr])
			free(envp[envptr]);

		envptr--;
	}
	while(argvptr >= 0) {
		if (argv[argvptr])
			free(argv[argvptr]);

		argvptr--;
	}
	if (err)
		exit(EXIT_FAILURE);
}

static void corosync_cfg_shutdown_callback(corosync_cfg_handle_t handle,
					   corosync_cfg_shutdown_flags_t flags)
{
	logt_print(LOG_DEBUG, "Received a CFG shutdown request\n");

	corosync_cfg_replyto_shutdown(handle, COROSYNC_CFG_SHUTDOWN_FLAG_YES);

	dispatch_notification("CMAN_REASON_TRY_SHUTDOWN", 0);
}


static void quorum_notification_callback(quorum_handle_t handle,
					 uint32_t quorate,
					 uint64_t ring_seq,
					 uint32_t view_list_entries,
					 uint32_t *view_list)
{
	logt_print(LOG_DEBUG,
		   "Received a quorum notification\n");
	dispatch_notification("CMAN_REASON_STATECHANGE", quorate);
}

static void confdb_object_create_callback(confdb_handle_t handle,
					  hdb_handle_t parent_object_handle,
					  hdb_handle_t object_handle,
					  const void *name_pt,
					  size_t name_len)
{
	logt_print(LOG_DEBUG,
		   "Received a config update notification\n");
	init_logging(1);
	dispatch_notification("CMAN_REASON_CONFIG_UPDATE", 0);
}

static void byebye_corosync(void)
{
	if (cfg_handle)
	{
		corosync_cfg_finalize(cfg_handle);
		cfg_handle = NULL;
	}
	if (quorum_handle)
	{
		quorum_finalize(quorum_handle);
		quorum_handle = NULL;
	}
	if (confdb_handle)
	{
		confdb_finalize(confdb_handle);
		confdb_handle = NULL;
	}
}

static void setup_corosync(int forever)
{
	int init = 0, active = 0;
	cs_error_t cs_err;

retry_init:

	if (!cfg_handle) {
		cs_err = corosync_cfg_initialize(&cfg_handle, &cfg_callbacks);
		if (cs_err != CS_OK)
			goto init_fail;
	}

	if (!quorum_handle) {
		cs_err = quorum_initialize(&quorum_handle, &quorum_callbacks);
		if (cs_err != CS_OK)
			goto init_fail;
		cs_err = quorum_trackstart(quorum_handle, CS_TRACK_CHANGES);
	}

	if (!confdb_handle) {
		cs_err = confdb_initialize(&confdb_handle, &confdb_callbacks);
		if (cs_err != CS_OK)
			goto init_fail;
		// TODO track changes
	}
	goto out_ok;

init_fail:
	if ((init++ < 5) || (forever)) {
		if (daemon_quit)
			goto out;

		sleep(1);
		goto retry_init;
	}
	logt_print(LOG_CRIT, "corosync_init error %d\n", errno);
	exit(EXIT_FAILURE);

out:
	byebye_corosync();
out_ok:
	exit(EXIT_SUCCESS);
}

static void loop(void)
{
	cs_error_t cs_result;
	int select_result;
	fd_set read_fds;
	int cfg_fd;
	int quorum_fd;
	int confdb_fd;

	do {
		FD_ZERO (&read_fds);
		corosync_cfg_fd_get(cfg_handle, &cfg_fd);
		confdb_fd_get(confdb_handle, &confdb_fd);
		quorum_fd_get(quorum_handle, &quorum_fd);
		FD_SET (cfg_fd, &read_fds);
		FD_SET (confdb_fd, &read_fds);
		FD_SET (quorum_fd, &read_fds);
		select_result = select(FD_SETSIZE, &read_fds, 0, 0, 0);

		if (daemon_quit)
			goto out;

		if (select_result == -1) {
			logt_print(LOG_CRIT, "Unable to select on cman_fd: %s\n", strerror(errno));
			byebye_corosync();
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(cfg_fd, &read_fds)) {
			cs_result = CS_OK;
			while (cs_result == CS_OK) {
				cs_result = corosync_cfg_dispatch(cfg_handle, CS_DISPATCH_ONE);
				if (cs_result != CS_OK) {
					byebye_corosync();
					logt_print(LOG_DEBUG, "waiting for corosync to reappear..\n");
					setup_corosync(1);
					logt_print(LOG_DEBUG, "corosync is back..\n");
				}
			}
		}
		if (FD_ISSET(confdb_fd, &read_fds)) {
			cs_result = CS_OK;
			while (cs_result == CS_OK) {
				cs_result = confdb_dispatch(confdb_handle, CS_DISPATCH_ONE);
				if (cs_result != CS_OK) {
					byebye_corosync();
					logt_print(LOG_DEBUG, "waiting for corosync to reappear..\n");
					setup_corosync(1);
					logt_print(LOG_DEBUG, "corosync is back..\n");
				}
			}
		}
		if (FD_ISSET(quorum_fd, &read_fds)) {
			cs_result = CS_OK;
			while (cs_result == CS_OK) {
				cs_result = quorum_dispatch(quorum_handle, CS_DISPATCH_ONE);
				if (cs_result != CS_OK) {
					byebye_corosync();
					logt_print(LOG_DEBUG, "waiting for corosync to reappear..\n");
					setup_corosync(1);
					logt_print(LOG_DEBUG, "corosync is back..\n");
				}
			}
		}
	} while (select_result && !daemon_quit);

out:
	logt_print(LOG_DEBUG, "shutting down...\n");
	byebye_corosync();
}

int main(int argc, char **argv)
{

	read_arguments(argc, argv);
	if (daemonize) {
		if (daemon(0, 0) < 0) {
			perror("Unable to daemonize");
			exit(EXIT_FAILURE);
		}
	}
	lockfile();
	init_logging(0);
	signal(SIGTERM, sigterm_handler);
	set_oom_adj(-16);
	if (rr)
		set_scheduler();

	setup_corosync(0);
	loop();

	return 0;
}
