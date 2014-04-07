/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <math.h>
#include "misc.h"

/**
 * Generate a pseudo-random double between 0 and 1.
 *
 * @param seed_p Pointer to the seed to generate pseudo-random output.
 * @return       The pseudo-random double in between 0 to 1.
 */
double generate_rand_zero_one(unsigned int *seed_p)
{
    /* Dividing an int by a double should return a double */
    return (rand_r(seed_p)) / ((double) (RAND_MAX*1.0));
}

/**
 * Generate a pseudo-random integer between min and max.
 *
 * @param seed_p Pointer to the seed to generate pseudo-random output.
 * @param min    Minimum integer value
 * @param max    Maximum integer value
 * @return       The pseudo-random integer in betwen min to max.
 */
int generate_int_range(unsigned int *seed_p,
		       int min,
		       int max)
{
    double rand_zero_one = generate_rand_zero_one(seed_p);
    double tmp_result = rand_zero_one * (max - min);

    assert(min <= max);
    
    if (tmp_result - floor(tmp_result) < ceil(tmp_result) - tmp_result)
	tmp_result = floor(tmp_result) + min;
    else
	tmp_result = ceil(tmp_result) + min;

    return ((int) tmp_result);
}
		     
/**
 * The function is called for each result to determine which fragment
 * will have the result.
 *
 * @param seed_p      Pointer to the seed to generate pseudo-random output.
 * @param total_frags Fragment count.
 * @return            The index of the fragment which has this result.
 */
int find_frag(unsigned int *seed_p,
	      int total_frags)
{
    int i;
    double rand_zero_one = generate_rand_zero_one(seed_p);

    for (i = 0; i < total_frags; i++)
    {
	if (rand_zero_one <= ((double) (i+1.0) / total_frags))
	{
	    return i;
	}
    }
    
    assert(i < total_frags);

    fprintf(stderr, "find_frag:Impossible for rand %f from 0 - 1\n",
	    rand_zero_one);
    return -1;
}
