#ifndef _HAND_MOTION_
#define _HAND_MOTION_
#include "handGesture.hpp"

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <vector>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

extern bool if_show_binary;
extern bool if_show_fgtip;
extern bool if_show_palm;

using namespace std;
using namespace cv;
typedef struct Motion{
	int last_fgTipNum;		// ��һ��������ָ��Ŀ
	int cur_fgTipNum;		// ��ǰ������ָ��Ŀ
	int last_handSize;		// ��һ�����ƴ�С
	int cur_handSize;		// ��ǰ���ƴ�С
	int last_handScale;		// ���Ʊ�����ռ����ͼ��ı���*100��
	int cur_handScale;		// ��ǰ���Ʊ���
	int cur_palmAxisAng;	// ��ǰ�������᷽�򣨽Ƕȣ�
	int last_palmAxisAng;	// ��һ���������᷽��
	Point last_palm_center; // ��һ������λ��
	Point cur_palm_center;	// ��ǰ����λ��
	bool is_zoom_mode;		// ����ģʽ��ʶ
	bool if_continue;		// ����֡�Ƿ�����
}Motion;
// ���ƴ�С=100%*���ƴ�С/ͼ���С
enum
{
	S = 0,	// 3%~5%
	M,		// 6%~10%
	L,		// 11%~30%
	XL,		// 31%~50%
	XXL		// 51%~60%
};
// �����˶��ٶ�=100%*�����ƶ�����/ͼ��Խ��߳�
enum
{
	COMMON = 0,	// 0%~11%
	QUICK		// 12%~35%
};
// �����ƶ����򣨴��£�
enum 
{	
	ORIGIN = 1,
	LEFT,
	RIGHT,
	UP,
	DOWN
};
// �����̶���
enum 
{
	NONE = -1,
	PAGEUP,
	PAGEDOWN,
	MOVE,
	PEN,	// pptӫ���
	ESC,
	LCLICK,
	RCLICK,
	LUPDOWN,
	ZOOMIN,  // �Ŵ�
	ZOOMINs, // �����Ŵ�
	ZOOMOUT,
	ZOOMOUTs,
	ROTATE
};

class HandMotion{
public:
	bool if_FA1; //�Ƿ�ѡ�񷽰�һ 
	HandMotion();
	void init(int gestnums,int sampRate);
	int sampGest(HandGesture &hg,GMM gmm,bool if_use_gmm,bool if_use_cb,vector<Point> &fgtipOut);
	int transMotion(HandGesture hg,Point &fgTip);
	bool isContinue();
private:
	float computeCurva(vector <Point> fgtip);
	float getAngle(Point s,Point f,Point e);
	float distanceP2P(Point a, Point b);
	int moveDirection();
	int searchMotion(vector<Point> fg,int firNum,int lastNum);
	int handSizeScale(HandGesture hg);
	int moveSpeed(HandGesture hg);
	// ��ͼ����ʾ���ͻ�����
	void showToClient(Mat src,HDC hdc,CRect *pRect);
	
	Motion motion;
	vector<Point> fgtips;
	vector<Point> lfgtips;
	bool if_last_gesture;
	int gestNums;		// ÿ���������Ƹ���
	int sampRate;		// ����Ƶ��
	int waitTime;		// �ȴ�ʱ��ms
};
#endif