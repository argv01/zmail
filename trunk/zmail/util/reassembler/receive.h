#ifndef INCLUDE_REASSEMBLER_RECEIVE_H
#define INCLUDE_REASSEMBLER_RECEIVE_H


struct glist;

void init_groups(void);
unsigned receive(char **, struct glist *);


#endif /* !INCLUDE_REASSEMBLER_RECEIVE_H */
