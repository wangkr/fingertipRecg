//////////// 四个线程函数：手势控制、GMM模型训练、CODEBOOK模型训练和进度条更新线程
#include "workthreads.h"

int gest_total = 0,gest_correct = 0;
UINT ProgressAdd( void * ptr)
{
	CpptHandController1Dlg * pMainDlg = (CpptHandController1Dlg*)ptr;
	int ptemp;
	
	if(if_gmm_training)
	while(!gmm.isErr()&&(ptemp = gmm.getProgress(PROGRESS_TRAIN)) <= END_FRAME)
	{
		if(pDlg->GetSafeHwnd())
			pDlg->SendMessage(WM_PROGRESS_MESSAGE,0,(int)(ptemp==END_FRAME?100:ptemp*100.0/END_FRAME));
		Sleep(100);
	}
	if(if_cb_training)
	while(!wkrCB_isErr()&&(ptemp = wkrCB_getProgress()) <= MAX_FRAME_STAGE2)
	{
		if(pDlg->GetSafeHwnd())
			pDlg->SendMessage(WM_PROGRESS_MESSAGE,0,(int)(ptemp==MAX_FRAME_STAGE2?100:ptemp*100.0/MAX_FRAME_STAGE2));
		Sleep(10);
	}
	pDlg->SendMessage(WM_PROGRESS_MESSAGE,0,100);
	pMainDlg->PostMessage( WM_PROGRESS_END, 0, 100 ); // 给主线程发结束消息，释放ProgressDlg对话框指针
	DWORD dwExitCode;
	GetExitCodeThread(pProgress_Thread->m_hThread,&dwExitCode );
	AfxEndThread(dwExitCode,TRUE);
	return 0;
}
/**
*CODEBOOK模型训练线程
*/
UINT CB_Training(LPVOID lParam)
{
	CB_TRAIN_ERR = 0;
	if_cb_training = true;
	if((CB_TRAIN_ERR = wkrCB_init(WEBCAMERA,FRAMESCALE)) == -1)
	{
		AfxMessageBox(_T("请打开摄像头!"));
		if_cb_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pCBTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE); // 线程内部推出
	}
	else if(CB_TRAIN_ERR == 0)
	{
		AfxMessageBox(_T("训练成功!"));
		if_cb_trained  = true;
		if_cb_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pCBTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE);
	}
	else if(CB_TRAIN_ERR == 1)
	{
		if_cb_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pCBTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE);
	}
	return 0;
}
/**
* GMM模型训练线程
*/
UINT GMM_Training(LPVOID lParam)
{
	int ERR = 0;
	if_gmm_training = true;
	if((ERR = gmm.Train(WEBCAMERA,FRAMESCALE)) == -1)
	{
		AfxMessageBox(_T("请打开摄像头!"));
		if_gmm_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pGMMTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE); // 线程内部推出
	}
	else if(ERR == -2)
	{
		AfxMessageBox(_T("未知错误，训练失败!"));
		if_gmm_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pGMMTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE);
	}
	else if(ERR == 1)
	{
		AfxMessageBox(_T("训练成功!"));
		if_gmm_trained  = true;
		if_gmm_training = false;
		DWORD dwExitCode;
		GetExitCodeThread(pGMMTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE);
	}
	else if(ERR == 0||ERR == 2)
	{
		DWORD dwExitCode;
		GetExitCodeThread(pGMMTrain_Thread->m_hThread,&dwExitCode );
		AfxEndThread(dwExitCode,TRUE);
	}
	return 0;
}
void showHandInfo(HandGesture hg,vector<Point> fgtip,HDC hdc,CRect *pRect)
{
	//////////////////////////////
	//imwrite("testpic\\src.jpg",hg.src_re);
	/////////////////////////////
	if(hg.detectIfHand()){
		if(if_show_palm){
			String angStr;char angchar[20];
			int handscale = (int)(hg.round(hg.bound_rect.area()*100.0/hg.ImageLen));
			itoa(handscale,angchar,10);
			angStr = angchar;
			HandGesture::printText(hg.src_re,angStr);
			circle(hg.src_re,hg.palm_center,2,CV_RGB(0xff,0,0),2);
			// 画出掌轴
			Point start,end;
			start.x = 50*cos(hg.palmAxisAng*3.1416/180)+hg.palm_center.x;
			start.y = 50*sin(hg.palmAxisAng*3.1416/180)+hg.palm_center.y;
		
			end.x = 100*cos(hg.palmAxisAng*3.1416/180+3.1416)+hg.palm_center.x;
			end.y = 100*sin(hg.palmAxisAng*3.1416/180+3.1416)+hg.palm_center.y;
			if(hg.palmAxisAng<=180)
				circle(hg.src_re,end,2,CV_RGB(0xff,0,0xbb),2);
			else
				circle(hg.src_re,start,2,CV_RGB(0xff,0,0xbb),2);
			line(hg.src_re,start,end,CV_RGB(0,255,0),2);
			/////////////////////
			//Mat src123;
			//src123 = Mat::zeros(hg.src_re.size(),hg.src_re.type());
			//drawContours(src123,hg.contours,hg.cIdx,CV_RGB(255,255,255),1);
			//line(src123,start,end,CV_RGB(255,255,255),2);
			//imwrite("testpic\\palmAxis.bmp",src123);
			/////////////////////
		}
		if(if_show_fgtip){
			// 画出指尖
			int fgNums = fgtip.size();
			/////////////////////
			//Mat src123;
			//src123 = imread("testpic\\sampPoints.bmp");
			/////////////////////
			for(int i = 0;i < fgNums;i++)
			{
				circle(hg.src_re,fgtip[i],3,CV_RGB(255,255,0),2);
				line(hg.src_re,hg.palm_center,fgtip[i],CV_RGB(0,255,255),1);
				////////////////////
				//circle(src123,fgtip[i],2,CV_RGB(255,0,0),2);
				////////////////////
			}
			///////////////////
			//imwrite("testpic\\fingtip.bmp",src123);
			///////////////////
		}
	}
	// 显示二值图像或者原图像
	if(if_show_binary)
		showToClient(hg.src_thre,hdc,pRect);
	else
		showToClient(hg.src_re,hdc,pRect);
	//////////////////////////////
	//imwrite("testpic\\threshold.bmp",hg.src_thre);
	//imwrite("testpic\\dst.jpg",hg.src_re);
	//////////////////////////
}
void showToClient(Mat src,HDC hdc,CRect *pRect)
{
	IplImage src1 = src;
	CvvImage m_CvvImage;
	m_CvvImage.CopyOf(&src1,1);
	m_CvvImage.DrawToHDC(hdc,pRect);
}
//统计并计算动态手势正确率
void statGestDR(int gest)
{
	
}
UINT pptContrl(LPVOID lParam)
{
	stime1 = GetTickCount();
	stime2 = GetTickCount();
	vector<Point> fgtips;
	int count = 0;
	int subcount = 0;
	int T = 0,F = 0;
	char gestNumStr[5];
	int gestNum = 0;
	// m初始化
	m.init(4,frame_rate);
	//char data[] = "testData\\static\\data1.txt";
	//FILE *pFile = fopen(data,"w+");
	for(;;)
	{
	try{
		Point fgTip = Point(0,0);
		// 是否选择背景减法模型
		if( if_gmm || if_cb )
		{
			if(m.sampGest(hg,gmm,if_gmm,if_cb,fgtips) == -1)
			{
				break;
			}
			showHandInfo(hg,fgtips,hdc,&rect_image);
			
			//手势必须连续
			if(m.isContinue()&&if_controlPC){
				int gest = m.transMotion(hg,fgTip);
				Mouse_Keybd(gest,CPoint(fgTip.x,fgTip.y));
			}
		}
		else
		{
			hg.getSrcFrame();
			showToClient(hg.src_re,hdc,&rect_image);
		}
		/*if(if_show_binary)
			showToClient(hg.src_thre,hdc,&rect_image);
		else
			showToClient(hg.src_re,hdc,&rect_image);*/
		
		/*if(spd == 1000){
			AfxMessageBox(_T("手势速度已采集!"));
			spd = 1001;
		}
		if(hds == 1000){
			AfxMessageBox(_T("手势大小已采集!"));
			hds = 1001;
		}*/
		
		Sleep(50);
		if(stop)
		{
			//////////////////////////
		/*char file[20] = "handSize.txt";
		char file2[20] = "handSpeed.txt";
		FILE *pFile1 = NULL,*pFile2 = NULL;
		pFile1 = fopen(file,"w+");
		pFile2 = fopen(file2,"w+");
		int statSize[20];
		int statSpeed[100];
		int ImageSize = hg.src_re.rows*hg.src_re.cols;
		int maxDist = sqrt((double)(hg.src_re.rows*hg.src_re.rows+hg.src_re.cols*hg.src_re.cols));
		if(pFile1 && pFile2){
			int j = 0;
			while(j < 20)statSize[j++] = 0;
			j = 0;
			while(j < 100)statSpeed[j++] = 0;
			for(int i = 0;i < 1000;i++)
			{
				double temp1 = hdSizeSamples[i]*100.0 / ImageSize;
				double temp2 = speedSamples[i] *100.0 / maxDist;
				int sizei  = (int)( temp1 / 5 );
				int speedi = (int)temp2;
				if(sizei < 20)statSize[sizei]++;
				if(speedi < 100)statSpeed[speedi]++;
			}
			for(int n = 0; n < 20;n++){
				fprintf(pFile1,"%d %%--%d %%:%d\t",n*5,(n+1)*5,statSize[n]);
				for(int k = 0;k < statSize[n];k+=2)
					fprintf(pFile1,".");
				fprintf(pFile1,"\n");
			}
			for(int l = 0; l < 100;l++){
				fprintf(pFile2,"%d %%--%d %%:%d\t",l,l+1,statSpeed[l]);
				for(int k = 0;k < statSpeed[l];k+=2)
					fprintf(pFile2,".");
				fprintf(pFile2,"\n");
			}
		}
		fclose(pFile1);
		fclose(pFile2);*/
			break;
		}
	}catch(std::range_error){
		AfxMessageBox(_T("Range Error!"));
		hg.cap.release();
		exit(0);
	}
	}
	
	//fclose(pFile);
	DWORD dwExitCode;
	GetExitCodeThread(pControl_Thread->m_hThread,&dwExitCode );
	AfxEndThread(dwExitCode,TRUE);
	return 0;
}
// ppt演示控制（键盘+鼠标）
void Mouse_Keybd(int gest,CPoint fgtip)
{
	CPoint cursor_pos;
	GetCursorPos(&cursor_pos);
	CPoint newcursor;
	switch(gest)
	{
	case ZOOMIN:
			// 如果鼠标左键没有释放，则释放
		if(!if_lbutton_down)
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		myVkInput(0x11,0x6B,2);
		Sleep(50);
		break;
	case ZOOMINs:
		// 如果鼠标左键没有释放，则释放
		if(!if_lbutton_down)
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		myVkInput(0x11,0x6B,2);
		Sleep(50);
		myVkInput(0x11,0x6B,2);
		break;
	case ZOOMOUT:
		// 如果鼠标左键没有释放，则释放
		if(!if_lbutton_down)
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		myVkInput(0x11,0x6D,2);
		Sleep(50);
		break;
	case ZOOMOUTs:
		// 如果鼠标左键没有释放，则释放
		if(!if_lbutton_down)
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		myVkInput(0x11,0x6D,2);
		Sleep(50);
		myVkInput(0x11,0x6D,2);
		break;
	case PAGEUP:
		etime1 = GetTickCount();
		if(etime1 - stime1 > 500){
			// 如果鼠标左键没有释放，则释放
			if(!if_lbutton_down)
				mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
			myVkInput(0x21,0,1);
			stime1 = etime1;
		}
		break;
	case PAGEDOWN:
		etime2 = GetTickCount();
		if(etime2 - stime2 > 500){
			if(!if_lbutton_down)
				mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
			myVkInput(0x22,0,1);
			stime2 = etime2;
		}
		break;
	case MOVE:
		cursor_pos.x += fgtip.x * moveStep(fgtip); newcursor.x = (cursor_pos.x >= m_nWindwMetricsX?m_nWindwMetricsX:cursor_pos.x);
		cursor_pos.y += fgtip.y * moveStep(fgtip); newcursor.y = (cursor_pos.y >= m_nWindwMetricsY?m_nWindwMetricsY:cursor_pos.y);
		if(cursor_pos.x > 0 && cursor_pos.y > 0)
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE,newcursor.x,newcursor.y,0,0);
		SetCursorPos(newcursor.x,newcursor.y);
		break;
	case PEN:
		// PEN和左键按下一样故不能释放
		if(!if_lbutton_down)
		myVkInput(0x11,0x50,2);
		break;
	case ESC:
		if(!if_lbutton_down)
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		myVkInput(0x1B,0,1);
		break;
	case LCLICK:
		if(!if_lbutton_down)
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
		break;
	case RCLICK:
		if(!if_lbutton_down)
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP,cursor_pos.x,cursor_pos.y,0,0);
		break;
	case LUPDOWN:
		if(!if_lbutton_down){
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN,cursor_pos.x,cursor_pos.y,0,0);
			if_lbutton_down = true;
		}
		else{
			mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP,cursor_pos.x,cursor_pos.y,0,0);
			if_lbutton_down = false;
		}
		break;
	}
}
int moveStep(CPoint fgtip)
{
	int dist = sqrt((double)(fgtip.x*fgtip.x+fgtip.y*fgtip.y));
	int movStep = 1;
	if(dist <= 0.01*hg.ImageDiagonal)
		movStep = 1;
	else if(dist <= 0.05*hg.ImageDiagonal)
		movStep = 2;
	else if(dist <= 0.10*hg.ImageDiagonal)
		movStep = 4;
	else 
		movStep = 8;
	return movStep;
}
void myVkInput(WORD data,WORD data2,int num)
{
	if(num == 1){
		INPUT input[2]; 
		memset(input,0,2 * sizeof(INPUT));

		input[0].type = INPUT_KEYBOARD; 
		input[0].ki.wVk = data; 
		SendInput(1,input,sizeof(INPUT));

		input[1].type = INPUT_KEYBOARD; 
		input[1].ki.wVk = data; 
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, input + 1, sizeof(INPUT));
	}
	else if(num == 2){
		INPUT input[4]; 
		memset(input,0,4 * sizeof(INPUT));

		input[0].type = INPUT_KEYBOARD;
		input[1].type = INPUT_KEYBOARD;
		input[0].ki.wVk = data;
		input[1].ki.wVk = data2;
		SendInput(2,input,sizeof(INPUT));

		input[2].type = INPUT_KEYBOARD;
		input[3].type = INPUT_KEYBOARD;
		input[2].ki.wVk = data2;
		input[3].ki.wVk = data;
		input[2].ki.dwFlags = KEYEVENTF_KEYUP;
		input[3].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(2,input + 2, sizeof(INPUT));
	}
}