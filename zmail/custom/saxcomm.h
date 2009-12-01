/////////////////////////////////////////////////////////////////////////////
// 
// Sax Comm Objects include file
// 
// This file contains all C function and constant definitions.
// 
// Copyright © 1993, Sax Software -- All rights reserved.  
//
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" 
    {
#endif
    
#ifndef INT
    #define INT int
#endif  

#define saxPUBLIC FAR PASCAL _export

typedef int (CALLBACK *LPEVENTHANDLER)(HWND, WORD, INT);
typedef void (CALLBACK *LPIDLELOOP)(void);

/////////////////////////////////////////////////////////////////////////////
//
//  Public functions:
///////////////////////////////////////////////////////////////////////////////
// Basic communications
HWND saxPUBLIC CreateCommObject(HWND hParent);
BOOL saxPUBLIC DestroyCommObject(HWND hWnd);
BOOL saxPUBLIC GetBreak(HWND hWnd);
void saxPUBLIC SetBreak(HWND hWnd, BOOL bBreak);
BOOL saxPUBLIC GetCarrier(HWND hWnd);
LONG saxPUBLIC GetCarrierTimeOut(HWND hWnd);
void saxPUBLIC SetCarrierTimeOut(HWND hWnd, LONG lTimeOut);
INT saxPUBLIC GetInBufferCount(HWND hWnd);
INT saxPUBLIC GetInBufferSize(HWND hWnd);
void saxPUBLIC SetInBufferSize(HWND hWnd, INT cbInBufferSize);
INT saxPUBLIC GetHandshaking(HWND hWnd);
void saxPUBLIC SetHandshaking(HWND hWnd, INT nHandshaking);
LONG saxPUBLIC GetInterval(HWND hWnd);
void saxPUBLIC SetInterval(HWND hWnd, LONG lInterval);
INT saxPUBLIC GetOutBufferCount(HWND hWnd);
INT saxPUBLIC GetOutBufferSize(HWND hWnd);
void saxPUBLIC SetOutBufferSize(HWND hWnd, INT cbOutBufferSize);
INT saxPUBLIC GetParityReplace(HWND hWnd);
void saxPUBLIC SetParityReplace(HWND hWnd, INT nParityReplace);
INT saxPUBLIC GetPort(HWND hWnd);
void saxPUBLIC SetPort(HWND hWnd, INT nPort);
BOOL saxPUBLIC GetPortOpen(HWND hWnd);
BOOL saxPUBLIC SetPortOpen(HWND hWnd, BOOL bOpen);
INT saxPUBLIC GetRThreshold(HWND hWnd);
void saxPUBLIC SetRThreshold(HWND hWnd, INT nThreshold);
INT saxPUBLIC GetSettings(HWND hWnd, LPSTR lpSettings);
void saxPUBLIC SetSettings(HWND hWnd, LPCSTR lpSettings);
INT saxPUBLIC GetSThreshold(HWND hWnd);
void saxPUBLIC SetSThreshold(HWND hWnd, INT nThreshold);
BOOL saxPUBLIC GetDSR(HWND hWnd);
BOOL saxPUBLIC GetCTS(HWND hWnd);
void saxPUBLIC SetRTS(HWND hWnd, BOOL bRTS);
void saxPUBLIC SetDTR(HWND hWnd, BOOL bEnable);
BOOL saxPUBLIC GetDTR(HWND hWnd);
BOOL saxPUBLIC GetRTS(HWND hWnd);
void saxPUBLIC SetDSRTimeOut(HWND hWnd, LONG lTimeOut);
LONG saxPUBLIC GetDSRTimeOut(HWND hWnd);
void saxPUBLIC SetCTSTimeOut(HWND hWnd, LONG lTimeOut);
LONG saxPUBLIC GetCTSTimeOut(HWND hWnd);
void saxPUBLIC SetDriverInputTimeOut(HWND hWnd, LONG lTimeOut);
LONG saxPUBLIC GetDriverInputTimeOut(HWND hWnd);
INT saxPUBLIC GetInput(HWND hWnd, LPSTR lpBuffer, INT cbBuffer);
void saxPUBLIC Output(HWND hWnd, LPCSTR lpBuffer, INT cbBuffer);
void saxPUBLIC ClearOutputBuffer(HWND hWnd);
void saxPUBLIC ClearInputBuffer(HWND hWnd);
INT saxPUBLIC GetNotification(HWND hWnd);
INT saxPUBLIC GetLineInput(HWND hWnd, LPSTR lpBuffer, INT cbBuffer);
BOOL saxPUBLIC GetNullDiscard(HWND hWnd);
void saxPUBLIC SetNullDiscard(HWND hWnd, BOOL bNullDiscard);
void saxPUBLIC SetNotification(HWND hWnd, INT nNotify);
void saxPUBLIC SetEventHandler(HWND hWnd, LPEVENTHANDLER);
void saxPUBLIC SetIdleLoop(HWND hWnd, LPIDLELOOP);
int saxPUBLIC DiscardUntilLookUp(HWND hWnd, LPSTR lpLookUp);
int saxPUBLIC InputUntilLookUp(HWND hWnd, LPSTR lpLookUp, LPSTR lpInput, int cbMax);
LONG saxPUBLIC GetLookUpTimeOut(HWND hWnd);
void saxPUBLIC SetLookUpTimeOut(HWND hWnd, LONG lTimeOut);
void saxPUBLIC SetDialogParent(HWND hWnd, HWND hParent);
HWND saxPUBLIC GetDialogParent(HWND hWnd);
void saxPUBLIC ComPause(HWND hWnd, LONG lMiliSeconds);  
BOOL saxPUBLIC FlushInputBuffer(HWND hObject, LONG lTimeOut);
BOOL saxPUBLIC FlushOutputBuffer(HWND hObject, LONG lTimeOut);
void saxPUBLIC DumpCommStatus(HWND hWnd);
BOOL saxPUBLIC IsCommDebugVersion(void);
void saxPUBLIC SetCommDebugOptions(BOOL bEvents, BOOL bFunctionCalls, 
    BOOL bSerialInput, BOOL bSerialOutput, BOOL bWarnings, 
    LPCSTR szFilename, BOOL bOnTop, int nOutput);
void saxPUBLIC DumpCommStatus(HWND hWnd);
// Modem functions
void saxPUBLIC OutputModem(HWND hWnd, LPCSTR lpModemString);
#define LoadModem LoadModemOptions
BOOL saxPUBLIC LoadModemOptions(HWND hWnd, int n);
void saxPUBLIC SetModemOptions(HWND hWnd, LPCSTR lpOptions);
INT saxPUBLIC GetModemOptions(HWND hWnd, LPSTR lpOptions);
BOOL saxPUBLIC WaitForOk(HWND hWnd);
int saxPUBLIC Dial(HWND hWnd, LPCSTR szPhoneNumber); 
BOOL saxPUBLIC InitModem(HWND hWnd);
BOOL saxPUBLIC InitModemForAnswer(HWND hWnd);
BOOL saxPUBLIC HangUp(HWND hWnd);
void saxPUBLIC SetDialMode(HWND hWnd, int nDialMode);
int saxPUBLIC GetDialMode(HWND hWnd);
LONG saxPUBLIC GetDialTimeOut(HWND hWnd);
void saxPUBLIC SetDialTimeOut(HWND hWnd, LONG lDialTimeOut);
BOOL saxPUBLIC FillListWithModemNames(HWND hWnd, HWND hListBox);
BOOL saxPUBLIC CheckModem(HWND hWnd);    
// TerminalEmulation
void saxPUBLIC SetAutoProcess(HWND hWnd, INT nAutoProcess);
INT saxPUBLIC GetAutoProcess(HWND hWnd);
INT saxPUBLIC GetCaptureMode(HWND hWnd);
void saxPUBLIC SetCaptureMode(HWND hWnd, INT nCaptureMode);
void saxPUBLIC SetAutoScroll(HWND hWnd, INT nAutoScroll);
INT saxPUBLIC GetAutoScroll(HWND hWnd);
void saxPUBLIC SetAutoSize(HWND hWnd, INT nAutoSize);
INT saxPUBLIC GetAutoSize(HWND hWnd);
void saxPUBLIC SetBackspace(HWND hWnd, INT nBackspace);
INT saxPUBLIC GetBackspace(HWND hWnd);
BOOL saxPUBLIC SetCaptureFilename(HWND hWnd, LPCSTR lpFilename);
INT saxPUBLIC GetColor(HWND hWnd);
void saxPUBLIC SetColor(HWND hWnd, INT nColor);
INT saxPUBLIC GetColorFilter(HWND hWnd);
void saxPUBLIC SetColorFilter(HWND hWnd, INT nColorFilter);
INT saxPUBLIC GetColumns(HWND hWnd);
void saxPUBLIC SetColumns(HWND hWnd, INT nColumns);
LONG saxPUBLIC GetContents(HWND hWnd, LPSTR lpBuffer, LONG lcbBuffer);
void saxPUBLIC SetContents(HWND hWnd, LPCSTR lpText, INT cbText);
LONG saxPUBLIC GetContentsLength(HWND hWnd);
INT saxPUBLIC GetCursorColumn(HWND hWnd);
void saxPUBLIC SetCursorColumn(HWND hWnd, INT col);
INT saxPUBLIC GetCursorRow(HWND hWnd);
void saxPUBLIC SetCursorRow(HWND hWnd, INT row);
BOOL saxPUBLIC GetDiscardNulls(HWND hWnd);
void saxPUBLIC SetDiscardNulls(HWND hWnd, BOOL bDiscard);
INT saxPUBLIC GetEmulation(HWND hWnd);
void saxPUBLIC SetEmulation(HWND hWnd, INT nEmulation);
INT saxPUBLIC GetRows(HWND hWnd);
void saxPUBLIC SetRows(HWND hWnd, INT nRows);
LONG saxPUBLIC GetScrollContents(HWND hWnd, LPSTR lpBuffer, LONG lcbBuffer);
LONG saxPUBLIC GetScrollContentsLength(HWND hWnd);
INT saxPUBLIC GetScrollRows(HWND hWnd);
void saxPUBLIC SetScrollRows(HWND hWnd, INT nScrollRows);
LONG saxPUBLIC GetSelectionLength(HWND hWnd);
void saxPUBLIC SetSelectionLength(HWND hWnd, INT nLength);
LONG saxPUBLIC GetSelectionStart(HWND hWnd);
void saxPUBLIC SetSelectionStart(HWND hWnd, INT nStart);
LONG saxPUBLIC GetSelectionText(HWND hWnd, LPSTR lpBuffer, LONG lcbBuffer);
BOOL saxPUBLIC GetEcho(HWND hWnd);
void saxPUBLIC SetEcho(HWND hWnd, BOOL bEcho);
void saxPUBLIC TerminalPrintString(HWND hWnd, LPCSTR lpText);
void saxPUBLIC SetInputEcho(HWND hWnd, BOOL bInputEcho);
BOOL saxPUBLIC GetInputEcho(HWND hWnd);
BOOL saxPUBLIC CopySelectionToClipboard(HWND hWnd);
// File transfer
INT saxPUBLIC GetProtocol(HWND hWnd);
void saxPUBLIC SetProtocol(HWND hWnd, INT nProtocol);
INT saxPUBLIC GetTransferDialogType(HWND hWnd);
void saxPUBLIC SetTransferDialogType(HWND hWnd, INT nDialog);
INT saxPUBLIC GetTransferStatus(HWND hWnd);
void saxPUBLIC AbortTransfer(HWND hWnd);
void saxPUBLIC AbortCurrentFile(HWND hWnd);
BOOL saxPUBLIC Upload(HWND hWnd, LPCSTR lpFilename);
BOOL saxPUBLIC Download(HWND hWnd, LPCSTR lpFilename);
int saxPUBLIC GetTransferType(HWND hWnd);
void saxPUBLIC WaitUntilTransferIsFinished(HWND hWnd);
INT saxPUBLIC GetSourceFilename(HWND hWnd, LPSTR lpBuffer, INT cbBuffer);
INT saxPUBLIC GetSourceFilenameLength(HWND hWnd);
INT saxPUBLIC GetDestFilename(HWND hWnd, LPSTR lpBuffer, INT cbBuffer);
INT saxPUBLIC GetDestFilenameLength(HWND hWnd);
void saxPUBLIC SetDestFilename(HWND hWnd, LPCSTR lpFilename);
void saxPUBLIC SetVScrollBar(HWND hWnd, HWND hWndScroll, int nCode);
void saxPUBLIC SetHScrollBar(HWND hWnd, HWND hWndScroll, int nCode);

// String resource ID's:
#define IDS_CAPTION     1
#define IDS_NOLICENSE   2
#define IDS_REQUIRESVB2 3

// Helpful macro's:
#define OFFSETIN(struc,field)     ((USHORT)&(((struc *)0)->field))
#define saxPUBLIC FAR PASCAL _export
//#define saxPUBLICINTERNAL FAR PASCAL
#define saxPUBLICINTERNAL
#define saxINTERNAL static NEAR

// Events that are generated by the DLL for OnComm
#define SCEV_NOEVENT      0     // Internal use
#define SCEV_SEND         1
#define SCEV_RECEIVE      2
#define SCEV_CTS          3
#define SCEV_DSR          4
#define SCEV_CD           5
#define SCEV_RING         6
#define SCEV_EOF          7
#define SCEV_TRANSFER   100
#define SCER_BREAK     1001
#define SCER_CTSTO     1002
#define SCER_DSRTO     1003
#define SCER_FRAME     1004
#define SCER_INTO      1005
#define SCER_OVERRUN   1006
#define SCER_CDTO      1007
#define SCER_RXOVER    1008
#define SCER_RXPARITY  1009
#define SCER_TXFULL    1010
// Errors used by visual basic
#define SCER_INVALIDPROPERTY   380
#define SCER_DEVICEUNAVAILABLE  68
#define SCER_PERMISSIONDENIED   70
#define SCER_OUTOFMEMORY         7
#define SCER_FILENOTFOUND       53
#define SCER_ERRORLOADINGDLL    48

#define EMULATION_NONE  0
#define EMULATION_TTY   1
#define EMULATION_ANSI  2
#define EMULATION_VT52  3
#define EMULATION_VT100 4

#define AUTOSCROLL_NONE       0
#define AUTOSCROLL_VERTICAL   1
#define AUTOSCROLL_HORIZONTAL 2
#define AUTOSCROLL_BOTH       3
#define AUTOSCROLL_KEYBOARD   4

#define AUTOPROCESS_NONE     0
#define AUTOPROCESS_SERIAL   1
#define AUTOPROCESS_KEYBOARD 2
#define AUTOPROCESS_BOTH     3

#define CAPTURE_STANDARD 0
#define CAPTURE_BINARY   1
#define CAPTURE_VISIBLE  2

#define AUTOSIZE_NONE     0
#define AUTOSIZE_TERMINAL 1
#define AUTOSIZE_PARENT   2

#define COLORFILTER_FULL 0 
#define COLORFILTER_BW   1 
#define COLORFILTER_GRAY 2

#define HANDSHAKING_NONE    0
#define HANDSHAKING_XONXOFF 1
#define HANDSHAKING_CTSRTS  2
#define HANDSHAKING_BOTH    3

#define NOTIFY_TIMER  0
#define NOTIFY_DRIVER 1

#define XFER_PROTOCOLMIN 0
#define XFER_PROTOCOLMAX 7

#define XFER_DLGMIN 0
#define XFER_DLGMAX 2

#define PROTOCOL_XMODEMCHECKSUM 0
#define PROTOCOL_XMODEMCRC      1
#define PROTOCOL_XMODEM1K       2
#define PROTOCOL_YMODEM         3
#define PROTOCOL_YMODEMG        4
#define PROTOCOL_ZMODEM         5
#define PROTOCOL_KERMIT         6
#define PROTOCOL_COMPUSERVEB    7

#define XFER_GETSTATUSDIALOGTYPE     1
#define XFER_GETCOMMID               2
#define XFER_GETSTATUSDIALOGPARENT   3
#define XFER_GETTRANSFERPROTOCOL     4
#define XFER_TERMINATED              5
#define XFER_REPORTFILESIZE          6
#define XFER_REPORTBYTESWRITTEN      7
#define XFER_REPORTMESSAGE           8
#define XFER_GETFILENAME             9
#define XFER_TERMINATEDFILE         10
#define XFER_QUERYABORT             11
#define XFER_GETTRANSFERTYPE        12
#define XFER_ABORTED                13
#define XFER_ABORTEDFILE            14

#define TRANSFERSTATUS_FAILURE              -1
#define TRANSFERSTATUS_SUCCES                0
#define TRANSFERSTATUS_WAITFORFILE           1
#define TRANSFERSTATUS_PREPARE               2
#define TRANSFERSTATUS_STARTING              3
#define TRANSFERSTATUS_TRANSFERING           4
#define TRANSFERSTATUS_FILEABORT             5
#define TRANSFERSTATUS_TRANSFERABORTING      6
#define TRANSFERSTATUS_ENDOFFILE             7

#define CAPTUREMODE_STANDARD 0
#define CAPTUREMODE_BINARY   1
#define CAPTUREMODE_VISIBLE  2

#define BACKSPACE_DESTRUCTIVE    0
#define BACKSPACE_NONDESTRUCTIVE 1

#define XFERDIALOG_HIDDEN   0
#define XFERDIALOG_MODAL    1
#define XFERDIALOG_MODELESS 2


#define TRANSFER_NOTHING  0
#define TRANSFER_UPLOAD   1
#define TRANSFER_DOWNLOAD 2

#define DIALMODE_PULSE 0
#define DIALMODE_TONE  1

#define DIAL_CONNECT    0
#define DIAL_NOCARRIER  1
#define DIAL_NODIALTONE 2
#define DIAL_BUSY       3 
#define DIAL_TIMEOUT    4

#define _MAXSETTINGSLEN 15

#ifdef __cplusplus
    }
#endif
