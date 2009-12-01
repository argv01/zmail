#ifndef INCLUDE_MSGS_COMPOSE_H
#define INCLUDE_MSGS_COMPOSE_H


#include <general.h>
#include <stdio.h>

struct Compose;
struct HeaderField;
struct mfolder;
struct mgroup;

struct HeaderField *create_header P((char *name, char *body));
struct HeaderField *duplicate_header P((struct HeaderField *hf));
void destroy_header P((struct HeaderField *hf));
void free_headers P((struct HeaderField **hf));
long store_headers P((struct HeaderField **fields, FILE *fp));
struct HeaderField *lookup_header P((struct HeaderField **hf,
	 char *name,
	 char *join,
	 int crunch));
void merge_headers P((struct HeaderField **hf1,
	 struct HeaderField **hf2,
	 char *join,
	 int crunch));
int output_headers P((struct HeaderField **hf, FILE *fp, int all));
int validate_from P((char *from));
int interactive_header P((struct HeaderField *hf, char *command, char *value));
int is_dynamic_header P((struct HeaderField *hf));
int dynhdr_to_choices P((char ***choices,
	 char **comment,
	 char **body,
	 char *name));
int dynamic_header P((struct Compose *compose, struct HeaderField *hf));
void personal_headers P((struct Compose *compose));
void mta_headers P((struct Compose *compose));
void gui_compose_headers P((struct Compose *compose));
long generate_headers P((struct Compose *compose, FILE *fp, int merge));
int start_compose P((void));
void assign_compose P((struct Compose *dest_compose, struct Compose *src_compose));
void reset_compose P((struct Compose *compose));
void stop_compose P((void));
void suspend_compose P((struct Compose *compose));
void resume_compose P((struct Compose *compose));
void clean_compose P((int sig));
void mark_replies P((struct Compose *compose));
int check_replies P((struct mfolder *fldr, struct mgroup *mgrp, int query));
int BinaryTextOk P((struct Compose *));
const char *MimeCharSetParam P((struct Compose *compose));
int SunStylePreambleString P((struct Compose *compose, int lines, char *buf));
int MimeBodyPreambleString P((struct Compose *compose, int lines, char *buf));
void MimeHeaderTransferEncoding P((struct Compose *compose));
void MimeHeaderContentType P((struct Compose *compose));
void MimeHeaderVersion P((struct Compose *compose));
void add_headers P((FILE *files[], int size, int log_file));
char *get_envelope_addresses P((struct Compose *compose));
char *set_header P((char *str, const char *curstr, int do_prompt));
void input_header P((char *h, char *p, int negate));
void input_address P((int h, char *p));
int generate_addresses P((char **, char *, int, struct mgroup *));
void request_receipt P((struct Compose *compose, int on, const char *p));
void request_priority P((struct Compose *compose, const char *p));
char *get_priority P((struct Compose *compose));
char **addr_vec P((const char *s));
int addr_count P((const char *p));
void set_address P((struct Compose *compose, int hdr_index, char **value));
char **get_address P((struct Compose *compose, int hdr_index));
void add_address P((struct Compose *compose, int hdr_index, char *value));


#endif /* !INCLUDE_MSGS_COMPOSE_H */
