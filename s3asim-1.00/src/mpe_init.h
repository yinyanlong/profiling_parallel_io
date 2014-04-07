/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _MPE_INIT_H
#define _MPE_INIT_H

#include <string.h>
#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <sys/time.h>
#ifdef HAVE_MPE
#include "mpe.h"
#endif


struct mpe_events_s
{
    /* MPE variables */
    int setup_start;
    int setup_end;
    int data_distribution_start;
    int data_distribution_end;
    int compute_start;
    int compute_end;
    int merge_results_start;
    int merge_results_end;
    int gather_results_start;
    int gather_results_end;
    int send_processed_results_start;
    int send_processed_results_end;
    int io_start;
    int io_end;
    int sync_start;
    int sync_end;
    int rand_start;
    int rand_end;

    /* MPI_Wtime() variables */
    double setup_time_tmp;
    double setup_time_total;
    double data_distribution_time_tmp;
    double data_distribution_time_total;
    double compute_time_tmp;
    double compute_time_total;
    double merge_results_time_tmp;
    double merge_results_time_total;
    double gather_results_time_tmp;
    double gather_results_time_total;
    double send_processed_results_time_tmp;
    double send_processed_results_time_total;
    double io_time_tmp;
    double io_time_total;
    double sync_time_tmp;
    double sync_time_total;
    double rand_time_tmp;
    double rand_time_total;
    double total_time;
};

int timing_reduce(int myid, int numprocs, struct mpe_events_s *mpe_events);

void print_timing(int myid, struct mpe_events_s *mpe_events);

int custom_MPE_Log_event(int event, int data, char *string, 
			 struct mpe_events_s *mpe_events);

int init_mpe_events(struct mpe_events_s *mpe_events);

int init_mpe_describe_state(struct mpe_events_s *mpe_events);

#endif
