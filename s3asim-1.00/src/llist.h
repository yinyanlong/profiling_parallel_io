/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _LLIST_H
#define _LLIST_H

#include "test.h"

struct lresult_s
{
	struct result_s result;
	struct lresult_s *prev;
	struct lresult_s *next;
};

struct lresult_s * merge_lresult(struct lresult_s *list1_p,
				 struct lresult_s *list2_p);

void free_lresult_list(struct lresult_s *list_head);

int cpy_result(struct result_s *dest_result_p, struct result_s *src_result_p);

struct lresult_s * create_list_head(struct result_s *result_p);

struct lresult_s * delete_list_head(struct lresult_s *list_head_p);

void push_list(struct lresult_s *list_head_p, struct result_s *result_p);

void print_lresult_list(struct lresult_s *list_head);
#endif
