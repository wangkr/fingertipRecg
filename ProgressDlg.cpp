// ProgressDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "pptHandController1.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"


// ProgressDlg �Ի���

IMPLEMENT_DYNAMIC(ProgressDlg, CDialogEx)

ProgressDlg::ProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(ProgressDlg::IDD, pParent)
{

	m_progress_rate = _T("");
}

ProgressDlg::~ProgressDlg()
{
}

void ProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
	DDX_Text(pDX, IDC_STATIC_RATE, m_progress_rate);
}


BEGIN_MESSAGE_MAP(ProgressDlg, CDialogEx)
	ON_WM_TIMER()
	ON_MESSAGE(WM_PROGRESS_MESSAGE, &ProgressDlg::OnProgressMessage)
END_MESSAGE_MAP()


// ProgressDlg ��Ϣ�������


void ProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnTimer(nIDEvent);
}


afx_msg LRESULT ProgressDlg::OnProgressMessage(WPARAM wParam, LPARAM lParam)
{
	int nProgress = lParam;

	m_progress.SetPos( nProgress );
	m_progress_rate.Format(_T("%d%%"),nProgress);
	UpdateData(FALSE);
	if( nProgress >= 100 ){
		//PostQuitMessage(0);
		Sleep(100);
		OnOK();
	}
	return 0;
}
