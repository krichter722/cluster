#ifndef _LIBFENCE_H_
#define _LIBFENCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FE_AGENT_SUCCESS	1	/* agent exited with EXIT_SUCCESS */
#define FE_AGENT_ERROR		2	/* agent exited with EXIT_FAILURE */
#define FE_AGENT_FORK		3	/* error forking agent */
#define FE_NO_CONFIG		4	/* ccs_connect error */
#define FE_NO_METHOD		5	/* zero methods defined */
#define FE_NO_DEVICE		6	/* zero devices defined in method */
#define FE_READ_AGENT		7	/* read (ccs) error on agent path */
#define FE_READ_ARGS		8	/* read (ccs) error on node/dev args */
#define FE_READ_METHOD		9	/* read (ccs) error on method */
#define FE_READ_DEVICE		10	/* read (ccs) error on method/device */
#define FE_NUM_METHOD		11	/* method number does not exist */
#define FE_AGENT_STATUS_ON	12
#define FE_AGENT_STATUS_OFF	13
#define FE_AGENT_STATUS_ERROR	14

#define FENCE_AGENT_NAME_MAX 256	/* including terminating \0 */
#define FENCE_AGENT_ARGS_MAX 4096	/* including terminating \0 */

struct fence_log {
	int error;
	int method_num;
	int device_num;
	char agent_name[FENCE_AGENT_NAME_MAX];
	char agent_args[FENCE_AGENT_ARGS_MAX];
};

int fence_node(char *name, struct fence_log *log, int log_size, int *log_count);

int unfence_node(char *name, struct fence_log *log, int log_size,
		 int *log_count);

/*
 * use_method_num == 0: run status on all devices of all methods
 * use_method_num > 0: run status on all devices of given method number,
 *                     where first method is use_method_num = 1
 *
 * Returns 0 on success: status is successful on all devices of all methods
 * (or all devices of specified method).  All devices are in the "on" state,
 * or some devices are on and some are off.
 *
 * Returns 2 on success: status is successful on all devices of all methods
 * (or all devices of a specified method).  All devices are in the "off" state.
 *
 * Returns -2 if no fencing methods are defined for the node, or if
 * use_method_num was specified and the specified method number does
 * not exist.
 *
 * Returns -EXXX for other failures.
 */

int fence_node_status(char *victim, struct fence_log *log, int log_size,
                      int *log_count, int use_method_num);

#ifdef __cplusplus
}
#endif

#endif
