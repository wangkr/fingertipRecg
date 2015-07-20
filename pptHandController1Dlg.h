
// pptHandController1Dlg.h : 头文件
//

// 图像帧缩放系数
#define FRAMESCALE 0.5
#define WEBCAMERA 0
#pragma once

// CpptHandController1Dlg 对话框
class CpptHandController1Dlg : public CDialogEx
{
// 构造
public:
	CpptHandController1Dlg(CWnd* pParent = NULL);	// 标准构造函数
	void setWindowLayer(int Alpha);
	int mainWinWidth,mainWinHeight;
	CRect mainWin;
// 对话框数据
	enum { IDD = IDD_PPTHANDCONTROLLER1_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	bool if_tm;				// 是否透明
	int layer_attr;
	bool if_hidden_setting; // 是否隐藏设置面板
	bool if_hidden_title;	// 是否隐藏标题栏
	bool if_started;
	bool if_skin_sample;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedStart();
	afx_msg void OnClickedSet();
	afx_msg void OnClickedButtonLoad();
	afx_msg void OnClickedButtonTrain();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	CSliderCtrl m_slider;
	CString m_frame_rate;
	afx_msg void OnClickedButtonExit();
	CSliderCtrl m_slider2;
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnBnClickedRadioFa1();
	afx_msg void OnBnClickedRadioFa2();
	afx_msg void OnBnClickedCheckTM();
	afx_msg void OnBnClickedCheckMX();
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedCheckGMM();
	afx_msg void OnBnClickedCheckCB();
	afx_msg void OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult);
	CString m_sample_jiange;
	afx_msg void OnBnClickedCheckBI();
	afx_msg void OnBnClickedCheckShowFg();
	afx_msg void OnBnClickedCheckPalm();
	afx_msg void OnBnClickedCheckControl();
};
