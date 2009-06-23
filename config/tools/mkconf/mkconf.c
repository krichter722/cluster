#include "clusterautoconfig.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <corosync/corotypes.h>
#include <corosync/quorum.h>
#include <corosync/confdb.h>
#include <corosync/cfg.h>
#include <ccs.h>

struct node_info {
	uint32_t nodeid;
	char name[256];
};

static uint32_t node_list_size;
static struct node_info *node_list;


static char *node_name(corosync_cfg_node_address_t *addr)
{
	static char name[256];

	if (getnameinfo((struct sockaddr *)addr->address, addr->address_length, name, sizeof(name),
			NULL, 0, NI_NAMEREQD))
		return NULL;
	else
		return name;
}

static char *ip_address(corosync_cfg_node_address_t *addr)
{
	static char name[256];
	struct sockaddr *sa = (struct sockaddr *)addr->address;
	char *addrpart;

	if (sa->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		addrpart = (char *)&sin->sin_addr;
	}
	else {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)sa;
		addrpart = (char *)&sin->sin6_addr;
	}

	if (inet_ntop(sa->sa_family, addrpart, name, sizeof(name)))
		return name;
	else
		return NULL;
}

static void quorum_notification_callback(
	quorum_handle_t handle,
        uint32_t quorate,
        uint64_t ring_seq,
	uint32_t view_list_entries,
	uint32_t *view_list)
{
	int i;

	node_list = malloc(sizeof(struct node_info) * view_list_entries);
	if (node_list) {
		for (i=0; i<view_list_entries; i++) {
			node_list[i].nodeid = view_list[i];
		}
		node_list_size = view_list_entries;
	}
}


static int refresh_node_list(int use_ip_addrs)
{
	int error;
	int i;
	quorum_handle_t quorum_handle;
	corosync_cfg_handle_t cfg_handle;
	quorum_callbacks_t quorum_callbacks = {.quorum_notify_fn = quorum_notification_callback};
	int max_addrs = 4;
	corosync_cfg_node_address_t addrs[max_addrs];
	int num_addrs;
	char *name = NULL;

	if (quorum_initialize(&quorum_handle, &quorum_callbacks) != CS_OK) {
		errno = ENOMEM;
		return -1;
	}
	if (corosync_cfg_initialize(&cfg_handle, NULL) != CS_OK) {
		quorum_finalize(quorum_handle);
		errno = ENOMEM;
		return -1;
	}

	quorum_trackstart(quorum_handle, CS_TRACK_CURRENT);

	error = quorum_dispatch(quorum_handle, CS_DISPATCH_ONE);
	if (error != CS_OK)
		return -1;

	quorum_finalize(quorum_handle);

	for (i=0; i < node_list_size; i++) {

		error = corosync_cfg_get_node_addrs(cfg_handle, node_list[i].nodeid, max_addrs, &num_addrs, addrs);
		if (error == CS_OK) {
			if (use_ip_addrs)
				name = ip_address(&addrs[0]);
			else
				name = node_name(&addrs[0]);
		}
		if (name) {
			sprintf(node_list[i].name, "%s", name);
		}
		else {
			sprintf(node_list[i].name, "Node-%x", node_list[i].nodeid);
		}

	}
	corosync_cfg_finalize(cfg_handle);
	return 0;
}

static void usage(char *prog)
{
	printf("Usage:\n\n");
	printf("  %s [options]\n", prog);
	printf("\n");

	printf("    -N          Generate sequential nodeids\n");
	printf("    -n <name>   Use this cluster name\n");
	printf("    -i          Use IP Addresses rather than resolved node names\n");
	printf("    -f <name>   Fence agent name (default: 'default')\n");
	printf("    -F <name>   Fence agent parameter name (default: 'ipaddr'\n");
	printf("    -v <num>    Config version number (default: 0)\n");

	printf("\n");

	printf("NOTE: It is stringly recommended that the existing cluster is shut down\n");
	printf("      completely before restarting any nodes using the generated file\n");

	printf("\n");
}

int main(int argc, char *argv[])
{
	unsigned int ccs_handle;
	char *value;
	int optchar;
	int nodeid = 0;
	int i;

	int seq_nodeids=0;
	int use_ip_addrs=0;
	int config_version = 0;
	char *cluster_name = NULL;
	const char *fence_type = "default";
	const char *fence_param = "ipaddr";
	const char *fence_agent = "fence_manual";

	/* Parse options... */
	do {
		optchar = getopt(argc, argv, "?hNn:if:F:v:a:");
		switch (optchar) {
		case 'N':
			seq_nodeids=1;
			break;
		case 'n':
			cluster_name = strdup(optarg);
			break;
		case 'i':
			use_ip_addrs=1;
			break;
		case 'f':
			fence_type = strdup(optarg);
			break;
		case 'a':
			fence_agent = strdup(optarg);
			break;
		case 'F':
			fence_param = strdup(optarg);
			break;
		case 'v':
			config_version = atoi(optarg);
			break;
		case '?':
		case 'h':
			usage(argv[0]);
			exit(0);
		case EOF:
			break;

		}
	} while (optchar != EOF);


	/* Get the list of nodes and names */
	if (refresh_node_list(use_ip_addrs)){
		fprintf(stderr, "Unable to get node information from corosync\n");
		return 1;
	}

	ccs_handle = ccs_connect();

	if (!cluster_name) {
		if (!ccs_get(ccs_handle, "/cluster/@name", &value)) {
			cluster_name = strdup(value);
			free(value);
		}
	}

	/* Print config file header */
	printf("<?xml version=\"1.0\" ?>\n");
	printf("<cluster name=\"%s\" config_version=\"0\">\n", cluster_name);
	printf("  <clusternodes>\n\n");
	for (i=0; i<node_list_size; i++) {
		printf("    <clusternode name=\"%s\" nodeid=\"%u\">\n", node_list[i].name,
		       seq_nodeids?++nodeid:node_list[i].nodeid);
		printf("      <fence>\n");
		printf("        <method name=\"single\">\n");
		printf("          <device name=\"%s\" %s=\"%s\"/>\n", fence_type, fence_param,node_list[i].name);
		printf("        </method>\n");
		printf("      </fence>\n");
		printf("    </clusternode>\n");
		printf("\n");
	}
	printf("  </clusternodes>\n");

	/* Make up something for fence devices */
	printf("<fencedevices>\n");

	printf("  <fencedevice name=\"%s\"> agent=\"%s\"/>\n", fence_type, fence_agent);

	printf("</fencedevices>\n");


	printf("</cluster>\n");

	ccs_disconnect(ccs_handle);
	return 0;
}
