/*
This module integrates the gnu HTML widget into ZMail to allow for inline
HTML viewing. The functions in this module are called from a special version
of m_msg.c stored in html.m_msg.c. At the present time this module is not in
the production build. This module is not even near being finished! It was 
only placed in the CVS system to prevent it from being lost. To get a copy
of the XmHTML widget source code go to www.nerdnet.nl/~koen/XmHTML on the 
net or search for XmHTML.
*/
/*
m_html.c

This module contains support for inline HTML viewing.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>

#include <XmHTML.h>
#include <Xm/Text.h>
#include "m_msg.h"

static char * ReadTheFile();
char * basename();
void html_link_to_file();

/*
  This function initializes the history of visited urs's.
*/
void html_initialize_history(m_data)
msg_data *m_data;
{
int i;
  m_data->html_page_stack = -1;
  for (i=0;i<MSG_MAX_HTML_STACK;i++)
    {
      m_data->html_page_name[i] = NULL;
      m_data->html_page_local[i] = False;
    }
}

/*
  This function terminates the history of visited urs's.
*/
void html_terminate_history(m_data)
msg_data *m_data;
{
int i;
  m_data->html_page_stack = -1;
  m_data->html_string = NULL;
  m_data->base_page = NULL;
  for (i=0;i<MSG_MAX_HTML_STACK;i++)
    {
      if (m_data->html_page_name[i] != NULL)
        free(m_data->html_page_name[i]);
      m_data->html_page_name[i] = NULL;
      m_data->html_page_local[i] = False;
    }
}

/*
  This function adds the url to the history of visited urs's.
*/
void html_push_history(m_data,name,local)
msg_data *m_data;
char *name;
Boolean local;
{
  if (m_data->html_page_stack < (MSG_MAX_HTML_STACK-1))
    {
      m_data->html_page_stack++;
      m_data->html_page_name[m_data->html_page_stack] = malloc(strlen(name)+1);
      if (m_data->html_page_name[m_data->html_page_stack])
        {
          strcpy(name,m_data->html_page_name[m_data->html_page_stack]);
          m_data->html_page_local[m_data->html_page_stack] = local;
        }
      else
        m_data->html_page_stack--;
    }
}

/*
  This function deletes a url from the history of visited urs's.
*/
void html_pop_history(m_data)
msg_data *m_data;
{
  if (m_data->html_page_stack < 0)
    return;
  if (m_data->html_page_name[m_data->html_page_stack])
    free(m_data->html_page_name[m_data->html_page_stack]);
  m_data->html_page_name[m_data->html_page_stack] = NULL;
  m_data->html_page_local[m_data->html_page_stack] = False;
  m_data->html_page_stack--;
}

char *html_top_of_stack(m_data,local)
msg_data *m_data;
Boolean *local;
{
  if (m_data->html_page_stack >= 0)
    {
      *local = m_data->html_page_local[m_data->html_page_stack];
      return(m_data->html_page_name[m_data->html_page_stack]);
    }
  else
    return(NULL);
}

/*
  This function displays the previously visited url.
*/
void html_go_to_previous(m_data)
msg_data *m_data;
{
}

/*
  This function gets the html base page.
*/
char *html_get_base_page(m_data)
msg_data *m_data;
{
  return(m_data->base_page);
}

/*
  This function handles clicks on anchors.
*/
static void
anchorCB(Widget w, XtPointer arg1, XmHTMLAnchorPtr href_data)
{
int size;
msg_data *m_data;

  XtVaGetValues(w, XmNuserData, &m_data, NULL);

  /* see if we have been called with a valid reason */
  if(href_data->reason != XmCR_ACTIVATE)
          return;
 
  switch(href_data->url_type)
  {
    /* a named anchor */
    case ANCHOR_JUMP:
      {
        int id;
        if((id = XmHTMLAnchorGetId(w, href_data->href))!= -1)
        {
          href_data->doit = True;
          href_data->visited = True;
          return;
        }
        return;
      }
      break;
 
    /* a local file or a url on the web */
    case ANCHOR_FILE_LOCAL:
      {
        String chPtr;
        fprintf(stderr, "anchor file local: %s\n", href_data->href);
 
        if((chPtr = strstr(href_data->href, "#")) != NULL)
        {
          char tmp[1024];
          strncpy(tmp, href_data->href, chPtr - href_data->href);
          tmp[chPtr - href_data->href] = '\0';
          html_link_to_file(m_data,tmp,chPtr);
        }
        else
          html_link_to_file(m_data,href_data->href,chPtr);
      }
      break;
    case ANCHOR_HTTP:
      {
        String chPtr;
        fprintf(stderr, "fetch http file: %s\n", href_data->href);
 
        if((chPtr = strstr(href_data->href, "#")) != NULL)
        {
          char tmp[1024];
          strncpy(tmp, href_data->href, chPtr - href_data->href);
          tmp[chPtr - href_data->href] = '\0';
          html_link_to_file(m_data,tmp,chPtr);
        }
        else
          html_link_to_file(m_data,href_data->href,chPtr);
      }
      break;
 
    /* all other types are unsupported */
    case ANCHOR_EXEC:
      fprintf(stderr, "execute: %s\n", href_data->href);
      break;
    case ANCHOR_FILE_REMOTE:
      fprintf(stderr, "fetch remote file: %s\n", href_data->href);
      break;
    case ANCHOR_FTP:
      fprintf(stderr, "fetch ftp file: %s\n", href_data->href);
      break;
    case ANCHOR_GOPHER:
      fprintf(stderr, "gopher: %s\n", href_data->href);
      break;
    case ANCHOR_WAIS:
      fprintf(stderr, "wais: %s\n", href_data->href);
      break;
    case ANCHOR_NEWS:
      fprintf(stderr, "open newsgroup: %s\n", href_data->href);
      break;
    case ANCHOR_TELNET:
      fprintf(stderr, "open telnet connection: %s\n", href_data->href);
      break;
    case ANCHOR_MAILTO:
      fprintf(stderr, "email to: %s\n", href_data->href);
      break;
    case ANCHOR_UNKNOWN:
    default:
      fprintf(stderr, "don't know this type of url: %s\n",href_data->href);
      break;
  }
}

/*
  This function loads images.
*/
static XmImageInfo*
loadImage(Widget w, String url)
{
XmImageInfo *image = NULL;
char filename[256];
char temp_file[256];
char link[256];
struct stat statbuf;
char cmd_line[256];
msg_data *m_data;

  XtVaGetValues(w, XmNuserData, &m_data, NULL);

  /* assume the image is in the detach directory */
  strcpy(temp_file, get_detach_dir());
  strcat(temp_file,"/");
  strcat(temp_file, basename(url));
  /* if it is in the detach directory get it from there */
  if (stat(temp_file,&statbuf) == 0)
    strcpy(filename,temp_file);
  /* else try to get it from the web */
  else
    {
      strcpy(link,html_get_base_page(m_data));
      if (link[strlen(link)-1] == '/')
        link[strlen(link)-1] = '\0';
      if (url[0] != '/')
        strcat(link,"/");
      strcat(link, url);
      sprintf(cmd_line,"webhead -o %s '%s' ",temp_file,link);
#ifdef HTML_DEBUG
      fprintf(stderr,"Executing %s\n",cmd_line);
#endif
      system(cmd_line);
      strcpy(filename,temp_file);
    }
  /* get the image */
  image = XmHTMLImageDefaultProc(w, filename, NULL, 0);
  return(image);
}

/*
  This function detaches the images and html that belong to the message.
*/
int html_detach_images(frame,w,message)
ZmFrame frame;
Widget w;
msg_data *message;
{
    int x;
    int i;
    char buf[256], name[256];
    Attach *attachments, *a;
    AttachInfo *ai;
    char *the_type;
    ZmFrame attachFrame = FrameGetData(w);
    struct stat statbuf;
    char temp_file[256];
 
/*
  Get a pointer to the attachments.
*/
    attachments = message->this_msg.m_attach;
    a = attachments;
    strcpy(name,"");
/*
  If we have no attachments then return.
*/
    if (!a) {
        return(0);
    }
/*
  If a compose frame then adjust attachment pointer.
*/
    if (FrameGetType(frame) != FrameCompose)
        a = (Attach *)a->a_link.l_next;
    x = 0;
    i = 0;
/*
  Loop through attachments detaching images and html.
*/
    do  {
      x++;
      the_type = attach_data_type(a) ? attach_data_type(a) :
              a->content_type ? a->content_type : "unknown";
/*
  If the attachment is not an image or html then ignore it.
*/
      if ((ci_strncmp(the_type,"image/",strlen("image/")) == 0) ||
          (ci_strncmp(the_type,"text/x-html",strlen("text/x-html")) == 0) ||
          (ci_strncmp(the_type,"text/html",strlen("text/html")) == 0))
        {
          if (a->content_name)
              (void) strcpy(name, basename(a->content_name));
          else if (a->a_name)
              (void) strcpy(name, basename(a->a_name));
          else
              (void) sprintf(name, catgets( catalog, CAT_MOTIF, 313,
                             "Attach.%d" ), x);
/*
  See if the attachment exist in the detach directory.
*/
          strcpy(temp_file, get_detach_dir());
          strcat(temp_file,"/");
          strcat(temp_file, name);
/*
  If the attachment does not exist in the directory then detach it.
*/
          if (stat(temp_file,&statbuf) != 0)
            {
              i++;
              sprintf(buf, "builtin detach -part %d -T -O ", x);
              do_cmd_line(w,buf);
            }
        }
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
/*
  Return the number of images and html detached.
*/
  return(i);
}

/*
  This function initializes the HTML widget.
*/
void html_initialize(m_data)
msg_data *m_data;
{
int i;
  m_data->html_w = 0;
  m_data->html_string = NULL;
  m_data->base_page = NULL;
  html_initialize_history(m_data);
}

/*
  This function creates the HTML widget.
*/
int html_create(frame,m_data)
ZmFrame frame;
msg_data *m_data;
{
Dimension hgt , wth;
Widget w;
/*
  Make the size of the HTML widget the same size as the text area.
*/
  w = XtParent(XtParent(m_data->text_w));
  XtVaGetValues(w , XmNheight, &hgt, XmNwidth, &wth, NULL); 
/*
  Create the HTML viewing widget.
*/
  m_data->html_w = XtVaCreateWidget(
      "HView_widget",
      xmHTMLWidgetClass,   w,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_FORM,
      XmNrightAttachment,  XmATTACH_FORM,
      XmNwidth,            wth,
      XmNheight,           hgt,
      XmNuserData,         m_data,
      NULL);

  XtAddCallback(m_data->html_w, XmNactivateCallback,
                (XtCallbackProc)anchorCB, NULL);
  XtVaSetValues(m_data->html_w,
                XmNimageProc, loadImage,
                NULL);
}

/*
  This function gets the HTML in the text widget into a string.
*/
char *html_get_string(m_data)
msg_data *m_data;
{
int html_size;
int offset , end_offset;
char *html_msg, *html_string, *the_html;
XmTextPosition pos, end_pos;

  the_html = NULL;
/*
  Get the HTML string out of the text widget if we fail then return.
*/
  html_msg = XmTextGetString(m_data->text_w);
  if (html_msg == NULL)
    return(NULL);
/*
  Find the beginning of the HTML
*/
  if (XmTextFindString(m_data->text_w,0,"<html>",XmTEXT_FORWARD,&pos) ||
      XmTextFindString(m_data->text_w,0,"<HTML>",XmTEXT_FORWARD,&pos))
    {
/*
  Find the end of the HTML
*/
      if (XmTextFindString(m_data->text_w,0,"</html>",XmTEXT_FORWARD,&end_pos) ||
          XmTextFindString(m_data->text_w,0,"</HTML>",XmTEXT_FORWARD,&end_pos))
        {
/*
  Set pointers to the beginning and end and get the size of the HTML string.
*/
          offset = pos;
          end_offset = end_pos;
          html_size = end_offset - offset + 7;
/*
  Get the HTML string.
*/
          the_html = malloc(html_size+1);
          if (the_html != NULL)
            {
              html_string = html_msg + offset;
              strncpy(the_html,html_string,html_size);
              the_html[html_size] = '\0';
            }
        }
    }
/*
  Free the text string.
*/
  XtFree(html_msg);
/*
  Return pointer to the html.
*/
  return(the_html);
}

/*
  This function displays what is in the HTML widget.
*/
void html_display(m_data)
msg_data *m_data;
{
/*
  If it looks like HTML
*/
  if (strstr(m_data->html_string,"<html>") ||
      strstr(m_data->html_string,"<HTML>"))
    {
      if (strstr(m_data->html_string,"</html>") ||
          strstr(m_data->html_string,"</HTML>"))
        {

/* 
  Display it in the widget 
*/
          XmHTMLTextSetString(m_data->html_w, m_data->html_string);
        }
    }
  else
    fprintf(stderr,"Document is not HTML\n");
  free(m_data->html_string);
  m_data->html_string = NULL;
}

/* 
  This function sets the html base page.
*/
void html_set_base_page(m_data,base_page)
msg_data *m_data;
char *base_page;
{
  if (m_data->base_page != NULL)
    free(m_data->base_page);
  m_data->base_page = malloc(strlen(base_page)+1);
  strcpy(m_data->base_page,base_page);
}

/*
  This function destroys the HTML widget.
*/
void html_destroy(m_data)
msg_data *m_data;
{
int i;
  if (m_data->html_w)
    {
      XtDestroyWidget(m_data->html_w);
    }
  m_data->html_w = 0;
}

/*
  This function terminates the HTML widget.
*/
void html_terminate(m_data)
msg_data *m_data;
{
int i;
  m_data->html_w = 0;
  html_terminate_history(m_data);
}

/*
  This function returns 1 if the message header indicates HTML content.
*/
int content_is_html(msg_no)
int msg_no;
{
char *p;
char *header_field();

  p = header_field(msg_no,"Content-Type");
  if (p != NULL)
    {
      if (
          (ci_strncmp(p,"text/x-html",strlen("text/x-html")) == 0) ||
          (ci_strncmp(p,"multipart/mixed",strlen("multipart/mixed")) == 0) ||
          (ci_strncmp(p,"text/html",strlen("text/html")) == 0)
        )
        return(1);
    }
  return(0);
}

/*
Read the entire file into memory. 

Input:	filename - the file to read
Output:	*size - the size of buffer
Return:	malloced buffer where file was red or
	NULL - error
*/
static char *ReadTheFile(filename,size)
char *filename;
int *size;
{
	int fd, fsize;
	struct stat stat_buf;
	char * file_image ;
	fd = open(filename,O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr ,"Can't open '%s': %s\n", filename, strerror(errno));
		return NULL;
	}
	fstat(fd, &stat_buf);
	if ((stat_buf.st_mode  & S_IFMT)  == S_IFDIR)
	{
		fprintf(stderr ,"'%s' is a directory\n", filename);
		return NULL;
	}
	if ((stat_buf.st_mode  & S_IFMT)  != S_IFREG )
	{
		fprintf(stderr ,"'%s' is not a regular file\n", filename);
		return NULL;
	}
	fsize = stat_buf.st_size;

	file_image = XtMalloc(fsize + 1);
	if (!file_image )
	{
		fprintf(stderr ,"Malloc error \n");
		return NULL;
	}
	/* now read the entire file in one swoop */
	if (fsize != read(fd, file_image, fsize))
	{
		fprintf(stderr ,"Can't read '%s': %s\n", filename, strerror(errno));
		close(fd);
		return NULL;
	}
	file_image[fsize] = 0;
	if (size)
		*size = fsize;
	return file_image ;
}

void html_link_to_file(m_data,link,loc)
msg_data *m_data;
char *link;
char *loc;
{
        char temp_file[256], cmd_line[256];
        int temp_file_size;
        struct stat statbuf;

        /* if it looks like a file name */
        if (strlen(basename(link)) > 0)
          {
            /* assume the html is in the detach directory */
            strcpy(temp_file, get_detach_dir());
            strcat(temp_file,"/");
            strcat(temp_file, basename(link));
            /* if it is an attachment then read it and display it */
            if (stat(temp_file,&statbuf) == 0)
              {
#ifdef HTML_DEBUG
                fprintf(stderr,"Read attachment\n");
#endif
	        /* read the HTML string into the html string and display it */
                timeout_cursors(True);
                if ((m_data->html_string=ReadTheFile(temp_file,&temp_file_size)) != NULL)
                  {
                    html_set_base_page(m_data,get_detach_dir());
                    html_display(m_data);
                    if(loc)
                      XmHTMLAnchorScrollToName(m_data->html_w, loc);
                    else
                      XmHTMLTextScrollToLine(m_data->html_w, 0);
                  }
                timeout_cursors(False);
                return;
              }
          }
        /* otherwise it is not an attachment so it could be on the web */
        /* make up a name for the temporary file */
        strcpy(temp_file, get_detach_dir());
        strcat(temp_file,"/");
        strcat(temp_file,"temp_url" );
        /* get rid of any temporary file with the same name */
        unlink(temp_file);
        sprintf(cmd_line,"webhead -o %s '%s' ",temp_file,link);
#ifdef HTML_DEBUG
        fprintf(stderr,"Executing %s\n",cmd_line);
#endif
        timeout_cursors(True);
        system(cmd_line);
        timeout_cursors(False);
        /* read the HTML string into the html string and display it */
        if ((m_data->html_string=ReadTheFile(temp_file,&temp_file_size)) != NULL)
          {
            html_set_base_page(m_data,link);
            html_display(m_data);
            if(loc)
              XmHTMLAnchorScrollToName(m_data->html_w, loc);
            else
              XmHTMLTextScrollToLine(m_data->html_w, 0);
          }
        /* get rid of any temporary file */
        unlink(temp_file);
}
