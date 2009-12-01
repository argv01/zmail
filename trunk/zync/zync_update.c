#include "popper.h"
#include <stdio.h>

static const char zync_update_rcsid[] =
    "$Id: zync_update.c,v 1.7 1995/10/26 20:21:24 bobg Exp $";

/* Update the headers of a given list of messages. */

int
zync_zuph(p)
    POP *p;
{
    int message_count;
    int *message_numbers;
    FILE *trace_fp;
    int *num_ptr;
    int *num_end;
    FILE *drop_fp;
    int offset;
    FILE *input_fp;
    char *trace_prefix;
    int c;

    do_drop(p);

    /* Parse the message list. */
    {
	struct number_list temp_list;

	if (parse_message_list(p, p->pop_parm[1], &temp_list) != 0)
	    return POP_FAILURE;
	message_count = temp_list.list_count;
	message_numbers = temp_list.list_numbers;
    }

    /* Send the success message. */
    pop_msg(p, POP_SUCCESS, "Send headers.");

    /* Seek to the end of the drop file. */
    trace_fp = p->trace;
    if (Verbose)
	pop_log(p, POP_DEBUG, "Seeking to end of drop file...");
    drop_fp = p->drop;
    offset = fseek(drop_fp, 0, SEEK_END);
    if (Verbose)
	pop_log(p, POP_DEBUG, "Size of drop file is %d octets.", offset);

    /* Set up loop over messages. */
    if (Verbose)
	pop_log(p, POP_DEBUG, "Reading message headers...");
    num_ptr = message_numbers;
    num_end = message_numbers + message_count;
    input_fp = p->input;
    trace_prefix = p->trace_prefix;

    /* For each message, read the message headers. */
    while (num_ptr < num_end)
    {
	int msg_num;
	struct msg_info *msg_info;
	int char_count;
	int old_offset;
	int new_offset;

      start:
	if (Verbose)
	    fprintf(trace_fp, "%sReceived: ", trace_prefix);
	c = getc(input_fp);
	switch (c)
	{
	  case EOF:
	    if (Verbose)
		fputs("EOF\n", trace_fp);
	    goto eof;
	  case '\n':
	    putc('\n', drop_fp);
	    if (Verbose)
		fputs("-\n", trace_fp);
	    char_count++;
	    goto start;
	  case '.':
	    goto dot;
	  default:
	    putc(c, drop_fp);
	    if (Verbose)
	    {
		putc('\"', trace_fp);
		putc(c, trace_fp);
	    }
	    char_count++;
	    goto middle;
	}

      middle:
	c = getc(input_fp);
	switch (c)
	{
	  case EOF:
	    if (Verbose)
		fputs("\"EOF\n", trace_fp);
	    goto eof;
	  case '\n':
	    putc('\n', drop_fp);
	    if (Verbose)
		fputs("\"\n", trace_fp);
	    char_count++;
	    goto start;
	  case '\r':
	    if (Verbose)
		fputs("\\r", trace_fp);
	    goto cr;
	  default:
	    putc(c, drop_fp);
	    if (Verbose)
		putc(c, trace_fp);
	    char_count++;
	    goto middle;
	}
	    
      cr:
	c = getc(input_fp);
	switch (c)
	{
	  case EOF:
	    if (Verbose)
		fputs("EOF", trace_fp);
	    goto eof;
	  case '\n':
	    putc('\n', drop_fp);
	    if (Verbose)
		putc('\n', trace_fp);
	    goto start;
	  case '\r':
	    putc('\r', drop_fp);
	    goto cr;
	  default:
	    putc('\r', drop_fp);
	    putc(c, drop_fp);
	    if (Verbose)
		putc(c, trace_fp);
	    goto middle;
	}

      dot:
	c = getc(input_fp);
	switch (c)
	{
	  case EOF:
	    if (Verbose)
		fputs(".EOF\n", trace_fp);
	    goto eof;
	  case '\r':
	    goto dot;
	  case '\n':
	    goto dot_newline;
	  case '.':
	    putc('.', drop_fp);
	    if (Verbose)
		fputs("\"..", trace_fp);
	    goto middle;
	  case 'm':
	    goto eat_sep_line;
	  default:
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY,
			"Bogus dot command.");
	    putc('.', drop_fp);
	    putc(c, drop_fp);
	    goto middle;
	}

	/* Found a separator. */
      eat_sep_line:
	c = getc(input_fp);
	switch (c)
	{
	  case EOF:
	    goto eof;
	  case '\n':
	    break;
	  default:
	    goto eat_sep_line;
	}

	if (num_ptr >= num_end - 1)
	{
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY,
			"Found message separator after last message.");
	    goto eat_start;
	}

      dot_newline:
	if (Verbose)
	    fputs(".\n", trace_fp);
	if (num_ptr < num_end - 1)
	{
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY,
			"Found terminator before last message.");
	}
	goto all_done;

      update_info:
	/* Update the message info structure. */
	old_offset = offset;
	new_offset = ftell(drop_fp);
	msg_num = *num_ptr;
	msg_info = NTHMSG(p, msg_num);
	msg_info->header_offset = old_offset;
	msg_info->header_length = new_offset - old_offset;
	offset = new_offset;
    }

    /* Come here when something's gone wrong. */
  eat_start:
    c = getc(input_fp);
    switch (c)
    {
      case EOF:
	goto eof;
      case '.':
	goto eat_dot;
      case '\n':
	goto eat_start;
      default:
	goto eat_middle;
    }

  eat_middle:
    c = getc(input_fp);
    switch (c)
    {
      case EOF:
	goto eof;
      case '\n':
	goto eat_start;
      default:
	goto eat_middle;
    }

  eat_dot:
    c = getc(input_fp);
    switch (c)
    {
      case EOF:
	goto eof;
      case '\r':
	goto eat_dot;
      case '\n':
	goto all_done;
      default:
	goto eat_middle;
    }

  all_done:
    pop_msg(p, POP_SUCCESS, "Messages updated.");
    return POP_SUCCESS;

  eof:
    pop_msg(p, POP_FAILURE, "End of file on input.");
    return POP_FAILURE;
}

#if 0
int zync_zupb(p)
    POP *p;
{
    pop_msg(p, POP_FAILURE, "ZUPB isn't implemented.");
    return POP_FAILURE;
}


int zync_zupf(p)
    POP *p;
{
    pop_msg(p, POP_FAILURE, "ZUPF isn't implemented.");
    return POP_FAILURE;
}
#endif
