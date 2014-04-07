/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <limits.h>
#include "worker_help.h"
#include "icomm.h"

char *decode_worker_state[MAX_WORKER_STATE] =
    {
        "NOT_USED",
	"WAIT_RESULT_SIZE",
	"WAIT_RESULT",
	"DONE_RESULT",
	"START_IRECV",
	"WAIT_OFFSET_LIST_COUNT",
	"WAIT_OFFSET_LIST",
	"DONE_OFFSET_LIST",
	"IO_FINISHED"
    };

/**
 * Before any work begins, the master distributes the test parameters
 * to the workers through this function. 
 *
 * @param myid          MPI myid.
 * @param numprocs      Number of processes used. 
 * @param mpe_events_p  Pointer to timing structure.  
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
int get_test_params(int myid,
		    int numprocs,
		    struct mpe_events_s *mpe_events_p,
		    struct test_params_s *test_params_p)
{
    int i, hint_key_len, hint_val_len, hint_nkeys;
    char hint_key[MPI_MAX_INFO_KEY], *hint_val = NULL;

    MPI_Bcast(test_params_p, sizeof(struct test_params_s), MPI_BYTE,
              MASTER_NODE, MPI_COMM_WORLD);
    if ((test_params_p->output_file = (char *) malloc(
	     test_params_p->output_file_len + 1)) == NULL)
    {
	custom_debug(WORKER_ERR, 
		     "W%d:malloc output_file of size %d failed\n",
		     myid, test_params_p->output_file_len + 1);
	return -1;
    }
    MPI_Bcast(test_params_p->output_file, 
	      test_params_p->output_file_len + 1,
	      MPI_BYTE, MASTER_NODE, MPI_COMM_WORLD);
    if (test_params_p->query_hist_list_count != 0)
    {
	if ((test_params_p->query_hist_list = (struct hist_s *)
	     malloc(test_params_p->query_hist_list_count * 
		    sizeof(struct hist_s))) == NULL)
	{
	    custom_debug(WORKER_ERR, "W%d:malloc test_params_p->hist_list of "
			 "size %d failed", myid, 
			 test_params_p->query_hist_list_count * 
			 sizeof(struct hist_s));
	    return -1;
	}
	MPI_Bcast(test_params_p->query_hist_list,
		  test_params_p->query_hist_list_count * 
		  sizeof(struct hist_s), MPI_BYTE,
		  MASTER_NODE, MPI_COMM_WORLD);
    }
    if (test_params_p->db_hist_list_count != 0)
    {
	if ((test_params_p->db_hist_list = (struct hist_s *)
	     malloc(test_params_p->db_hist_list_count * 
		    sizeof(struct hist_s))) == NULL)
	{
	    custom_debug(WORKER_ERR, "W%d:malloc test_params_p->hist_list of "
			 "size %d failed", myid, 
			 test_params_p->db_hist_list_count * 
			 sizeof(struct hist_s));
	    return -1;
	}
	MPI_Bcast(test_params_p->db_hist_list,
		  test_params_p->db_hist_list_count * 
		  sizeof(struct hist_s), MPI_BYTE,
		  MASTER_NODE, MPI_COMM_WORLD);
    }
    test_params_p->info_p = malloc(sizeof(MPI_Info));
    if (!test_params_p->info_p)
    {
        custom_debug(WORKER_ERR, "malloc of info_p with size %d failed.\n",
		     sizeof(MPI_Info));
        return -1;
    }
    MPI_Info_create(test_params_p->info_p);
    MPI_Bcast(&hint_nkeys, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (i = 0; i < hint_nkeys; i++)
    {
        MPI_Bcast(&hint_key_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(hint_key, hint_key_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&hint_val_len, 1, MPI_INT, 0, MPI_COMM_WORLD);

	hint_val = malloc((hint_val_len + 1)*sizeof(char));
	if (!hint_val)
	{
	    custom_debug(WORKER_ERR, "hint_val malloc of size %d failed.\n",
			 hint_val_len);
	    return -1;
	}

	MPI_Bcast(hint_val, hint_val_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Info_set(*(test_params_p->info_p), hint_key, hint_val);
	
	free(hint_val);
    }

    return 0;
}

/**
 * The worker uses this function to send a request to the master
 * process to request work.  The wait_status is encoded in the frag
 * section if the query is done or all queries are done.  Otherwise
 * the worker gets the (query,frag) information.
 *
 * @param myid          MPI myid.
 * @param wait_status_p Pointer to the wait status of the worker.
 * @param work_info_p   Pointer to work information structure.
 * @param mpe_events_p  Pointer to timing structure.  
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
int send_recv_work_req(int myid,
		       int *wait_status_p,
		       struct work_info_s *work_info_p,
		       struct mpe_events_s *mpe_events_p,
		       struct test_params_s *test_params_p)
{
    MPI_Status status;
    int send_req, err;

    send_req = WORK_REQ;
    err = MPI_Send(&send_req, 1, MPI_INT, MASTER_NODE, WORKER_CONTROL,
		   MPI_COMM_WORLD);
    custom_debug(WORKER_MSG, 
		 "W%d:Sent msg %s to master ret=%d.\n", 
		 myid, decode_req[send_req], err);

    err = MPI_Recv(work_info_p, sizeof(struct work_info_s), 
		   MPI_BYTE, MASTER_NODE, 
		   WORKER_RECV_WORK_INFO, MPI_COMM_WORLD, &status);
    custom_debug(WORKER_MSG, 
		 "W%d:Recvd WORK_INFO (query=%d,frag=%d,result_start=%d,"
		 "result_count=%d) from master ret=%d.\n", 
		 myid, work_info_p->query, work_info_p->frag, 
		 work_info_p->result_start, work_info_p->result_count, err);

    switch (work_info_p->frag)
    {
	case CUR_QUERY_SCHED_DONE:
	    *wait_status_p = CUR_QUERY_SCHED_DONE;
	    break;
	case ALL_QUERY_SCHED_DONE:
	    *wait_status_p = ALL_QUERY_SCHED_DONE;
	    break;
	default:
	    *wait_status_p = RET_QUERY_FRAG;
    }
    return 0;
 }

/**
 * Results are compared so that they can be placed in a sorted linked
 * list.  This is the comparator function which is used by qsort.
 *
 * @param ptr_1 Void pointer 1 to result structure.
 * @param ptr_2 Void pointer 2 to result structure.
 * @return      Less than 0 if ptr_1 < ptr_2, equal to 0 if ptr_1 == ptr_2, 
 *              and greater than 0 if ptr_1 > ptr2.
 */
static int compare_results(const void *ptr_1, const void *ptr_2)
{
    struct result_s *result_1p = (struct result_s *) ptr_1;
    struct result_s *result_2p = (struct result_s *) ptr_2;

#if 0
    custom_debug(WORKER, "cr: (score1=%d, score2=%d, subtraction=%d )\n",
		 result_1p->score,
		 result_2p->score,
		 (int) (result_1p->score - result_2p->score));
#endif

    /* Sort from high to low */
    if (result_2p->score == result_1p->score)
	return (int) result_2p->result_id - result_1p->result_id;
    else
	return (int) (result_2p->score - result_1p->score);
}

/**
 * Debugging function to print out the member variables of the result
 * structure.
 *
 * @param cur_result_p Result which will have its member variables printed.
 */
void print_result(struct result_s *cur_result_p)
{
    int i;

    fprintf(stderr, "result(score=%d,result_id=%d,proc_id=%d,size=%d,data=",
	    cur_result_p->score,
	    cur_result_p->result_id,
	    cur_result_p->proc_id,
	    cur_result_p->size);
    
    for (i = 0; i < cur_result_p->size; i++)
    {
	fprintf(stderr, "%c",
		cur_result_p->result_data[i]);
    }
    fprintf(stderr, "\n");
}

#define MAX_RESULT_COUNT 1000000
/**
 * The work is calculated based on a very simple model for determining
 * the actual computational time of comparing each (query,frag).  The
 * results are generated and saved in the query_frag_isend_matrix in
 * an array before sending to the master.  After the compute time has
 * been generated, it is simulated through usleep().
 *
 * @param myid                    MPI myid.
 * @param work_info_p             Pointer to work_info structure.      
 * @param query_frag_isend_matrix Pointer to matrix of isend request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
int do_work(int myid,
	    struct work_info_s *work_info_p,
	    struct isend_info_s **query_frag_isend_matrix,
	    struct mpe_events_s *mpe_events_p,
	    struct test_params_s *test_params_p)
{
    unsigned int seed;
    unsigned long usecs = 0;
    int cur_result_size = -1;
    int cur_query_sequence_size = -1; 
    int cur_database_sequence_size = -1;
    int cur_result_size_max = -1;

    int i, j;
    struct result_s *tmp_result_list = NULL; 
    int *tmp_result_list_count_p = 
	&(query_frag_isend_matrix
	  [work_info_p->query][work_info_p->frag].tmp_result_list_count);

    int64_t tmp_buf_size = 0;
    
    assert(work_info_p->frag >= 0);

    /* Computational time generation algoritm:
     *
     * 0. Get the amount of results from the master.
     
     * 1. Determine the query size randomly generated on the query #
     * and in the range of query_min and query_max or through a
     * histogram of empirical data.
    
     * 2. For each result, determine the size the database_sequence
     * sequence randomly generated on the (query # and result #) and
     * in the range of database_sequence_size_min and
     * database_sequence_size_max or in the more complicated scheme we
     * will generate the database sequence size through a histogram of
     * empirical data.
     *
     * 3. For each result, multiply the max(query_size,
     * database_sequence_size) by three (result_size_max) to generate
     * the largest possible result size.  Then determine the actual
     * result size randomly generated on the (query #, frag #, result
     * #) from the range of the result_size_min and result_size_max.
     *
     * Computational time = K(Startup cost + C*(total_size of all results)) 
     */

    *tmp_result_list_count_p = work_info_p->result_count;
    assert(*tmp_result_list_count_p < MAX_RESULT_COUNT);

    /* Determine the query sequence size
     * - simple uniform random distribution (or)
     * - nonuniform random distribution based on histogram */    
    seed = work_info_p->query + 1001;
    if (test_params_p->query_hist_list_count == 0)
    {
	cur_query_sequence_size = generate_int_range(
	    &seed,
	    test_params_p->query_size_min,
	    test_params_p->query_size_max);
    }
    else
    {
	cur_query_sequence_size = generate_rand_hist(myid,
						     &seed,
						     QUERY,
						     work_info_p,
						     mpe_events_p,
						     test_params_p);
    }
    
    if ((tmp_result_list = (struct result_s *) 
	 malloc(*tmp_result_list_count_p * sizeof(struct result_s))) == NULL)
    {
	custom_debug(WORKER_ERR, 
		     "W%d: malloc tmp_result_list of size %d failed\n", myid, 
		     *tmp_result_list_count_p * sizeof(struct result_s));
	return -1;
    }
    memset(tmp_result_list, 0, (*tmp_result_list_count_p) * 
	   sizeof(struct result_s));
    
    for (i = 0; i < *tmp_result_list_count_p; i++)
    {
	/* Determine the database sequence size
	 * - simple uniform random distribution (or)
	 * - nonuniform random distribution based on histogram */
	seed = (3 * work_info_p->query + 1) * 
	    (work_info_p->result_start + (7 * i) + 1);
	if (test_params_p->db_hist_list_count == 0)
	{
	    cur_database_sequence_size = generate_int_range(
		&seed,
		test_params_p->database_sequence_size_min,
		test_params_p->database_sequence_size_max);
	}
	else
	{
	    cur_database_sequence_size = generate_rand_hist(myid,
							    &seed,
							    DATABASE,
							    work_info_p,
							    mpe_events_p,
							    test_params_p);
	}
	
	/* max(cur_query_sequence_size, cur_database_sequence_size) */
	if (cur_database_sequence_size > cur_query_sequence_size)
	    cur_result_size_max = cur_database_sequence_size * 3;
	else
	    cur_result_size_max = cur_query_sequence_size * 3;
	seed = seed * 2 * (work_info_p->query + 1) + i;
	cur_result_size = generate_int_range(
	    &seed,
	    test_params_p->result_size_min,
	    cur_result_size_max);
	
	custom_debug(WORKER_COMPUTE,
		     "W%d:cur_query_sequence_size=%d,"
		     "cur_database_sequence_size=%d,cur_result_size=%d\n",
		     myid, cur_query_sequence_size, 
		     cur_database_sequence_size, cur_result_size);
	
	if ((tmp_result_list[i].result_data
	     = (char *) malloc(cur_result_size * sizeof(char))) == NULL)
	{
	    custom_debug(WORKER_ERR, 
			 "W%d: malloc tmp_result_list[%d].result_data"
			 "of size %d failed\n", 
			 myid, i, cur_result_size * sizeof(char));
	    return -1;
	}
	
	for (j = 0; j < cur_result_size; j++)
	{
	    tmp_result_list[i].result_data[j]
		= 48 + rand_r(&seed) % 10;
	}
	
	tmp_result_list[i].size      = cur_result_size;
	/* The result_id is guaranteed to be unique for a given query */
	tmp_result_list[i].result_id = work_info_p->result_start + i;
	tmp_result_list[i].proc_id   = myid;
	tmp_result_list[i].score     = generate_int_range(
	    &seed, 0, 100);
	
	/* Generate a time value for this result */
	usecs += (unsigned long) cur_result_size;
	
	/* Add up all the bytes used to send the results minus the 
	 * result_data ptr */
	tmp_buf_size += sizeof(struct result_s) - sizeof(char *);
	if (test_params_p->parallel_io == FALSE)
	    tmp_buf_size += tmp_result_list[i].size;
    }

    query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].tmp_result_list 
	= tmp_result_list;
    query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].result_size 
	= tmp_buf_size;

    /* Sort the results */
    qsort(tmp_result_list, *tmp_result_list_count_p,
	  sizeof(struct result_s),
	  compare_results);
    
#if 0
    for (i = 0; i < *tmp_result_list_count_p; i++)
    {
	print_result(&tmp_result_list[i]);
    }
#endif
#define STARTUP_COST 50000 /* 1/20 second */
    usecs = (STARTUP_COST + usecs) / test_params_p->compute_speed;

    custom_debug(WORKER_COMPUTE, "W%d:do_work:results=%d,"
		 "usleep=%d(usecs),query=%d,frag=%d\n", myid,
		 *tmp_result_list_count_p, usecs, work_info_p->query,
                 work_info_p->frag);

    /* Compute =) */
    usleep(usecs);
    return 0;
}

/**
 * The results in the query_frag_isend_matrix are first converted to a
 * linked list.  Then this linked list is merged with the previous
 * results (if there are any), which are also in a linked list.
 *
 * @param myid                    MPI myid.
 * @param work_info_p             Pointer to work_info structure.      
 * @param query_result_list       The head of the result list for a query.
 * @param query_frag_isend_matrix Pointer to matrix of isend request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
int merge_results(int myid,
		  struct work_info_s *work_info_p,
		  struct query_result_s *query_result_list,
		  struct isend_info_s **query_frag_isend_matrix,
		  struct mpe_events_s *mpe_events_p,
		  struct test_params_s *test_params_p)
{
    struct lresult_s *lresult_head_p = NULL;
    int i;
    int64_t tmp_total_size = 0;

    struct result_s *tmp_result_list = 
	query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].tmp_result_list;
    int tmp_result_list_count = 
	query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].tmp_result_list_count;

    /* Convert the results to linked list form */
    for (i = 0; i < tmp_result_list_count; i++)
    {
	tmp_total_size += tmp_result_list[i].size;
	if (i == 0)
	    lresult_head_p = create_list_head(&(tmp_result_list[i]));
	else
	    push_list(lresult_head_p, &(tmp_result_list[i]));
    }

    if (lresult_head_p == NULL)
    {
	custom_debug(WORKER, "W%d:Merging 0 results\n", myid);
	return 0;
    }
    else
	custom_debug(WORKER, "W%d:Merging results\n", myid);	

    /* Merge them with the previous results */
    if (query_result_list[work_info_p->query].lresult_head == NULL)
	query_result_list[work_info_p->query].lresult_head = lresult_head_p;
    else
	query_result_list[work_info_p->query].lresult_head = 
	    merge_lresult(query_result_list[work_info_p->query].lresult_head,
			  lresult_head_p);

    /* Add the tmp offsets and the tmp size to the lresult values */
    query_result_list[work_info_p->query].lresult_count += 
	tmp_result_list_count;
    query_result_list[work_info_p->query].lresult_total_size +=
	tmp_total_size;

    custom_debug(WORKER_MSG, "W%d:query=%d,lresult_count=%d,"
		 "lresult_total_size=%d\n",
		 myid,
		 work_info_p->query,
		 query_result_list[work_info_p->query].lresult_count,
		 query_result_list[work_info_p->query].lresult_total_size);

    return 0;
}

/**
 * Given work info (query,frag), the worker sends the results to the
 * master.  The results are first copied into a contiguous buffer.
 * The size is sent in a first isend and then the actual result is
 * sent in the next isend.  The result data is sent only in the
 * master-writing mode.
 *
 * @param myid                     MPI myid.
 * @param work_info_p              Pointer to work information structure.
 * @param last_query_offset_recv_p Pointer to information set in this function
 *                                 of the last query to look for offset irecv.
 * @param query_frag_isend_matrix  Pointer to matrix of isend request 
 *                                 information. 
 * @param mpe_events_p             Pointer to timing structure.  
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int worker_isend_results(int myid,
			 struct work_info_s *work_info_p,
			 int *last_query_offset_recv_p,
			 struct isend_info_s **query_frag_isend_matrix,
			 struct mpe_events_s *mpe_events_p,
			 struct test_params_s *test_params_p)
{
    int i, err;
    struct result_s *tmp_result_list = 
	query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].tmp_result_list;
    int tmp_result_list_count = 
	query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].tmp_result_list_count;
    char *tmp_buf = NULL;
    int64_t tmp_comp = 0;

#if 0 /* Not necessary, we already know this from test_params */
    if (test_params_p->parallel_io == TRUE)
	send_req = GATHER_RESULTS_META_REQ;
    else
	send_req = GATHER_RESULTS_FULL_REQ;
#endif

    /* Create the buffer */
    if (tmp_result_list_count != 0)
    {
	if ((tmp_buf = 
	     malloc(query_frag_isend_matrix
		    [work_info_p->query][work_info_p->frag].result_size * 
		    sizeof(char))) == NULL)
	{
	    custom_debug(WORKER_ERR, 
			 "W%d: malloc tmp_buf of size %d failed\n", myid, 
			 query_frag_isend_matrix
			 [work_info_p->query][work_info_p->frag].result_size *
			 sizeof(char));
	    return -1;
	}
    }

    /* Pack the results into a buffer */
    for (i = 0; i < tmp_result_list_count; i++)
    {
	memcpy(((char *) (tmp_buf + tmp_comp)), 
	       &(tmp_result_list[i].result_id), sizeof(int));
	tmp_comp += sizeof(int);

	memcpy(((char *) (tmp_buf + tmp_comp)), 
	       &(tmp_result_list[i].proc_id), sizeof(int));
        tmp_comp += sizeof(int);

	memcpy(((char *) (tmp_buf + tmp_comp)), 
	       &(tmp_result_list[i].score), sizeof(int));
        tmp_comp += sizeof(int);

	memcpy(((char *) (tmp_buf + tmp_comp)), 
	       &(tmp_result_list[i].size), sizeof(int));
        tmp_comp += sizeof(int);
	
	if (test_params_p->parallel_io == FALSE)
	{
	    memcpy(((char *) (tmp_buf + tmp_comp)), 
		   tmp_result_list[i].result_data,
		   tmp_result_list[i].size * sizeof(char));
	    tmp_comp += tmp_result_list[i].size * sizeof(char);
	}
	free(tmp_result_list[i].result_data);
    }
    free(tmp_result_list);

    assert(tmp_comp == query_frag_isend_matrix
	   [work_info_p->query][work_info_p->frag].result_size);

    query_frag_isend_matrix
        [work_info_p->query][work_info_p->frag].result = tmp_buf;

    /* Isend the size of the packed result buffer */
    err = MPI_Isend(
	&(query_frag_isend_matrix
	  [work_info_p->query][work_info_p->frag].result_size),
	sizeof(int64_t), MPI_BYTE,
	MASTER_NODE, WORKER_SEND_RESULT_SIZE, MPI_COMM_WORLD,
	&(query_frag_isend_matrix
	  [work_info_p->query][work_info_p->frag].result_size_req));
    
    custom_debug(WORKER_MSG, 
		 "W%d:ISent WORKER_RESULT_SIZE %Ld to master "
		 "query=%d,frag=%d,ret=%d.\n", 
		 myid, 
		 query_frag_isend_matrix
		 [work_info_p->query][work_info_p->frag].result_size, 
		 work_info_p->query, work_info_p->frag, err);
    
    query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].state = WAIT_RESULT_SIZE;

    if (query_frag_isend_matrix
	[work_info_p->query][work_info_p->frag].result_size != 0)
    {
	/* Isend the packed result buffer */
	err = MPI_Isend(
	    tmp_buf, 
	    query_frag_isend_matrix
	    [work_info_p->query][work_info_p->frag].result_size, MPI_BYTE,
	    MASTER_NODE, WORKER_SEND_RESULT_DATA, MPI_COMM_WORLD,
	    &(query_frag_isend_matrix
	      [work_info_p->query][work_info_p->frag].result_req));

	custom_debug(WORKER_MSG, 
		     "W%d:ISent WORKER_RESULT to master "
		     "query=%d,frag=%d,ret=%d.\n", myid,
		     work_info_p->query, work_info_p->frag, err);

	*last_query_offset_recv_p = work_info_p->query;
    }
    return 0;
}

/**
 * Used by worker_check_isend_results() to check on the next possible
 * piece of work that was completed.  All (query,frag) tuples which
 * were not checked by this worker are skipped in the
 * worker_check_isend_results() function.
 *
 * @param myid                     MPI myid.
 * @param last_query_result_sent_p Pointer to the work_info structure of the 
 *                                 last (query,frag) sent.
 * @param last_query_result_comp_p Pointer to the work_info structure of the 
 *                                 next (query,frag) to be checked (changed in
 *                                 this function).
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
static int increment_work(int myid,
			  struct work_info_s *last_query_result_sent_p,
			  struct work_info_s *last_query_result_comp_p,
			  struct test_params_s *test_params_p)
{
    /* Try the next (query,frag) */
    if (last_query_result_comp_p->query < last_query_result_sent_p->query)
    {
	/* Go to the next (query,frag) */
	if (last_query_result_comp_p->frag == test_params_p->total_frags - 1)
	{
	    (last_query_result_comp_p->query)++;
	    last_query_result_comp_p->frag = 0;
	}
	else
	{
	    (last_query_result_comp_p->frag)++;
	}
    }
    else if (last_query_result_comp_p->query == 
	     last_query_result_sent_p->query)
    {
	if (last_query_result_comp_p->frag < last_query_result_sent_p->frag)
	{
	    /* Increment the frag */
	    (last_query_result_comp_p->frag)++;
	}
	else if (last_query_result_comp_p->frag == 
		 last_query_result_sent_p->frag)
	{
	    /* Can't do anything here */
	    custom_debug(WORKER, "W%d:last_query_result_comp == "
                         "last_query_result_sent (query=%d,frag=%d)\n",
			 myid, last_query_result_comp_p->query,
			 last_query_result_comp_p->frag);
	}
	else
	{
	    /* Impossible*/
	    custom_debug(WORKER_ERR, "W%d:Impossible that last_query_result_"
			 "comp_p->frag (q=%d,f=%d) > "
			 "last_query_result_sent_p->frag (q=%d,f=%d)\n",
			 myid, last_query_result_comp_p->query, 
			 last_query_result_comp_p->frag,
			 last_query_result_sent_p->query,
			 last_query_result_sent_p->frag);
	    return -1;
	}
    }
    else
    {
	/* Impossible*/
	custom_debug(WORKER_ERR, "W%d:Impossible that last_query_result_"
		     "comp_p->query > last_query_result_sent_p->query\n",
		     myid);
	return -1;
    }
    
    /* If things passed, we get here */
    return 0;
}
		   
/**
 * Used by worker_check_isend_results() to back off to the previous
 * (query,frag) since it was not complete.
 *
 * @param myid                     MPI myid.
 * @param last_query_result_comp_p Pointer to the work_info structure of the 
 *                                 next (query,frag) to be checked (changed in
 *                                 this function).
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
static int decrement_work(int myid,
			  struct work_info_s *last_query_result_comp_p,
			  struct test_params_s *test_params_p)
{
    /* Special case: If we are trying out the first query
     * just reset the frag to it's initial value (-1) */
    if (last_query_result_comp_p->query == 0 &&
	last_query_result_comp_p->frag == 0)
    {
	last_query_result_comp_p->frag = -1;
	return 0;
    }
    
    /* Fall back to the last (query,frag) completed */
    if (last_query_result_comp_p->frag == 0)
    {
	(last_query_result_comp_p->query)--;
	last_query_result_comp_p->frag = 
	    test_params_p->total_frags - 1;
    }
    else
    {
	(last_query_result_comp_p->frag)--;
    }
    
    return 0;
}

/**
 * The worker occasionally uses this function to check on the state of
 * all its isends to the master for every (query,frag) it has
 * processed.  It checks the isends in order so that it can avoid
 * checking a large amount of isend requests every time it is called.
 *
 * @param myid                     MPI myid.
 * @param wait_status_p            Pointer to the wait status of the worker.
 * @param last_query_result_sent_p Pointer to the work_info structure of the 
 *                                 last (query,frag) sent.
 * @param last_query_result_comp_p Pointer to the work_info structure of the 
 *                                 next (query,frag) to be checked (changed in
 *                                 this function).
 * @param query_result_list        The head of the result list for a query.
 * @param query_frag_isend_matrix  Pointer to matrix of isend request 
 *                                 information. 
 * @param mpe_events_p             Pointer to timing structure.
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int worker_check_isend_results(int myid,
			       int wait_status, 
			       struct work_info_s *last_query_result_sent_p,
			       struct work_info_s *last_query_result_comp_p,
			       struct query_result_s *query_result_list,
			       struct isend_info_s **query_frag_isend_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p)
{
    struct isend_info_s *cur_isend_p = NULL;
    int flag = -1;
    int ret = 0;

    /* There is no reason to check on completed results if we 
     * haven't sent any results out. */
    if (last_query_result_sent_p->query == 0 &&
	last_query_result_sent_p->frag == -1)
    {
	return 0;
    }    

    /* Try the next piece of work*/
    ret = increment_work(myid,
			 last_query_result_sent_p, 
			 last_query_result_comp_p,
			 test_params_p);
    assert(ret >= 0);

#if 0 /* Only a temporary comment, I will delete this soon. */
    fprintf(stderr, "after increment_work: "
	    "last_query_result_comp_p->(query,frag) = (%d,%d)\n",
	    last_query_result_comp_p->query,
	    last_query_result_comp_p->frag);
#endif

    while (ret == 0)
    {	
	cur_isend_p = &(query_frag_isend_matrix
			[last_query_result_comp_p->query]
			[last_query_result_comp_p->frag]);

	/* 1 - Check the result size req
	 * 2 - Check the result req */
	if (cur_isend_p->state == WAIT_RESULT_SIZE)
	{
	    custom_debug(WORKER_GATHER,
			 "W%d:Test/Wait for isend RESULT_SIZE "
			 "(query=%d,proc=%d,req_addr=%d)\n", myid,
			 last_query_result_comp_p->query, MASTER_NODE,
			 &(cur_isend_p->result_size_req));
	    check_isend(myid,
			wait_status,
			&flag,
			&(cur_isend_p->result_size_req),
			&(cur_isend_p->state),
			WAIT_RESULT,
			mpe_events_p,
			test_params_p);
	    if (flag == 0)
	    {
		decrement_work(myid, 
			       last_query_result_comp_p,
			       test_params_p);
		return 0;
	    }
	}

	if (cur_isend_p->state == WAIT_RESULT)
	{
	    custom_debug(WORKER_GATHER,
			 "W%d:Test/Wait for isend RESULT "
			 "(query=%d,proc=%d,req_addr=%d)\n", myid,
			 last_query_result_comp_p->query, MASTER_NODE,
			 &(cur_isend_p->result_req));
	    check_isend(myid,
			wait_status,
			&flag,
			&(cur_isend_p->result_req),
			&(cur_isend_p->state),
			DONE_RESULT,
			mpe_events_p,
			test_params_p);
	    if (flag == 0)
	    {
		decrement_work(myid, 
			       last_query_result_comp_p,
			       test_params_p);
		return 0;
	    }
	    free(cur_isend_p->result);
	}

	/* This is as far as we can go, we can't check past what 
	 * fragments we're processed */
	if (last_query_result_comp_p->query == 
	    last_query_result_sent_p->query &&
	    last_query_result_comp_p->frag == 
	    last_query_result_sent_p->frag)
	{
	    custom_debug(WORKER_ISEND,
			 "W%d:Isend finished up to result_req "
			 "(query=%d,frag=%d)\n",
			 myid,
			 last_query_result_comp_p->query, 
			 last_query_result_comp_p->frag);
	    return 0;
	}

	ret = increment_work(myid,
			     last_query_result_sent_p, 
			     last_query_result_comp_p,
			     test_params_p);	
    }
    return 0;
}

/**
 * If the worker is using parallel I/O, it will have to receive the
 * offset array from the master.  It posts a irecv here for this
 * query.  Since it is possible that a irecv was already posted for
 * this query, this function should not be called in that case.
 *
 * @param myid              MPI myid.
 * @param work_info_p       Pointer to work information structure.
 * @param query_result_list The head of the result list for a query.
 * @param mpe_events_p      Pointer to timing structure.
 * @param test_params_p     Pointer to test_params.    
 * @return                  0 on success.
 */
int worker_post_irecv(int myid,
		      struct work_info_s *work_info_p,
		      struct query_result_s *query_result_list,
		      struct mpe_events_s *mpe_events_p,
		      struct test_params_s *test_params_p)
{
    int err;

    custom_debug(WORKER, 
		 "W%d:worker_post_irecv (query=%d,lresult_count=%d)\n",
		 myid, work_info_p->query, 
		 query_result_list[work_info_p->query].lresult_count);

    if (query_result_list[work_info_p->query].lresult_count != 0)
    {
	assert(query_result_list[work_info_p->query].offset_list_count_req == 
	       MPI_REQUEST_NULL);

	err = MPI_Irecv(&(query_result_list[work_info_p->query].
			offset_list_count),
		       sizeof(int64_t), MPI_BYTE, MASTER_NODE,
		       WORKER_RECV_OFFSETS_COUNT, MPI_COMM_WORLD, 
		       &(query_result_list[work_info_p->query].
			 offset_list_count_req));
	custom_debug(
	    WORKER_MSG,
	    "W%d:IRecv OFFSETS_COUNT from M (query=%d,addr=%d) ret=%d\n", myid,
	    work_info_p->query,
	    &(query_result_list[work_info_p->query].offset_list_count_req),
	    err);
	
	query_result_list[work_info_p->query].state = 
	    WAIT_OFFSET_LIST_COUNT;
    }
    return 0;
}

/**
 * Check for all the irecvs from all queries that had results from
 * this worker.  Post irecvs for the actual offset array when the size
 * irecv is completed.  Check for both the sizes and the actual offset
 * arrays.
 *
 * @param myid                     MPI myid.
 * @param wait_status_p            Pointer to the wait status of the worker.
 * @param last_query_offset_recv   Last query to look for offset irecv.

 * @param last_query_offset_comp_p Pointer to information set in this function
 *                                 of the last query to look for offset irecvs.
 * @param query_result_list        The head of the result list for a query.
 * @param mpe_events_p             Pointer to timing structure.
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int worker_check_irecv_offsets(int myid,
			       int wait_status,
			       int last_query_offset_recv,
			       int *last_query_offset_comp_p,
			       struct query_result_s *query_result_list,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p)
{
    struct query_result_s *cur_query_result_p = NULL;
    int err = -1, flag = -1;

    /* We first check for the reception of the offset_list_count,
     * then we check for the actual offset_list 
     *
     * There are two things that should break out of the loop.
     * 1. We have checked up to the final_query.
     * 2. Uncompleted test 
     *
     * Try the next query since we have completed the last one 
     */
    (*last_query_offset_comp_p)++;

    for (;*last_query_offset_comp_p <= last_query_offset_recv; 
	 (*last_query_offset_comp_p)++)
    {
	cur_query_result_p = &(query_result_list[*last_query_offset_comp_p]);

	assert(cur_query_result_p->state == NOT_USED ||
	       cur_query_result_p->state == WAIT_OFFSET_LIST_COUNT ||
	       cur_query_result_p->state == WAIT_OFFSET_LIST ||
	       cur_query_result_p->state == DONE_OFFSET_LIST);

	if (cur_query_result_p->state == NOT_USED)
	{
	    custom_debug(WORKER_IRECV, "W%d:Query[%d] not used\n", myid,
			 *last_query_offset_comp_p);
	    continue;
	}

	if (cur_query_result_p->state == WAIT_OFFSET_LIST_COUNT)
	{
	    custom_debug(WORKER_IRECV,
			 "W%d:Test/Wait for irecv WAIT_OFFSET_LIST_COUNT "
			 "(query=%d,req_addr=%d)\n", myid,
			 *last_query_offset_comp_p,
			 &(cur_query_result_p->offset_list_count_req));
	    
	    check_irecv(myid,
			wait_status,
			&flag,
			&(cur_query_result_p->offset_list_count_req),
			&(cur_query_result_p->state),
			WAIT_OFFSET_LIST,
			mpe_events_p,
			test_params_p);
	    if (flag == 0)
	    {
		/* Fall back to the last query completed */
		(*last_query_offset_comp_p)--;	    
		return 0;
	    }
	    if ((cur_query_result_p->offset_list = (int64_t *) 
		 malloc(cur_query_result_p->offset_list_count *
			sizeof(int64_t))) == NULL)
	    {
		custom_debug(WORKER_ERR,
			     "W%d:malloc query_result_list[%d]"
			     ".offset_list of size %d failed\n",
			     myid, *last_query_offset_comp_p,
			     cur_query_result_p->offset_list_count *
			     sizeof(int64_t));
		return -1;
	    }
	    err = MPI_Irecv(cur_query_result_p->offset_list,
			    cur_query_result_p->offset_list_count *
			    sizeof(int64_t), MPI_BYTE, MASTER_NODE,
			    WORKER_RECV_OFFSETS, MPI_COMM_WORLD,
			    &(cur_query_result_p->offset_list_req));
	    custom_debug(
		WORKER_MSG,
		"W%d:IRecv %Ld OFFSETS from M (query=%d,addr=%d) ret=%d\n", 
		myid, cur_query_result_p->offset_list_count, 
		*last_query_offset_comp_p, 
		&(cur_query_result_p->offset_list_req), err);
	}

	if (cur_query_result_p->state == WAIT_OFFSET_LIST)
	{
	    custom_debug(WORKER_IRECV,
                         "W%d:Test/Wait for irecv WAIT_OFFSET_LIST "
                         "(query=%d,req_addr=%d)\n", myid,
                         *last_query_offset_comp_p,
                         &(cur_query_result_p->offset_list_req));

            check_irecv(myid,
                        wait_status,
                        &flag,
                        &(cur_query_result_p->offset_list_req),
                        &(cur_query_result_p->state),
                        DONE_OFFSET_LIST,
                        mpe_events_p,
                        test_params_p);
            if (flag == 0)
            {
                /* Fall back to the last query completed */
                (*last_query_offset_comp_p)--;
                return 0;
            }

	    /* perhaps do I/O here */
	}

	/* This is as far as we can go, we can't check past what
	 * queries we've processed */
	if (*last_query_offset_comp_p == last_query_offset_recv)
	{
	    custom_debug(WORKER_IRECV,
			 "W%d:Irecv offsets finished up to (query=%d)\n",
			 myid,
			 *last_query_offset_comp_p);
	    return 0;
	}
    }
    /* The only case that leads out of here means we must go down
     * a notch */
    (*last_query_offset_comp_p)--;

    if (*last_query_offset_comp_p > test_params_p->query_count - 1)
    {
	custom_debug(
	    WORKER_ERR,
	    "W%d:Finished the offset lists up to and including query=%d.\n"
	    "Reducing last_query_offset_comp to %d\n",
	    myid,
	    *last_query_offset_comp_p, *last_query_offset_comp_p - 1);
	*last_query_offset_comp_p = test_params_p->query_count - 1;
    }

    return 0;
}

/**
 * If the worker-writing mode is chosen with collective I/O, the
 * all workers must participate since it is in the same communication
 * group.  When it does not actually write any data, the worker will
 * also call MPI_File_write_all().
 *
 * @param myid          MPI myid.
 * @param mpe_events_p  Pointer to timing structure.
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
static int worker_do_dummy_io(int myid,
			      struct mpe_events_s *mpe_events_p,
			      struct test_params_s *test_params_p)
{
    MPI_Status status;
    
    if (test_params_p->no_io == TRUE)
    {
        if (test_params_p->io_method == COLLECTIVE_IO)
            MPI_Barrier(MPI_COMM_WORLD);
        return 0;
    }

    MPI_File_set_view(test_params_p->fh, 0, MPI_BYTE, MPI_BYTE, 
		      "native", *(test_params_p->info_p));
    MPI_File_write_all(test_params_p->fh, NULL, 0, MPI_BYTE, &status);
    MPI_File_sync(test_params_p->fh);
    return 0;
}

/**
 * The noncontiguous memory regions of the result data for a completed
 * query with the master's offset array are copied to a contiguous
 * memory buffer.  Disp is offset by the first offset to be written.
 * A hindexed MPI Datatype is created for the possibly noncontiguous
 * file regions.  The write operation may be collective or individual
 * depending on the test_params.  This function is used by
 * do_all_worker_io().
 *
 * @param myid              MPI myid.
 * @param cur_io_query      The query for which we are going to write results.
 * @param query_result_list The head of the result list for a query.
 * @param mpe_events_p      Pointer to timing structure.
 * @param test_params_p     Pointer to test_params.    
 */
static int worker_do_io(int myid,
			int cur_io_query,
			struct query_result_s *query_result_list,
			struct mpe_events_s *mpe_events_p,
			struct test_params_s *test_params_p)
{
    int i;
    MPI_Status status;
    MPI_Datatype file_type;
    MPI_Aint *arr_disp;
    int *arr_blk;
    MPI_Offset disp = -1;
    int64_t total_comp = 0;
    char *tmp_buf = NULL;
    struct lresult_s *cur_lresult_head = 
	query_result_list[cur_io_query].lresult_head;

    assert(query_result_list[cur_io_query].lresult_head != NULL);

    if (test_params_p->no_io == TRUE)
    {
	if (test_params_p->io_method == COLLECTIVE_IO)
	    MPI_Barrier(MPI_COMM_WORLD);
	free_lresult_list(query_result_list[cur_io_query].lresult_head);
	return 0;
    }

    /* In the future, we need to change this to match the proper offsets */
    if ((tmp_buf = (char *) 
	 malloc(query_result_list[cur_io_query].lresult_total_size * 
		sizeof(char))) == NULL)
    {
	custom_debug(WORKER_ERR,
                     "W%d: malloc tmp_buffer of size %d failed\n", myid,
		     query_result_list[cur_io_query].lresult_total_size *
		     sizeof(char));
	return -1;
    }
    
    if ((arr_blk = (int *) malloc(
	     query_result_list[cur_io_query].lresult_count * 
	     sizeof(int))) == NULL)
    {
	custom_debug(WORKER_ERR,
                     "W%d: malloc arr_blk of size %d failed\n", myid,
		     query_result_list[cur_io_query].lresult_count *
		     sizeof(int));
	return -1;
	
    }
    if ((arr_disp = (MPI_Aint *) malloc(
	     query_result_list[cur_io_query].lresult_count * 
	     sizeof(MPI_Aint))) == NULL)
    {
	custom_debug(WORKER_ERR,
                     "W%d: malloc arr_disp of size %d failed\n", myid,
		     query_result_list[cur_io_query].lresult_count *
		     sizeof(MPI_Aint));
	return -1;
    }
    
    i = 0;
    /* Copy noncontiguous memory regions into a contiguous memory buffer */
    while (cur_lresult_head->next != NULL)
    {
	memcpy(tmp_buf + total_comp,
	       cur_lresult_head->result.result_data,
	       cur_lresult_head->result.size);
	total_comp += cur_lresult_head->result.size;
	arr_blk[i] = cur_lresult_head->result.size;
	i++;
	cur_lresult_head = cur_lresult_head->next;
    }
    memcpy(tmp_buf + total_comp,
	   cur_lresult_head->result.result_data,
	   cur_lresult_head->result.size);
    total_comp += cur_lresult_head->result.size;
    arr_blk[i] = cur_lresult_head->result.size;
    i++;

    assert(i == query_result_list[cur_io_query].lresult_count);
    assert(total_comp == query_result_list[cur_io_query].lresult_total_size);

    /* Create noncontiguous file datatype by subtracting from the
     * first offset, which enables writing to files larger than 2
     * GBytes as long as the last offset in this dataype is within 2
     * GBytes of the first ofset. */
    for (i = 0; i < query_result_list[cur_io_query].lresult_count; i++)
    {
	assert((query_result_list[cur_io_query].offset_list[i] -
		query_result_list[cur_io_query].offset_list[0]) <
	       INT_MAX);
	arr_disp[i] = (MPI_Aint) 
	    (query_result_list[cur_io_query].offset_list[i] - 
	     query_result_list[cur_io_query].offset_list[0]);
    }

    disp = query_result_list[cur_io_query].offset_list[0];
    MPI_Type_create_hindexed(query_result_list[cur_io_query].lresult_count,
			     arr_blk, arr_disp, MPI_BYTE, &file_type);
    MPI_Type_commit(&file_type);
    MPI_File_set_view(test_params_p->fh, disp, MPI_BYTE, file_type,
                      "native", *(test_params_p->info_p));
    if (test_params_p->io_method == COLLECTIVE_IO)
    {
	MPI_File_write_all(test_params_p->fh, tmp_buf, total_comp, 
			   MPI_BYTE, &status);
    }
    else
	MPI_File_write(test_params_p->fh, tmp_buf, total_comp, 
		       MPI_BYTE, &status);

    MPI_Type_free(&file_type);

    free_lresult_list(query_result_list[cur_io_query].lresult_head);

    free(tmp_buf);
    free(arr_blk);
    free(arr_disp);

    MPI_File_sync(test_params_p->fh);
    
    custom_debug(WORKER_IO, "W%d:Wrote %Ld bytes for query=%d to file %s\n",
		 myid,
		 total_comp,
		 cur_io_query,
		 test_params_p->output_file);
    return 0;
}

/**
 * This function will only be called if the worker-writing method is
 * used.  When the master notifies the worker of getting ready to
 * write, the worker will try to do I/O to the queries which it has
 * received offset arrays for.  If collective I/O is used, the worker
 * will be careful about when to call this function to ensure that all
 * processes write together.
 *
 * @param myid              MPI myid.
 * @param wait_status_p     Pointer to the wait status of the worker.
 * @param last_io_ready     Last I/O operation possible
 * @param last_io_comp_p    Pointer to the I/O query operation to try next
 *                          (modified in this function).
 * @param query_result_list The head of the result list for a query.
 * @param mpe_events_p      Pointer to timing structure.
 * @param test_params_p     Pointer to test_params.    
 */
int do_all_worker_io(int myid,
		     int wait_status,
		     int last_io_ready,
		     int *last_io_comp_p,
		     struct query_result_s *query_result_list,
		     struct mpe_events_s *mpe_events_p,
		     struct test_params_s *test_params_p)
{
    custom_debug(WORKER, "W%d:Starting I/O for query=%d up to query=%d\n",
		 myid, *last_io_comp_p, last_io_ready);

    /* Don't do any I/O until the master says so if we are doing 
     * collective I/O, this will cause weird race conditions */
    if (test_params_p->io_method == COLLECTIVE_IO && 
	wait_status == RET_QUERY_FRAG)
    {
	return 0;
    }

    /* Try the next query */
    (*last_io_comp_p)++;
    for (; *last_io_comp_p <= last_io_ready; (*last_io_comp_p)++)
    {
	assert(query_result_list[*last_io_comp_p].state == DONE_OFFSET_LIST ||
	       query_result_list[*last_io_comp_p].state == NOT_USED);

	if (query_result_list[*last_io_comp_p].state == DONE_OFFSET_LIST)
	{
	    custom_debug(WORKER, "W%d:Begin I/O for Query %d\n",
			 myid, *last_io_comp_p);
	    worker_do_io(myid,
			 *last_io_comp_p,
			 query_result_list,
			 mpe_events_p,
			 test_params_p);
	    free(query_result_list[*last_io_comp_p].offset_list);
	    query_result_list[*last_io_comp_p].state = IO_FINISHED;
	}
	else if (query_result_list[*last_io_comp_p].state == NOT_USED)
	{
	    if (test_params_p->io_method == COLLECTIVE_IO)
	    {
		worker_do_dummy_io(myid, mpe_events_p, test_params_p);
		custom_debug(WORKER_IO, 
			     "W%d:No real data only dummy collective I/O "
			     "for Query %d (state=%s)\n", myid, 
			     *last_io_comp_p, decode_worker_state
			     [query_result_list[*last_io_comp_p].state]);
	    }
	    else
	    {
		custom_debug(WORKER_IO, 
			     "W%d:No real data (no I/O) for Query %d "
			     "(state=%s)\n", myid, *last_io_comp_p,
			     decode_worker_state
			     [query_result_list[*last_io_comp_p].state]);
	    }
	    continue;
	}
	else
	{
	    custom_debug(WORKER_IO, 
			 "W%d:Error: Couldn't do I/O for Query %d "
			 "(state=%s)\n", myid, *last_io_comp_p,
			 decode_worker_state
			 [query_result_list[*last_io_comp_p].state]);
	    /* Revert to last query */
	    (*last_io_comp_p)--;
	    return 0;
	}

	if (*last_io_comp_p == last_io_ready)
	    return 0;
    }
    
    (*last_io_comp_p)--;
    return 0;
}

/**
 * Do a final check on the query_result_list for any uncompleted
 * irecvs.  None should exist.
 *
 * @param myid              MPI myid.
 * @param numprocs          Number of processes used. 
 * @param query_result_list The head of the result list for a query.
 * @param mpe_events_p      Pointer to timing structure.
 * @param test_params_p     Pointer to test_params.    
 */
void worker_check_all_irecv(int myid,
			    int numprocs,
			    struct query_result_s *query_result_list,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p)
{
    MPI_Status status;
    int i, flag = -1;
    
    for (i = 0; i < test_params_p->query_count; i++)
    {
	MPI_Test(&(query_result_list[i].offset_list_count_req),
		 &flag, &status); 
	if (flag != 1)
	{
	    custom_debug(WORKER_ERR, "W%d: query_result_list[%d] "
			 "offset_list_count_req=%d instead of "
			 "NULL or MPI_REQUEST_NULL\n", myid,
			 i, query_result_list[i].offset_list_count_req);
	}
	MPI_Test(&(query_result_list[i].offset_list_req),
		 &flag, &status); 
	if (flag != 1)
	{
	    custom_debug(WORKER_ERR, "W%d: query_result_list[%d] "
			 "offset_list_req=%d instead of "
			 "NULL or MPI_REQUEST_NULL\n", myid,
			 i, query_result_list[i].offset_list_count_req);
	}
    }
}

/**
 * Do a final check on the query_frag_isend_matrix for any uncompleted
 * isends.  None should exist.
 *
 * @param myid                    MPI myid.
 * @param numprocs                Number of processes used. 
 * @param query_frag_isend_matrix Pointer to matrix of isend request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 */
void worker_check_all_isend(int myid,
			    int numprocs,
			    struct isend_info_s **query_frag_isend_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p)
{
    MPI_Status status;
    int i, j, flag = -1;

    for (i = 0; i < test_params_p->query_count; i++)
    {
	for (j = 0; j < test_params_p->total_frags; j++)
	{
	    MPI_Test(&(query_frag_isend_matrix[i][j].result_size_req),
		     &flag, &status);
	    if (flag != 1)
	    {
		custom_debug(WORKER_ERR, "W%d: query_frag_isend_matrix[%d]"
			     "[%d] result_size_req=%d instead of "
			     "NULL or MPI_REQUEST_NULL\n", myid,
			     i, j, 
			     query_frag_isend_matrix[i][j].result_size_req);
	    }
	    MPI_Test(&(query_frag_isend_matrix[i][j].result_req),
                     &flag, &status);
	    if (flag != 1)
            {
                custom_debug(WORKER_ERR, "W%d: query_frag_isend_matrix[%d]"
                             "[%d] result_req=%d instead of "
                             "NULL or MPI_REQUEST_NULL\n", myid,
                             i, j,
                             query_frag_isend_matrix[i][j].result_req);
            }
	}
    }
}
