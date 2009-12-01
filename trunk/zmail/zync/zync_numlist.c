#include "popper.h"
#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif /* HAVE_MEMORY_H */

#define INITIAL_LIST_CAPACITY 16


static int do_parse();


/*
 * Parse a message list given as a string, and convert it to an array of ints,
 * returned via a struct number_list.  The caller must free the array of ints!
 * Sends a failure message if something goes wrong.
 */

int
parse_message_list(p, string, list_ret)
    POP *p;
    char *string;
    struct number_list *list_ret;
{
    return do_parse(p, string, 1, NUMMSGS(p), list_ret, 0);
}


/*
 * Same as above but for bucket lists.
 */

int parse_bucket_list(p, string, total_buckets, list_ret)
    POP *p;
    char *string;
    int total_buckets;
    struct number_list *list_ret;
{
    return do_parse(p, string, 0, total_buckets - 1, list_ret, 1);
}


/*
 * Common routine to parse message or bucket list.
 */

static int do_parse(p, string, min, max, list_ret, is_bucket)
    POP *p;
    char *string;
    int min, max;
    struct number_list *list_ret;
    int is_bucket;
{
    int *numbers;
    int count;
    int capacity;
    char *str_ptr;
    int c;
    int first_num;
    int second_num;
    int new_count;
    int loop_num;
    int *num_ptr;

    /* Allocate a new list. */
    numbers = (int *)malloc(INITIAL_LIST_CAPACITY * sizeof(int));
    if (numbers == NULL)
	goto cant_allocate;
    count = 0;
    capacity = INITIAL_LIST_CAPACITY;
    str_ptr = string;

  first_start:
    c = *str_ptr;
    if (c < '0' || c > '9')
	goto done;		/* 1st char may be non-numeric to indicate
				 * an empty list */
    first_num = c - '0';
    str_ptr++;
    goto first_next;

  first_next:
    c = *str_ptr;
    str_ptr++;
    switch (c)
    {
      case 0:
      case ',':
	goto add_single;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	first_num = 10 * first_num + (c - '0');
	goto first_next;
      case '-':
      case ':':
	goto second_start;
      default:
	goto syntax_error;
    }

    /* Add a single number to the list. */
  add_single:
    if (first_num < min || first_num > max)
	goto out_of_bounds;
    if (count >= capacity)
    {
	capacity *= 2;
	numbers = (int *)realloc(numbers, capacity * sizeof(int));
	if (numbers == NULL)
	    goto cant_allocate;
    }
    numbers[count] = first_num;
    count++;
    if (c != 0)
	goto first_start;
    else
	goto done;

    /* We found a hyphen, start parsing the second number. */
  second_start:
    c = *str_ptr;
    if (c < '0' || c > '9')
	goto syntax_error;
    second_num = c - '0';
    str_ptr++;
    goto second_middle;

  second_middle:
    c = *str_ptr;
    str_ptr++;
    switch (c)
    {
      case 0:
      case ',':
	goto add_range;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	second_num = 10 * second_num + (c - '0');
	goto second_middle;
      default:
	goto syntax_error;
    }

    /* We got a range, put it in the list. */
  add_range:
    if (second_num < min || second_num > max)
    {
	first_num = second_num;
	goto out_of_bounds;
    }
    if (second_num < first_num)
	goto backward_range;
    new_count = count + (second_num - first_num + 1);
    if (new_count > capacity)
    {
	while (capacity < new_count)
	    capacity *= 2;
	numbers = (int *)realloc((char *)numbers, capacity * sizeof(int));
	if (numbers == NULL)
	    goto cant_allocate;
    }
    loop_num = first_num;
    num_ptr = numbers + count;
    while (loop_num <= second_num)
    {
	*num_ptr = loop_num;
	loop_num++;
	num_ptr++;
    }
    count = new_count;
    if (c != 0)
	goto first_start;
    else
	goto done;

  done:
    list_ret->list_count = count;
    list_ret->list_numbers = numbers;
    return 0;

  cant_allocate:
    pop_msg(p, POP_FAILURE, "Can't allocate space for %s list.",
	    is_bucket ? "bucket" : "message");
    return 1;

  syntax_error:
    pop_msg(p, POP_FAILURE,
	    "Syntax error in %s list: unexpected character %d.",
	    is_bucket ? "bucket" : "message", c);
    return 1;

  out_of_bounds:
    pop_msg(p, POP_FAILURE, "%s number %d is out of bounds.",
	    is_bucket ? "Bucket" : "Message", first_num);
    return 1;

  backward_range:
    pop_msg(p, POP_FAILURE,
	    "Error in %s list: end of range is less than start.",
	    is_bucket ? "bucket" : "message");
    return 1;
}
