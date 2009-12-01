#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <except.h>
#include <excfns.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "fatal.h"
#include "headers.h"
#include "status.h"


static struct TotalStatus {
    unsigned num_pieces; /* 0 if we don't know. */
    unsigned num_pieces_so_far; /* count down the pieces list */
} status;


static struct glist part_list;


int
partials_status(unsigned total_num, char *waiting_room)
{
    /* Someone screwing with us in terms of renaming files or touching */
    /* a file called "last-*" can't be helped. */

    DIR *dirp;
    PartStatus part_status;
    struct dirent *direntp;
    int numbered;
    char buf[PATH_MAX];
    status.num_pieces = total_num;
    status.num_pieces_so_far = 0;

    /* return 1 if all the parts have arrived */
    /* return 0 if we're still waiting for pieces */
    /* fill in the TotalStatus struct if we're still waiting for pieces */
    
    ASSERT(dirp = opendir(waiting_room), strerror(ENOENT), WHERE(partials_status));

    if (status.num_pieces >0) {
	sprintf(buf, "%s/last-%u", waiting_room, status.num_pieces);
	ASSERT(creat(buf, S_IRUSR | S_IWUSR) >= 0, strerror(errno), WHERE(partials_status));
    } else {
    
	/* look for a file named last-#, which indicates the largest numbered */
	/* file has arrived.  If it isn't there, we know we don't have all */
	/* the parts yet.  Also, the # is really the number of parts, so we */
	/* have to bother opening the file to figure that out. */

#define LASTMARKER "last-"

	while ((direntp = readdir(dirp)) != NULL){
	    if (!strncmp(direntp->d_name, LASTMARKER, sizeof(LASTMARKER) - 1)) {
		status.num_pieces = 
		    atoi(&direntp->d_name[sizeof(LASTMARKER) - 1]); /* I love that */
		sprintf(buf, "%s/%s", waiting_room, direntp->d_name);
		if (status.num_pieces > 0)
		    break;
	    }
	}

	rewinddir(dirp);
    }
    
    glist_Init(&part_list, sizeof(PartStatus), 8);

    if (status.num_pieces > 0) {
	/* this section assumes that if we have N pieces and there */
	/* are N files that atoi between 1 and N, we're golden. */
	/* if atoi can return the same positive int value for more than */
	/* one filename, this isn't a valid assumption */
	while ((direntp = readdir(dirp)) != NULL){
	    int i;
	    i = atoi(direntp->d_name);
	    if ((i > 0) && (i <= status.num_pieces)) {
		status.num_pieces_so_far++;
		part_status.number = atoi(direntp->d_name);
		file_status(&part_status, waiting_room);
		glist_Add(&part_list, &part_status);
	    }
	}
	if (status.num_pieces_so_far == status.num_pieces) {
	    /* free the stuff in the totalStatus structure since we */
	    /* don't need it anymore */
	    glist_Destroy(&part_list);
	    unlink(buf);
	    return 1;
	}
    } else {
	/* if we don't know how many files to expect, we have to assume */
	/* any file that has just digits in its filename is a valid part */
	while ((direntp = readdir(dirp)) != NULL){
	    /* is direntp->d_name a number */
	    size_t i;
	    numbered = 1;
	    for (i = strlen(direntp->d_name) -1; i > 0; i--)
		if (isdigit(direntp->d_name[i]) == 0)	
		    numbered = 0;  
	    if (numbered && (isdigit(direntp->d_name[0]))) {
		status.num_pieces_so_far++;
		part_status.number = atoi(direntp->d_name);
		file_status(&part_status, waiting_room);
		glist_Add(&part_list, &part_status);
	    }
	}
    }

    closedir(dirp);
    return 0;
}

void
file_status(PartStatus *part, char *directory)
{
    struct stat statbuf;
    static char path[PATH_MAX];

    if (directory[strlen(directory) -1]  == '/')
	sprintf(path, "%s%u", directory, part->number);
    else
    	sprintf(path, "%s/%u", directory, part->number);

    if (stat(path, &statbuf) != 0) {
	/* XXX something didn't work */
    } else {
	part->mod_date = statbuf.st_mtime;
	part->size = statbuf.st_size;
    }
}

int
update_status_report(const char *directory, struct glist *headers)
{
#define STATUSFILENAME "status"

    FILE *status_ptr;
    char status_file[PATH_MAX];
    const char *from, *other;
    PartStatus *walker;

    
    if (directory[strlen(directory) -1]  == '/')
	sprintf(status_file, "%s%s", directory, STATUSFILENAME);
    else
    	sprintf(status_file, "%s/%s", directory, STATUSFILENAME);

    status_ptr = efopen(status_file, "w", WHERE(update_status_report));


    if ((from = header_find(headers, "From")) != 0)
	fprintf(status_ptr, "\
A very large message from%s\
is being received.  This message has been split into several\n\
parts and will be reassembled when all parts have been received.\n",
		from);
    else
	fputs("\
A very large message is being received.  This message has been\n\
split into several parts and will be reassembled when all parts\n\
have been received.\n", status_ptr);
	
    fprintf(status_ptr,"\n\
You have received %u part%s.\n\
\n\
Message Information:\n\n", status.num_pieces_so_far, status.num_pieces_so_far == 1 ? "" : "s");


    if ((from = header_find(headers, "From")) != 0)
	fprintf(status_ptr,"\tFrom:%s", from);
    if ((other = header_find(headers, "To")) != 0)
	fprintf(status_ptr,"\tTo:%s", other);
    if ((other = header_find(headers, "Cc")) != 0)
	fprintf(status_ptr,"\tCc:%s", other);
    if ((other = header_find(headers, "Subject")) != 0)
	fprintf(status_ptr,"\tSubject:%s", other);
    
    fputs("\nThe following parts have been received:\n", status_ptr);
    {
	int counter;
	PartStatus *walker;

	glist_FOREACH(&part_list, PartStatus, walker, counter)
	    fprintf(status_ptr, "\n\
\tPart number: %u\n\
\tTime of arrival: %s\
\tSize of part: %d bytes\n",
		    walker->number, ctime(&walker->mod_date), walker->size);
    }

    fatal_sink = status_ptr;
    glist_Destroy(&part_list);
}

int
remove_status_report(const char *directory)
{
    char status_file[PATH_MAX];

    if (directory[strlen(directory) -1]  == '/')
	sprintf(status_file, "%s%s", directory, STATUSFILENAME);
    else
    	sprintf(status_file, "%s/%s", directory, STATUSFILENAME);

    return unlink(status_file);

}
