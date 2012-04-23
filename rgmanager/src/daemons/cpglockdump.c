#include <stdio.h>
#include <cpglock.h>

int
main(int argc, char **argv)
{
	return cpg_lock_dump(stdout);
}
