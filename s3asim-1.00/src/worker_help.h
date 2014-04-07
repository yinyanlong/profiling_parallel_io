/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _WORKER_HELP_H
#define _WORKER_HELP_H

#include "llist.h"
#include "hist_params.h"

#define NOT_USED               0

/* State for isend */
#define WAIT_RESULT_SIZE       1
#define WAIT_RESULT            2
#define DONE_RESULT            3

/* State for irecv */
#define START_IRECV            4
#define WAIT_OFFSET_LIST_COUNT 5
#define WAIT_OFFSET_LIST       6
#define DONE_OFFSET_LIST       7
#define IO_FINISHED            8
#define MAX_WORKER_STATE       9

extern int64_t global_debug_mask;
char *decode_req[MAX_REQ];

struct query_result_s
{
    int64_t lresult_total_size;
    struct lresult_s *lresult_head; /**< Merged linked list of all results */
    int lresult_count;

    int state;
    int64_t offset_list_count; /**< Count of offsets to be used */
    int64_t *offset_list;      /**< List of offsets for writing a
			        * query to file.  The count should be
			        * the same as lresult_count. */
    MPI_Request offset_list_count_req;
    MPI_Request offset_list_req;
};

struct isend_info_s
{
    int tmp_result_list_count;
    struct result_s *tmp_result_list;
    
    int state;
    int64_t result_size;
    char *result;

    MPI_Request result_size_req;
    MPI_Request result_req;
};

int get_test_params(int myid,
		    int numprocs,
		    struct mpe_events_s *mpe_events_p,
		    struct test_params_s *test_params_p);

int send_recv_work_req(int myid,
		       int *all_done_p,
		       struct work_info_s *work_info_p,
		       struct mpe_events_s *mpe_events_p,
		       struct test_params_s *test_params_p);
void print_result(struct result_s *cur_result_p);

int do_work(int myid,
	    struct work_info_s *work_info_p,
	    struct isend_info_s **query_frag_isend_matrix,
	    struct mpe_events_s *mpe_events_p,
	    struct test_params_s *test_params_p);

int merge_results(int myid,
		  struct work_info_s *work_info_p,
		  struct query_result_s *query_result_list,
		  struct isend_info_s **query_frag_isend_matrix,
		  struct mpe_events_s *mpe_events_p,
		  struct test_params_s *test_params_p);

int worker_isend_results(int myid,
			 struct work_info_s *work_info_p,
			 int *last_query_offset_recv_p,
			 struct isend_info_s **query_frag_isend_matrix,
			 struct mpe_events_s *mpe_events_p,
			 struct test_params_s *test_params_p);

int worker_check_isend_results(int myid,
			       int wait_status, 
			       struct work_info_s *last_query_result_sent_p,
			       struct work_info_s *last_query_result_comp_p,
			       struct query_result_s *query_result_list,
			       struct isend_info_s **query_frag_isend_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p);

int worker_post_irecv(int myid,
		      struct work_info_s *work_info_p,
		      struct query_result_s *query_result_list,
		      struct mpe_events_s *mpe_events_p,
		      struct test_params_s *test_params_p);

int worker_check_irecv_offsets(int myid,
			       int wait_status,
			       int last_query_offset_recv,
			       int *last_query_offset_comp_p,
			       struct query_result_s *query_result_list,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p);

int do_all_worker_io(int myid,
		     int wait_status,
		     int last_io_ready,
		     int *last_io_comp_p,
		     struct query_result_s *query_result_list,
		     struct mpe_events_s *mpe_events_p,
		     struct test_params_s *test_params_p);

void worker_check_all_irecv(int myid,
			    int numprocs,
			    struct query_result_s *query_result_list,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p);

void worker_check_all_isend(int myid,
			    int numprocs,
			    struct isend_info_s **query_frag_isend_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p);
#endif
