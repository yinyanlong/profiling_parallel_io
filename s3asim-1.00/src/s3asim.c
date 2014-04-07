/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#define _GNU_SOURCE
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>
#include <errno.h>
#include "mpi.h"
#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>
#include "custom_debug.h"
#ifdef HAVE_MPE
#include "mpe.h"
#endif
#include "test.h"
#include "master.h"
#include "worker.h"
#include "hist_params.h"

#ifdef TEST_MODE
int64_t global_debug_mask = MASTER_IO | MASTER_ERR | WORKER_ERR;
#else
#if 1
int64_t global_debug_mask = MASTER_IO | WORKER_IO | MASTER_ERR | WORKER_ERR;
#else
int64_t global_debug_mask = DEBUG_ALL;
#endif
#endif

const static char *io_method_name[MAX_IO] = 
    {
	"individual",
	"collective"
    };

/* Static function prototypes */
static void print_usage(char *exefile);
static int parse_args(int argc, char *argv[], 
		      struct test_params_s *test_params_p);
static void print_settings(struct test_params_s *test_params_p, int numprocs);
static int precalculate_results(
    struct test_params_s *test_params_p,
    struct frag_preresult_s **query_frag_preresult_matrix);

/**
 * Main function which selects the process to be a master or a worker
 * based on MPI myid.
 *
 * @param argc Number of arguments.
 * @param argv Pointer to the argument pointers.
 * @return     0 on success.
 */
int main(int argc, char **argv)
{
    int myid, numprocs, i;
    int err = -1;
    struct test_params_s test_params;
    struct mpe_events_s mpe_events;
    struct frag_preresult_s **query_frag_preresult_matrix = NULL;

    memset(&test_params, 0, sizeof(struct test_params_s));
    MPI_Init(&argc, &argv);
#ifdef HAVE_MPE    
    MPI_Pcontrol(0);
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    init_mpe_events(&mpe_events);
#ifdef HAVE_MPE
    if (myid == MASTER_NODE)
    {
	init_mpe_describe_state(&mpe_events);
    }
#endif
    if (myid == MASTER_NODE)
    {
	err = parse_args(argc, argv, &test_params);
	if (err >= 0)
	    print_settings(&test_params, numprocs);
	if (test_params.output_file != NULL)
	    MPI_File_delete(test_params.output_file, MPI_INFO_NULL);
	if (numprocs < 2)
	{
	    fprintf(stderr, "Must use at least 2  processes.\n");
	    err = -1;
	}
    }

    MPI_Bcast(&err, 1, MPI_INT, MASTER_NODE, MPI_COMM_WORLD);
    /* Quit if the parse_args failed */
    if (err != 0)
    {
	MPI_Finalize();
	return 0;
    }

    /* Master precalculates all the results for the queries and
     * reads in the database histogram parameters */
    if (myid == MASTER_NODE)
    {
	if ((query_frag_preresult_matrix = (struct frag_preresult_s **) 
	     malloc(test_params.query_count * 
		    sizeof(struct frag_preresult_s *))) == NULL)
	{
	    custom_debug(
		MASTER_ERR, 
		"M:malloc query_frag_preresult_matrix of size %d failed\n",
		test_params.query_count * sizeof(struct frag_preresult_s *));
	    return -1;
	}
	for (i = 0; i < test_params.query_count; i++)
	{
	    if ((query_frag_preresult_matrix[i] = (struct frag_preresult_s *)
		 malloc(test_params.total_frags * 
			sizeof(struct frag_preresult_s))) == NULL)
	    {
		custom_debug(
		    MASTER_ERR,
		    "M:malloc query_frag_preresult_matrix[%d] "
		    "of size %d failed\n", i,
		    test_params.total_frags *
		    sizeof(struct frag_preresult_s));
		return -1;
	    }
	    memset(query_frag_preresult_matrix[i], 0, 
		   test_params.total_frags * sizeof(struct frag_preresult_s));
	}
	precalculate_results(&test_params, query_frag_preresult_matrix);
	test_params.query_frag_preresult_matrix = query_frag_preresult_matrix;
	if (test_params.query_params_file != NULL)
	{
	    read_hist_params(&test_params, QUERY);
#if 0
	    print_hist_params(&test_params);
#endif
	}
	if (test_params.db_params_file != NULL)
	{
	    read_hist_params(&test_params, DATABASE);
#if 0
	    print_hist_params(&test_params);
#endif
	}
    }

    MPI_Barrier(MPI_COMM_WORLD);
#ifdef HAVE_MPE
    MPI_Pcontrol(1);
#endif

    /* Divide up into either a Master or Worker */
    mpe_events.total_time = MPI_Wtime();
    if (myid == 0)
    {
	err = master(myid,
		     numprocs,
		     &mpe_events,
		     &test_params);
	if (err != 0)
	    custom_debug(MASTER_ERR, "master failed\n");
	else
	    custom_debug(MASTER, "master (proc %d) reached last barrier\n",
			 myid);
    }
    else
    {
	err = worker(myid,
		     numprocs,
		     &mpe_events,
		     &test_params);
	if (err != 0)
	    custom_debug(WORKER_ERR, "worker failed\n");
	else
	    custom_debug(WORKER, "worker (proc %d) reached last barrier\n",
			 myid);
    }

    custom_MPE_Log_event(mpe_events.sync_start,
			 0, NULL, &mpe_events);
    MPI_Barrier(MPI_COMM_WORLD);
    custom_MPE_Log_event(mpe_events.sync_end,
			 0, NULL, &mpe_events);
    MPI_Pcontrol(0);    
    mpe_events.total_time = MPI_Wtime() - mpe_events.total_time;
#if 0
    print_timing(myid, &mpe_events);
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    timing_reduce(myid, numprocs, &mpe_events);
    
    /* Clean up precomputed results and file */
    if (myid == MASTER_NODE)
    {
	if (test_params.query_params_file != NULL)
	{
	    free(test_params.query_params_file);
	    free(test_params.query_hist_list);
	}
	if (test_params.db_params_file != NULL)
	{
	    free(test_params.db_params_file);
	    free(test_params.db_hist_list);
	}

	MPI_File_delete(test_params.output_file, MPI_INFO_NULL);
	for (i = 0; i < test_params.query_count; i++)
	{
	    free(query_frag_preresult_matrix[i]);
	}
	free(query_frag_preresult_matrix);
    }

    MPI_Info_free(test_params.info_p);
    free(test_params.info_p);
    free(test_params.output_file);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}

/**
 * Prints out the usage of the executable 
 *
 * @param exefile Pointer to the name of the executable.
 */
static void print_usage(char *exefile)
{
    fprintf(
	stderr, 
	"Usage: %s [OPTION]... [VAR=VALUE]...\n\n"
	"  -h, --help              display this help and exit\n"
	"  -f, --total-fragments   number of fragments of data (default 4)\n"
	"  -c, --query-count       number of total queries (default 2)\n"
	"  -q, --query-size-min    min size of each query (default 10)\n"
	"  -Q, --query-size-max    max size of each query (default 1000)\n"
	"  -d, --database-sequence-size-min\n"
	"                 min size of each database-sequence (default 10)\n"
	"  -D, --database-sequence-size-max \n"
	"                 max size of each database-sequence (default 1000)\n"
	"  -y, --query_params_file query params file (default N/A)\n"
	"                          (-q and -Q options will be ignored)\n"
	"  -Y, --db_params_file    db params file (default N/A)\n"
	"                          (-d and -D options will be ignored)\n"
	"  -r, --result-size-min   min size of each result (default 10)\n"
	"  -m, --result-count-min  min count of each result (default 10)\n"
	"  -M, --result-count-max  max count of each result (default 1000)\n"
	"  -K, --compute-speed     speedup of compute time (default 1.0)\n"
	"  -i, --io-method         note: If using serial I/O, only\n"
	"                          individual I/O can be used. (default)\n"
        "                          0 - individual I/O\n"
        "                          1 - collective I/O\n"
	"  -p, --parallel-io       1 - use parallel I/O  (default 0)\n"
	"  -s, --query-sync        1 - sync per query (default 0)\n"
	"  -o, --output_file       output file (default test)\n"
	"  -I, --no_io             0 - default 1 - turn I/O off for testing\n"
	"  -a, --atomicity         0 - default 1 - turn atomicity on\n"
	"  -e, --end_write         1 - write all data at end\n"
	"  -H, --mpi-io-hint       set as many MPI-IO hints as is desired\n"
	"                          through repeated use (interface is \n"
	"                          key=value) - for example, to turn off \n"
	"                          data sieving for writes in ROMIO, use\n"
	"                          \"-H romio_ds_write=enable\"\n\n",
	exefile);
}

/**
 * Parse the command line arguments and set the test_params structure
 *
 * @param argc          Number of arguments.
 * @param argv          Pointer to the argument pointers.
 * @param test_params_p Pointer to test_params. 
 * @return              0 on success.
 */
static int parse_args(int argc, char *argv[], 
		      struct test_params_s *test_params_p) 
{
    int hint_key_len, hint_val_len;
    char *break_p, hint_key[MPI_MAX_INFO_KEY], *hint_val;

    static struct option longopts[] =
	{
	    {"help", 0, NULL, 'h'},
	    {"total-fragments", 1, NULL, 'f'}, 
	    {"query-count", 1, NULL, 'c'}, 
	    {"query-size-min", 1, NULL, 'q'}, 
	    {"query-size-max", 1, NULL, 'Q'}, 
	    {"database-sequence-size-min", 1, NULL, 'd'}, 
	    {"database-sequence-size-max", 1, NULL, 'D'}, 
	    {"query_params_file", 1, NULL, 'y'},
	    {"db_params_file", 1, NULL, 'Y'},
	    {"result-size-min", 1, NULL, 'r'}, 
	    {"result-count-min", 1, NULL, 'm'}, 
	    {"result-count-max", 1, NULL, 'M'}, 
	    {"compute-speed", 1, NULL, 'K'}, 
	    {"io-method", 1, NULL, 'i'}, 
	    {"parallel-io", 1, NULL, 'p'}, 
	    {"query-sync", 1, NULL, 's'},
	    {"output_file", 1, NULL, 'o'},
	    {"no_io", 1, NULL, 'I'},
	    {"atomicity", 1, NULL, 'a'},
	    {"end-write", 1, NULL, 'e'},
	    {"mpi-io-hint", 1, NULL, 'H'},
	    {0,0,0,0}
	};

    test_params_p->total_frags                = 10;
    test_params_p->query_count                = 10;
    test_params_p->query_size_min             = 1;
    test_params_p->query_size_max             = 10;
    test_params_p->database_sequence_size_min = 1;
    test_params_p->database_sequence_size_max = 5;
    test_params_p->query_params_file_len      = 0;
    test_params_p->query_params_file          = NULL;
    test_params_p->db_params_file_len         = 0;
    test_params_p->db_params_file             = NULL;
    test_params_p->result_size_min            = 1;
    test_params_p->result_count_min           = 1;
    test_params_p->result_count_max           = 5;
    test_params_p->compute_speed              = 1.0;
    test_params_p->io_method                  = INDIVIDUAL_IO;
    test_params_p->parallel_io                = 1;
    test_params_p->query_sync                 = 0;
    test_params_p->output_file_len            = 0;
    test_params_p->output_file                = NULL;
    test_params_p->no_io                      = 0;
    test_params_p->atomicity                  = 0;
    test_params_p->end_write                  = 0;

    test_params_p->info_p = malloc(sizeof(MPI_Info));
    if (!test_params_p->info_p)
    {
        fprintf(stderr, "malloc of info_p with size %d failed.\n",
		sizeof(MPI_Info));
        return -1;
    }
    MPI_Info_create(test_params_p->info_p);

    /* Index of current long option into opt_lng array */
    int option_index = 0; 
    int err = -1;
    char *optstring=":hf:c:q:Q:d:D:y:Y:r:m:M:K:i:p:s:o:I:a:e:H:";

    while (1)
    {
	err = getopt_long(argc, argv, optstring, longopts, &option_index);
	if (err == -1)
	    break;
	switch (err)
	{
	    case 'h':
		print_usage(argv[0]);
		return -1;
	    case 'f':
		test_params_p->total_frags = atoi(optarg);
		if (test_params_p->total_frags < 1 ||
		    test_params_p->total_frags > 1000)
		{
		    fprintf(stderr, "Must use less than 1000 fragments\n");
		    return -1;
		}
		break;
	    case 'c':
		test_params_p->query_count = atoi(optarg);
		if (test_params_p->query_count < 1 ||
		    test_params_p->query_count > INT_MAX)
		{
		    fprintf(stderr, "Must have a query count > 0 and less "
			    "than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'q':
		test_params_p->query_size_min = atoi(optarg);
		if (test_params_p->query_size_min < 1 ||
		    test_params_p->query_size_min > INT_MAX)
		{
		    fprintf(stderr, "Must have a query size min > 0 and less "
			    "than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'Q':
		test_params_p->query_size_max = atoi(optarg);
		if (test_params_p->query_size_max < 1 ||
		    test_params_p->query_size_max > INT_MAX)
		{
		    fprintf(stderr, "Must have a query size max > 0 and less "
			    "than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'y':
		test_params_p->query_params_file_len = strlen(optarg);
		if ((test_params_p->query_params_file = malloc(
			 test_params_p->query_params_file_len + 1)) == NULL)
		{
		    fprintf(stderr, "malloc query_params_file failed\n");
		    return -1;
		}
		strcpy(test_params_p->query_params_file, optarg);
		break;
	    case 'Y':
		test_params_p->db_params_file_len = strlen(optarg);
		if ((test_params_p->db_params_file = malloc(
			 test_params_p->db_params_file_len + 1)) == NULL)
		{
		    fprintf(stderr, "malloc db_params_file failed\n");
		    return -1;
		}
		strcpy(test_params_p->db_params_file, optarg);
		break;
	    case 'd':
		test_params_p->database_sequence_size_min = atoi(optarg);
		if (test_params_p->database_sequence_size_min < 1 ||
		    test_params_p->database_sequence_size_min > INT_MAX)
		{
		    fprintf(stderr, "Must have a database sequence size min "
			    "> 0 and less than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'D':
		test_params_p->database_sequence_size_max = atoi(optarg);
		if (test_params_p->database_sequence_size_max < 1 ||
		    test_params_p->database_sequence_size_max > INT_MAX)
		{
		    fprintf(stderr, "Must have a database sequence size max "
			    "> 0 and less than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'r':
		test_params_p->result_size_min = atoi(optarg);
		if (test_params_p->result_size_min < 1 ||
		    test_params_p->result_size_min > INT_MAX)
		{
		    fprintf(stderr, "Must have a result size min "
			    "> 0 and less than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'm':
		test_params_p->result_count_min = atoi(optarg);
		if (test_params_p->result_count_min < 1 ||
		    test_params_p->result_count_min > INT_MAX)
		{
		    fprintf(stderr, "Must have a result count min "
			    "> 0 and less than %d", INT_MAX);
		    return -1;
		}
		break;
	    case 'M':
		test_params_p->result_count_max = atoi(optarg);
		if (test_params_p->result_count_max < 1 ||
		    test_params_p->result_count_max > 1000000)
		{
		    fprintf(stderr, "Must have a result count < 1,000,000\n");
		    return -1;
		}
		break;
	    case 'K':
		test_params_p->compute_speed = atof(optarg);
		if (test_params_p->compute_speed <= 0 ||
                    test_params_p->compute_speed > 1000000)
                {
                    fprintf(stderr, "Must have a result count > 0 and "
			    "< 1,000,000\n");
                    return -1;
                }
                break;
	    case 'i':
		test_params_p->io_method = atoi(optarg);
		if (test_params_p->io_method < 0 ||
		    test_params_p->io_method > MAX_IO)
		{
                    fprintf(stderr, "Must have an I/O method of less than "
			    "%d\n", MAX_IO);
		    return -1;
		}
		break;
	    case 'p':
		test_params_p->parallel_io = atoi(optarg);
		if (test_params_p->parallel_io != 0 &&
		    test_params_p->parallel_io != 1)
		{
		    fprintf(stderr, "Must set parallel I/O to "
			    "0 or 1\n");
		    return -1;
		}
		break;
	    case 's':
		test_params_p->query_sync = atoi(optarg);
		if (test_params_p->query_sync != 0 &&
		    test_params_p->query_sync != 1)
		{
		    fprintf(stderr, "Must set query sync to "
			    "0 or 1\n");
		    return -1;
		}
		break;
	    case 'o':
		test_params_p->output_file_len = strlen(optarg);
		if ((test_params_p->output_file = malloc(
			 test_params_p->output_file_len + 1)) == NULL)
		{
		    fprintf(stderr, "malloc output_file failed\n");
		    return -1;
		}
		strcpy(test_params_p->output_file, optarg);
		break;
	    case 'I':
		test_params_p->no_io = atoi(optarg);
		if (test_params_p->no_io != 0 &&
		    test_params_p->no_io != 1)
		{
		    fprintf(stderr, "malloc no_io must be either 0 or 1\n");
		    return -1;
		}
		break;
	    case 'a':
		test_params_p->atomicity = atoi(optarg);
		if (test_params_p->atomicity != 0 &&
		    test_params_p->atomicity != 1)
		{
		    fprintf(stderr, "Atomicity must be either 0 or 1\n");
		    return -1;
		}
		break;
	    case 'e':
		test_params_p->end_write = atoi(optarg);
		if (test_params_p->end_write != 0 &&
		    test_params_p->end_write != 1)
		{
		    fprintf(stderr, "End write must be either 0 or 1\n");
		    return -1;
		}
		break;
	    case 'H':
		break_p = index(optarg, '=');
                if (!break_p)
                {
                    fprintf(stderr, "Hint %s does not contain a '='.\n", 
			    optarg);
                    return -1;
                }
                hint_key_len = break_p - optarg;
                hint_val_len = strlen(optarg) - hint_key_len - 1;
                hint_val = malloc((hint_val_len + 1)*sizeof(char));
                if (!hint_val)
                {
                    fprintf(stderr, "hint_val malloc of size %d failed.\n",
			    hint_val_len);
                    return -1;
                }
                strncpy(hint_key, optarg, hint_key_len);
                strncpy(hint_val, break_p + 1, hint_val_len);
                hint_key[hint_key_len] = '\0';
                hint_val[hint_val_len] = '\0';
                MPI_Info_set(*(test_params_p->info_p), hint_key, hint_val);
                free(hint_val);
                break;
	    default:
		fprintf(stderr, "Option -%c is invalid\n",
			(char) optopt);
		print_usage(argv[0]);
		return -1;
	}
    }

    if (test_params_p->output_file == NULL)
    {
	fprintf(stdout, "Using default output file 'default'\n");
	test_params_p->output_file_len = strlen("default");
	if ((test_params_p->output_file = malloc(
		 test_params_p->output_file_len + 1)) == NULL)
	{
	    fprintf(stdout, "malloc output_file failed\n");
	    return -1;
	}
	strcpy(test_params_p->output_file, "default");
    }

    if (test_params_p->query_params_file == NULL)
    {
	fprintf(stdout, "Using simple uniform query size distribution\n");
    }
    if (test_params_p->db_params_file == NULL)
    {
	fprintf(stdout, "Using simple uniform database size distribution\n");
    }

    if (test_params_p->parallel_io == FALSE &&
	test_params_p->io_method == COLLECTIVE_IO)
    {
	fprintf(stdout, "Cannot use collective I/O with MASTER writing. "
		"Turning the hint to INDIVIDUAL I/O\n");
	test_params_p->io_method = INDIVIDUAL_IO;
    }

    if (test_params_p->io_method == COLLECTIVE_IO &&
	test_params_p->query_sync == FALSE)
    {
	fprintf(stdout, "Enabling query_sync = TRUE.\n"
		"Collective I/O must be synchronized across queries.\n\n");
	test_params_p->query_sync = TRUE;
    }

    if (test_params_p->atomicity == TRUE)
	fprintf(stdout, "Turning atomicity on.\n");

    if (test_params_p->no_io == TRUE)
	fprintf(stdout, "Turning I/O off for debugging.\n");

    return 0;
}    

/**
 * Get the hint information from the MPI_Info variable stored in the
 * test_params and print it out.
 *
 * @param test_params_p Pointer to test_params. 
 * @return              0 on success.
 */
static int print_hints(struct test_params_s *test_params_p)
{
    int i, hint_key_len, hint_val_len, hint_nkeys, flag;
    char hint_key[MPI_MAX_INFO_KEY], *hint_val;
    
    MPI_Info_get_nkeys(*(test_params_p->info_p), &hint_nkeys);
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
            fprintf(stderr, "hint_val malloc of size %d failed.\n",
		    hint_val_len);
            return -1;
        }

        MPI_Info_get(*(test_params_p->info_p), hint_key,
                     hint_val_len + 1, hint_val, &flag);
        assert(flag);
        fprintf(
            stdout,
            "hint %d \"%30s\" = %s\n",
            i, hint_key, hint_val);
        free(hint_val);
    }
    if (!hint_nkeys)
        fprintf(
	    stdout,
            "hints                                   = N/A\n");

    return 0;
}

/**
 * Print out the setting information for the application run.
 *
 * @param test_params_p Pointer to test_params. 
 * @param numprocs      Number of processes used. 
 * @return              0 on success.
 */
static void print_settings(struct test_params_s *test_params_p, int numprocs) 
{
    int result_size_max = 0, result_data_max = 0;
    if (test_params_p->query_size_max > 
	test_params_p->database_sequence_size_max)
	result_size_max = test_params_p->query_size_max;
    else
	result_size_max = test_params_p->database_sequence_size_max;
    
    result_data_max = (3*result_size_max)*test_params_p->result_count_max;
    fprintf(
	stdout,
	"#################### Experimental Settings #################\n"
	"numprocs                                = %d\n"
	"total_frags                             = %d\n"
	"query_count                             = %d\n",
	numprocs,
	test_params_p->total_frags,
	test_params_p->query_count);
    if (test_params_p->query_params_file == NULL)
    	fprintf(
	    stdout,
	    "query_size_min                          = %d\n"
	    "query_size_max                          = %d\n",
	    test_params_p->query_size_min,
	    test_params_p->query_size_max);
    if (test_params_p->db_params_file == NULL)
    	fprintf(
	    stdout,
	    "database_sequence_size_min              = %d\n"
	    "database_sequence_size_max              = %d\n",
	    test_params_p->database_sequence_size_min,
	    test_params_p->database_sequence_size_max);
    if (test_params_p->query_params_file != NULL)
        fprintf(
	    stdout,
	    "query_params_file                       = %s\n",
	    test_params_p->query_params_file);
    if (test_params_p->db_params_file != NULL)
    	fprintf(
	    stdout,
	    "db_params_file                          = %s\n",
	    test_params_p->db_params_file);
    fprintf(
	stdout,
	"result_size_min                         = %d\n"
	"result_count_min                        = %d\n"
	"result_count_max                        = %d\n"
	"compute_speed                           = %f\n"
	"io_method                               = %s\n"
	"parallel_io                             = %d\n"
	"query_sync                              = %d\n"
	"output_file                             = %s\n"
	"atomicity                               = %d\n"
	"end_write                               = %d\n",
	test_params_p->result_size_min,
	test_params_p->result_count_min,
	test_params_p->result_count_max,
	test_params_p->compute_speed,
	io_method_name[test_params_p->io_method],
	test_params_p->parallel_io,
	test_params_p->query_sync,
	test_params_p->output_file,
	test_params_p->atomicity,
	test_params_p->end_write);
    if (test_params_p->query_params_file == NULL &&
	test_params_p->db_params_file == NULL)
	fprintf(
	    stdout,
	    "maximum data / query                    = %d\n",
	    result_data_max);
    print_hints(test_params_p);
    fprintf(
	stdout,
	"############################################################\n\n");
}

/**
 * In order to help generate repeatable pseudo-random results for a
 * given set of input parameters, the master precalculates how many
 * results will be generated for each query.
 *
 * @param test_params_p               Pointer to test_params. 
 * @param query_frag_preresult_matrix Pointer to the matrix of (query,frag)
 *                                    results. 
 * @return                            0 on success.
 */
static int precalculate_results(
    struct test_params_s *test_params_p,
    struct frag_preresult_s **query_frag_preresult_matrix)
{
    int i, j;
    unsigned int seed;
    int cur_result_count = 0;

    /* 1. Generate the amount of results for each query 
     * 2. Generate in which fragments will those results go */
    for (i = 0; i < test_params_p->query_count; i++)
    {
	seed = i+1;
	cur_result_count = generate_int_range(&seed, 
					      test_params_p->result_count_min,
					      test_params_p->result_count_max);
	
	for (j = 0; j < cur_result_count; j++)
	{
	    query_frag_preresult_matrix[i]
		[find_frag(&seed, test_params_p->total_frags)].result_count++;
	}
    }

    /* Calculate the correct result_start location for each frag */
    for (i = 0; i < test_params_p->query_count; i++)
    {
        for (j = 0; j < test_params_p->total_frags; j++)
        {
	    if (j == 0)
		query_frag_preresult_matrix[i][j].result_start = 0;
	    else
		query_frag_preresult_matrix[i][j].result_start =
		    query_frag_preresult_matrix[i][j-1].result_start +
		    query_frag_preresult_matrix[i][j-1].result_count;
        }
    }

    /* Print out the results */
#if 0
    fprintf(stderr, "Results (query #, total result_count) (result_start,result_count) --- \n");
    fprintf(stderr,
	    "Query     |");
    for (i = 0; i < test_params_p->total_frags; i++)
    {
	fprintf(stderr, " Frag %2d  ", i);
    }
    fprintf(stderr, "\n");
    for (i = 0; i < test_params_p->query_count; i++)
    { 
	int total_result_count = 0;
	for (j = 0; j < test_params_p->total_frags; j++)
	    total_result_count += 
		query_frag_preresult_matrix[i][j].result_count;
	fprintf(stderr, "(%3d,%3d) |", i, total_result_count);	    
	for (j = 0; j < test_params_p->total_frags; j++)
	{
	    fprintf(stderr, "(%3d,%3d) ",
		    query_frag_preresult_matrix[i][j].result_start, 
		    query_frag_preresult_matrix[i][j].result_count);
	}
	fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
#endif

    return 0;
}
