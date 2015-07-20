#include "stdafx.h"
#include <process.h>
#include <Windows.h>
#include "pptHandController1.h"
#include "pptHandController1Dlg.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"
#include "handMotion.hpp"
///////////////////////
extern int speedSamples[500];
extern int hdSizeSamples[500];
extern int spd;
extern int hds;
// pptHandController.cpp的全局变量
extern CWinThread *pControl_Thread;
extern CWinThread *pGMMTrain_Thread;
extern CWinThread *pCBTrain_Thread;
extern CWinThread *pProgress_Thread;

extern CDC *pDC;
extern HDC hdc;
extern CWnd *pwnd;
extern CRect rect_image;
// xml文件名
extern const char gmmParamsFile[20];
// 定义手势
extern HandGesture hg;
// 指尖点坐标
extern vector<Point> fg;
// 手势
extern HandMotion m;
//GMM 模型声明及其状态变量
extern GMM gmm;
extern bool if_gmm_trained;
extern bool if_gmm_training;
extern bool if_gmm_loaded;
extern bool if_gmm_stop;
extern bool if_gmm;	// 是否采用GMM模型
extern bool if_cb_trained;
extern bool if_cb_training;
extern bool if_cb_stop;
extern bool if_cb;	// 是否采用CODEBOOK模型
extern bool if_show_binary;
extern bool if_show_fgtip;
extern bool if_show_palm;
extern int  CB_TRAIN_ERR;

extern bool if_lbutton_down;
extern bool if_controlPC;
extern int frame_rate;
extern bool stop;
// 屏幕分辨率
extern int m_nWindwMetricsX,m_nWindwMetricsY;
extern ProgressDlg *pDlg;
extern DWORD stime1,etime1,stime2,etime2,stime3,etime3,stime4,etime4;;
/////////////////////////
////测试变量声明
extern string testVideoFile[6];
extern int testGestureID;
////////////////////////////

// 线程函数
UINT pptContrl(LPVOID lParam);
UINT GMM_Training(LPVOID lParam);
UINT CB_Training(LPVOID lPraram);
UINT ProgressAdd(void *ptr);

void Mouse_Keybd(int gest ,CPoint fgtip);
void myVkInput(WORD data,WORD data2,int num);

void showHandInfo(HandGesture hg,vector<Point> fgtip,HDC hdc,CRect *pRect);
int moveStep(CPoint fgtip);
void showToClient(Mat src,HDC hdc,CRect *pRect);