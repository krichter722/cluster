#include <stdio.h>
#include <time.h>

/* XXX "list_head()", reslist.h should include on its own? */
#include "list.h"
/* XXX "restart_counter_t", dtto */
#include "restart_counter.h"

#include "reslist.h"

int main(int argc, char *argv[])
{
	char buffer[255];
	char *aux;
	for (;;) {
		printf("Time string: ");
		if (!fgets(buffer, sizeof(buffer), stdin) && feof(stdin))
			break;
		aux = strchr(buffer,'\n');
		if (aux)
			*aux = '\0';
		printf("Expanded   : %d\n", expand_time(buffer));
	}
	return EXIT_SUCCESS;
}
