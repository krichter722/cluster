/** @file
 * Locking.
 */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <lock.h>
#include <sys/types.h>
#include <sys/select.h>
#include <pthread.h>

#include <cpglock.h>

/* Default lockspace stuff */
static cpg_lock_handle_t _cpgh = NULL;
static pthread_mutex_t _default_lock = PTHREAD_MUTEX_INITIALIZER;

static void
dlm2cpg(struct dlm_lksb *dlm, struct cpg_lock *cpg)
{
	memset(cpg, 0, sizeof(*cpg));
	cpg->local_id = dlm->sb_lkid;
	switch(dlm->sb_status) {
	case 0:
		cpg->state = LOCK_HELD;
		break;
	case EINPROG:
		cpg->state = LOCK_PENDING;
		break;
	default:
		cpg->state = LOCK_FREE;
		break;
	}
}

static void
cpg2dlm(struct cpg_lock *cpg, struct dlm_lksb *dlm)
{
	memset(dlm, 0, sizeof(*dlm));
	dlm->sb_lkid = cpg->local_id;
	switch(cpg->state) {
	case LOCK_HELD:
		dlm->sb_status = 0;
		break;
	case LOCK_PENDING:
	default:
		/* XXX LOCK_FREE -> DLM state? */
		dlm->sb_status = EINPROG;
		break;
	}
}


static int
_cpg_lock(int mode,
	  struct dlm_lksb *lksb,
	  int options,
          const char *resource)
{
	int ret = 0;

	struct cpg_lock l;

	if (options == LKF_NOQUEUE) 
		ret = cpg_lock(_cpgh, resource, 1, &l);
	else
		ret = cpg_lock(_cpgh, resource, 0, &l);
	
	if (ret == 0) {
		cpg2dlm(&l, lksb);
	}

	return ret;
}


static int
_cpg_unlock(struct dlm_lksb *lksb)
{
	struct cpg_lock l;

	dlm2cpg(lksb, &l);
	return cpg_unlock(_cpgh, &l);
}


static void
_cpg_lock_finished(const char *name)
{
	pthread_mutex_lock(&_default_lock);
	cpg_lock_fin(_cpgh);
	pthread_mutex_unlock(&_default_lock);
}


int
cpg_lock_initialize(void)
{
	int ret, err;

	pthread_mutex_lock(&_default_lock);
	if (_cpgh) {
		pthread_mutex_unlock(&_default_lock);
		return 0;
	}

	cpg_lock_init(&_cpgh);
	ret = (_cpgh == NULL);
	err = errno;
	pthread_mutex_unlock(&_default_lock);

	/* Set up function pointers */
	clu_lock = _cpg_lock;
	clu_unlock = _cpg_unlock;
	clu_lock_finished = _cpg_lock_finished;

	errno = err;
	return ret;
}
