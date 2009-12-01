extern "C" {
#include <spool.h>
#include "catalog.h"
#include "error.h"
#include "impress.h"
#include "setopts.h"
#include "vars.h"
}
#include <dynstr.h>


void
printer_init()
{
  SLPrinterStruct *printers;
  int numPrinters;
  
  if (SLGetPrinterList(&printers, &numPrinters))
    error(SysErrWarning, catgets(catalog, CAT_SHELL, 907, "Could not find printers: %s"), SLErrorString(SLerrno));
  else
    {
      ZDynstr names;
      
      for (unsigned scan = 0; scan < numPrinters; scan++)
	{
	  if (scan) names += ", ";
	  names += printers[scan].local_name;
	}      

      set_var(VarPrinter, "=", names);
    }
}
