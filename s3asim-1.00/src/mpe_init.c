/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include <string.h>
#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <sys/time.h>
#include "test.h"

/**
 * All MPE information and timing information is stored in the
 * mpe_events structure.  The timings are reduced to the master and
 * are printed by the master.
 *
 * @param myid         MPI myid.
 * @param numprocs     Number of processes used. 
 * @param mpe_events_p Pointer to timing structure.
 * @return             0 on success.
 */
int timing_reduce(int myid, int numprocs, struct mpe_events_s *mpe_events)
{
    double tmp_time, tmp_total_time, tmp_ave_time, 
	master_other_time, worker_other_time;

    MPI_Reduce(&(mpe_events->total_time), &tmp_total_time, 1, 
	       MPI_DOUBLE, MPI_MAX, MASTER_NODE, MPI_COMM_WORLD);
    master_other_time = tmp_total_time;
    worker_other_time = tmp_total_time;
    MPI_Reduce(&(mpe_events->setup_time_total), &tmp_time, 1, 
	       MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	fprintf(stdout, 
		"----------------------------------"
		"----------------------------------\n");

	fprintf(stdout, 
		"Phase             "
		"|    M: time    |%% of total"
		"|    W: time    |%% of total\n");
	tmp_ave_time = (tmp_time - mpe_events->setup_time_total) 
	    / (numprocs - 1);
	master_other_time -= mpe_events->setup_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Setup             | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->setup_time_total,
		mpe_events->setup_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->data_distribution_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->data_distribution_time_total) 
	    / (numprocs - 1);
	master_other_time -= mpe_events->data_distribution_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Data Distribution | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->data_distribution_time_total,
		mpe_events->data_distribution_time_total / 
		tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->compute_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->compute_time_total) / 
	    (numprocs - 1);
	master_other_time -= mpe_events->compute_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Compute           | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->compute_time_total,
		mpe_events->compute_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->merge_results_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->merge_results_time_total) / 
	    (numprocs - 1);
	master_other_time -= mpe_events->merge_results_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Merge Results     | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->merge_results_time_total,
		mpe_events->merge_results_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->gather_results_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->gather_results_time_total) / 
	    (numprocs - 1);
	master_other_time -= mpe_events->gather_results_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Gather Results    | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->gather_results_time_total,
		mpe_events->gather_results_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->io_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->io_time_total) / 
	    (numprocs - 1);
	master_other_time -= mpe_events->io_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"I/O               | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->io_time_total,
		mpe_events->io_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);
    }
    MPI_Reduce(&(mpe_events->sync_time_total), &tmp_time, 1,
               MPI_DOUBLE, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
    if (myid == MASTER_NODE)
    {
	tmp_ave_time = (tmp_time - mpe_events->sync_time_total) / 
	    (numprocs - 1);
	master_other_time -= mpe_events->sync_time_total;
	worker_other_time -= tmp_ave_time;
	fprintf(stdout, 
		"Sync              | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		mpe_events->sync_time_total,
		mpe_events->sync_time_total / tmp_total_time * 100,
		tmp_ave_time,
		tmp_ave_time / tmp_total_time * 100);

	fprintf(stdout, 
		"Other             | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		master_other_time,
		master_other_time / tmp_total_time * 100,
		worker_other_time,
		worker_other_time / tmp_total_time * 100);
	fprintf(stdout, 
		"----------------------------------"
		"----------------------------------\n");
	fprintf(stdout, 
		"Total             | %8.3f      | %5.1f    | "
		"%8.3f      | %5.1f\n",
		tmp_total_time,
		tmp_total_time / tmp_total_time * 100,
		tmp_total_time,
		tmp_total_time / tmp_total_time * 100);
    }

    return 0;
}

/**
 * Print the timing information for the mpe_events structure.  This is
 * generally used for debugging.
 *
 * @param myid         MPI myid.
 * @param mpe_events_p Pointer to timing structure.
 */
void print_timing(int myid, struct mpe_events_s *mpe_events)
{
    fprintf(stderr,
	    "P%d:setup time                  = %9.4f\n"
	    "P%d:data distribution time      = %9.4f\n"
	    "P%d:compute time                = %9.4f\n"
	    "P%d:merge results time          = %9.4f\n"
	    "P%d:gather results time         = %9.4f\n"
	    "P%d:send processed results time = %9.4f\n"	    
	    "P%d:I/O time                    = %9.4f\n"
	    "P%d:sync time                   = %9.4f\n"
	    "P%d:total time                  = %9.4f\n",
	    myid, mpe_events->setup_time_total,
	    myid, mpe_events->data_distribution_time_total,
	    myid, mpe_events->compute_time_total,
	    myid, mpe_events->merge_results_time_total,
	    myid, mpe_events->gather_results_time_total,
	    myid, mpe_events->send_processed_results_time_total,
	    myid, mpe_events->io_time_total,
	    myid, mpe_events->sync_time_total,
	    myid, mpe_events->total_time);
}

/**
 * A set of events are defined by another function.  Using the event
 * numbers, the correct timing procedure is done here.  Start or stop
 * timers and add to profiling information.
 *
 * @param event        Event number to process.
 * @param data         If MPE is used, this int is logged.
 * @param string       If MPE is used, this string is logged.
 * @param mpe_events_p Pointer to timing structure.
 * @return             0 on success.
 */
int custom_MPE_Log_event(int event, int data, char *string,
			 struct mpe_events_s *mpe_events)
{
    if (event == mpe_events->setup_start)
    {
	mpe_events->setup_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->setup_end)
    {
	mpe_events->setup_time_total += 
	    MPI_Wtime() - mpe_events->setup_time_tmp;
    }
    else if (event == mpe_events->data_distribution_start)
    {
	mpe_events->data_distribution_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->data_distribution_end)
    {
	mpe_events->data_distribution_time_total += 
	    MPI_Wtime() - mpe_events->data_distribution_time_tmp;
    }
    else if (event == mpe_events->compute_start)
    {
	mpe_events->compute_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->compute_end)
    {
	mpe_events->compute_time_total += 
	    MPI_Wtime() - mpe_events->compute_time_tmp;
    }
    else if (event == mpe_events->merge_results_start)
    {
	mpe_events->merge_results_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->merge_results_end)
    {
	mpe_events->merge_results_time_total += 
	    MPI_Wtime() - mpe_events->merge_results_time_tmp;
    }
    else if (event == mpe_events->gather_results_start)
    {
	mpe_events->gather_results_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->gather_results_end)
    {
	mpe_events->gather_results_time_total += 
	    MPI_Wtime() - mpe_events->gather_results_time_tmp;
    }
    else if (event == mpe_events->send_processed_results_start)
    {
	mpe_events->send_processed_results_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->send_processed_results_end)
    {
	mpe_events->send_processed_results_time_total += 
	    MPI_Wtime() - mpe_events->send_processed_results_time_tmp;
    }
    else if (event == mpe_events->io_start)
    {
	mpe_events->io_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->io_end)
    {
	mpe_events->io_time_total += 
	    MPI_Wtime() - mpe_events->io_time_tmp;
    }
    else if (event == mpe_events->sync_start)
    {
	mpe_events->sync_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->sync_end)
    {
	mpe_events->sync_time_total += 
	    MPI_Wtime() - mpe_events->sync_time_tmp;
    }
    else if (event == mpe_events->rand_start)
    {
	mpe_events->rand_time_tmp = MPI_Wtime();
    }
    else if (event == mpe_events->rand_end)
    {
	mpe_events->rand_time_total += 
	    MPI_Wtime() - mpe_events->rand_time_tmp;
    }
    else
    {
	fprintf(stderr, "Error!  Impossible event %d\n",
		event);
	return -1;
    }

#ifdef HAVE_MPE    
    return MPE_Log_event(event, data, string);
#endif

    return 0;
}

/**
 * If MPE is used, this function defines the event numbers with the
 * MPE_Log_get_event_number() function.  Otherwise, the event numbers
 * are predefined.
 *
 * @param mpe_events_p Pointer to timing structure.
 * @return             0 on success.
 */
int init_mpe_events(struct mpe_events_s *mpe_events)
{
    memset(mpe_events, 0, sizeof(struct mpe_events_s));
#ifdef HAVE_MPE
    mpe_events->setup_start = MPE_Log_get_event_number();    
    mpe_events->setup_end   = MPE_Log_get_event_number();
    mpe_events->data_distribution_start = MPE_Log_get_event_number();    
    mpe_events->data_distribution_end   = MPE_Log_get_event_number();
    mpe_events->compute_start = MPE_Log_get_event_number();
    mpe_events->compute_end   = MPE_Log_get_event_number();
    mpe_events->merge_results_start = MPE_Log_get_event_number();
    mpe_events->merge_results_end   = MPE_Log_get_event_number();
    mpe_events->gather_results_start = MPE_Log_get_event_number();
    mpe_events->gather_results_end   = MPE_Log_get_event_number();
    mpe_events->send_processed_results_start = MPE_Log_get_event_number();
    mpe_events->send_processed_results_end   = MPE_Log_get_event_number();
    mpe_events->io_start = MPE_Log_get_event_number();
    mpe_events->io_end   = MPE_Log_get_event_number();
    mpe_events->sync_start = MPE_Log_get_event_number();
    mpe_events->sync_end   = MPE_Log_get_event_number();
    mpe_events->rand_start = MPE_Log_get_event_number();
    mpe_events->rand_end   = MPE_Log_get_event_number();
#else    
    mpe_events->setup_start = 0;    
    mpe_events->setup_end   = 1;
    mpe_events->data_distribution_start = 2;    
    mpe_events->data_distribution_end   = 3;
    mpe_events->compute_start = 4;
    mpe_events->compute_end   = 5;
    mpe_events->merge_results_start = 6;
    mpe_events->merge_results_end   = 7;
    mpe_events->gather_results_start = 8;
    mpe_events->gather_results_end   = 9;
    mpe_events->send_processed_results_start = 10;
    mpe_events->send_processed_results_end   = 11;
    mpe_events->io_start = 12;
    mpe_events->io_end   = 13;
    mpe_events->sync_start = 14;
    mpe_events->sync_end   = 15;
    mpe_events->rand_start = 16;
    mpe_events->rand_end   = 17;
#endif
    return 0;
}

#ifdef HAVE_MPE
/**
 * If MPE is used, this function defines the MPE states and their
 * respective colors for easy viewing with jumpshot later on.
 *
 * @param mpe_events_p Pointer to timing structure.
 * @return             0 on success.
 */
int init_mpe_describe_state(struct mpe_events_s *mpe_events)
{
    MPE_Describe_state(mpe_events->setup_start,
		       mpe_events->setup_end,
		       "(1) Setup", "white");
    MPE_Describe_state(mpe_events->data_distribution_start,
		       mpe_events->data_distribution_end,
		       "(2) Data Distribution", "green");
    MPE_Describe_state(mpe_events->compute_start,
		       mpe_events->compute_end,
		       "(3) Compute", "violet");
    MPE_Describe_state(mpe_events->merge_results_start,
		       mpe_events->merge_results_end,
		       "(4) Merge Results", "grey");
    MPE_Describe_state(mpe_events->gather_results_start,
		       mpe_events->gather_results_end,
		       "(5) Gather Results", "yellow");
    MPE_Describe_state(mpe_events->send_processed_results_start,
		       mpe_events->send_processed_results_end,
		       "(6) Send Processed Results", "orange");
    MPE_Describe_state(mpe_events->io_start,
		       mpe_events->io_end,
		       "(7) Actual I/O", "purple");
    MPE_Describe_state(mpe_events->sync_start,
		       mpe_events->sync_end,
		       "(8) Synchronization", "turquoise");
    return 0;
}
#endif
