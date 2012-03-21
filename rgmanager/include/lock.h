#ifndef _LOCK_H
#define _LOCK_H

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libdlm.h>

/* Default lockspace wrappers */
int clu_lock_init(const char *default_lsname);
int cpg_lock_initialize(void);

extern int (*clu_lock)(int, struct dlm_lksb *, int, const char *);
extern int (*clu_unlock)(struct dlm_lksb *lksb);
extern void (*clu_lock_finished)(const char *);

#endif
