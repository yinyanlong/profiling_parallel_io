/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _TEST_H
#define _TEST_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "mpi.h"
#include <assert.h>
#ifdef HAVE_MPE
#include "mpe.h"
#endif
#include "custom_debug.h"
#include "request.h"
#include "mpe_init.h"
#include "misc.h"

struct hist_s
{
    int64_t total_range_end; /**< sum of this and previous ranges */
    int64_t range_end;       /**< actual data range ending*/
    int64_t count;           /**< count in this frequency discretization */
};

struct test_params_s 
{
    int total_frags;        /**< Number of frags per query*/
    int query_count;        /**< Number of total queries */
    int query_size_min;     /**< Smallest query possible */
    int query_size_max;     /**< Largest query possible */
    int result_size_min;    /**< Smallest result possible 
			     * (typically smallest match*3) */
    int database_sequence_size_min; /**< Smallest sequence in database */
    int database_sequence_size_max; /**< Largest sequence in database */
    int result_count_min;   /**< Smallest result count possible */
    int result_count_max;   /**< Smallest result count possible */
    double compute_speed;   /**< Change the compute time */
    int io_method;          /**< Use various I/O techniques */
    int parallel_io;        /**< Enable parallel I/O */
    int query_sync;         /**< Force fake synchornization to model 
			     * iterative data dependent applications */
    int output_file_len;
    char *output_file;
    MPI_File fh;            /**< We will open the file at the beginning of 
			     * the application and close it at the end */
    MPI_Info *info_p;       /**< We set the info at the begining and use it
			     * until the end */
    int end_write;
    int no_io;              /**< For debugging, we don't want to do any I/O */
    int atomicity;          /**< Atomicity on (1) or off (0) */

    /** Precomputed results*/
    struct frag_preresult_s **query_frag_preresult_matrix; 

    /** Query sequence histogram file */
    int query_params_file_len;
    char *query_params_file;
    int query_hist_list_count;
    struct hist_s *query_hist_list;
    
    /** Database sequence histogram file */
    int db_params_file_len;
    char *db_params_file;
    int db_hist_list_count;
    struct hist_s *db_hist_list;
};

struct frag_preresult_s 
{
    int result_start;
    int result_count;
};

struct frag_result_info_s
{
    int query;
    int frag;
    int result_list_count;
};

struct work_info_s
{
    int query;
    int frag;
    int result_start;
    int result_count;
};

struct result_info_s
{
    int result_id;
    int proc_id;
    int score;
    int size;
};

struct result_s
{
    int result_id;
    int proc_id;
    int score;
    int size;
    char *result_data;
};

#define MASTER_NODE 0

#define TRUE  1
#define FALSE 0

#define INDIVIDUAL_IO 0
#define COLLECTIVE_IO 1
#define MAX_IO        2

#endif
