/*
  Program: fixhtml.c
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_XREF           256
#define MAX_NAME_LEN       256
#define MAX_TYPE_LEN       32
#define EXIST_IN           0
#define AT_START_OF        1
#define AT_END_OF          2
#define EXACT_MATCH        3
#define FIXHTML            "fixhtml"
#define PREAMBLE           "localhost"
#define IGNORE_THESE       "/zmail.ignoreinhtml"
#define DOT_IGNORE_THESE   "/.zmail.ignoreinhtml"
#define IMG_SRC            "<img src"
#define A_HREF             "<a href"
#define A_QUOTE            "\""
#define A_TERMINATOR       ">"
#define A_IMAGE            "image/"
#define A_NULL_STR         ""
#define A_SPACE            " "
#define NONRECURSIVE       "nonrecursive"
#define TEXT_HTML          "text/x-html"
#define CANNOT_OPEN_INPUT  "Cannot open %s as input\n"
#define CANNOT_OPEN_OUTPUT "Cannot open %s as output\n"
#define HOME_DIR           "HOME"
#define LIB_DIR            "ZMLIB"
#define DEFAULT_LIB        "./lib"
#define FOR_READING        "r"
#define FOR_WRITING        "w"
#define XREF_EXT           ".xrf"
#define TEMP_EXT           ".tmp"
#define XREF_TABLE_FORMAT  "%s %s %s"


char things_to_ignore[MAX_XREF][MAX_NAME_LEN+1];
int type_of_compare[MAX_XREF];
int n_to_ignore;
char type[MAX_XREF][MAX_TYPE_LEN+1];
char file_name[MAX_XREF][MAX_NAME_LEN+1];
char replacement_name[MAX_XREF][MAX_NAME_LEN+1];
int  n_xref;

/*
  Case insensitive compare function.
*/
int cistrcmp(str1,str2)
char *str1;
char *str2;
{
int dif;
  while (((dif = (toupper(*str1) - toupper(*str2))) == 0) && 
          (*str1 != '\0') &&
          (*str2 != '\0'))
    {
      str1++;
      str2++;
    }
  return(dif);
}

/*
  Case insensitive compare up to a fixed number of bytes function.
*/
int cistrncmp(str1,str2,len)
char *str1;
char *str2;
int len;
{
int dif , i;
  i = 0;
  while (((dif = (toupper(*str1) - toupper(*str2))) == 0) && 
          (*str1 != '\0') && 
          (*str2 != '\0'))
    {
      str1++;
      str2++;
      i++;
      if (i >= len)
        break;
    }
  return(dif);
}

/*
  This function is a case insensitive pattern match between a string and a
  pattern. It returns a pointer to the pattern within the string or NULL if
  the pattern is not found within the string.
*/
char *cistrstr(str,pat)
char *str;
char *pat;
{
char *anchor , *pattern;
  while (*str != '\0')
    {
      if (toupper(*str) == toupper(*pat))
        {
          anchor = str;
          pattern = pat;
          while (*pattern != '\0')
            {
              if (toupper(*anchor) == toupper(*pattern))
                {
                  anchor++;
                  pattern++;
                }
              else
                break;
            }
          if (*pattern == '\0')
            return(str);
        }
      str++;
    }
  return(NULL);
}

/*
  This function compares the line in the buffer to things to ignore in the
  things to ignore table. If it finds a match it returns a 1 which causes the
  line not to be written to the output file. The type of pattern match is
  determined by the entry in the type of compare table.
*/
int ignore_this_line(buf)
char *buf;
{
int i;
  for (i=0;i<n_to_ignore;i++)
    {
      if (type_of_compare[i] == EXIST_IN) /* String exist in buf */
        {
          if (strstr(buf,things_to_ignore[i]) != NULL)
            return(1);
        }
      else if (type_of_compare[i] == AT_START_OF) /* String at start of buf */
        {
          if (cistrncmp(buf,
                      things_to_ignore[i],
                      strlen(things_to_ignore[i])) == 0)
            return(1);
        }
      else if (type_of_compare[i] == AT_END_OF) /* String at end of buf */
        {
          if (cistrncmp(buf+strlen(buf)-strlen(things_to_ignore[i]),
                      things_to_ignore[i],
                      strlen(things_to_ignore[i])) == 0)
            return(1);
        }
      else /* String exact match to buf */
        {
          if (cistrcmp(buf,things_to_ignore[i]) == 0)
            return(1);
        }
    }
  return(0);
}

/*
  This function reads things to ignore from the zmail.ignoreinhtml file and
  puts them and their compare type in a table of things to ignore. If a line
  in the html contains a match to a thing in the ignore table the line is not
  written to the output file.
*/
int read_things_to_ignore()
{
FILE *infile;
char *path, filename[256];
char temp[256];
struct stat buf;
int rc;
char start_char , end_char;

/* Get the path and name of the html ignore file */
  n_to_ignore = 0;
  strcpy(filename,A_NULL_STR);
  if ((path = getenv(HOME_DIR)) != NULL)
    {
       strcpy(filename,path);
       strcat(filename,DOT_IGNORE_THESE);
       rc=stat(filename,&buf);
    }
  if ((rc != 0) && ((path = getenv(LIB_DIR)) != NULL))
    {
       strcpy(filename,path);
       strcat(filename,IGNORE_THESE);
       rc=stat(filename,&buf);
    }
  if (rc != 0)
    {
      strcpy(filename,DEFAULT_LIB);
      strcat(filename,IGNORE_THESE);
    }
/* Open the ignore file */
  if ((infile=fopen(filename,FOR_READING)) == NULL)
    {
      printf(CANNOT_OPEN_INPUT,filename);
      return;
    }
/* Read the ignore file and put its contents into the ignore table */
  while (fgets(things_to_ignore[n_to_ignore],MAX_NAME_LEN,infile) != NULL)
    {
      start_char = things_to_ignore[n_to_ignore][0];
      end_char = things_to_ignore[n_to_ignore][strlen(things_to_ignore[n_to_ignore])-2];
/* If the pattern has a * on both ends it is a exist in pattern */
      if ((start_char == '*') && (end_char == '*'))
        {
          type_of_compare[n_to_ignore] = EXIST_IN;
          things_to_ignore[n_to_ignore][strlen(things_to_ignore[n_to_ignore])-2] = '\0';
          strcpy(temp,&things_to_ignore[n_to_ignore][1]);
          strcpy(things_to_ignore[n_to_ignore],temp);
        }
/* If it has a * on the end it is a at start of pattern */
      else if (end_char == '*')
        {
          type_of_compare[n_to_ignore] = AT_START_OF;
          things_to_ignore[n_to_ignore][strlen(things_to_ignore[n_to_ignore])-2] = '\0';
        }
/* If it has a * st the start it is a at end of pattern */
      else if (start_char == '*')
        {
          type_of_compare[n_to_ignore] = AT_END_OF;
          strcpy(temp,&things_to_ignore[n_to_ignore][1]);
          strcpy(things_to_ignore[n_to_ignore],temp);
        }
/* If it has no * then it is an exact match pattern */
      else
        type_of_compare[n_to_ignore] = EXACT_MATCH;
/* Do not overflow the ignore table */
      if (n_to_ignore < (MAX_XREF-1))
        n_to_ignore++;
    }
/* Close the ignore it html file */
  fclose(infile);
}

/*
  This function reads the xref file into internal tables 
*/
void read_xref_file(xreffilename)
char *xreffilename;
{
FILE *infile;
char temp[256];

      n_xref = 0;
/* Read the xref table and count the rows */
      if ((infile=fopen(xreffilename,FOR_READING)) != NULL)
        {
          while (fscanf(infile,XREF_TABLE_FORMAT,
                         type[n_xref],
                         file_name[n_xref],
                         replacement_name[n_xref]) != EOF)
            {
/* Do not overflow the table */
              if (n_xref < (MAX_XREF-1))
                n_xref++;
            }
          fclose(infile);
        }
}

/*
  This program tries to solve two problems that occur when sending html as mail.

  The first problem is that the html may contain references to attachments that
  were sent along with the mail. These attachments are now probably located in
  a different place than they were when the html was written. Thus, the 
  references in the html must be changed to reflect the new locations if the
  attachments that were sent along with the html are to be found by the html
  viewer.
  This program scans for references in the html to attachments included with
  the html, and if it finds a match it replaces the reference with a reference
  to the new location of the attachment.

  The second problem is that the html sent may contain lines that your html
  viewer finds objectionable. This may lead the html viewer to refuse to 
  display the html as html but rather to display it as text. 
  This program scans the html file and eliminates lines that contain 
  substrings that the user has indicated occur in lines that are better 
  ignored. The patterns for substrings that occur in lines to ignore are 
  contained in a file named zmail.ignoreinhtml in the lib directory or 
  .zmail.ignoreinhtml in the users hone directory.
*/
main(argc,argv)
int argc;
char **argv;
{
char filename[256];
char xreffilename[256];
char tempfilename[256];
char cmd_line[256];
char buf[1024];
char tempbuf[1024];
char *pos , *src , *dest , *begin_str , *end_str , *anchor;
FILE *infile , *outfile;
char tempname[256];
int i , recursive , offset;

/* If there is no second arg on the command line then exit */
      if (argc < 2)
        exit(0);
/* If there is no third arg or if the third arg is nonrecursive then do not
   go recursive when executing this program */
      recursive = 1;
      if (argc > 2)
        if (cistrcmp(argv[2],NONRECURSIVE) == 0)
          recursive = 0;
/* 
  If the string defined in PREAMBLE is at the beginning of the file name
  take it off.
*/
      pos = strstr(argv[1],PREAMBLE);
      if (pos != NULL)
        pos = pos + strlen(PREAMBLE);
      else
        pos = argv[1];
/* Generate file names */
      strcpy(filename,pos);
      if (recursive == 0)
        strcpy(xreffilename,argv[3]);
      else
        {
          strcpy(xreffilename,filename);
          strcat(xreffilename,XREF_EXT);
        }
      strcpy(tempfilename,filename);
      strcat(tempfilename,TEMP_EXT);
/* Read table of things to ignore in the html */
      read_things_to_ignore();
/* Read the xref file into tables */
      read_xref_file(xreffilename);
/* If there are no xreferences and no lines to ignore then there is no
   reason to be running this program so exit */
   if ((n_xref == 0) && (n_to_ignore == 0))
     exit(0);
/* Open the html file */
      if ((infile=fopen(filename,FOR_READING)) == NULL)
        {
          printf(CANNOT_OPEN_INPUT,filename);
          exit(0);
        }
/* Open the temporary file */
      if ((outfile=fopen(tempfilename,FOR_WRITING)) == NULL)
        {
          printf(CANNOT_OPEN_OUTPUT,tempfilename);
          fclose(infile);
          exit(0);
        }
/* Process the input file into the temporary file */
      while (fgets(buf,1024,infile) != NULL)
        {
/* If there is anything to ignore check to see if the line is one to ignore */
          if (n_to_ignore > 0)
            {
              if (ignore_this_line(buf))
                continue;
            }
/* If there are any xreferences search the line for image declarations 
   that need to be rereferenced */
          src = buf;
          while ((n_xref > 0) && ((pos = cistrstr(src,IMG_SRC)) != NULL))
            {
/* This line has a image declaration in it so find the beginning of 
   the string that contains the location of the image */
              begin_str = strstr(pos,A_QUOTE);
              if (begin_str != NULL)
                {
                  begin_str++;
                  anchor = begin_str;
/* Find the end of the string that contains the image location */
                  end_str = strstr(begin_str,A_QUOTE);
                  if (end_str != NULL)
                    {
/* Get the name of the image location */
                      dest = tempname;
                      while (begin_str < end_str)
                        {
                          *dest++ = *begin_str++;
                        }
                      *dest = '\0';
/* See if the name of the image is like one in the xref table */
                      for (i=0;i<n_xref;i++)
                        {
/* If a match replace the location with the xref replacement location */
                          if (cistrncmp(type[i],A_IMAGE,6) == 0)
                            {
                              offset = strlen(tempname)-strlen(file_name[i]);
                              if ((offset >= 0) &&
                                  (cistrncmp(tempname+offset,
                                           file_name[i],
                                           strlen(file_name[i])) == 0))
                                {
                                  *anchor = '\0';
                                  strcpy(tempbuf,buf);
                                  strcat(tempbuf,replacement_name[i]);
                                  strcat(tempbuf,end_str);
                                  strcpy(buf,tempbuf);
                                }
                            }
                        }
                    }
                }
/* Reset the search location the the end of the image declaration */
              src = strstr(pos,A_TERMINATOR);
              if (src == NULL)
                break;
            }
/* If there are any xreferences search the line for hyperlink declarations 
   that need to be rereferenced */
          src = buf;
          while ((n_xref > 0) && ((pos = cistrstr(src,A_HREF)) != NULL))
            {
/* This line has a hyperlink declaration in it so find the beginning of 
   the string that contains the location of the hyperlink */
              begin_str = strstr(pos,A_QUOTE);
              if (begin_str != NULL)
                {
                  begin_str++;
                  anchor = begin_str;
/* Find the end of the string that contains the hyperlink location */
                  end_str = strstr(begin_str,A_QUOTE);
                  if (end_str != NULL)
                    {
/* Get the name of the hyperlink location */
                      dest = tempname;
                      while (begin_str < end_str)
                        {
                          *dest++ = *begin_str++;
                        }
                      *dest = '\0';
/* See if the name of the hyperlink is like one in the xref table */
                      for (i=0;i<n_xref;i++)
                        {
/* If a match replace the location with the xref replacement location */
                          if (cistrcmp(type[i],TEXT_HTML) == 0)
                            {
                              offset = strlen(tempname)-strlen(file_name[i]);
                              if ((offset >= 0) &&
                                  (cistrncmp(tempname+offset,
                                           file_name[i],
                                           strlen(file_name[i])) == 0))
                                {
                                  *anchor = '\0';
                                  strcpy(tempbuf,buf);
                                  strcat(tempbuf,replacement_name[i]);
                                  strcat(tempbuf,end_str);
                                  strcpy(buf,tempbuf);
                                }
                            }
                        }
                    }
                }
/* Reset the search location the the end of the hyperlink declaration */
              src = strstr(pos,A_TERMINATOR);
              if (src == NULL)
                break;
            }
/* Put the line to the temporary file */
          fputs(buf,outfile);
        }
/* Close the html and temporary files */
      fclose(infile);
      fclose(outfile);
/* Open the temporary file as input and the html file as output */
      if ((infile=fopen(tempfilename,FOR_READING)) == NULL)
        {
          printf(CANNOT_OPEN_INPUT,tempfilename);
          exit(0);
        }
      if ((outfile=fopen(filename,FOR_WRITING)) == NULL)
        {
          printf(CANNOT_OPEN_OUTPUT,filename);
          fclose(infile);
          exit(0);
        }
/* Copy the temporary file to the html file */
      while (fgets(buf,1024,infile) != NULL)
        {
          fputs(buf,outfile);
        }
/* Close the temporary file and the html file */
      fclose(infile);
      fclose(outfile);
/* Get rid of the temporary file */
      unlink(tempfilename);
/* If we are in the recursive mode, we fix up the html files in the xref table. 
   We fix up each file by running it through fixhtml in the nonrecursive mode.
   We pass the name of the file to fix up as the first arg. We pass 
   nonrecursive as the second arg. We pass the name of the xref file as 
   the third arg. */
   if (recursive == 1)
     {
       for (i=0;i<n_xref;i++)
         {
           if (cistrcmp(type[i],TEXT_HTML) == 0)
             {
               strcpy(cmd_line,FIXHTML);
               strcat(cmd_line,A_SPACE);
               strcat(cmd_line,replacement_name[i]);
               strcat(cmd_line,A_SPACE);
               strcat(cmd_line,NONRECURSIVE);
               strcat(cmd_line,A_SPACE);
               strcat(cmd_line,xreffilename);
               strcat(cmd_line,A_SPACE);
               system(cmd_line); 
             }
         }
/* If we are in the recursive mode we get rid of the xref file */
       unlink(xreffilename);
     }
}
