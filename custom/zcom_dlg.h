// zcom_dlg.h : header file
//

#ifndef _ZCOM_DLG_H_
#define _ZCOM_DLG_H_

#include "wingui\zdlgs.h"

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog dialog

class CSettingsDialog : public CDialog
{
// Construction
public:
	CSettingsDialog(CCommXfr *pComm, CWnd* pParent = NULL);

// Dialog Data
	CCommXfr *m_pComm;
	//{{AFX_DATA(CSettingsDialog)
	enum { IDD = IDD_COMM_SETTINGS };
	CComboBox	m_Port;
	CComboBox	m_Speed;
	CComboBox	m_Settings;
	CComboBox	m_Handshaking;
	CString	m_csSpeed;
	CString	m_csSettings;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CSettingsDialog)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif /* _ZCOM_DLG_H_ */
