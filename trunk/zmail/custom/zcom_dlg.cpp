// zcom_dlg.cpp : implementation file
//

#include "afxwin.h"
#include "scomfc.h"
#include "zcom_dlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog dialog

CSettingsDialog::CSettingsDialog(CCommXfr *pComm, CWnd* pParent)
	: CDialog(CSettingsDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSettingsDialog)
	m_csSpeed = "";
	m_csSettings = "";	
	//}}AFX_DATA_INIT
	m_pComm = pComm;
}

void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettingsDialog)
	DDX_Control(pDX, IDC_COMM_PORT, m_Port);
	DDX_Control(pDX, IDC_COMM_SPEED, m_Speed);
	DDX_Control(pDX, IDC_COMM_SETTINGS, m_Settings);
	DDX_Control(pDX, IDC_COMM_HANDSHAKING, m_Handshaking);
	DDX_CBString(pDX, IDC_COMM_SPEED, m_csSpeed);
	DDX_CBString(pDX, IDC_COMM_SETTINGS, m_csSettings);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSettingsDialog, CDialog)
	//{{AFX_MSG_MAP(CSettingsDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsDialog message handlers

void CSettingsDialog::OnOK()
{
	UpdateData();
	if (!m_pComm->GetPortOpen())
		m_pComm->SetPort(m_Port.GetCurSel() + 1);
	m_pComm->SetSettings(m_csSpeed + "," + m_csSettings);
	m_pComm->SetHandshaking(m_Handshaking.GetCurSel());
	CDialog::OnOK();
}

BOOL CSettingsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_Port.SetCurSel(m_pComm->GetPort() - 1);
	
	CString csSettings = m_pComm->GetSettings();	
	              
	m_Speed.SelectString(-1, csSettings.Left(csSettings.Find(',') - 1));
	m_Settings.SelectString(-1, csSettings.Mid(csSettings.Find(',') + 1));
	m_Handshaking.SetCurSel(m_pComm->GetHandshaking());
	return TRUE; 
}
