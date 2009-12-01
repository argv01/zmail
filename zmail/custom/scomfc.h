#ifndef _SCOMFC_H_
#define _SCOMFC_H_


class CComm : public CWnd
{
    DECLARE_DYNAMIC(CComm)

// Constructors
public:
    CComm();
    BOOL Create();
    BOOL Create(LPCSTR lpszCaption, DWORD dwStyle,  
                const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
public:
    void ClearInputBuffer();
    void ClearOutputBuffer();
    void FlushInputBuffer(LONG lTimeOut);
    void FlushOutputBuffer(LONG lTimeOut);
    BOOL GetBreak();
    BOOL GetCarrier();
    LONG GetCarrierTimeOut();
    BOOL GetCTS();
    LONG GetCTSTimeOut();
    BOOL GetDiscardNulls();
    LONG GetDriverInputTimeOut();
    BOOL GetDSR();
    LONG GetDSRTimeOut();
    BOOL GetDTR();
    int GetInBufferCount();
    int GetInBufferSize();
    int GetHandshaking();
    LONG Getinterval();
    int GetOutBufferCount();
    int GetOutBufferSize();
    int GetNotification();
    int GetParityReplace();
    int GetPort();
    BOOL GetPortOpen();
    int GetRThreshold();
    BOOL GetRTS();
    int GetSettings(LPSTR lpSettings);
    CString GetSettings();
    int GetSThreshold();
    void SetBreak(BOOL bBreak); 
    void SetCarrierTimeOut(LONG lTimeOut);  
    void SetCTSTimeOut(LONG lTimeOut);
    void SetDiscardNulls(BOOL bDiscard);
    void SetDriverInputTimeOut(LONG lTimeOut);
    void SetDSRTimeOut(LONG lTimeOut);
    void SetDTR(BOOL bEnable);
    void SetInBufferSize(int cbInBufferSize);
    void SetHandshaking(int nHandshaking);  
    void Setinterval(LONG linterval);
    void SetNotification(int nNotify);
    void SetOutBufferSize(int cbOutBufferSize);
    void SetParityReplace(int nParityReplace);
    void SetPort(int nPort);    
    BOOL SetPortOpen(BOOL bOpen);
    void SetRThreshold(int nThreshold);
    void SetRTS(BOOL bRTS);
    void SetSettings(LPCSTR lpSettings);
    void SetSThreshold(int nThreshold);
    int InputUntilLookUp(LPSTR lpLookUp, LPSTR lpInput, int cbMax);
    int InputUntilLookUp(LPSTR lpLookUp, CString &Input, int cbMax = 1024);
    int DiscardUntilLookUp(LPSTR lpLookUp);
    LONG GetLookUpTimeOut();
    void SetLookUpTimeOut(LONG lTimeOut);
    void WaitUntilTransferIsFinished();
    void Pause(LONG lMiliSeconds);
    void SetDialogParent(CWnd *Parent);

	void OutputModem(LPCSTR lpModemString);
	BOOL LoadModemOptions(int n);
	BOOL WaitForOk();
	int Dial(const char *szPhoneNumber); 
	BOOL CheckModem();
	BOOL InitModem();
	BOOL InitModemForAnswer();
	BOOL HangUp();
	void SetDialMode(int nDialMode);
	int GetDialMode();
	LONG GetDialTimeOut();
	void SetDialTimeOut(LONG lDialTimeOut);
	BOOL FillListWithModemNames(HWND hListBox);    
        
// Operations       
    int Input(LPSTR lpBuffer, int cbBuffer);
    CString Input();
    void Output(LPCSTR lpBuffer, int cbBuffer);
    void Output(LPCSTR lpBuffer);       
    int LineInput(LPSTR lpBuffer, int cbBuffer);
    CString LineInput();
    
    BOOL GetNullDiscard();
    void SetNullDiscard(BOOL bNullDiscard);
    void TerminalPrintString(LPCSTR lpText);
    virtual void ProcessEvent(int nEvent);
    
    virtual void Initialize();

    virtual void OnCommError() {}
    virtual void OnCommEvent() {}
    
    virtual void OnReceive() {}
    virtual void OnSend() {}
    virtual void OnChangeCTS() {}
    virtual void OnChangeDSR() {}
    virtual void OnChangeCarrier() {}
    virtual void OnChangeRing() {}
    virtual void OnEOF() {}
    
    virtual void OnBreakError() {}
    virtual void OnCTSTimeOut() {}
    virtual void OnDSRTimeOut() {}
    virtual void OnFrameError() {}
    virtual void OnInputTimeOut() {}
    virtual void OnOverrun() {}
    virtual void OnCarrierTimeOut() {}
    virtual void OnReceiveOverflow() {}
    virtual void OnParityError() {}
    virtual void OnTransmitOverflow() {}
// Implementation
public:
    virtual ~CComm();
protected:
    virtual WNDPROC* GetSuperWndProcAddr(); 
};

class CCommXfr : public CComm
{
    DECLARE_DYNAMIC(CCommXfr)

// Constructors
public:
    CCommXfr(); 
// Operations   
    void AbortTransfer(); 
    void AbortCurrentFile();
    BOOL Download(LPCSTR lpFilename);
    int GetProtocol();
    int GetTransferDialogType();
    int GetTransferFilename(LPSTR lpBuffer, int cbBuffer);
    CString GetTransferFilename();
    int GetTransferFilenameLength();
    int GetTransferStatus();
    void SetProtocol(int nProtocol);
    void SetTransferDialogType(int nDialog);
    BOOL Upload(LPCSTR lpFilename);
    CString GetDestFilename();
    void SetDestFilename(LPCSTR lpFilename);
    CString GetSourceFilename();
    int GetTransferType();

// Overridables:
public: 
    virtual void ProcessEvent(int nEvent);
    
    virtual void OnTransferAborted() {}
    virtual void OnTransferFinished() {}
    virtual void OnTransferWaiting() {}
    virtual void OnTransferAborting() {}
    virtual void OnFilePrepare() {}
    virtual void OnFileStart() {}
    virtual void OnFileProgress () {}
    virtual void OnFileAborted() {}
    virtual void OnFileFinished() {}
};

class CCommXfrTrm : public CCommXfr
{
    DECLARE_DYNAMIC(CCommXfrTrm)

// Constructors
public:
    CCommXfrTrm();
    
// Operations   
    int GetAutoProcess();
    int GetAutoScroll();
    int GetAutoSize();
    int GetBackspace();
    int GetCaptureMode();
    CString GetCaptureFilename();
    int GetColor();
    int GetColorFilter();
    int GetColumns();
    LONG GetContents(LPSTR lpBuffer, LONG lcbBuffer);
    CString GetContents();
    LONG GetContentsLength();
    int GetCursorColumn();
    int GetCursorRow();
    BOOL GetEcho();
    BOOL GetInputEcho();
    int GetEmulation();
    int GetRows();
    LONG GetScrollContents(LPSTR lpBuffer, LONG lcbBuffer);
    CString GetScrollContents();
    LONG GetScrollContentsLength();
    int GetScrollRows();
    LONG GetSelectionLength();
    LONG GetSelectionStart();
    void SetAutoProcess(int nAutoProcess);
    void SetCaptureMode(int nCaptureMode);  
    void SetAutoScroll(int nAutoScroll);
    void SetAutoSize(int nAutoSize);
    void SetBackspace(int nBackspace);
    BOOL SetCaptureFilename(LPSTR lpFilename);
    void SetColor(int nColor);
    void SetColorFilter(int nColorFilter);
    void SetColumns(int nColumns);
    void SetContents(LPSTR lpText, int cbText);
    void SetContents(LPSTR lpText);
    void SetCursorColumn(int col);
    void SetCursorRow(int row);
    void SetEcho(BOOL bEcho);
    void SetInputEcho(BOOL bEcho);
    void SetEmulation(int nEmulation);
    void SetRows(int nRows);
    void SetScrollRows(int nScrollRows);
    void SetSelectionLength(int nLength);
    void SetSelectionStart(int nStart);
    LONG GetSelectionText(LPSTR lpBuffer, LONG lcbBuffer);
	void SetVScroll(CWnd *pScroll, int nCode);
	void SetHScroll(CWnd *pScroll, int nCode);
    
// overridables:
    virtual void Initialize();

};

#define PROTOCOL_XMODEMCHECKSUM 0
#define PROTOCOL_XMODEMCRC      1
#define PROTOCOL_XMODEM1K       2
#define PROTOCOL_YMODEM         3
#define PROTOCOL_YMODEMG        4
#define PROTOCOL_ZMODEM         5
#define PROTOCOL_KERMIT         6
#define PROTOCOL_COMPUSERVEB    7

#define CAPTUREMODE_STANDARD 0
#define CAPTUREMODE_BINARY   1
#define CAPTUREMODE_VISIBLE  2

#define BACKSPACE_DESTRUCTIVE 0
#define BACKSPACE_NONDESTRUCTIVE 1

#define EMULATION_NONE  0
#define EMULATION_TTY   1
#define EMULATION_ANSI  2
#define EMULATION_VT52  3
#define EMULATION_VT100 4

#define AUTOSCROLL_NONE 0
#define AUTOSCROLL_VERTICAL 1
#define AUTOSCROLL_HORIZONTAL 2
#define AUTOSCROLL_BOTH 3
#define AUTOSCROLL_KEYBOARD 4

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

#define XFERDIALOG_HIDDEN   0
#define XFERDIALOG_MODAL    1 
#define XFERDIALOG_MODELESS 2

class CCommView : public CView
{
protected: // create from serialization only
    CCommView();
    DECLARE_DYNCREATE(CCommView)

// Attributes
public:
    CCommXfrTrm *m_pComm;
    
// Operations
public:
    
// Implementation
public:
    virtual void OnDraw( CDC* );
    virtual ~CCommView();   
    virtual void CCommView::OnInitialUpdate();
    
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;  
#endif

// Message map functions
protected:
    //{{AFX_MSG(CCommView)   
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


#endif	/* _SCOMFC_H_ */
