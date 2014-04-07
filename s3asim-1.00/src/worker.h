/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _WORKER_H
#define _WORKER_H

#include "test.h"

int worker(int myid,
           int numprocs,
           struct mpe_events_s *mpe_events_p,
           struct test_params_s *test_params_p);

#endif
