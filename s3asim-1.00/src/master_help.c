/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "master_help.h"
#include "icomm.h"

/**
 * Determines what to send to workers who are requesting work.  If
 * there is work remaining, send out a (query,frag) pair.  Otherwise,
 * there may be a global query sync or the queries could already be
 * fully scheduled.
 *
 * @param query_info_list Pointer to array of progress for each query. 
 * @param cur_query_p     Index of current query in the query list.
 * @param work_info_p     Pointer to work_info structure.      
 * @param test_params_p   Pointer to test_params.    
 * @return                Status of progress for queries and fragments.
 */
static int check_query_status(struct query_info_s *query_info_list, 
			      int *cur_query_p,
			      struct work_info_s *work_info_p,
			      struct test_params_s *test_params_p)
{
    if (query_info_list[*cur_query_p].frag_sched < 
	test_params_p->total_frags)
    {
	work_info_p->query = *cur_query_p;
	work_info_p->frag = query_info_list[*cur_query_p].frag_sched;
	work_info_p->result_start = 
	    test_params_p->query_frag_preresult_matrix[work_info_p->query]
	    [work_info_p->frag].result_start;
	work_info_p->result_count = 
	    test_params_p->query_frag_preresult_matrix[work_info_p->query]
	    [work_info_p->frag].result_count;

	(query_info_list[*cur_query_p].frag_sched)++;
	return RET_QUERY_FRAG;
    }
    else /* Starting a new fragment */
    {
	/* Write results and synchronize before starting next query */
	if (test_params_p->query_sync == TRUE)
	{
	    if (*cur_query_p == test_params_p->query_count - 1)
		return ALL_QUERY_SCHED_DONE;
	    else
		return CUR_QUERY_SCHED_DONE;
	}
	else
	{
	    if (*cur_query_p == test_params_p->query_count - 1)
	    {
		return ALL_QUERY_SCHED_DONE;
	    }
	    else
	    {
		(*cur_query_p)++;

		work_info_p->query = *cur_query_p;
		work_info_p->frag = 0;
		work_info_p->result_start = 
		    test_params_p->query_frag_preresult_matrix
		    [work_info_p->query]
		    [work_info_p->frag].result_start;
		work_info_p->result_count = 
		    test_params_p->query_frag_preresult_matrix
		    [work_info_p->query]
		    [work_info_p->frag].result_count;
		
		query_info_list[*cur_query_p].frag_sched = 1;

		return RET_QUERY_FRAG;
	    }
	}
    }
    
    return -1; /* Shouldn't get here. */
}

/**
 * Send a response to the worker who requested some work.  A work_info
 * structure is passed in the message with a control tag in the frag
 * information on what to do next.
 *
 * @param worker          MPI id of worker process who requested work.
 * @param cur_query_p     Pointer to the index of current query in
 *                        the query list.
 * @param work_info_p     Pointer to work_info structure.      
 * @param query_info_list Pointer to array of progress for each query. 
 * @param mpe_events_p    Pointer to timing structure.
 * @param test_params_p   Pointer to test_params.    
 * @return                Status of progress for queries and fragments.
 */
int send_work(int worker,
	      int *cur_query_p,
	      struct work_info_s *work_info_p,
	      struct query_info_s  *query_info_list,
	      struct mpe_events_s *mpe_events_p,
	      struct test_params_s *test_params_p)
{
    int send_req, err;
    send_req = check_query_status(query_info_list,
				  cur_query_p,
				  work_info_p,
				  test_params_p);

    assert(send_req == RET_QUERY_FRAG || send_req == CUR_QUERY_SCHED_DONE
	   || send_req == ALL_QUERY_SCHED_DONE);

    switch(send_req)
    {
	case CUR_QUERY_SCHED_DONE:
	    work_info_p->frag = CUR_QUERY_SCHED_DONE;
	    break;
	case ALL_QUERY_SCHED_DONE:
	    work_info_p->frag = ALL_QUERY_SCHED_DONE;
	    break;
	    
	case RET_QUERY_FRAG:
	    break;
	default:
	    custom_debug(MASTER, "M:Trying to send impossible work "
			 "(query=%d,frag=%d)\n",
			 work_info_p->query,
			 work_info_p->frag);
	    return -1;
    }

    /* Send the req and the query #, frag # */
    err = MPI_Send(work_info_p, sizeof(struct work_info_s),
		   MPI_BYTE, worker, 
		   MASTER_SEND_WORK_INFO, MPI_COMM_WORLD);
    
    custom_debug(MASTER_MSG, 
		 "M:Sent WORK_INFO (query=%d,frag=%d,result_start=%d,"
		 "result_count=%d) to proc %d ret=%d\n", work_info_p->query, 
		 work_info_p->frag, work_info_p->result_start, 
		 work_info_p->result_count, worker, err); 

    return send_req;
}

/**
 * After a worker has been assigned a (query,frag), then setup an
 * irecv request to check for the size of the result.
 *
 * @param worker                  MPI id of worker process who requested work.
 * @param work_info_p             Pointer to work_info structure.      
 * @param query_frag_irecv_matrix Pointer to matrix of irecv request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
int post_irecv(int worker,
	       struct work_info_s *work_info_p,
	       struct irecv_info_s **query_frag_irecv_matrix,
	       struct mpe_events_s *mpe_events_p,
	       struct test_params_s *test_params_p)
{
    int err = -1;
    struct irecv_info_s *cur_irecv = 
	&(query_frag_irecv_matrix[work_info_p->query][work_info_p->frag]);

    if (cur_irecv->result_size_req != MPI_REQUEST_NULL)
    {
        custom_debug(MASTER_ERR,
		     "M:ERROR posting irecv result_size "
		     "(query=%d,frag=%d,worker=%d,ret=%d)\n",
		     work_info_p->query, work_info_p->frag, worker, err);
	return -1;
    }
    err = MPI_Irecv(&(cur_irecv->result_size), sizeof(int64_t), MPI_BYTE,
		    worker, MASTER_RECV_RESULT_SIZE, MPI_COMM_WORLD,
		    &(cur_irecv->result_size_req));
    custom_debug(MASTER_MSG,
		 "M:Irecv result_size (query=%d,frag=%d,worker=%d,ret=%d)\n",
		 work_info_p->query, work_info_p->frag, worker, err);
    cur_irecv->state = WAIT_RESULT_SIZE;
    cur_irecv->worker = worker;
    return 0;
}

/**
 * Check the query_frag_irecv_matrix to see how the irecvs are
 * progressing.  If a size has been received, set up a irecv for the
 * actual result.  If the master-writing method is used, the worker
 * will additionally send the result data, which is included in the
 * size.
 *
 * @param cur_query                Index of current query in the query list.
 * @param last_irecv_result_done_p Pointer to the index of the last query 
 *                                 completed.
 * @param wait_status              Status of master process (wait or no wait).
 * @param query_info_list          Pointer to array of progress for each query.
 * @param query_frag_irecv_matrix  Pointer to matrix of irecv request 
 *                                 information. 
 * @param mpe_events_p             Pointer to timing structure.
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int master_check_irecv_results(int cur_query,
			       int *last_irecv_result_done_p,
			       int wait_status,
			       struct query_info_s  *query_info_list,
			       struct irecv_info_s **query_frag_irecv_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p)
{
    struct irecv_info_s *cur_irecv_p = NULL;
    int i, j, flag, err;

    for (i = 0; i <= cur_query && i < test_params_p->query_count; i++)
    {
	for (j = 0; j < test_params_p->total_frags &&
		 j < query_info_list[i].frag_sched; j++)
	{
	    cur_irecv_p = &(query_frag_irecv_matrix[i][j]);
	    if (cur_irecv_p->state == NOT_USED)
	    {
		return 0;
	    }

	    if (cur_irecv_p->state == WAIT_RESULT_SIZE)
	    {
		custom_debug(MASTER_GATHER,
			     "M:Test/Wait for irecv RESULT_SIZE "
			     "(query=%d,frag=%d,worker=%d,req_addr=%d)\n",
			     i, j, cur_irecv_p->worker,
			     &(cur_irecv_p->result_size_req));

		check_irecv(MASTER_NODE,
			    wait_status,
			    &flag,
			    &(cur_irecv_p->result_size_req),
			    &(cur_irecv_p->state),
			    WAIT_RESULT_DATA,
			    mpe_events_p,
			    test_params_p);

		if (flag == 0)
		    return 0;

		custom_debug(MASTER_GATHER,
			     "M:Result size = %Ld\n", 
			     cur_irecv_p->result_size);

		if (cur_irecv_p->result_size != 0)
		{
		    if ((cur_irecv_p->result = (char *) 
			 malloc(cur_irecv_p->result_size * sizeof(char))) == 
			NULL)
		    {
			custom_debug(MASTER_ERR,
				     "M:malloc cur_irecv_p->result_size "
				     "of size %d failed\n", 
				     cur_irecv_p->result_size * sizeof(char));
			return -1;
		    }
		    err = MPI_Irecv(cur_irecv_p->result,  
				    cur_irecv_p->result_size * sizeof(char), 
				    MPI_BYTE, cur_irecv_p->worker,
				    MASTER_RECV_RESULT_DATA, 
				    MPI_COMM_WORLD,
				    &(cur_irecv_p->result_req));
		    custom_debug(MASTER_MSG,
				 "M:Irecv RESULT_DATA (query=%d,frag=%d,"
				 "worker=%d,size=%Ld,ret=%d)\n",
				 i, j, cur_irecv_p->worker, 
				 cur_irecv_p->result_size * sizeof(char),
				 err);
		}
		else
		{
		    cur_irecv_p->state = DONE_RESULT;
		    (query_info_list[i].frag_comp)++;
		}
	    }
	    
	    if (cur_irecv_p->state == WAIT_RESULT_DATA)
	    {
		custom_debug(MASTER_GATHER,
			     "M:Test/Wait for irecv RESULT_DATA "
			     "(query=%d,frag=%d,req_addr=%d)\n",
			     i, j,
			     &(cur_irecv_p->result_req));

		check_irecv(MASTER_NODE,
			    wait_status,
			    &flag,
			    &(cur_irecv_p->result_req),
			    &(cur_irecv_p->state),
			    DONE_RESULT,
			    mpe_events_p,
			    test_params_p);
		if (flag == 0)
		    return 0;

		process_results(i, j,
				query_info_list,
				query_frag_irecv_matrix,
				mpe_events_p,
				test_params_p);

		(query_info_list[i].frag_comp)++;
	    }

	    /* All the results for query i have been finished */
	    if (j == test_params_p->total_frags - 1 &&
		i > *last_irecv_result_done_p)
		*last_irecv_result_done_p = i;
	}
    }
    return 0;
}

/**
 * After result data has been received for a (query,frag), reformat
 * the data stored in the irecv data into a new result list.  The new
 * result list which will be the final result list if it is the first
 * result data added for this query.  Otherwise, the new result list
 * is merged with the current result list.
 *
 * @param cur_query                Index of current query in the query list.
 * @param cur_frag                 Index of current frag for cur_query.
 * @param query_info_list          Pointer to array of progress for each query.
 * @param query_frag_irecv_matrix  Pointer to matrix of irecv request 
 *                                 information. 
 * @param mpe_events_p             Pointer to timing structure.
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int process_results(int cur_query,
		    int cur_frag,
		    struct query_info_s  *query_info_list,
		    struct irecv_info_s **query_frag_irecv_matrix,
		    struct mpe_events_s *mpe_events_p,
		    struct test_params_s *test_params_p)
{
    int i = 0;
    struct result_s cur_result;
    struct lresult_s *lresult_head = NULL;
    int tmp_comp = 0;
    struct irecv_info_s *cur_irecv_p = 
	&(query_frag_irecv_matrix[cur_query][cur_frag]);

    assert(cur_irecv_p->result != NULL);

    if (cur_irecv_p->result_size == 0)
    {
	custom_debug(MASTER_GATHER, "M:No data (query=%d,frag=%d\n",
		     cur_query, cur_frag);
	return 0;
    }

    while (1)
    {
	memcpy(&(cur_result.result_id), 
	       (char *) (cur_irecv_p->result + tmp_comp), sizeof(int));
	tmp_comp += sizeof(int);

	memcpy(&(cur_result.proc_id), cur_irecv_p->result + tmp_comp, 
	       sizeof(int));
	tmp_comp += sizeof(int);

	memcpy(&(cur_result.score), cur_irecv_p->result + tmp_comp, 
	       sizeof(int));
	tmp_comp += sizeof(int);

	memcpy(&(cur_result.size), cur_irecv_p->result + tmp_comp, 
	       sizeof(int));
	tmp_comp += sizeof(int);

	if (test_params_p->parallel_io == FALSE)
	{
	    if ((cur_result.result_data = 
		 malloc(cur_result.size*sizeof(char))) == NULL)
	    {
		custom_debug(MASTER_ERR, 
			     "M:malloc result_data of size %d failed\n",
			     cur_result.size*sizeof(char));
		return -1;
	    }
	    memcpy(cur_result.result_data, cur_irecv_p->result + tmp_comp, 
		   cur_result.size * sizeof(char));
	    tmp_comp += cur_result.size * sizeof(char);
	}
	else
	{
	    cur_result.result_data = NULL;
	}

        if (i == 0)
        {
            lresult_head = create_list_head(&cur_result);
        }
        else
            push_list(lresult_head, &cur_result);

        if (test_params_p->parallel_io == FALSE)
            free(cur_result.result_data);

	i++;
	
	if (tmp_comp == cur_irecv_p->result_size)
	{
	    custom_debug(MASTER, "M:%d results added "
			 "(query=%d,frag=%d,size=%d)\n", i,
			 cur_query, cur_frag, cur_irecv_p->result_size);
	    break;
	}
	else if (tmp_comp > cur_irecv_p->result_size)
	{
	    custom_debug(MASTER, "M:Results error "
			 "(query=%d,frag=%d,size=%Ld,processed=%d)\n",
			 cur_query, cur_frag, cur_irecv_p->result_size,
			 tmp_comp);
	    return -1;
	}
    }
    
    assert(tmp_comp == cur_irecv_p->result_size);
    
    if (query_info_list[cur_query].lresult_head == NULL)
    {
	query_info_list[cur_query].lresult_head = lresult_head;
    }
    else
    {
	struct lresult_s *tmp_list_head = NULL;
	/* Merge the results together */
	tmp_list_head = merge_lresult(
	    query_info_list[cur_query].lresult_head, lresult_head);
	query_info_list[cur_query].lresult_head = tmp_list_head;
    }

#if 0
    print_lresult_list(query_result_list[frag_result_info.query]);
#endif

    custom_debug(MASTER_MSG, 
		 "M:Query[%d](frags scheduled=%d,frags completed=%d,max=%d)\n",
		 cur_query,
		 query_info_list[cur_query].frag_sched,
		 query_info_list[cur_query].frag_comp,
		 test_params_p->total_frags);
    free(cur_irecv_p->result);
    cur_irecv_p->result = NULL;

    return 0;
}

/**
 * For a specific worker, generate the I/O offset array for where its
 * result data should be written in the shared output result file.  A
 * first pass over the results finds the size of the offset array to
 * be allocated.  The second pass figures out what they should be.
 *
 * @param worker                  MPI id of worker process.
 * @param cur_io_query_p          Pointer to the index of current I/O query
 *                                in the query_proc_isend_matrix.
 * @param cur_file_offset         Where we are in the output file.
 * @param query_proc_isend_matrix Pointer to an element within the matrix 
 *                                which has the isend information.
 * @param query_result_list       The head of the result list for a query.
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
static int generate_io_offset_pairs(
    int worker,
    int *cur_io_query_p,
    int64_t cur_file_offset,
    struct isend_info_s **query_proc_isend_matrix,
    struct lresult_s *query_result_list,
    struct mpe_events_s *mpe_events_p,
    struct test_params_s *test_params_p)
{
    int file_offset_list_count = 0;
    int64_t tmp_offset = cur_file_offset;
    int tmp_count = 0;
    struct lresult_s *list_head_p = query_result_list;

    custom_debug(MASTER, 
		 "M:Start generate_io_offset_pairs for query=%d "
		 "init offset=%d\n", *cur_io_query_p, tmp_offset);
    
    if (list_head_p == NULL)
    {
	custom_debug(MASTER_ERR, "M:no results for query=%d\n",
		     *cur_io_query_p);
	return 0;
    }
    /* First pass to figure out how many pieces */
    while (list_head_p->next != NULL)
    {
	if (list_head_p->result.proc_id == worker)
	    file_offset_list_count++;
	list_head_p = list_head_p->next;
    }
    if (list_head_p->result.proc_id == worker)
	file_offset_list_count++;

    custom_debug(MASTER, "M:found %d piece(s) in query=%d for proc %d\n",
		 file_offset_list_count, *cur_io_query_p,
		 worker);

    if (file_offset_list_count != 0)
    {
	if ((query_proc_isend_matrix[*cur_io_query_p][worker].offset_list = 
	     (int64_t *) malloc(file_offset_list_count * 
				sizeof(int64_t))) == NULL)
	{
	    custom_debug(MASTER_ERR,
			 "M:malloc query_proc_isend_matrix[%d][%d] of "
			 "size %d failed\n", *cur_io_query_p, worker,
			 file_offset_list_count * sizeof(int64_t));
	    return -1;
	}
    }
    
    /* Second pass to generate the offsets for each piece */
    list_head_p = query_result_list;
    while (list_head_p->next != NULL)
    {
	if (list_head_p->result.proc_id == worker)
	{
	    query_proc_isend_matrix[*cur_io_query_p][worker].
		offset_list[tmp_count] = tmp_offset;
	    tmp_count++;
	}
	tmp_offset += list_head_p->result.size;
	list_head_p = list_head_p->next;
    }
    if (list_head_p->result.proc_id == worker)
    {
	query_proc_isend_matrix[*cur_io_query_p][worker].
	    offset_list[tmp_count] = tmp_offset;
	tmp_count++;
    }
    tmp_offset += list_head_p->result.size;
    
    assert(file_offset_list_count == tmp_count);
    
    query_proc_isend_matrix[*cur_io_query_p][worker].offset_list_count =
	tmp_count;
    
#if 0
    if (tmp_count != 0)
	print_offset_list(query_proc_offset_info_matrix[*cur_io_query_p][
			      worker].offset_list, tmp_count);
#endif
    return 0;
}

/**
 * When using parallel I/O, the workers will write their own results
 * to file.  Since the file is shared, the workers need to know the
 * offsets of where their results belong in the file (since it is
 * globally ordered).  This function sends out 2 messages to each
 * worker for a given query.  First, it sends out the number of
 * offsets and then it sends the actual offset array.
 *
 * @param numprocs                Number of processes used. 
 * @param cur_file_offset         Where we are in the output file.
 * @param cur_query               The query which has been globally sorted 
 *                                and is ready for I/O. 
 * @param query_info_list         Pointer to array of progress for each query. 
 * @param query_proc_isend_matrix Pointer to an element within the matrix 
 *                                which has the isend information.
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
int isend_offsets(int numprocs,
		  int cur_file_offset,
		  int cur_query,
		  struct query_info_s *query_info_list,
		  struct isend_info_s **query_proc_isend_matrix,
		  struct mpe_events_s *mpe_events_p,
		  struct test_params_s *test_params_p)
{
    int i, err;

    for (i = 1; i < numprocs; i++)
    {
	generate_io_offset_pairs(i,
				 &cur_query,
				 cur_file_offset,
				 query_proc_isend_matrix,
				 query_info_list[cur_query].lresult_head,
				 mpe_events_p,
				 test_params_p);

	if (query_proc_isend_matrix[cur_query][i].offset_list_count > 0)
	{
	    /* Send off offset_list_count & offset_list */
	    err = MPI_Isend(&(query_proc_isend_matrix
			    [cur_query][i].offset_list_count),
			    sizeof(int64_t), MPI_BYTE, i,
			    MASTER_SEND_OFFSETS_COUNT, MPI_COMM_WORLD,
			    &(query_proc_isend_matrix
			      [cur_query][i].offset_list_count_req));
	    custom_debug(
		MASTER_ISEND, 
		"M:ISent OFFSETS_COUNT (addr=%d) to proc %d ret=%d\n",
		&(query_proc_isend_matrix[cur_query][i].
		  offset_list_count_req), 
		i, err);
	    err = MPI_Isend(query_proc_isend_matrix
			    [cur_query][i].offset_list,
			    query_proc_isend_matrix
			    [cur_query][i].offset_list_count *
			    sizeof(int64_t), MPI_BYTE, i,
			    MASTER_SEND_OFFSETS, MPI_COMM_WORLD,
			    &(query_proc_isend_matrix
			      [cur_query][i].offset_list_req));
	    custom_debug(
		MASTER_ISEND, 
		"M:ISent %Ld OFFSETS (addr=%d) to proc %d ret=%d\n",
		query_proc_isend_matrix[cur_query][i].offset_list_count,
		&(query_proc_isend_matrix[cur_query][i]
		  .offset_list_req), 
		i, err);
	    query_proc_isend_matrix[cur_query][i].state = 
		WAIT_OFFSET_LIST_COUNT;
	}
	else
	{
	    query_proc_isend_matrix[cur_query][i].state = 
		DONE_OFFSET_LIST;
	}
    }
    return 0;
}

/**
 * Find the next start_file_offset by going through the result list
 * and adding up the sizes of the results. 
 *
 * @param query_result_list       The head of the result list for a query.
 * @return                        The start_file_offset.
 */
int64_t calculate_start_file_offset(struct lresult_s *query_result_list)
{
    int64_t start_file_offset = 0;
    struct lresult_s *list_head_p = query_result_list;
    
    while (list_head_p->next != NULL)
    {
	start_file_offset += list_head_p->result.size;
	list_head_p = list_head_p->next;
    }
    start_file_offset += list_head_p->result.size;
    
    return start_file_offset;
}

/**
 * If the master-writing mode is chosen, this function is called to
 * write the results to file in order.  The result data is copied into
 * a temporary buffer and then written contiguously to file.
 *
 * @param cur_query_p             The query which has been globally sorted 
 *                                and is ready for I/O. 
 * @param cur_file_offset_p       A pointer to where we are in the output file.
 * @param query_info_list         Pointer to array of progress for each query. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 * @return                        0 on success.
 */
int do_all_io(int *cur_io_query_p,
	      int64_t *cur_file_offset_p,
	      struct query_info_s *query_info_list,
	      struct mpe_events_s *mpe_events_p,
	      struct test_params_s *test_params_p)
{
    char *tmp_buf = NULL;
    struct lresult_s *cur_io_query_result_p = 
	query_info_list[*cur_io_query_p].lresult_head;
    int total_size = 0, total_comp = 0;
    MPI_Status status;

    if (test_params_p->no_io == TRUE)
    {
        if (test_params_p->io_method == COLLECTIVE_IO)
            MPI_Barrier(MPI_COMM_WORLD);
        return 0;
    }

    if (cur_io_query_result_p == NULL)
    {
	custom_debug(MASTER_IO, "M:Wrote %d bytes for query=%d to file %s\n",
		     total_size,
		     *cur_io_query_p,
		     test_params_p->output_file);
	return 0;
    }

    while (cur_io_query_result_p-> next != NULL)
    {
	total_size += cur_io_query_result_p->result.size;
	cur_io_query_result_p = cur_io_query_result_p-> next;
    }
    /* Don't forget the last one */
    total_size += cur_io_query_result_p->result.size;
    
    if ((tmp_buf = (char *) malloc(total_size*sizeof(char))) == NULL)
    {
	custom_debug(MASTER_ERR, "M:malloc tmp_buf of size %d failed\n",
		     total_size*sizeof(char));
	return -1;
    }
    
    cur_io_query_result_p = query_info_list[*cur_io_query_p].lresult_head;
    while (cur_io_query_result_p-> next != NULL)
    {
	memcpy(tmp_buf + total_comp, 
	       cur_io_query_result_p->result.result_data, 
	       cur_io_query_result_p->result.size);
	total_comp += cur_io_query_result_p->result.size;
	cur_io_query_result_p = cur_io_query_result_p-> next;
    }
    /* Don't forget the last one */
    memcpy(tmp_buf + total_comp, 
	   cur_io_query_result_p->result.result_data, 
	   cur_io_query_result_p->result.size);
    total_comp += cur_io_query_result_p->result.size;

    assert(total_comp == total_size);
    
    MPI_File_set_view(test_params_p->fh, *cur_file_offset_p, 
		      MPI_BYTE, MPI_BYTE, 
		      "native", *(test_params_p->info_p));
    MPI_File_write(test_params_p->fh, tmp_buf, total_size, 
		   MPI_BYTE, &status);
    free(tmp_buf);
    MPI_File_sync(test_params_p->fh);

    (*cur_file_offset_p) += total_size;
    
    custom_debug(MASTER_IO, "M:Wrote %d bytes for query=%d to file %s\n",
		 total_size,
		 *cur_io_query_p,
		 test_params_p->output_file);

    return 0;
}

/**
 * If the worker-writing mode is chosen with collective I/O, the
 * master must also participate since it is in the same communication
 * group.  While it does not actually write any data, the master will
 * also call MPI_File_write_all().
 *
 * @param mpe_events_p  Pointer to timing structure.
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
int do_dummy_io(struct mpe_events_s *mpe_events_p,
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
 * Check the query_proc_isend_matrix to see how the isends are
 * progressing.  Mark the state of each process separately.
 *
 * @param wait_status              Status of master process (wait or no wait).
 * @param numprocs                 Number of processes used. 
 * @param last_query_offset_sent   The last query where all the offsets were 
 *                                 sent.
 * @param last_query_offset_comp_p Pointer to the index of the last query 
 *                                 which completed all isends.
 * @param query_info_list          Pointer to array of progress for each query.
 * @param query_proc_isend_matrix  Pointer to an element within the matrix 
 *                                 which has the isend information.
 * @param mpe_events_p             Pointer to timing structure.
 * @param test_params_p            Pointer to test_params.    
 * @return                         0 on success.
 */
int master_check_isend_offsets(int wait_status,
			       int numprocs,
			       int last_query_offset_sent,
			       int *last_query_offset_comp_p,
			       struct query_info_s *query_info_list,
			       struct isend_info_s **query_proc_isend_matrix,
			       struct mpe_events_s *mpe_events_p,
			       struct test_params_s *test_params_p)
{
    struct isend_info_s *cur_isend_p = NULL;
    int i, flag, err = -1;
    
    custom_debug(MASTER, "M:master_check_isend_offsets (sent=%d,comp=%d)\n",
		  last_query_offset_sent,
		 *last_query_offset_comp_p);

    /* Try the next one since we completed the last one */
    (*last_query_offset_comp_p)++;    
    while (1)
    {
	for (i = 1; i < numprocs; i++)
	{
	    cur_isend_p = &(query_proc_isend_matrix
			    [(*last_query_offset_comp_p)][i]);
	    if (cur_isend_p->state == NOT_USED)
	    {
		/* Fall back to the last query completed */
		custom_debug(MASTER_GATHER, 
			     "M:(query=%d,proc=%d) is NOT_USED\n",
			     *last_query_offset_comp_p, i);
		(*last_query_offset_comp_p)--;
		return 0;
	    }

	    if (cur_isend_p->state == WAIT_OFFSET_LIST_COUNT)
	    {
		custom_debug(MASTER_GATHER,
                             "M:Test/Wait for isend OFFSET_LIST_COUNT "
                             "(query=%d,proc=%d,req_addr=%d,ret=%d)\n",
                             *last_query_offset_comp_p, i,
                             &(cur_isend_p->offset_list_count_req),
                             err);
		check_isend(MASTER_NODE,
                            wait_status,
                            &flag,
                            &(cur_isend_p->offset_list_count_req),
                            &(cur_isend_p->state),
                            WAIT_OFFSET_LIST,
                            mpe_events_p,
                            test_params_p);
		if (flag == 0)
		{
		    /* Fall back to the last query completed */
		    (*last_query_offset_comp_p)--;
		    return 0;
		}
	    }	    
	    
	    if (cur_isend_p->state == WAIT_OFFSET_LIST)
	    {
		custom_debug(MASTER_GATHER,
                             "M:Test/Wait for isend OFFSET_LIST "
                             "(query=%d,proc=%d,req_addr=%d,ret=%d)\n",
                             *last_query_offset_comp_p, i,
                             &(cur_isend_p->offset_list_count_req),
                             err);
		check_isend(MASTER_NODE,
                            wait_status,
                            &flag,
                            &(cur_isend_p->offset_list_req),
                            &(cur_isend_p->state),
                            DONE_OFFSET_LIST,
                            mpe_events_p,
                            test_params_p);
		if (flag == 0)
		{
		    /* Fall back to the last query completed */
		    (*last_query_offset_comp_p)--;
		    return 0;
		}

		free(cur_isend_p->offset_list);
	    }	    
	}
	custom_debug(MASTER_ISEND, 
		     "M:All isends for query=%d completed (max_queries=%d)\n",
		     *last_query_offset_comp_p,
		     test_params_p->query_count);

	if (*last_query_offset_comp_p == last_query_offset_sent)
	    return 0;
	else  
	    (*last_query_offset_comp_p)++;
    }
    
    /* Should never get here */
    return -1;
}

/**
 * Debugging function to check the generated offset array.
 *
 * @param offset_list       The array of offsets.
 * @param offset_list_count The number of offsets lin the list.
 */
void print_offset_list(int64_t *offset_list,
		       int offset_list_count)
{
    int i = 0;
    int row_end;
    for (i = 0; i < offset_list_count; i++)
    {
	if (i % 10 == 0)
	{
	    if (offset_list_count - 1 < i + 9)
		row_end = offset_list_count;
	    else
		row_end = i + 9;

	    fprintf(stderr, "offset_list[%d-%d]=(%Ld ", i, row_end,
		    offset_list[i]);
	}
	else if (i % 10 == 9)
	{
	    fprintf(stderr, "%Ld)\n", offset_list[i]);
	}
	else
	{
	    fprintf(stderr, "%Ld ", offset_list[i]);
	}
    }
    fprintf(stderr, ")\n");
}

/**
 * Do a final check on the query_frag_irecv_matrix for any uncompleted
 * irecvs.  None should exist.
 *
 * @param numprocs                Number of processes used. 
 * @param query_frag_irecv_matrix Pointer to matrix of irecv request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 */
void master_check_all_irecv(int numprocs,
			    struct irecv_info_s **query_frag_irecv_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p)
{
    MPI_Status status;
    int i, j, flag = -1;
    
    for (i = 0; i < test_params_p->query_count; i++)
    {
	for (j = 0; j < test_params_p->total_frags; j++)
        {
	    MPI_Test(&(query_frag_irecv_matrix[i][j].result_size_req),
                     &flag, &status);
	    if (flag != 1)
	    {
		custom_debug(WORKER_ERR, "M: query_frag_irecv_matrix[%d]"
			     "[%d] result_size_req=%d instead of "
			     "NULL or MPI_REQUEST_NULL\n",
			     i, j, 
			     query_frag_irecv_matrix[i][j].result_size_req);
	    }
	    MPI_Test(&(query_frag_irecv_matrix[i][j].result_req),
                     &flag, &status);
	    if (flag != 1)
	    {
		custom_debug(WORKER_ERR, "M: query_frag_irecv_matrix[%d]"
			     "[%d] result_req=%d instead of "
			     "NULL or MPI_REQUEST_NULL\n",
			     i, j, 
			     query_frag_irecv_matrix[i][j].result_req);
	    }
        }
    }
}

/**
 * Do a final check on the query_frag_isend_matrix for any uncompleted
 * isends.  None should exist.
 *
 * @param numprocs                Number of processes used. 
 * @param query_frag_isend_matrix Pointer to matrix of isend request 
 *                                information. 
 * @param mpe_events_p            Pointer to timing structure.
 * @param test_params_p           Pointer to test_params.    
 */
void master_check_all_isend(int numprocs,
			    struct isend_info_s **query_proc_isend_matrix,
			    struct mpe_events_s *mpe_events_p,
			    struct test_params_s *test_params_p)
{
    MPI_Status status;
    int i, j, flag = -1;
    
    for (i = 0; i < test_params_p->query_count; i++)
    {
        for (j = 0; j < numprocs; j++)
        {
	    MPI_Test(&(query_proc_isend_matrix[i][j].offset_list_count_req),
                     &flag, &status);
	    if (flag != 1)
            {
                custom_debug(WORKER_ERR, "M: query_proc_isend_matrix[%d]"
                             "[%d] offset_list_count_req=%d instead of "
                             "NULL or MPI_REQUEST_NULL\n",
                             i, j,
                             query_proc_isend_matrix[i][j].
			     offset_list_count_req);
            }
	    MPI_Test(&(query_proc_isend_matrix[i][j].offset_list_req),
                     &flag, &status);
	    if (flag != 1)
            {
                custom_debug(WORKER_ERR, "M: query_proc_isend_matrix[%d]"
                             "[%d] offset_list_req=%d instead of "
                             "NULL or MPI_REQUEST_NULL\n",
                             i, j,
                             query_proc_isend_matrix[i][j].
			     offset_list_req);
            }
	}
    }	    
}
