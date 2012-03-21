#include <lock.h>

int (*clu_lock)(int, struct dlm_lksb *, int, const char *) = NULL;
int (*clu_unlock)(struct dlm_lksb *lksb) = NULL;
void (*clu_lock_finished)(const char *) = NULL;

