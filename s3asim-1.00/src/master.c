/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "master.h"
#include "master_help.h"

/**
 * This handles the high-level functions of the master process during
 * the majority of the application run.  There is a setup phase,
 * followed by a work phase, and then cleanup operations.
 *
 * @param myid          MPI myid.
 * @param numprocs      Number of processes used. 
 * @param mpe_events_p  Pointer to timing structure.      
 * @param test_params_p Pointer to test_params.    
 * @return              0 on success.
 */
int master(int myid,
	   int numprocs,
	   struct mpe_events_s *mpe_events_p,
	   struct test_params_s *test_params_p)
{
    MPI_Status status;
    int wait_status = FALSE;

    /* Variables to keep state of isends, irecvs, work */    
    int last_query_result_comp = -1;
    int last_query_offset_sent = -1;
    int last_query_offset_comp = -1;
    int cur_query = 0;
 
    int64_t cur_file_offset = 0; /* How far are we into the file */
    int i, j, workers_wait_io_notified = 0,
	workers_done_notified = 0;
    int recv_req;
    struct work_info_s work_info;

    struct query_info_s *query_info_list = NULL;
    struct isend_info_s **query_proc_isend_matrix = NULL;
    struct irecv_info_s **query_frag_irecv_matrix = NULL;

    int hint_key_len, hint_val_len, hint_nkeys, flag = -1;
    char hint_key[MPI_MAX_INFO_KEY], *hint_val = NULL;

    /* Setup Phase 
     * - We will broadcast global information to each worker node.*/
    custom_MPE_Log_event(mpe_events_p->setup_start, 0, NULL, mpe_events_p);
    MPI_Bcast(test_params_p, sizeof(struct test_params_s), MPI_BYTE, 
	      MASTER_NODE, MPI_COMM_WORLD);
    MPI_Bcast(test_params_p->output_file,
              test_params_p->output_file_len + 1,
              MPI_BYTE, MASTER_NODE, MPI_COMM_WORLD);
    if (test_params_p->query_hist_list_count != 0)
	MPI_Bcast(test_params_p->query_hist_list,
		  test_params_p->query_hist_list_count * 
		  sizeof(struct hist_s), MPI_BYTE,
		  MASTER_NODE, MPI_COMM_WORLD);
    if (test_params_p->db_hist_list_count != 0)
	MPI_Bcast(test_params_p->db_hist_list,
		  test_params_p->db_hist_list_count * 
		  sizeof(struct hist_s), MPI_BYTE,
		  MASTER_NODE, MPI_COMM_WORLD);
    MPI_Info_get_nkeys(*(test_params_p->info_p), &hint_nkeys);
    MPI_Bcast(&hint_nkeys, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (i = 0; i < hint_nkeys; i++)
    {
	MPI_Info_get_nthkey(*(test_params_p->info_p), i, hint_key);
	hint_key_len = strlen(hint_key);
	MPI_Info_get_valuelen(*(test_params_p->info_p), hint_key,
			      &hint_val_len, &flag);
	assert(flag);

	hint_val = malloc((hint_val_len + 1)*sizeof(char));
	if (!hint_val)
	{
	    custom_debug(MASTER_ERR,
			 "hint_val malloc of size %d failed.\n",
			 hint_val_len);
	    return -1;
	}

	MPI_Info_get(*(test_params_p->info_p), hint_key,
		     hint_val_len + 1, hint_val, &flag);
	assert(flag);

        MPI_Bcast(&hint_key_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(hint_key, hint_key_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&hint_val_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(hint_val, hint_val_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        free(hint_val);
    }

    /* There is no collective I/O when the master is writing */
    assert(!(test_params_p->io_method == COLLECTIVE_IO &&
	     test_params_p->parallel_io == FALSE));

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

    /* Create the bookkeeping data structures on the master.*/
    if ((query_info_list = (struct query_info_s *) 
	 malloc(test_params_p->query_count * 
		sizeof(struct query_info_s))) == NULL)
    {
	custom_debug(MASTER_ERR, 
		     "M:malloc query_info_list of size %d failed\n",
		     test_params_p->query_count * sizeof(struct query_info_s));
	return -1;
    }
    memset(query_info_list, 0, test_params_p->query_count * 
	   sizeof(struct query_info_s));

    if ((query_proc_isend_matrix = (struct isend_info_s **)
	 malloc(test_params_p->query_count * 
		sizeof(struct isend_info_s *))) == NULL)
    {
	custom_debug(MASTER_ERR,
		     "M:malloc query_proc_isend_matrix of size %d "
		     "failed\n", test_params_p->query_count * 
		     sizeof(struct isend_info_s *));
	return -1;
    }
    for (i = 0; i < test_params_p->query_count; i++)
    {
	if ((query_proc_isend_matrix[i] = (struct isend_info_s *) 
	     malloc(numprocs * sizeof(struct isend_info_s))) == NULL)
	{
	    custom_debug(MASTER_ERR,
			 "M:malloc query_proc_isend_matrix[%d] of "
			 "size %d failed\n", i, test_params_p->query_count * 
			 sizeof(struct isend_info_s));
	    return -1;
	}
	memset(query_proc_isend_matrix[i], 0, 
	       numprocs * sizeof(struct isend_info_s));
	for (j = 0; j < numprocs; j++)
	{
	    query_proc_isend_matrix[i][j].offset_list_req 
		= MPI_REQUEST_NULL;
	    query_proc_isend_matrix[i][j].offset_list_count_req 
		= MPI_REQUEST_NULL;
	}
    }

    if ((query_frag_irecv_matrix = (struct irecv_info_s **)
	 malloc(test_params_p->query_count * 
		sizeof(struct irecv_info_s *))) == NULL)
    {
	custom_debug(MASTER_ERR,
		     "M:malloc query_frag_irecv_matrix of size %d "
		     "failed\n", test_params_p->query_count * 
		     sizeof(struct irecv_info_s *));
	return -1;
    }
    for (i = 0; i < test_params_p->query_count; i++)
    {
	if ((query_frag_irecv_matrix[i] = (struct irecv_info_s *) 
	     malloc(test_params_p->total_frags * 
		    sizeof(struct irecv_info_s))) == NULL)
	{
	    custom_debug(MASTER_ERR,
			 "M:malloc query_frag_irecv_matrix[%d] of "
			 "size %d failed\n", i, test_params_p->query_count * 
			 sizeof(struct irecv_info_s));
	    return -1;
	}
	memset(query_frag_irecv_matrix[i], 0, 
	       test_params_p->total_frags * sizeof(struct irecv_info_s));
	for (j = 0; j < test_params_p->total_frags; j++)
	{
	    query_frag_irecv_matrix[i][j].result_size_req = MPI_REQUEST_NULL;
	    query_frag_irecv_matrix[i][j].result_req = MPI_REQUEST_NULL;
	}
    }
    custom_MPE_Log_event(mpe_events_p->setup_end, 0, NULL, mpe_events_p);

    memset(&work_info, 0, sizeof(struct work_info_s));
    while (1)
    {
	/* Send work or completion msgs to workers */
	custom_MPE_Log_event(mpe_events_p->data_distribution_start, 
			     0, NULL, mpe_events_p);
	MPI_Recv(&recv_req, 1, MPI_INT, MPI_ANY_SOURCE, MASTER_CONTROL, 
		 MPI_COMM_WORLD, &status);
	custom_MPE_Log_event(mpe_events_p->data_distribution_end, 
			     0, NULL, mpe_events_p);
	
	custom_debug(MASTER_MSG, "M:Recvd msg %s from proc %d\n", 
		     decode_req[recv_req], status.MPI_SOURCE); 
	
	if (recv_req == WORK_REQ)
	{
	    /* Data Distribution */
	    custom_MPE_Log_event(mpe_events_p->data_distribution_start, 
				 0, NULL, mpe_events_p);
	    wait_status = send_work(status.MPI_SOURCE,
				    &cur_query,
				    &work_info,
				    query_info_list,
				    mpe_events_p,
				    test_params_p);
	    custom_MPE_Log_event(mpe_events_p->data_distribution_end, 
				 0, NULL, mpe_events_p);	    
	    if (wait_status == ALL_QUERY_SCHED_DONE)
	    {
		workers_wait_io_notified++;
		workers_done_notified++;
	    }
	    else if (wait_status == CUR_QUERY_SCHED_DONE &&
		     test_params_p->query_sync == TRUE)
	    {
		workers_wait_io_notified++;
	    }
	    else if (wait_status == RET_QUERY_FRAG)
	    {
		wait_status = RET_QUERY_FRAG;
		/* Post the irecv for result size */
		custom_MPE_Log_event(mpe_events_p->gather_results_start,
				     0, NULL, mpe_events_p);
		post_irecv(status.MPI_SOURCE,
			   &work_info,
			   query_frag_irecv_matrix,
			   mpe_events_p,
			   test_params_p);
		custom_MPE_Log_event(mpe_events_p->gather_results_end,
				     0, NULL, mpe_events_p);
	    }
	    else
	    {
		custom_debug(MASTER_ERR, "M:Impossible msg\n");
		return -1;
	    }
	}
	else
	{
	    custom_debug(MASTER_ERR, "M:Impossible msg\n");
	    return -1;
	}
	
	/* Check to see if we have received any new results or 
	 * wait for all the results if all the queries have been scheduled */
	custom_MPE_Log_event(mpe_events_p->gather_results_start, 
			     0, NULL, mpe_events_p);
	master_check_irecv_results(cur_query,
				   &last_query_result_comp,
				   wait_status,
				   query_info_list,
				   query_frag_irecv_matrix,
				   mpe_events_p,
				   test_params_p);
	custom_MPE_Log_event(mpe_events_p->gather_results_end, 
			     0, NULL, mpe_events_p);

	/* Wait until everyone knows about the I/O to do I/O */
	if ((wait_status == ALL_QUERY_SCHED_DONE && 
	     workers_done_notified != numprocs - 1 ) ||
	    (wait_status == CUR_QUERY_SCHED_DONE &&
	     workers_wait_io_notified != numprocs - 1))
	{
	    continue;
	}

	/* Do whatever isends or I/O is possible */
	if (test_params_p->parallel_io == TRUE ||
	    test_params_p->end_write == FALSE)
	{
	    while (last_query_offset_sent + 1 <= last_query_result_comp)
	    {
		last_query_offset_sent++;
		if (test_params_p->parallel_io == TRUE ) 
		{
		    custom_MPE_Log_event(
			mpe_events_p->send_processed_results_start, 
			0, NULL, mpe_events_p);
		    isend_offsets(numprocs,
				  cur_file_offset,
				  last_query_offset_sent,
				  query_info_list,
				  query_proc_isend_matrix,
				  mpe_events_p,
				  test_params_p);
		    custom_MPE_Log_event(
			mpe_events_p->send_processed_results_end, 
			0, NULL, mpe_events_p);
		    /* Add to the file offset to not overwrite bytes */
		    if (query_info_list[last_query_offset_sent].lresult_head 
			!= NULL)
			cur_file_offset += calculate_start_file_offset(
			    query_info_list[last_query_offset_sent].
			    lresult_head);
		    /* If we are using collective I/O, we have to write zero */
		    if (test_params_p->io_method == COLLECTIVE_IO &&
			test_params_p->end_write == FALSE)
		    {
			custom_debug(MASTER, 
				     "M:starting dummy I/O for query=%d\n",
				     last_query_offset_sent);
			do_dummy_io(mpe_events_p, test_params_p);
		    }
		}
		else /* Master does I/O */
		{
		    /* do all the I/O */
		    custom_MPE_Log_event(mpe_events_p->io_start, 
					 0, NULL, mpe_events_p);
		    do_all_io(&last_query_offset_sent,
			      &cur_file_offset,
			      query_info_list,
			      mpe_events_p,
			      test_params_p);
		    custom_MPE_Log_event(mpe_events_p->io_end, 
					 0, NULL, mpe_events_p);
		}
		/* Free the data structures associated 
		 * with the wrriten data */
		if (query_info_list[last_query_offset_sent].lresult_head 
		    != NULL)
		    free_lresult_list(query_info_list[last_query_offset_sent].
				      lresult_head);	    
	    }
	}	

	if (test_params_p->parallel_io == TRUE)
	{
	    /* clean up if we are using isend */
	    custom_MPE_Log_event(mpe_events_p->send_processed_results_start, 
				 0, NULL, mpe_events_p);
	    master_check_isend_offsets(wait_status,
				       numprocs,
				       last_query_offset_sent,
				       &last_query_offset_comp,
				       query_info_list,
				       query_proc_isend_matrix,
				       mpe_events_p,
				       test_params_p);
	    custom_MPE_Log_event(mpe_events_p->send_processed_results_end, 
				 0, NULL, mpe_events_p);
	}
   
	if (wait_status == ALL_QUERY_SCHED_DONE)
	    break;

	/* After we barrier, move along to the next query if we 
	 * are query-synced. */
	if (test_params_p->query_sync == TRUE)
	{
	    if (wait_status == CUR_QUERY_SCHED_DONE)
	    {
		custom_debug(MASTER, "M:MPI_Barrier()\n");
		custom_MPE_Log_event(mpe_events_p->sync_start, 
				     0, NULL, mpe_events_p);
		MPI_Barrier(MPI_COMM_WORLD);
		custom_MPE_Log_event(mpe_events_p->sync_end, 
				     0, NULL, mpe_events_p);
		workers_wait_io_notified = 0;
		if (test_params_p->parallel_io == TRUE)
		{
		    assert(cur_query == last_query_offset_comp);
		}
		else if (test_params_p->end_write == FALSE)
		    assert(cur_query == last_query_offset_sent);
		cur_query++;

		custom_debug(MASTER, "M:new query=%d,total=%d\n",
			     cur_query, test_params_p->query_count);
	    }
	    else if (wait_status == ALL_QUERY_SCHED_DONE)
	    {
		break;
	    }
	}
    }

    /* Do I/O at the end if we so choose */
    if (test_params_p->end_write == TRUE)
    {
	last_query_offset_sent = -1;
	while (last_query_offset_sent + 1 <= last_query_result_comp)
	{
	    last_query_offset_sent++;
	    if (test_params_p->parallel_io == TRUE ) 
	    {
		/* If we are using collective I/O, we have to write zero */
		if (test_params_p->io_method == COLLECTIVE_IO)
		{
		    custom_debug(MASTER_IO, 
				 "M:starting dummy I/O for query=%d of %d\n",
				 last_query_offset_sent, 
				 last_query_result_comp);
		    do_dummy_io(mpe_events_p, test_params_p);
		}
	    }
	    else
	    {
	    /* do all the I/O */
	    custom_MPE_Log_event(mpe_events_p->io_start, 
				 0, NULL, mpe_events_p);
	    do_all_io(&last_query_offset_sent,
		      &cur_file_offset,
		      query_info_list,
		      mpe_events_p,
		      test_params_p);
	    custom_MPE_Log_event(mpe_events_p->io_end, 
				 0, NULL, mpe_events_p);
	    /* Free the data structures associated 
	     * with the wrriten data */
	    if (query_info_list[last_query_offset_sent].lresult_head != NULL)
		free_lresult_list(query_info_list[last_query_offset_sent].
				  lresult_head);	    
	    }
	}
    }

    /* Check for pending messages */
    master_check_all_irecv(numprocs,
			   query_frag_irecv_matrix,
			   mpe_events_p,
			   test_params_p);
    master_check_all_isend(numprocs,
			   query_proc_isend_matrix,
			   mpe_events_p,
			   test_params_p);

    /* Clean up*/
    for (i = 0; i < test_params_p->query_count; i++)
    {
	free(query_proc_isend_matrix[i]);
	free(query_frag_irecv_matrix[i]);
    }
    free(query_proc_isend_matrix);
    free(query_frag_irecv_matrix);
    
    free(query_info_list);

    MPI_File_close(&(test_params_p->fh));
    
    custom_debug(MASTER_IO, "M:Processed a total of %Ld\n", cur_file_offset);
    custom_debug(MASTER, "M:Finished. Exiting...\n");
    return 0;
}
