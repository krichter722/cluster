#ifndef IPC_CMAN_H_DEFINED
#define IPC_CMAN_H_DEFINED

#include <netinet/in.h>

#define CMAN_SERVICE 9

// These don't
enum req_cman_types {
	MESSAGE_REQ_CMAN_SENDMSG = 0,
	MESSAGE_REQ_CMAN_IS_LISTENING,
	MESSAGE_REQ_CMAN_BIND,
	MESSAGE_REQ_CMAN_UNBIND
};

enum res_cman_types {
	MESSAGE_RES_CMAN_SENDMSG = 0,
	MESSAGE_RES_CMAN_IS_LISTENING,
	MESSAGE_RES_CMAN_BIND,
	MESSAGE_RES_CMAN_UNBIND
};

#define TOTEMIP_ADDRLEN (sizeof(struct in6_addr))

struct req_lib_cman_bind {
        coroipc_request_header_t header __attribute__((aligned(8)));
	unsigned int port;
};

struct req_lib_cman_sendmsg {
	coroipc_request_header_t header __attribute__((aligned(8)));
	unsigned int to_port;
	unsigned int to_node;
	unsigned int msglen;
	char message[];
};

struct res_lib_cman_sendmsg {
	coroipc_response_header_t header __attribute__((aligned(8)));
	unsigned int from_port;
	unsigned int from_node;
	unsigned int msglen;
	char message[];
};

struct req_lib_cman_is_listening {
	coroipc_request_header_t header __attribute__((aligned(8)));
	unsigned int port;
	unsigned int nodeid;
};

struct res_lib_cman_is_listening {
	coroipc_response_header_t header __attribute__((aligned(8)));
	unsigned int status;
};


#endif
