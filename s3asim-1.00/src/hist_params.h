/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _HIST_PARAMS_H
#define _HIST_PARAMS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "mpi.h"
#include <assert.h>
#include "test.h"

#define QUERY        0 
#define DATABASE     1
#define MAX_SEQUENCE 2

int read_hist_params(struct test_params_s *test_params_p, 
		     int sequence_type);

void print_hist_params(struct test_params_s *test_params_p,
		       int sequence_type);

int generate_rand_hist(int myid, 
		       unsigned int *seed_p,
		       int sequence_type,
		       struct work_info_s *work_info_p,
		       struct mpe_events_s *mpe_events_p,
		       struct test_params_s *test_params_p);
#endif
