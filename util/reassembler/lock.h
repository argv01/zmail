#ifndef INCLUDE_REASSEMBLER_LOCK_H
#define INCLUDE_REASSEMBLER_LOCK_H

/* don't keep trying to get a lock beyond 16 minutes or so? */
#define MAXWAIT 1024

int lock(const char *);
int unlock(void);

#endif /* !INCLUDE_REASSEMBLER_LOCK_H */
