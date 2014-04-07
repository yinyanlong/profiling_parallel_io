/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "llist.h"

#define LIST1 1
#define LIST2 2

/**
 * Takes two locally ordered linked lists and merges them into one
 * sorted linked list which is returned.  The ordering is based on the
 * score and then secondly the result_id.  The memory from the input
 * result lists is freed.
 *
 * @param list1_p Locally ordered linked list 1.
 * @param list2_p Locally ordered linked list 2.
 * @return        Ordered merged linked list.
 */
struct lresult_s * merge_lresult(struct lresult_s *list1_p,
				 struct lresult_s *list2_p)
{
    struct lresult_s *list_head = NULL;
    int choose = -1;

    assert(list1_p != NULL && list2_p != NULL);

    /* Merge the results based on the score and result_id.  For a given
     * query, all result_ids should be unique.  They are based on a fragment */
    if (list1_p->result.score == list2_p->result.score)
    {
	if (list1_p->result.result_id == list2_p->result.result_id)
	{
	    fprintf(stderr, "Error: Results are identical (impossible)\n");
	    return NULL;
	}
	else if (list1_p->result.result_id > list2_p->result.result_id)
	{
	    choose = LIST1;
	}
	else
	    choose = LIST2;
    }
    else if (list1_p->result.score > list2_p->result.score)
    {
	choose = LIST1;
    }
    else
	choose = LIST2;

    /* Initialize the list head */
    if (choose == LIST1)
    {
	list_head = create_list_head(&(list1_p->result));
	list1_p = delete_list_head(list1_p);
    }
    else
    {
	list_head = create_list_head(&(list2_p->result));
	list2_p = delete_list_head(list2_p);
    }
    
    while(list1_p != NULL || list2_p != NULL)
    {
	/* If either list is empty, just add on the elements of the 
	 * other list */
	if (list1_p == NULL)
	{
	    push_list(list_head, &(list2_p->result));
	    list2_p = delete_list_head(list2_p);
	    continue;
	}
	else if (list2_p == NULL)
	{
	    push_list(list_head, &(list1_p->result));
	    list1_p = delete_list_head(list1_p);
	    continue;
	}
	
	/* Merge the sorted results based on score, result_id, */
	if (list1_p->result.score == list2_p->result.score)
	{
	    if (list1_p->result.result_id == list2_p->result.result_id)
	    {
		fprintf(stderr, "Error: Results are identical\n"
			"1 - (score=%d,result_id=%d,proc_id=%d)\n"
			"2 - (score=%d,result_id=%d,proc_id=%d)\n",
			list1_p->result.score,
			list1_p->result.result_id,
			list1_p->result.proc_id,
			list2_p->result.score,
			list2_p->result.result_id,
			list2_p->result.proc_id);
	    }
	    else if (list1_p->result.result_id > list2_p->result.result_id)
	    {
		choose = LIST1;
	    }
	    else
		choose = LIST2;
	}
	else if (list1_p->result.score > list2_p->result.score)
	{
	    choose = LIST1;
	}
	else
	    choose = LIST2;

	if (choose == LIST1)
	{
	    push_list(list_head, &(list1_p->result));
	    list1_p = delete_list_head(list1_p);
	}
	else
	{
	    push_list(list_head, &(list2_p->result));
	    list2_p = delete_list_head(list2_p);
	}
    }
	 
    return list_head;
}

/**
 * For every element in the result list, it frees the data of each
 * list element and then frees the list element itself.
 *
 * @param list_head Result list to be freed.
 */
void free_lresult_list(struct lresult_s *list_head)
{
	struct lresult_s *cur_lresult_p = list_head;
	struct lresult_s *next_lresult_p = cur_lresult_p->next;

	while (next_lresult_p != NULL)
	{
		free(cur_lresult_p->result.result_data);
		free(cur_lresult_p);
		cur_lresult_p = next_lresult_p;
		next_lresult_p = cur_lresult_p->next;
	}
	
	/* Free the last one */
	free(cur_lresult_p->result.result_data);
	free(cur_lresult_p);
}

/**
 * Copy information from one result to another.  If the src result has
 * result data, copy that as well.
 *
 * @param dest_result_p Result which will be overwritten.
 * @param src_result_p  Result which will be used as the source.
 * @return              0 on success.
 */
int cpy_result(struct result_s *dest_result_p, struct result_s *src_result_p)
{
    dest_result_p->result_id = src_result_p->result_id;
    dest_result_p->score     = src_result_p->score;
    dest_result_p->size      = src_result_p->size;
    dest_result_p->proc_id   = src_result_p->proc_id;
    
    if (dest_result_p->result_data != NULL)
    {
	fprintf(stderr, "cpy_result: freeing dest_result_p->result_data");
	free(dest_result_p->result_data);
    }
    
    /* Don't copy the result data if it doesn't exit. */
    if (src_result_p->result_data != NULL)
    {
	if ((dest_result_p->result_data = 
	     malloc(dest_result_p->size*sizeof(char))) == NULL)
	{
	    fprintf(stderr, "malloc dest_result_p->result_data of size %d "
		    "failed\n",
		    dest_result_p->size*sizeof(char));
	}
	memcpy(dest_result_p->result_data, 
	       src_result_p->result_data, 
	       dest_result_p->size);
    }
    return 0;
}

/**
 * Print result attributes and data of an entire result list.
 *
 * @param list_head The head pointer of a result list.
 */
void print_lresult_list(struct lresult_s *list_head)
{
    int i;
    struct lresult_s *cur_lresult_p;
    
    cur_lresult_p = list_head;
    fprintf(stderr, "printing lresult list...\n");
    while (cur_lresult_p->next != NULL)
    {
	fprintf(stderr, "(score=%d,proc_id=%d,result_id=%d,size=%d",
		cur_lresult_p->result.score,
		cur_lresult_p->result.proc_id, 
		cur_lresult_p->result.result_id,
		cur_lresult_p->result.size);
	if (cur_lresult_p->result.result_data != NULL)
	{
	    fprintf(stderr, ",result_data=");
	    for (i = 0; i <  cur_lresult_p->result.size; i++)
	    {
		fprintf(stderr, "%c", cur_lresult_p->result.result_data[i]);
	    }
	}
	fprintf(stderr, ")\n");
	cur_lresult_p = cur_lresult_p->next;
    }
    
    /* Print the last result */
    fprintf(stderr, "(score=%d,proc_id=%d,result_id=%d,size=%d",
	    cur_lresult_p->result.score,
	    cur_lresult_p->result.proc_id, 
	    cur_lresult_p->result.result_id,
	    cur_lresult_p->result.size);
    if (cur_lresult_p->result.result_data != NULL)
    {
	fprintf(stderr, ",result_data=");
	for (i = 0; i <  cur_lresult_p->result.size; i++)
	{
	    fprintf(stderr, "%c", cur_lresult_p->result.result_data[i]);
	}
    }
    fprintf(stderr, ")\n");
}

/**
 * Allocate and fill in the attributes for a list head from a result.
 *
 * @param result_p Result information to be added to the list head.
 * @return         The list head.
 */
struct lresult_s * create_list_head(struct result_s *result_p)
{
    struct lresult_s * lresult_p;
    
    if ((lresult_p = (struct lresult_s *) malloc(sizeof(struct lresult_s)))
	== NULL)
    {
	fprintf(stderr, "malloc lresult_p failed\n");
	return NULL;
    }
    
    memset(lresult_p, 0, sizeof(struct lresult_s));
    cpy_result(&(lresult_p->result), result_p);
    
    lresult_p->prev = NULL;
    lresult_p->next = NULL;
    
    return lresult_p;
}

/**
 * Add an element to the result list.
 *
 * @param list_head A result list pointer, where the next pointer will point
 *                  to the new element.
 * @param result_p  Result information to be added to the new element in the
 *                  list.
 */
void push_list(struct lresult_s *list_head, struct result_s *result_p)
{
    struct lresult_s * list_last_p = list_head;
    struct lresult_s * list_new_p = NULL;
    
    while(list_last_p->next != NULL)
	list_last_p = list_last_p->next;
    
    if ((list_new_p = (struct lresult_s *) 
	 (malloc(sizeof(struct lresult_s)))) == NULL)
    {
	fprintf(stderr, "malloc list_new_p failed\n");
	return;
    }
    
    memset(list_new_p, 0, sizeof(struct lresult_s));
    cpy_result(&(list_new_p->result), result_p);
    
    list_last_p->next = list_new_p;
    
    list_new_p->next = NULL;
    list_new_p->prev = list_last_p;
}

/**
 * Attempts to delete the list_head.  It could return NULL, which
 * indicates that the list no longer exists.
 *
 * @param list_head Pointer to the head of a result list.
 * @return          The new list head.
 */
struct lresult_s * delete_list_head(struct lresult_s *list_head)
{
    struct lresult_s * list_new_head_p = NULL;
    
    if (list_head->next != NULL)
    {
	list_new_head_p = list_head->next;
	list_new_head_p->prev = NULL;
    }
    
    free(list_head->result.result_data);
    free(list_head);
    
    return list_new_head_p;
}

