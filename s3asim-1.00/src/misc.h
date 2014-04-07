/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _MISC_H
#define _MISC_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

double generate_rand_zero_one(unsigned int *seed);

int generate_int_range(unsigned int *seed,
		       int min,
		       int max);

int find_frag(unsigned int *seed,
	      int total_frags);

#endif
