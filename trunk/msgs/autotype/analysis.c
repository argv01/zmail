#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "analysis.h"
#include "osconfig.h"
#include "zmail.h"		/* for test_binary() */


const char *
autotype_via_analysis(filename)
    const char *filename;
{
    static const char * const BinaryType = "application/octet-stream";
    static const char * const  AsciiType = "text/plain";
    
    return test_binary(filename) ? BinaryType : AsciiType;
}
