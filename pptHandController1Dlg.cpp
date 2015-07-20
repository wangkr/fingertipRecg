
// pptHandController1Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include <process.h>
#include "pptHandController1.h"
#include "pptHandController1Dlg.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"
#include "handMotion.hpp"
#include "workthreads.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
DWORD stime1,etime1,stime2,etime2,stime3,etime3,stime4,etime4;

// 4个线程：手势控制、GMM和CB模型训练和进度条更新线程
CWinThread *pControl_Thread  = 0;
CWinThread *pGMMTrain_Thread = 0;
CWinThread *pCBTrain_Thread  = 0;
CWinThread *pProgress_Thread = 0;
//HANDLE h_cb_training;
ProgressDlg *pDlg;

CDC *pDC;
HDC hdc;
CWnd *pwnd;
CRect rect_image;
// xml文件名
const char gmmParamsFile[20] = "data\\gmm_Params.xml";
// 定义手势
HandGesture hg;
// 指尖点坐标
vector<Point> fg;
// 手势
HandMotion m;
//GMM 模型声明及其状态变量
GMM gmm;
bool if_gmm_trained  = false;
bool if_gmm_training = false;
bool if_gmm_loaded   = false;
bool if_gmm_stop     = false;
bool if_gmm = false;	// 是否采用GMM模型
bool if_cb_trained  = false;
bool if_cb_training = false;
bool if_cb_stop     = false;
bool if_cb  = false;	// 是否采用CODEBOOK模型
bool if_show_binary = false;
bool if_show_fgtip  = false;
bool if_show_palm   = false;
int  CB_TRAIN_ERR = 0;

bool if_controlPC = false;
bool if_lbutton_down;
int frame_rate = 4;
bool stop;
// 屏幕分辨率
int m_nWindwMetricsX,m_nWindwMetricsY;
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
/////////////////////////
//测试数据
extern int gest_total,gest_correct;
extern int testGestureID;
char dgestdir[6][10] = {"Click\\","Down\\","Left\\","Move\\","Right\\",
"Up\\"};
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg LRESULT OnNcHitTest(CPoint point);
protected:
	afx_msg LRESULT OnProgressEdn(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnClose();
//	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_WM_NCHITTEST()
	ON_MESSAGE(WM_PROGRESS_END, &CAboutDlg::OnProgressEdn)
	ON_WM_CLOSE()
//	ON_WM_TIMER()
END_MESSAGE_MAP()

// CpptHandController1Dlg 对话框

CpptHandController1Dlg::CpptHandController1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CpptHandController1Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_frame_rate = _T("");
	layer_attr = 70;
	frame_rate = 4;
	if_hidden_setting = true;
	if_hidden_title = false;
	if_started = false;
	if_lbutton_down = false;

	if_skin_sample = false;
	if_tm = false;
	if_gmm = false;
	stop  = false;
	m_sample_jiange = _T("");
}

void CpptHandController1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER1, m_slider);
	DDX_Control(pDX, IDC_SLIDER2, m_slider2);
	DDX_Text(pDX, IDC_STATIC_JIANGE, m_sample_jiange);
}

BEGIN_MESSAGE_MAP(CpptHandController1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_HSCROLL()
	ON_WM_NCHITTEST()
	ON_BN_CLICKED(ID_START, &CpptHandController1Dlg::OnClickedStart)
	ON_BN_CLICKED(ID_SET, &CpptHandController1Dlg::OnClickedSet)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CpptHandController1Dlg::OnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_TRAIN, &CpptHandController1Dlg::OnClickedButtonTrain)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CpptHandController1Dlg::OnClickedButtonExit)
	ON_BN_CLICKED(IDC_RADIO_FA1, &CpptHandController1Dlg::OnBnClickedRadioFa1)
	ON_BN_CLICKED(IDC_RADIO_FA2, &CpptHandController1Dlg::OnBnClickedRadioFa2)
	ON_BN_CLICKED(IDC_CHECK1, &CpptHandController1Dlg::OnBnClickedCheckTM)
	ON_BN_CLICKED(IDC_CHECK3, &CpptHandController1Dlg::OnBnClickedCheck3)
	ON_BN_CLICKED(IDC_BUTTON1, &CpptHandController1Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CHECK_GMM, &CpptHandController1Dlg::OnBnClickedCheckGMM)
	ON_BN_CLICKED(IDC_CHECK_CB, &CpptHandController1Dlg::OnBnClickedCheckCB)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CpptHandController1Dlg::OnNMCustomdrawSlider1)
	ON_BN_CLICKED(IDC_CHECK4, &CpptHandController1Dlg::OnBnClickedCheckBI)
	ON_BN_CLICKED(IDC_CHECK5, &CpptHandController1Dlg::OnBnClickedCheckShowFg)
	ON_BN_CLICKED(IDC_CHECK6, &CpptHandController1Dlg::OnBnClickedCheckPalm)
	ON_BN_CLICKED(IDC_CHECK7, &CpptHandController1Dlg::OnBnClickedCheckControl)
END_MESSAGE_MAP()


// CpptHandController1Dlg 消息处理程序

BOOL CpptHandController1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	pwnd = GetDlgItem(IDC_STATIC_IMAGE);	
	pDC = pwnd->GetDC();
	hdc = pDC->GetSafeHdc();
	pwnd->GetClientRect(&rect_image);
	// 获取主窗口大小并设置当前窗口大小
	CRect	rect_main;
	GetWindowRect(&rect_main);
	mainWinWidth = rect_main.Width();
	mainWinHeight = rect_main.Height();
	MoveWindow(rect_main.TopLeft().x,rect_main.TopLeft().y,mainWinWidth,mainWinHeight*0.42,1);
	// 设置背景颜色
	SetBackgroundColor(RGB(222,222,222));
	// 初始化滑块
	m_slider.SetRange(1,40);
	m_slider.SetTicFreq(1);
	m_slider.SetPos(10);

	m_slider2.SetRange(1,255);
	m_slider2.SetPos(70);

	// 初始化提示信息
	m_sample_jiange = "10";
	// 获取屏幕分辨率
	m_nWindwMetricsX  = GetSystemMetrics(SM_CXSCREEN);
	m_nWindwMetricsY  = GetSystemMetrics(SM_CYSCREEN);

	CButton *pbtn = (CButton*)GetDlgItem(IDC_RADIO_FA1);
	pbtn->SetCheck(TRUE);

	UpdateData(FALSE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CpptHandController1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CpptHandController1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CpptHandController1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CpptHandController1Dlg::OnClickedStart()
{
	if(if_gmm_training||if_cb_training)
	{
		AfxMessageBox(_T("模型训练中..."));
		return;
	}

	if(!if_started){
		// 自动隐藏设置面板
		if(!if_hidden_setting)
		{
			OnClickedSet();
		}
		
		// hg初始化
		if(-1==hg.init(FRAMESCALE,WEBCAMERA)){
			AfxMessageBox(_T("无法打开摄像头!"));
			return;
		}
		hg.if_skin_samp = if_skin_sample;
		if(if_skin_sample)
		{
			hg.sampleSkinColor(hdc,&rect_image);
		}
		// 置顶
		SetWindowPos(&wndTopMost,0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
		GetDlgItem(ID_START)->SetWindowTextW(_T("Stop"));
		stop = false;
		pControl_Thread = AfxBeginThread(pptContrl,0);

	}
	else{
		stop = true;
		// 等待pptContrl 线程处理完
		Sleep(100);
		hg.cap.release();
		// 取消置顶
		SetWindowPos(&wndNoTopMost,0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
		GetDlgItem(ID_START)->SetWindowTextW(_T("Start"));
		// 将背景刷白
		COLORREF color = GetBkColor(hdc);
		CBrush brush(color);
		CBrush *pbrush = pDC->SelectObject(&brush);
		pDC->FillRect(rect_image,pbrush);
		/////////////////////
		////测试变量
		//if(if_controlPC){
		//char datadir[50] = "testData\\dynamic\\Right\\data.txt";
		//FILE *pFile = fopen(datadir,"w+");
		//fprintf(pFile,"Total:%d\nCorrect:%d\nDR:%.3lf",\
		//gest_total,gest_correct,gest_correct*1.0/gest_total);
		//fclose(pFile);
		//gest_total = gest_correct = 0;
		//}
		////////////////////
		}
	if_started = !if_started;
}

void CpptHandController1Dlg::OnClickedSet()
{
	CRect rect_main;
	GetWindowRect(&rect_main);
	
	if(if_hidden_setting)
	{
		MoveWindow(rect_main.TopLeft().x,rect_main.TopLeft().y,mainWinWidth,mainWinHeight,1);
		GetDlgItem(ID_SET)->SetWindowTextW(_T("Hidden<<"));
	}
	else
	{
		MoveWindow(rect_main.TopLeft().x,rect_main.TopLeft().y,mainWinWidth,mainWinHeight*0.42,1);
		GetDlgItem(ID_SET)->SetWindowTextW(_T("Settings>>"));
	}
	if_hidden_setting = !if_hidden_setting;
}

void CpptHandController1Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *pSlider = (CSliderCtrl*)pScrollBar;
	switch(pSlider->GetDlgCtrlID())
	{
	case IDC_SLIDER1:
		hg.sample_jiange = m_slider.GetPos();
		m_sample_jiange.Format(_T("%d"),hg.sample_jiange);
		UpdateData(FALSE);
		break;
	case IDC_SLIDER2:
		int last_layer_attr = layer_attr;
		layer_attr = m_slider2.GetPos();
		if(layer_attr != last_layer_attr&&!if_started){
			setWindowLayer(layer_attr);
			Sleep(500);
			setWindowLayer(255);
		}
		else if(if_started){
			setWindowLayer(layer_attr);
		}
		break;
	}
	

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}
int pageup=0,pagedown=0;

// 设置窗体透明度
void CpptHandController1Dlg::setWindowLayer(int Alpha)
{
	 SetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE,
        GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)|WS_EX_LAYERED);

    HINSTANCE hInst = LoadLibrary(_T("User32.DLL"));

    if (hInst)
    {
        typedef BOOL (WINAPI *MYFUNC) (HWND,COLORREF,BYTE,DWORD);

        MYFUNC fun = NULL;

        CHAR str[] = "SetLayeredWindowAttributes";

        fun = (MYFUNC) GetProcAddress(hInst,str);

        if (fun)
        {
            fun(this->GetSafeHwnd(),RGB(0,255,255),Alpha,LWA_ALPHA);
        }

        FreeLibrary(hInst);
    }
}
// 退出
void CpptHandController1Dlg::OnClickedButtonExit()
{
	if(if_gmm_training||if_cb_training)
	{
		if(MessageBox(_T("正在训练模型，是否停止训练，并关闭？"),_T("正在训练..."),MB_ICONSTOP|MB_YESNO)==IDYES)
		{
			if(if_gmm_training)gmm.stopTrain();
			if(if_cb_training)wkrCB_stopTrain();

			cvWaitKey(2000);
		}
		else
			return;
	}
	// 如果训练过模型则提示保存
	if(if_gmm_trained){
		if(MessageBox(_T("模型发生改变，是否保存当前模型参数？"),_T("参数保存"),MB_ICONQUESTION|MB_YESNO) == IDYES)
		{
			if(gmm.gmmParamsWrite(gmmParamsFile) == false)
			{
				AfxMessageBox(_T("保存失败!"));
			}
		}
	}
	//// 关闭线程
	//CloseHandle(h_cb_training);
	if(!stop)
	hg.cap.release();
	// 释放内存
	wkrCB_free();

	OnOK();
}

LRESULT CpptHandController1Dlg::OnNcHitTest(CPoint point)
{
	UINT uRet = CDialogEx::OnNcHitTest(point);
	return (HTCLIENT == uRet) ? HTCAPTION : uRet;
}
// 方案1
void CpptHandController1Dlg::OnBnClickedRadioFa1()
{
	m.if_FA1 = true;
}
// 方案2
void CpptHandController1Dlg::OnBnClickedRadioFa2()
{
	m.if_FA1 = false;
}
// 窗体透明度
void CpptHandController1Dlg::OnBnClickedCheckTM()
{
	if_tm = !if_tm;
	if(if_tm)
		setWindowLayer(layer_attr);
	else
		setWindowLayer(255);
}

void CpptHandController1Dlg::OnClickedButtonLoad()
{
	if(gmm.gmmParamsRead(gmmParamsFile) == true)
	{
		AfxMessageBox(_T("加载成功!"));
		if_gmm_loaded = true;
	}
	else
	{
		AfxMessageBox(_T("加载失败!"));
	}
}

void CpptHandController1Dlg::OnClickedButtonTrain()
{
	if(if_started){
		AfxMessageBox(_T("请先“停止”视频显示"));
		return;
	}
	if(if_cb_training){
		AfxMessageBox(_T("正在进行【CODEBOOK】模型训练，请稍后..."));
		return;
	}
	// 创建进度条对话框
	if(!if_gmm_trained){
		if(if_gmm_training)
		{
			AfxMessageBox(_T("模型训练中..."));
			return;
		}
		pDlg = new ProgressDlg();
		pDlg->Create( ProgressDlg::IDD, NULL );
		pDlg->ShowWindow(SW_SHOW );
		// 创建两个新的线程
		pGMMTrain_Thread = AfxBeginThread(GMM_Training,0);// 处理模型训练
		pProgress_Thread = AfxBeginThread(ProgressAdd,(void*)this); // 处理进度条
	}
	else 
	{
		if(MessageBox(_T("训练完成！是否重新训练？"),_T("训练完成"),MB_ICONQUESTION|MB_YESNO)==IDYES)
		{
			gmm.zeroProgress(PROGRESS_TRAIN);/// 进度清零
			pDlg = new ProgressDlg();
			pDlg->Create( ProgressDlg::IDD, NULL );
			pDlg->ShowWindow(SW_SHOW );
			if_gmm_trained = false;
			// 创建两个新的线程
			pGMMTrain_Thread = AfxBeginThread(GMM_Training,0);// 处理模型训练
			pProgress_Thread = AfxBeginThread(ProgressAdd,(void*)this); // 处理进度条
		}
	}
}

afx_msg LRESULT CAboutDlg::OnProgressEdn(WPARAM wParam, LPARAM lParam)
{
	if(pDlg)
	{
		delete pDlg;
		pDlg = NULL;
	}
	return 0;
}

void CAboutDlg::OnClose()
{
	CDialogEx::OnClose();
}

void CpptHandController1Dlg::OnBnClickedCheck3()
{
	if_skin_sample = !if_skin_sample;
}

void CpptHandController1Dlg::OnBnClickedButton1()
{
	if(if_started){
		AfxMessageBox(_T("请先“停止”视频显示"));
		return;
	}
	if(if_gmm_training){
		AfxMessageBox(_T("正在进行【GMM】模型训练，请稍后..."));
		return;
	}
	

	// 创建进度条对话框
	if(!if_cb_trained){
		if(if_cb_training)
		{
			AfxMessageBox(_T("模型训练中..."));
			return;
		}
		pDlg = new ProgressDlg();
		pDlg->Create( ProgressDlg::IDD, NULL );
		pDlg->ShowWindow(SW_SHOW );
		// 创建两个新的线程
		pCBTrain_Thread = AfxBeginThread(CB_Training,0);// 处理模型训练
		pProgress_Thread = AfxBeginThread(ProgressAdd,(void*)this); // 处理进度条
		//h_cb_training = (HANDLE)_beginthreadex(0,0,(unsigned int(__stdcall *)(void *))CB_Training,0,0,0);
		//if(h_cb_training == 0)
		//{
		//	AfxMessageBox(_T("CODEBOOK训练线程创建失败!"));
		//	return;
		//}/*
		//if(CB_TRAIN_ERR == 0)
		//{
		//	AfxMessageBox(_T("模型训练成功!"));
		//}
		//else if(CB_TRAIN_ERR == -1)
		//{
		//	AfxMessageBox(_T("请打开摄像头!"));
		//}*/
		////pCBTrain_Thread = AfxBeginThread(CB_Training,0);// 处理模型训练
		//pProgress_Thread = AfxBeginThread(ProgressAdd,(void*)this); // 处理进度条
	}
	else 
	{
		if(MessageBox(_T("初始化完成！是否重新初始化？"),_T("初始化完成"),MB_ICONQUESTION|MB_YESNO)==IDYES)
		{
			wkrCB_zeroProgress();/// 进度清零
			pDlg = new ProgressDlg();
			pDlg->Create( ProgressDlg::IDD, NULL );
			pDlg->ShowWindow(SW_SHOW );
			if_cb_trained = false;
			// 创建两个新的线程
			pCBTrain_Thread = AfxBeginThread(CB_Training,0);// 处理模型训练
			pProgress_Thread = AfxBeginThread(ProgressAdd,(void*)this); // 处理进度条
		}
	}
}


void CpptHandController1Dlg::OnBnClickedCheckGMM()
{
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK_GMM);
	CButton *pbt1 = (CButton*)GetDlgItem(IDC_CHECK_CB);
	if(!if_gmm_trained&&!if_gmm_loaded)
	{
		pbt->SetCheck(FALSE);
		AfxMessageBox(_T("请先训练模型！"));
		return;
	}
	if_gmm = !if_gmm;
	if_cb = if_gmm==true? false:if_cb;
	pbt->SetCheck((if_gmm==true));
	pbt1->SetCheck((if_cb==true));
}


void CpptHandController1Dlg::OnBnClickedCheckCB()
{
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK_CB);
	CButton *pbt1 = (CButton*)GetDlgItem(IDC_CHECK_GMM);
	if(!if_cb_trained)
	{
		pbt->SetCheck(FALSE);
		AfxMessageBox(_T("请先初始化模型！"));
		return;
	}
	if_cb = !if_cb;
	if_gmm = if_cb==true?false:if_gmm;
	pbt->SetCheck((if_cb==true));
	pbt1->SetCheck((if_gmm==true));
}


void CpptHandController1Dlg::OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CpptHandController1Dlg::OnBnClickedCheckBI()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK4);
	if_show_binary = !if_show_binary;
	pbt->SetCheck(if_show_binary==true);
}


void CpptHandController1Dlg::OnBnClickedCheckShowFg()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK5);
	if_show_fgtip = !if_show_fgtip;
	pbt->SetCheck(if_show_fgtip==true);
}


void CpptHandController1Dlg::OnBnClickedCheckPalm()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK6);
	if_show_palm = !if_show_palm;
	pbt->SetCheck(if_show_palm==true);
}


void CpptHandController1Dlg::OnBnClickedCheckControl()
{
	CButton *pbt = (CButton*)GetDlgItem(IDC_CHECK7);
	if_controlPC = !if_controlPC;
	pbt->SetCheck(if_controlPC==true);
}
