#ifndef INCLUDE_REASSEMBLER_STATUS_H
#define INCLUDE_REASSEMBLER_STATUS_H

typedef struct ps {
    unsigned number;
    time_t mod_date;
    off_t size;
} PartStatus;

struct glist;

int partials_status(unsigned, char *);
PartStatus *new_piece_status(void);
void file_status(PartStatus *, char *);
int update_status_report(const char *, struct glist *);
int remove_status_report(const char *);

#endif /* INCLUDE_REASSEMBLER_STATUS_H */
