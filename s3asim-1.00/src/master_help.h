/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _MASTER_HELP_H
#define _MASTER_HELP_H

#include "llist.h"

#define NOT_USED               0

/* State for Irecv */
#define WAIT_RESULT_SIZE       1
#define WAIT_RESULT_DATA       2
#define DONE_RESULT            3

/* State for Isend */
#define WAIT_OFFSET_LIST_COUNT 4
#define WAIT_OFFSET_LIST       5
#define DONE_OFFSET_LIST       6

extern int64_t global_debug_mask;
char *decode_req[MAX_REQ];

struct query_info_s
{
    int frag_sched;
    int frag_comp;
    struct lresult_s *lresult_head;
};

struct isend_info_s 
{
    int state;
    int64_t offset_list_count;
    int64_t *offset_list;

    MPI_Request offset_list_count_req;
    MPI_Request offset_list_req;
};

struct irecv_info_s
{
    int worker;

    int state;
    int64_t result_size;
    char *result;

    MPI_Request result_size_req;
    MPI_Request result_req;
};

int send_work(int worker,
	      int *cur_query_p,
	      struct work_info_s *work_info_p,
	      struct query_info_s  *query_info_list,
	      struct mpe_events_s *mpe_events_p,
	      struct test_params_s *test_params_p);

int post_irecv(int worker,
	       struct work_info_s *work_info_p,
	       struct irecv_info_s **query_frag_irecv_matrix,
	       struct mpe_events_s *mpe_events_p,
	       struct test_params_s *test_params_p);

int master_check_irecv_results(int cur_query,
			       int *last_irecv_result_done_p,
			       int wait_status,
			       struct query_info_s  *query_info_list,
			       struct irecv_info_s **query_frag_irecv_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p);

int process_results(int cur_query,
		    int cur_frag,
		    struct query_info_s  *query_info_list,
		    struct irecv_info_s **query_frag_irecv_matrix,
		    struct mpe_events_s *mpe_events_p,
		    struct test_params_s *test_params_p);

int64_t calculate_start_file_offset(struct lresult_s *query_result_list);

int isend_offsets(int numprocs,
		  int cur_file_offset,
		  int cur_query,
		  struct query_info_s *query_info_list,
		  struct isend_info_s **query_proc_isend_matrix,
		  struct mpe_events_s *mpe_events_p,
		  struct test_params_s *test_params_p);

int master_check_isend_offsets(int wait_status,
			       int numprocs,
			       int last_query_offset_sent,
			       int *last_query_offset_comp_p,
			       struct query_info_s *query_info_list,
			       struct isend_info_s **query_proc_isend_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p);

int do_dummy_io(struct mpe_events_s *mpe_events_p,
                struct test_params_s *test_params_p);

int do_all_io(int *cur_io_query_p,
	      int64_t *cur_file_offset_p,
	      struct query_info_s *query_info_list,
	      struct mpe_events_s *mpe_events_p,
	      struct test_params_s *test_params_p);

void print_offset_list(int64_t *offset_list,
		       int offset_list_count);

void master_check_all_irecv(int numprocs,
			    struct irecv_info_s **query_frag_irecv_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p);

void master_check_all_isend(int numprocs,
			    struct isend_info_s **query_proc_isend_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p);
#endif
