/*
 * zcomm.c: serial communications routines for talking to a POP3-protocol 
 *          post-office server. WIN16 specific.
 *
 * Wilbur Wong
 * January 10, 1995
 */

#include <afxwin.h>
#include "scomfc.h"
#include "zcomm.h"
#include "zcom_dlg.h"
#include "wingui/frame.h"

//
// global Comm Object pointer
//
void* pCommObj = NULL;

//
// Create a Comm Object. Initialize for terminal emulation.
// A CWnd (MFC) window will pop up allowing the user to interact
// with his serial port and display data sent and received.
//
void*
create_comm( void )
{
//	return NULL;	/* used to disable serial connectivity for test purposes */

	CCommXfrTrm* pComm = NULL;
	CWnd* pWnd = NULL;		// not tied to any parent window
//	CWnd* pWnd = AfxGetApp()->m_pMainWnd;

	// Size the terminal window accordingly.
	CRect rc;
	rc.top    = 20;
	rc.left   = 20;
	rc.bottom = 220;
	rc.right  = 320;
	
	// Instantiate a serial communications object.
	// You must explicitly delete this object when done.
	pComm = new CCommXfrTrm();
	
	// Store Comm Object pointer in global variable */
	pCommObj = (void*)pComm;
	
	// Create a terminal emulator.
	pComm->Create("ZMail Terminal", WS_OVERLAPPEDWINDOW | WS_HSCROLL 
			| WS_VSCROLL | WS_VISIBLE, rc, pWnd, 0);
	pComm->SetParent(pWnd);
    pComm->SetAutoSize(AUTOSIZE_TERMINAL);
    pComm->ShowWindow(SW_SHOWNORMAL);
	pComm->SetEmulation(EMULATION_ANSI);	// can disable altogether	
    pComm->SetScrollRows(25);

	return (void*)pComm;
}
	 
//
// Display a dialog box requesting port and modem settings
// to be stored in Comm Object.
//
void 
configure_comm( void* pComm )
{
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return;
	}
			
    CSettingsDialog SettingsDlg((CCommXfrTrm*)pComm);
    SettingsDlg.DoModal();
}

//
// Open a comm port and login to a unix host.
//
void 
connect_comm( void* pComm, LPSTR user, LPSTR password )
{
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return;
	}
			
	((CCommXfrTrm*)pComm)->SetPortOpen(TRUE);
	if (((CCommXfrTrm*)pComm)->GetPortOpen());
	{
		((CCommXfrTrm*)pComm)->SetInputEcho(TRUE);

		((CCommXfrTrm*)pComm)->Output(" ATDT");
		((CCommXfrTrm*)pComm)->Output("897-0752");	// hardcoded for now.
		((CCommXfrTrm*)pComm)->Output("\r");
		((CCommXfrTrm*)pComm)->FlushOutputBuffer(100000);

		((CCommXfrTrm*)pComm)->SetLookUpTimeOut(30000);
		
		((CCommXfrTrm*)pComm)->DiscardUntilLookUp("login:");
		((CCommXfrTrm*)pComm)->Output(user);
		((CCommXfrTrm*)pComm)->Output("\r");
		((CCommXfrTrm*)pComm)->FlushOutputBuffer(100000);

		((CCommXfrTrm*)pComm)->DiscardUntilLookUp("Password:");
		((CCommXfrTrm*)pComm)->Output(password);
		((CCommXfrTrm*)pComm)->Output("\r");
		((CCommXfrTrm*)pComm)->FlushOutputBuffer(100000);

		((CCommXfrTrm*)pComm)->DiscardUntilLookUp("zander 1%");
	}
}

//
// Telnet into a server system and service (port)
//
void
open_comm( void* pComm, LPSTR host, LPSTR port )
{
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return;
	}
	if (((CCommXfrTrm*)pComm)->GetPortOpen());
	{
		((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_NONE);
		((CCommXfrTrm*)pComm)->SetInputEcho(TRUE);

		((CCommXfrTrm*)pComm)->Output("telnet ");
		((CCommXfrTrm*)pComm)->Output(host);
		((CCommXfrTrm*)pComm)->Output(" ");
		((CCommXfrTrm*)pComm)->Output(port);
		((CCommXfrTrm*)pComm)->Output("\r");
		((CCommXfrTrm*)pComm)->FlushOutputBuffer(30000);
		((CCommXfrTrm*)pComm)->Pause(5000);
	
		((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_BOTH);

//		((CCommXfrTrm*)pComm)->DiscardUntilLookUp("+OK");
		
//		((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_KEYBOARD);
//		CString lineBuf(((CCommXfrTrm*)pComm)->LineInput());
		 
//		((CCommXfrTrm*)pComm)->Output("retr 600");
//		((CCommXfrTrm*)pComm)->Output("\r");
//		((CCommXfrTrm*)pComm)->FlushOutputBuffer(100000);
	}
}

//
// Close the comm port and free the Comm Object.
//
void 
disconnect_comm( void* pComm )
{
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return;
	}
			
	// Cleanup and delete comm object.
	((CCommXfrTrm*)pComm)->SetPortOpen(FALSE);
	((CCommXfrTrm*)pComm)->DestroyWindow();
	delete ((CCommXfrTrm*)pComm);
	pCommObj = NULL;
}

//
// Read characters from the open comm port
//
int
read_comm( void* pComm, LPSTR lpBuffer, int nLen )
{
    int ret;
    
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return -1;
	}

	((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_NONE);
	((CCommXfrTrm*)pComm)->SetInputEcho(TRUE);
	
	ret = ((CCommXfrTrm*)pComm)->Input( lpBuffer, nLen+1 );
	((CCommXfrTrm*)pComm)->Pause(500);
	
	((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_BOTH);

	return ret;
}			
	
//
// Write characters to the open comm port
//
int
write_comm( void* pComm, LPSTR lpBuffer, int nLen )
{
    if (!pComm)
	{
		AfxMessageBox("Invalid Comm Object.");
		return -1;
	}
	
	((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_NONE);
	((CCommXfrTrm*)pComm)->SetInputEcho(TRUE);
	
	((CCommXfrTrm*)pComm)->Output( lpBuffer, nLen );
	
	((CCommXfrTrm*)pComm)->FlushOutputBuffer(3000);
	((CCommXfrTrm*)pComm)->Pause(500);
	((CCommXfrTrm*)pComm)->SetAutoProcess(AUTOPROCESS_BOTH);
	
	return nLen;
}	 

//
// Retrieve a Sax Comm Object pointer previously created by
// create_comm(). The pointer is assumed to be stored in the
// global variable pCommObj (can be changed for final release).
//
void *
get_comm( void )
{
	return pCommObj;
}

