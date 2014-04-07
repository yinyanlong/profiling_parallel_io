/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "worker.h"
#include "worker_help.h"

/**
 * This handles the high-level functions of the worker process during
 * the majority of the application run.  There is a setup phase,
 * followed by a work phase, and then cleanup operations.
 *
 * @param myid          MPI myid.
 * @param numprocs      Number of processes used. 
 * @param mpe_events_p  Pointer to timing structure.      
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
int worker(int myid,
	   int numprocs,
	   struct mpe_events_s *mpe_events_p,
	   struct test_params_s *test_params_p)
{
    int i, j, wait_status = FALSE;

    /* Variables to keep state of isends, irecvs, work */
    struct work_info_s last_query_result_sent;
    struct work_info_s last_query_result_comp;
    int last_query_offset_recv = -1;
    int last_query_offset_comp = -1;
    int last_io_comp = -1;
    struct work_info_s prev_work_info;
    struct work_info_s cur_work_info;

    struct query_result_s *query_result_list = NULL;
    struct isend_info_s **query_frag_isend_matrix = NULL;

    /* Setup */
    custom_MPE_Log_event(mpe_events_p->setup_start, 0, NULL, mpe_events_p);
    get_test_params(myid,
		    numprocs,
		    mpe_events_p,
		    test_params_p);

    if (test_params_p->io_method == COLLECTIVE_IO)
	MPI_File_open(MPI_COMM_WORLD, test_params_p->output_file,
		      MPI_MODE_CREATE | MPI_MODE_WRONLY, 
		      *(test_params_p->info_p), 
		      &(test_params_p->fh));
    else
	MPI_File_open(MPI_COMM_SELF, test_params_p->output_file,
		      MPI_MODE_CREATE | MPI_MODE_WRONLY, 
		      *(test_params_p->info_p), 
		      &(test_params_p->fh));

    /* Set MPI atomic mode */
    MPI_File_set_atomicity(test_params_p->fh,
			   test_params_p->atomicity);

    /* Create the bookkeeping data structures on the worker. */
    if ((query_result_list = (struct query_result_s *)
         malloc(test_params_p->query_count *
                sizeof(struct query_result_s))) == NULL)
    {
        custom_debug(WORKER_ERR, 
		     "W%d:malloc query_result_list of size %d failed\n",
		     myid, 
		     test_params_p->query_count * 
		     sizeof(struct query_result_s));
        return -1;
    }
    memset(query_result_list, 0, test_params_p->query_count *
           sizeof(struct query_result_s));
    for (i = 0; i < test_params_p->query_count; i++)
    {
	query_result_list[i].state = NOT_USED;
	query_result_list[i].offset_list_req = MPI_REQUEST_NULL;
	query_result_list[i].offset_list_count_req = MPI_REQUEST_NULL;
    }

    if ((query_frag_isend_matrix = (struct isend_info_s **)
         malloc(test_params_p->query_count *
                sizeof(struct isend_info_s *))) == NULL)
    {
        custom_debug(MASTER_ERR,
                     "M:malloc query_frag_isend_matrix of size %d "
                     "failed\n", test_params_p->query_count *
                     sizeof(struct isend_info_s *));
        return -1;
    }
    for (i = 0; i < test_params_p->query_count; i++)
    {
        if ((query_frag_isend_matrix[i] = (struct isend_info_s *)
             malloc(test_params_p->total_frags * 
		    sizeof(struct isend_info_s))) == NULL)
        {
            custom_debug(MASTER_ERR,
                         "M:malloc query_frag_isend_matrix[%d] of "
                         "size %d failed\n", i, test_params_p->query_count *
                         sizeof(struct isend_info_s));
            return -1;
        }
        memset(query_frag_isend_matrix[i], 0,
               test_params_p->total_frags * sizeof(struct isend_info_s));
        for (j = 0; j < test_params_p->total_frags; j++)
	{
            query_frag_isend_matrix[i][j].result_size_req = MPI_REQUEST_NULL;
            query_frag_isend_matrix[i][j].result_req = MPI_REQUEST_NULL;
	}
    }
    custom_MPE_Log_event(mpe_events_p->setup_end, 0, NULL, mpe_events_p);

    memset(&prev_work_info, 0, sizeof(struct work_info_s));
    memset(&cur_work_info, 0, sizeof(struct work_info_s));
    last_query_result_comp.query = 0;
    last_query_result_comp.frag  = -1;
    last_query_result_sent.query = 0;
    last_query_result_sent.frag  = -1;

    cur_work_info.query = NO_WORK;

    while (1)
    {
	custom_MPE_Log_event(mpe_events_p->data_distribution_start, 
			     0, NULL, mpe_events_p);
	send_recv_work_req(myid, 
			   &wait_status,
			   &cur_work_info,
			   mpe_events_p,
			   test_params_p);
	custom_MPE_Log_event(mpe_events_p->data_distribution_end, 
			     0, NULL, mpe_events_p);

	if (wait_status == RET_QUERY_FRAG)
	{
	    /* Compute */
	    custom_MPE_Log_event(mpe_events_p->compute_start, 
				 0, NULL, mpe_events_p);
	    do_work(myid,
		    &cur_work_info,
		    query_frag_isend_matrix,
		    mpe_events_p,
		    test_params_p);
	    custom_MPE_Log_event(mpe_events_p->compute_end, 
				 0, NULL, mpe_events_p);


	    if (test_params_p->parallel_io == TRUE)
	    {
		/* Merge results if we are going to write them */
		custom_MPE_Log_event(mpe_events_p->merge_results_start,
				     0, NULL, mpe_events_p);
		merge_results(myid,
			      &cur_work_info,
			      query_result_list,
			      query_frag_isend_matrix,
			      mpe_events_p,
			      test_params_p);
		custom_MPE_Log_event(mpe_events_p->merge_results_end,
				     0, NULL, mpe_events_p);
	    }
	    
	    /* Isend Results */
	    custom_MPE_Log_event(mpe_events_p->gather_results_start, 
				 0, NULL, mpe_events_p);
	    worker_isend_results(myid,
				 &cur_work_info,
				 &last_query_offset_recv,
				 query_frag_isend_matrix,
				 mpe_events_p,
				 test_params_p);
	    custom_MPE_Log_event(mpe_events_p->gather_results_end, 
				 0, NULL, mpe_events_p);

	    /* If there were no results generated skip merging and 
	     * processing */
	    if (query_frag_isend_matrix[cur_work_info.query]
		[cur_work_info.frag].tmp_result_list_count != 0)
	    {
		last_query_result_sent.query = cur_work_info.query;
		last_query_result_sent.frag = cur_work_info.frag;
	    }	    

	    /* If this is the first fragment for this query, post an
	     * irecv for the size of the offset list */
	    if (test_params_p->parallel_io == TRUE &&
		query_result_list[cur_work_info.query].state == NOT_USED)
	    {
		custom_MPE_Log_event(
		    mpe_events_p->send_processed_results_start, 
		    0, NULL, mpe_events_p);
		worker_post_irecv(myid,
				  &cur_work_info,
				  query_result_list,
				  mpe_events_p,
				  test_params_p);
		custom_MPE_Log_event(
		    mpe_events_p->send_processed_results_end, 
		    0, NULL, mpe_events_p);
	    }
	    
	}	    
	else
	{
	    /* If we have received CUR_QUERY_SCHED_DONE or
	     * ALL_QUERY_SCHED_DONE from the master then we know that
	     * we can update the last_query_result_sent and progress
	     * to checking for offset lists if we are doing parallel
	     * I/O */
	    assert(cur_work_info.query >= last_query_result_sent.query &&
		   cur_work_info.query >= last_query_result_comp.query);
	    
	    last_query_result_sent.query = cur_work_info.query;
	    last_query_result_sent.frag  = test_params_p->total_frags - 1;
	    last_query_offset_recv = cur_work_info.query;
	}


      	/* Check on the isends of results and see how they are progressing */
	custom_MPE_Log_event(mpe_events_p->gather_results_start, 
			     0, NULL, mpe_events_p);
	worker_check_isend_results(myid,
				   wait_status,
				   &last_query_result_sent,
				   &last_query_result_comp,
				   query_result_list,
				   query_frag_isend_matrix,
				   mpe_events_p,
				   test_params_p);
	custom_MPE_Log_event(mpe_events_p->gather_results_end, 
			     0, NULL, mpe_events_p);

	/* Check the irecvs of offsets and if they are done, do the write 
	 * and clear out the buffers */
	if (test_params_p->parallel_io == TRUE)
	{
	    custom_MPE_Log_event(mpe_events_p->send_processed_results_start, 
				 0, NULL, mpe_events_p);
	    custom_debug(WORKER, "W%d:checking irecv offset up to query=%d "
			 "starting from query=%d\n", myid, 
			 last_query_offset_recv, last_query_offset_comp);
	    worker_check_irecv_offsets(myid,
				       wait_status,
				       last_query_offset_recv,
				       &last_query_offset_comp,
				       query_result_list,
				       mpe_events_p,
				       test_params_p);
	    custom_MPE_Log_event(mpe_events_p->send_processed_results_end, 
				 0, NULL, mpe_events_p);

	    if (test_params_p->end_write == FALSE)
	    {
		custom_MPE_Log_event(mpe_events_p->io_start, 
				     0, NULL, mpe_events_p);
		do_all_worker_io(myid,
				 wait_status,
				 last_query_offset_comp,
				 &last_io_comp,
				 query_result_list,
				 mpe_events_p,
				 test_params_p);
		custom_debug(WORKER, "W%d:finished I/O up to query=%d\n",
			     myid, last_io_comp);
		custom_MPE_Log_event(mpe_events_p->io_end, 
				     0, NULL, mpe_events_p);
	    }

	    if (wait_status == ALL_QUERY_SCHED_DONE)
	    {
		custom_debug(WORKER, "W%d:All queries done!\n",
			     myid);
		break;
	    }
	}

	/* If we are query-syncing, we need to barrier and start the next 
	 * query */
	if (test_params_p->query_sync == TRUE &&
	    wait_status == CUR_QUERY_SCHED_DONE)
	{
	    custom_debug(WORKER, "W%d:MPI_Barrier()\n", myid);
	    custom_MPE_Log_event(mpe_events_p->sync_start,
				 0, NULL, mpe_events_p);
	    MPI_Barrier(MPI_COMM_WORLD);
	    custom_MPE_Log_event(mpe_events_p->sync_end,
				 0, NULL, mpe_events_p);
	}
	    
	/* We're done */
	if (wait_status == ALL_QUERY_SCHED_DONE &&
	    (test_params_p->query_sync == TRUE ||
	     test_params_p->parallel_io == FALSE))
	{
	    break;
	}
    }

    /* Do I/O at the end if we so choose */
    if (test_params_p->end_write == TRUE &&
	test_params_p->parallel_io == TRUE)
    {
	custom_MPE_Log_event(mpe_events_p->io_start, 0, NULL, mpe_events_p);
	do_all_worker_io(myid,
			 wait_status,
			 last_query_offset_comp,
			 &last_io_comp,
			 query_result_list,
			 mpe_events_p,
			 test_params_p);
	custom_MPE_Log_event(mpe_events_p->io_end, 0, NULL, mpe_events_p);
    }

    /* Check for pending messages */
    worker_check_all_irecv(myid,
			   numprocs,
			   query_result_list,
			   mpe_events_p,
			   test_params_p);
    worker_check_all_isend(myid,
			   numprocs,
			   query_frag_isend_matrix,
			   mpe_events_p,
			   test_params_p);

    /* Clean up */
    for (i = 0; i < test_params_p->query_count; i++)
    {
	free(query_frag_isend_matrix[i]);
    }
    free(query_frag_isend_matrix);

    free(query_result_list);

    if (test_params_p->query_hist_list_count != 0)
	free(test_params_p->query_hist_list);
    if (test_params_p->db_hist_list_count != 0)
	free(test_params_p->db_hist_list);

    MPI_File_close(&(test_params_p->fh));

    custom_debug(WORKER, "W%d:Finished\n", myid);
    return 0;
}

