#include "config.h"

#include <sys/types.h>
#include <sys/un.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>

#include "copyright.cf"

#define OP_LIST				1
#define OP_DUMP				2

static char *prog_name;
static int operation;
static int opt_ind;
static int verbose;
static int ls_all_nodes;

static void print_usage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("%s [options] [ls|dump]\n", prog_name);
	printf("\n");
	printf("Options:\n");
	printf("  -v               Verbose output, extra information\n");
	printf("  -n               Show all node information\n");
	printf("  -h               Print this help, then exit\n");
	printf("  -V               Print program version information, then exit\n");
	printf("\n");
	printf("Display debugging information\n");
	printf("dump fence         Show debug log from fenced\n");
	printf("dump dlm           Show debug log from dlm_controld\n");
	printf("dump gfs           Show debug log from gfs_controld\n");
	printf("\n");
}

#define OPTION_STRING "hVvn"

static void decode_arguments(int argc, char **argv)
{
	int cont = 1;
	int optchar;

	while (cont) {
		optchar = getopt(argc, argv, OPTION_STRING);

		switch (optchar) {
		case 'n':
			ls_all_nodes = 1;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			break;

		case 'V':
			printf("%s %s (built %s %s)\n",
				prog_name, PACKAGE_VERSION, __DATE__, __TIME__);
			printf("%s\n", REDHAT_COPYRIGHT);
			exit(EXIT_SUCCESS);
			break;

		case ':':
		case '?':
			fprintf(stderr, "Please use '-h' for usage.\n");
			exit(EXIT_FAILURE);
			break;

		case EOF:
			cont = 0;
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", optchar);
			exit(EXIT_FAILURE);
			break;
		};
	}

	while (optind < argc) {
		if (strcmp(argv[optind], "dump") == 0) {
			operation = OP_DUMP;
			opt_ind = optind + 1;
			break;
		} else if (strcmp(argv[optind], "ls") == 0 ||
		           strcmp(argv[optind], "list") == 0) {
			operation = OP_LIST;
			opt_ind = optind + 1;
			break;
		}
		optind++;
	}

	if (!operation)
		operation = OP_LIST;
}

int main(int argc, char **argv)
{
	prog_name = argv[0];
	decode_arguments(argc, argv);

	switch (operation) {
	case OP_LIST:
		if (verbose || ls_all_nodes) {
			system("fence_tool ls -n");
			system("dlm_tool ls -n");
			system("gfs_control ls -n");
		} else {
			system("fence_tool ls");
			system("dlm_tool ls");
			system("gfs_control ls");
		}
		break;

	case OP_DUMP:
		if (opt_ind && opt_ind < argc) {
			if (!strncmp(argv[opt_ind], "gfs", 3))
				system("gfs_control dump");

			if (!strncmp(argv[opt_ind], "dlm", 3))
				system("dlm_tool dump");

			if (!strncmp(argv[opt_ind], "fence", 5))
				system("fence_tool dump");
		}
		break;
	}

	return 0;
}

