/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "icomm.h"

#define ISEND     0
#define IRECV     1
#define MAX_ICOMM 2

const static char *decode_icomm[MAX_ICOMM] =
    {
        "ISEND",
	"IRECV"
    };

/**
 * Check either an isend or irecv request to gauge progress.  This
 * function is called by check_irecv() and check_isend() functions.
 *
 * @param icomm_type     Either ISEND or IRECV.
 * @param myid           MPI myid.
 * @param wait_status    Status of the master/worker processes.
 * @param flag_p         Pointer to request completion.
 * @param req_p          Pointer to MPI request.
 * @param req_status_p   Pointer to current state of request.
 * @param new_req_status The next state for the request (if successful).
 * @param mpe_events_p   Pointer to timing structure.
 * @param test_params_p  Pointer to test_params. 
 * @return               0 on success.
 */
static int check_isendrecv(int icomm_type,
			   int myid,
			   int wait_status,
			   int *flag_p,
			   MPI_Request *req_p,
			   int *req_status_p,
			   int new_req_status,
			   struct mpe_events_s *mpe_events_p,
			   struct test_params_s *test_params_p)
{
    MPI_Status status;
    int err;
    uint64_t debug_mask;
    
    if (icomm_type == ISEND)
    {
	if (myid == MASTER_NODE)
	    debug_mask = MASTER_ISEND;
	else
	    debug_mask = WORKER_ISEND;
    }
    else
    {
	if (myid == MASTER_NODE)
            debug_mask = MASTER_IRECV;
        else
            debug_mask = WORKER_IRECV;
    }

    *flag_p = 0;
    
    if (wait_status == CUR_QUERY_SCHED_DONE ||
        wait_status == ALL_QUERY_SCHED_DONE)
    {
        err = MPI_Wait(req_p, &status);
        *flag_p = 1;
    }
    else
    {
        err = MPI_Test(req_p, flag_p, &status);
    }

    if (*flag_p == 1)
    {
	if (myid == MASTER_NODE)
	    custom_debug(debug_mask,
			 "M:%s req_p completed\n", 
			 decode_icomm[icomm_type]);
	else
	    custom_debug(debug_mask,
                         "W%d:%s req_p completed\n", myid,
			 decode_icomm[icomm_type]);

	*req_p = MPI_REQUEST_NULL;
	*req_status_p = new_req_status;
    }
    else
    {
	if (myid == MASTER_NODE)
	    custom_debug(debug_mask,
			 "M:%s req_p incomplete\n", 
			 decode_icomm[icomm_type]);
	else
	    custom_debug(debug_mask,
                         "W%d:req_p incomplete\n", myid,
			 decode_icomm[icomm_type]);
    }
    
    return 0;
}

/**
 * Check isend request, shared by both the master and workers.  It
 * depends heavily on check_isendrecv().
 *
 * @param myid           MPI myid.
 * @param wait_status    Status of the master/worker processes.
 * @param flag_p         Pointer to request completion.
 * @param req_p          Pointer to MPI request.
 * @param req_status_p   Pointer to current state of request.
 * @param new_req_status The next state for the request (if successful).
 * @param mpe_events_p   Pointer to timing structure.
 * @param test_params_p  Pointer to test_params. 
 * @return               0 on success.
 */
int check_isend(int myid,
		int wait_status,
		int *flag_p,
		MPI_Request *req_p,
		int *req_status_p,
		int new_req_status,
		struct mpe_events_s *mpe_events_p,
		struct test_params_s *test_params_p)
{
    return check_isendrecv(ISEND,
			   myid,
			   wait_status,
			   flag_p,
			   req_p,
			   req_status_p,
			   new_req_status,
			   mpe_events_p,
			   test_params_p);
}

/**
 * Check irecv request, shared by both the master and workers.  It
 * depends heavily on check_isendrecv().
 *
 * @param myid           MPI myid.
 * @param wait_status    Status of the master/worker processes.
 * @param flag_p         Pointer to request completion.
 * @param req_p          Pointer to MPI request.
 * @param req_status_p   Pointer to current state of request.
 * @param new_req_status The next state for the request (if successful).
 * @param mpe_events_p   Pointer to timing structure.
 * @param test_params_p  Pointer to test_params. 
 * @return               0 on success.
 */
int check_irecv(int myid,
		int wait_status,
		int *flag_p,
		MPI_Request *req_p,
		int *req_status_p,
		int new_req_status,
		struct mpe_events_s *mpe_events_p,
		struct test_params_s *test_params_p)
{
    return check_isendrecv(IRECV,
			   myid,
			   wait_status,
			   flag_p,
			   req_p,
			   req_status_p,
			   new_req_status,
			   mpe_events_p,
			   test_params_p);
}
