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
	int last_fgTipNum;		// 上一个手势手指数目
	int cur_fgTipNum;		// 当前手势手指数目
	int last_handSize;		// 上一个手势大小
	int cur_handSize;		// 当前手势大小
	int last_handScale;		// 手势比例（占整个图像的比例*100）
	int cur_handScale;		// 当前手势比例
	int cur_palmAxisAng;	// 当前手掌掌轴方向（角度）
	int last_palmAxisAng;	// 上一个手掌掌轴方向
	Point last_palm_center; // 上一个掌心位置
	Point cur_palm_center;	// 当前掌心位置
	bool is_zoom_mode;		// 缩放模式标识
	bool if_continue;		// 手势帧是否连续
}Motion;
// 手势大小=100%*手势大小/图像大小
enum
{
	S = 0,	// 3%~5%
	M,		// 6%~10%
	L,		// 11%~30%
	XL,		// 31%~50%
	XXL		// 51%~60%
};
// 手势运动速度=100%*手势移动距离/图像对角线长
enum
{
	COMMON = 0,	// 0%~11%
	QUICK		// 12%~35%
};
// 手势移动方向（大致）
enum 
{	
	ORIGIN = 1,
	LEFT,
	RIGHT,
	UP,
	DOWN
};
// 鼠标键盘动作
enum 
{
	NONE = -1,
	PAGEUP,
	PAGEDOWN,
	MOVE,
	PEN,	// ppt荧光笔
	ESC,
	LCLICK,
	RCLICK,
	LUPDOWN,
	ZOOMIN,  // 放大
	ZOOMINs, // 连续放大
	ZOOMOUT,
	ZOOMOUTs,
	ROTATE
};

class HandMotion{
public:
	bool if_FA1; //是否选择方案一 
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
	// 将图像显示到客户界面
	void showToClient(Mat src,HDC hdc,CRect *pRect);
	
	Motion motion;
	vector<Point> fgtips;
	vector<Point> lfgtips;
	bool if_last_gesture;
	int gestNums;		// 每个动作手势个数
	int sampRate;		// 采样频率
	int waitTime;		// 等待时间ms
};
#endif