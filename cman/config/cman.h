
/* How we announce ourself in syslog */
#define CMAN_NAME "CMAN"

#define MAX_CLUSTER_MEMBER_NAME_LEN 255

/* Defaults for configuration variables */
#define NOCCS_KEY_FILENAME      DEFAULT_CONFIG_DIR "/cman_authkey"
#define DEFAULT_PORT            5405
#define DEFAULT_CLUSTER_NAME    "RHCluster"
#define DEFAULT_MAX_QUEUED       128
#define DEFAULT_QUORUMDEV_POLL   10000
#define DEFAULT_SHUTDOWN_TIMEOUT 5000
#define DEFAULT_CCSD_POLL        1000
