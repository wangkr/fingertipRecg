#pragma once
#include "afxcmn.h"
#define WM_PROGRESS_MESSAGE WM_USER+1
#define WM_PROGRESS_END		WM_USER+2
#define MYCANCELL			WM_USER+3

// ProgressDlg �Ի���

class ProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ProgressDlg)

public:
	ProgressDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~ProgressDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_progress;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
protected:
	afx_msg LRESULT OnProgressMessage(WPARAM wParam, LPARAM lParam);
public:
	CString m_progress_rate;
};
