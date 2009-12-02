/* @(#)sendmime.c   (C) 1996 by Pawel Wiecek */
/*                  coven@pwr.wroc.pl        */
/* corrections by Pawel Wiecek 1997.07.13    */

/* This file defines (almost) everything needed for
   sending MIME compliant messages                  */

#include "mush.h"
#include <string.h>

/* we really need a function, not a macro... */
#undef strdup

struct mimepart_info {
 char *filename;
 char *mimetype;
 char *encoding;
 char temporary;
};

static struct mimepart_info mime_parts[64];    /* hope that's enough ;^) */

static int mime_body=0, mime_attcnt=0;
static char *mime_output;
static FILE *mime_outfile;
int mime_used=0;             /* this will be seen outside */

char *filename_to_mimetype();
FILE *mime_tempfile();
void send_fd();

/* ask_mime_part asks about some MIME message part's parameters
 * if isattach == 0 it asks about mime type and encoding of
 * the message body
 * if isattach != 0 it asks about filename, mime type and encoding
 * of an attachment                                                  */

void ask_mime_part(int isattach) {
 char *guess_type, *final_type, *filename, *guess_encoding, *final_encoding,
      *tempfile, *ptr;
 FILE *fp_temp, *fp_pipe;
 int res;

 ptr=do_set(set_options,"metasend");
 if(!ptr || !*ptr) {
  error("MIME support is not enabled! Set metasend variable to enable it.");
  return;
 }
 if(isattach) {
  if(mime_attcnt>=63) {
   error("you can attach up to 63 files");
   return;
  }
askfile:
  filename=strdup(set_header("File to attach: ","",TRUE));
  if(*filename=='|') {
   fp_temp=mime_tempfile(&tempfile);
   wprint("Running %s...\n",filename+1);
   fp_pipe=popen(filename+1,"r");
   send_fd(fp_pipe,fp_temp);
   pclose(fp_pipe);
   fclose(fp_temp);
   guess_type="application/octet-stream";
  } else {
   res=0;
   ptr=getpath(filename,&res);
   free(filename);
   if(res==-1) {
    error(ptr);
    goto askfile;           /* aargh! such an ugly way to do this! */
   }
   filename=strdup(ptr);
   guess_type=filename_to_mimetype(filename);
  }
 } else {
  guess_type=mime_parts[0].mimetype;
  free(mime_parts[0].encoding);
 }
 final_type=strdup(set_header("Mime type: ",guess_type,TRUE)); 
 if(strncmp(final_type,"text/",5)) guess_encoding="base64";
                              else guess_encoding="quoted-printable";
 final_encoding=strdup(set_header("Encoding: ",guess_encoding,TRUE));
 while(strcmp(final_encoding,"7bit")&&
       strcmp(final_encoding,"base64")&&
       strcmp(final_encoding,"quoted-printable")&&
       strcmp(final_encoding,"x-uue")) {
  error("unknown encoding: %s",final_encoding);
  free(final_encoding);
  final_encoding=strdup(set_header("Encoding: ",guess_encoding,TRUE));
 }
 if(isattach) {
  mime_attcnt++;
  mime_parts[mime_attcnt].temporary=(*filename=='|');
  mime_parts[mime_attcnt].filename=mime_parts[mime_attcnt].temporary?
                                   tempfile:filename;
  mime_parts[mime_attcnt].mimetype=final_type;
  mime_parts[mime_attcnt].encoding=final_encoding;
 } else {
  mime_body=1;
  mime_parts[0].filename=NULL;
  mime_parts[0].mimetype=final_type;
  mime_parts[0].encoding=final_encoding;
  mime_parts[0].temporary=1;
  free(guess_type);
 }
 mime_used=1;
}

/* enmime converts message to MIME format, if needed */

FILE *enmime(FILE *source) {
 FILE *body_fp;
 char cmd[4096];     /* hope that's enough */
 char *metasend;     /* prgram we use as metasend */
 int i;

 if(mime_used) {
  /* first find out what is metasend... if there's none we won't use it ;^) */
  metasend=do_set(set_options,"metasend");
  if(!metasend)
   return source;
  /* make tempfile with message body... */
  body_fp=mime_tempfile(&(mime_parts[0].filename));
  if(!body_fp) return source;
  send_fd(source,body_fp);
  fclose(body_fp);
  /* prepare outfile... */
  mime_outfile=mime_tempfile(&mime_output);
  if(!mime_outfile) return source;
  fclose(mime_outfile);
  /* filter all the stuff through metasend (or whatever) */
  sprintf(cmd,"%s -b -o %s -f %s -m \"%s\" -e %s -D \"Message body\"",
          metasend,mime_output,mime_parts[0].filename,
          mime_parts[0].mimetype,mime_parts[0].encoding);
  for(i=1;i<=mime_attcnt;i++)
   sprintf(cmd+strlen(cmd)," -n -f %s -m \"%s\" -e %s -D %s",
           mime_parts[i].filename,mime_parts[i].mimetype,
           mime_parts[i].encoding,mime_parts[i].temporary?
           "\"Output from pipe\"":basename(mime_parts[i].filename));
  /* ugly way to call this... */
  system(cmd);
  /* reopen outfile... */
  mime_outfile=fopen(mime_output,"r");
  return mime_outfile;
 } else {
  /* no need to convert anything - just return what we got */
  return source;
 }
}

/* mime_clear removes any additional files left by enmime, frees allocated
 * strings and closes files that were open                                 */

void mime_clear(void) {
 int i;

 for(i=0;i<=mime_attcnt;i++) {
  if(mime_parts[i].temporary)
   unlink(mime_parts[i].filename);
  free(mime_parts[i].filename);
  free(mime_parts[i].mimetype);
  free(mime_parts[i].encoding);
 }
 fclose(mime_outfile);
 unlink(mime_output);
 free(mime_output);
}

/* filename_to_mimetype guesses mime type for given filename
 * the guess is based on filename's extension (if any) and mime.types
 * files                                                                */

char *ext2mime_file();          /* supporting */

char *filename_to_mimetype(char *filename) {
 char *bname, *ext, *type, *home, *file, buf[MAXPATHLEN];

 bname=basename(filename);
 ext=strrchr(bname,'.');
 if(ext) {
  ext++;
  home=getenv("HOME");
  if(home) {                     /* just to make sure... */
   sprintf(buf,"%s/.mimetypes",home);
   type=ext2mime_file(ext,buf);
   if(type) return type;
  }
  file=do_set(set_options,"mimetypes");
  if(file) {
   type=ext2mime_file(ext,file);
   if(type) return type;
  }
  type=ext2mime_file(ext,"/etc/mime.types");
  if(type) return type;
 }
 return "application/octet-stream";        /* this is the default */
}

/* ext2mime_file does the mime type guess job using one mime.types file.
 * Also, it gets an extension as a parameter, not a filename.
 * If given mime.types file doesn't exist we pretend it's empty.
 * The guess we make here is not perfect, but it will often work - you
 * can correct guessed type anyway :^)                                   */

char *ext2mime_file(char *ext, char *file) {
 FILE *f;
 char line[512], rext[16];   /* hopefully enough */
 static char type[64];       /* same as above :^) */
 char *p1, *p2;

 f=fopen(file,"r");
 if(!f)                      /* probably doesn't exist */
  return NULL;
 while(fgets(line,512,f)) {
  if(!line[0] || line[0]=='#')  /* skip blanks and comments */
   continue;
  p1=line;
  p2=type;
  while(!isspace(*p1))
   *(p2++)=*(p1++);
  *p2='\0';
  while(1) {
   while(isspace(*p1))
    p1++;
   if(!*p1) break;      /* end of line reached */
   p2=rext;
   while(!isspace(*p1))
    *(p2++)=*(p1++);
   *p2='\0';
   if(!strcmp(ext,rext)) {
    fclose(f);
    return type;
   }
  }
 }
 fclose(f);
 return NULL;
}

/* mime_tempfile creates temporary filename for use by another functions
 * in this module. It uses same algorithm as is used for other tempfiles
 * used by mush */

FILE *mime_tempfile(char **filename) {
 char buf[MAXPATHLEN], *dir;
 FILE *fp;

 if(!(dir=getdir(do_set(set_options,"tmpdir"))))
alted:
  dir=ALTERNATE_HOME;
 mkstemp(sprintf(buf,"%s/.mimeXXXXXX",dir));
 *filename=strdup(buf);
 if(!(fp=mask_fopen(*filename,"w"))) {
  if(strcmp(dir,ALTERNATE_HOME))
   goto alted;
  error("can't create %s",*filename);
  return NULL;
 }
 return fp;
}

/* send_fd sends contents of one file pointer to another one
 * Borrowed from NCSA's util.c file */

void send_fd(FILE *src, FILE *dst) {
 char c;

 while(1) {
  c=fgetc(src);
  if(feof(src))
   return;
  fputc(c,dst);
 }
}

/* mime_prepare prepares internal variables for sending new message */

void mime_prepare(void) {
 char *ptr;

 mime_body=mime_attcnt=0;
 ptr=do_set(set_options,"alwaysmime");
 mime_used=!!ptr;
 ptr=do_set(set_options,"defaultbodytype");
 if(ptr && *ptr) mime_parts[0].mimetype=strdup(ptr);
            else mime_parts[0].mimetype=strdup("text/plain");
 if(strncmp(mime_parts[0].mimetype,"text/",5))
  mime_parts[0].encoding=strdup("base64");
 else
  mime_parts[0].encoding=strdup("quoted-printable");
 mime_parts[0].filename=NULL;
 mime_parts[0].temporary=1;
}

