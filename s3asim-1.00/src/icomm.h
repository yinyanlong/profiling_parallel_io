/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _ICOMM_H
#define _ICOMM_H

#include "test.h"

int check_isend(int myid,
		int wait_status,
		int *flag_p,
		MPI_Request *req_p,
		int *req_status_p,
		int new_req_status,
		struct mpe_events_s *mpe_events_p,
		struct test_params_s *test_params_p);

int check_irecv(int myid,
		int wait_status,
		int *flag_p,
		MPI_Request *req_p,
		int *req_status_p,
		int new_req_status,
		struct mpe_events_s *mpe_events_p,
		struct test_params_s *test_params_p);

#endif
