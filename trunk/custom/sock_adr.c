#include "sock_adr.h"

void
sock_AddrToStr(buf, addr)
char *buf;
CVPTR addr;
{
    const char *s = (const char *) addr;
    sprintf(buf, "%d.%d.%d.%d",
	(int)(unsigned char) s[0],
	(int)(unsigned char) s[1],
	(int)(unsigned char) s[2],
	(int)(unsigned char) s[3]);
}

void
sock_AddrPortToStr(buf, s, port)
char *buf, *s;
sock_Port port;
{
    sprintf(buf, "%d.%d.%d.%d:%d",
	(int)(unsigned char) s[0],
	(int)(unsigned char) s[1],
	(int)(unsigned char) s[2],
	(int)(unsigned char) s[3], port);
}
