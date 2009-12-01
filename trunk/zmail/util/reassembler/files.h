#ifndef INCLUDE_REASSEMBLER_FILES_H
#define INCLUDE_REASSEMBLER_FILES_H


#include <sys/types.h>


FILE *popen_delivery(void);

int file_exists(const char *);
void filename_disarm(char *);

int mkdir_exist(const char *, mode_t);
int mkdirhier(char *, mode_t);


/*
 * highwater approximation of the largest number of
 * decimal digits required to print an unsigned long
 *
 * digits == bits * ln(2) / ln(10)
 *	  == bytes * 8 * ln(2) / ln(10)
 *	  == bytes * ln(2^8) / ln(10)
 *	  == bytes * log(2^8,10)
 *
 * but log(2^8,10) ~= 2.41, which is < 5/2
 *
 * so digits < bytes * 5/2
 *
 */
#define MAX_SUFFIX_LEN (sizeof(unsigned long)*5/2)

void filename_uniqify(char *);


#endif /* !INCLUDE_REASSEMBLER_FILES_H */
