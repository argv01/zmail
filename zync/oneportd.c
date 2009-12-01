/*
 * oneportd.c -- fakes inetd for one port
 * S. Spencer Sun, 08/02/94
 *
 * As noted below, this does minimal error-checking and will hang around
 * forever if you leave it alone, so be sure to kill it when you're done
 * with it.  You can pass it two command line flags: -p <port> tells it
 * to bind to that port number (default 15232, the zpop port) and -r tells
 * it to pass the SOL_REUSEADDR option to setsockopt() prior to bind().
 *
 * The first argument that is not -p or -r is taken as the pathname to
 * a program to run, and all remaining arguments are passed to that
 * program as arguments via execv().
 *
 * So, for example:
 * % ./oneportd -p 20000 /path/to/zync/zyncd -c /path/to/zync/zync.config &
 * [1] 1234
 * % telnet localhost 20000
 * Trying 127.0.0.1...
 * Connected to localhost.
 * Escape character is '^]'.
 * +OK Welcome to Z-Mail ZPOP server  v.79dev
 */

#include "osconfig.h"
#include <stdio.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include "zcsyssel.h" /* for sys/select.h where appropriate */

#include <netinet/in.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

static char oneport_rcsid[] = "$Id: oneportd.c,v 1.4 1995/08/24 19:58:08 spencer Exp $";

int
server_socket(port, reuse)
  unsigned short port;
  int reuse;
{
  int s;
  struct sockaddr_in sa;

  if (-1 == (s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
  {
    perror("socket");
    return -1;
  }

  memset((void *)&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port);
  if (reuse)
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)reuse, sizeof(reuse));
  if (-1 == bind(s, (char *)&sa, sizeof(struct sockaddr_in)))
  {
    perror("bind");
    return -1;
  }
  return s;
}

int
main(argc, argv)
  int argc;
  char **argv;
{
  unsigned short port = 15232;
  int reuse = 0, ch, s, new;
  struct sockaddr_in sa;
  extern int optind;
  extern char *optarg;
  FILE *fp;
  fd_set rfds;
  char buf[1025];

  fprintf(stderr, "Warning, this program does minimal error-checking and will hang around forever\n");
  fprintf(stderr, "if you leave it alone.  Don't forget to kill it when you're done with it.\n");

  while ((ch = getopt(argc, argv, "p:r")) != EOF)
  {
    if ('p' == ch)
      port = (unsigned short)atoi(optarg);
    else reuse = 1;
  }
  argv += optind;

  s = server_socket(port, reuse);
  if (listen(s, 1) < 0)
  {
    perror("listen");
    return 1;
  }
  for (;;)
  {
    ch = sizeof(sa);
    new = accept(s, (char *)&sa, &ch);
    if (s < 0)
    {
      perror("accept");
      close(s);
      return 1;
    }
    if (fork() != 0)
      continue;
    else
    {
      close(0); dup(new); close(1); dup(new); close(2); dup(new);
      execv(argv[0], argv);
    }
  }
  return 0;
}
