/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "hist_params.h"

const static char *decode_sequence[MAX_SEQUENCE] =
    {
	"QUERY",
	"DATABASE"
    };

#define STR_BUF_SIZE 1024
/**
 * Reads in histogram information from a file for either query or
 * database distributions.
 *
 * @param test_params_p Pointer to test_params. 
 * @param sequence_type Either a QUERY or DATABASE type. 
 * @return              0 on success.
 */
int read_hist_params(struct test_params_s *test_params_p, 
		     int sequence_type)
{
    int64_t tmp_int64 = -1, tmp_range_end = 0;
    int tmp_int, i;
    FILE *Fp = NULL;
    char buf[STR_BUF_SIZE];
    char *pch = NULL;

    /* General variables */
    char *params_file = NULL;
    int *hist_list_count_p = NULL;
    struct hist_s **hist_list_p = NULL;

    if (sequence_type == QUERY)
    {
	params_file = test_params_p->query_params_file;
	hist_list_count_p = &(test_params_p->query_hist_list_count);
	hist_list_p = &(test_params_p->query_hist_list);
    }
    else if (sequence_type == DATABASE)
    {
	params_file = test_params_p->db_params_file;
	hist_list_count_p = &(test_params_p->db_hist_list_count);
	hist_list_p = &(test_params_p->db_hist_list);
    }
    else
    {
	fprintf(stderr, "read_hist_params: Impossible sequence type %d",
		sequence_type);
	return -1;
    }

    Fp = fopen(params_file, "r");
    if (Fp == NULL)
    {
	fprintf(stderr, "read_hist_params: File %s cannot be opened"
		" for reading\n", params_file);
    }

    if (fgets(buf, STR_BUF_SIZE, Fp) == NULL)
    {
	fprintf(stderr, "read_hist_params: count failed");
	return -1;
    }
    sscanf(buf, "%d", &tmp_int);
    if (tmp_int <=0 || tmp_int >= 2000000)
    {
	fprintf(stderr, "read_hist_params: tmp_int %d unacceptable\n", 
		tmp_int);
	return -1;
    }
    
    *hist_list_count_p = tmp_int;
    if ((*hist_list_p = (struct hist_s *) 
	 malloc(*hist_list_count_p * sizeof(struct hist_s))) == NULL)
    {
	fprintf(stderr, "read_hist_params: malloc *hist_list_p "
		"of size %d failed", tmp_int * sizeof(struct hist_s));
	return -1;
    }

    for (i = 0; i < *hist_list_count_p; i++)
    {
	if (fgets(buf, STR_BUF_SIZE, Fp) == NULL)
	    break;
	else 
	{
	    pch = strtok(buf, " ");
	    if (pch == NULL)
		break;
	    sscanf(buf, "%Ld", &tmp_int64);
	    if (tmp_int64 < 0)
	    {
		fprintf(stderr, 
			"read_hist_params: tmp_int64 %Ld unacceptable\n",
			tmp_int64);
		return -1;
	    }
	    (*hist_list_p)[i].range_end = tmp_int64;
	    pch = strtok(NULL, " ");
	    if (pch == NULL)
		break;
	    sscanf(pch, "%Ld", &tmp_int64);
	    if (tmp_int64 < 0)
	    {
		fprintf(stderr, 
			"read_hist_params: tmp_int64 %Ld unacceptable\n",
			tmp_int64);
		return -1;
	    }
	    (*hist_list_p)[i].count = tmp_int64;
	    tmp_range_end += tmp_int64;
	    (*hist_list_p)[i].total_range_end = tmp_range_end;
	}
    }

    assert(i <= *hist_list_count_p);
    
    fclose(Fp);
    return 0;
}
 
/**
 * Prints out the histogram information from the test_params structure
 * that has already been parsed from a file for either query or
 * database distributions.
 *
 * @param test_params_p Pointer to test_params. 
 * @param sequence_type Either a QUERY or DATABASE type. 
 */
void print_hist_params(struct test_params_s *test_params_p,
		       int sequence_type)
{
    int i;

    /* general variables */
    char *params_file = NULL;
    int *hist_list_count_p = NULL;
    struct hist_s **hist_list_p = NULL;

    if (sequence_type == QUERY)
    {
	params_file = test_params_p->query_params_file;
	hist_list_count_p = &(test_params_p->query_hist_list_count);
	hist_list_p = &(test_params_p->query_hist_list);
    }
    else if (sequence_type == DATABASE)
    {
	params_file = test_params_p->db_params_file;
	hist_list_count_p = &(test_params_p->db_hist_list_count);
	hist_list_p = &(test_params_p->db_hist_list);
    }
    else
    {
	fprintf(stderr, "read_hist_params: Impossible sequence type %d",
		sequence_type);
    }

    fprintf(stderr, "%s (%s): hist count = %d\n", decode_sequence[QUERY],
	    params_file,
	    *hist_list_count_p);
    for (i = 0; i < *hist_list_count_p; i++)
    {
	fprintf(stderr, "hist[%d] range=%Ld count=%Ld range_end=%Ld \n", i,
		(*hist_list_p)[i].range_end,
		(*hist_list_p)[i].count,
		(*hist_list_p)[i].total_range_end);
    }
}

/**
 * Based on the histogram information provided by either the query or
 * database distributions, generate a sequence size.
 *
 * @param myid          MPI myid.
 * @param seed_p        Seed to generate pseudo-random output.
 * @param sequence_type Either a QUERY or DATABASE type. 
 * @param work_info_p   Pointer to work information structure.
 * @param mpe_events_p  Pointer to timing structure.
 * @param test_params_p Pointer to test_params. 
 * @return              The sequence size, or negative values on error.
 */
int generate_rand_hist(int myid, 
		       unsigned int *seed_p,
		       int sequence_type,
		       struct work_info_s *work_info_p,
		       struct mpe_events_s *mpe_events_p,
		       struct test_params_s *test_params_p)
{
    int i;
    int rand_select = -1;
    int cur_sequence_size = -1;
    
    /* general variables */
    char *params_file = NULL;
    int *hist_list_count_p = NULL;
    struct hist_s **hist_list_p = NULL;

    if (sequence_type == QUERY)
    {
        params_file = test_params_p->query_params_file;
        hist_list_count_p = &(test_params_p->query_hist_list_count);
        hist_list_p = &(test_params_p->query_hist_list);
    }
    else if (sequence_type == DATABASE)
    {
        params_file = test_params_p->db_params_file;
        hist_list_count_p = &(test_params_p->db_hist_list_count);
        hist_list_p = &(test_params_p->db_hist_list);
    }
    else
    {
        fprintf(stderr, "read_hist_params: Impossible sequence type %d",
                sequence_type);
    }

    /* Find which histogram box to look in */
    rand_select = generate_int_range(
	seed_p,
	0,
	(*hist_list_p)[*hist_list_count_p - 1].total_range_end - 1);

    assert(rand_select <= 
	   (*hist_list_p)[*hist_list_count_p - 1].total_range_end);

    custom_MPE_Log_event(mpe_events_p->rand_start, 0,
			 NULL, mpe_events_p);
    for (i = 0; i < *hist_list_count_p; i++)
    {
	if (rand_select < (*hist_list_p)[i].total_range_end)
	    break;
    }

    *seed_p = *seed_p * 101 * (work_info_p->query + 3) + 5 * i;
    /* Generate a random value from the range of the histogram box */
    if (i == 0)
    {
	cur_sequence_size = generate_int_range(
	    seed_p,
	    0,
	    (*hist_list_p)[i].range_end - 1);
	assert(cur_sequence_size - 0 <=
	       ((*hist_list_p)[i].range_end - 1) -
	       0);
    }
    else
    {
	cur_sequence_size = generate_int_range(
	    seed_p,
	    (*hist_list_p)[i-1].range_end,
	    (*hist_list_p)[i].range_end - 1);
	assert(cur_sequence_size - (*hist_list_p)[i-1].range_end <=
	       ((*hist_list_p)[i].range_end - 1) - 
	       (*hist_list_p)[i-1].range_end);
    }
    custom_MPE_Log_event(mpe_events_p->rand_end, 0,
			 NULL, mpe_events_p);
    custom_debug(WORKER_COMPUTE, "W%d:%s rand=%d,hist_box=%d,length=%d\n",
		 myid, decode_sequence[sequence_type], rand_select, 
		 i, cur_sequence_size);

    return cur_sequence_size;
}
