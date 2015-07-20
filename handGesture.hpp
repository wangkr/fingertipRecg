#ifndef _HAND_GESTURE_
#define _HAND_GESTURE_
#include "CvvImage.hpp"
#include "gmm.hpp"
#include "codeBook.hpp"
#include "myROI.hpp"

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <vector>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

using namespace std;
using namespace cv;


// 哈夫曼圆拟合参数
#define HOUGH_PARAM1 21.0
#define HOUGH_PARAM2 60.0
#define MINRADIUS	 17
#define MAXRADIUS	 38
#define PI 3.1415;
#define RECT_WIDTH 10
// 轮廓采样点距离
#define SAMPLESTEP 20
// 曲率阈值（acos(0.809)≈35.9°）
#define CURVA_THRESHOLD 0.809
// 亮度
#define V_low  30
#define V_high  250
// 饱和度
#define S_low  10
#define S_high  170
// 色相
#define H_low_max  40
#define H_high_min  100
#define if_high_light  1 //是否高光补偿

#define MopEx_value 2    // 开运算
#define YCrCb_lower cvScalar(46,131,119,0)
#define YCrCb_upper cvScalar(197,162,134,0)

#define NSAMPLES 6
#define ORIGCOL2COL CV_BGR2HLS
#define COL2ORIGCOL CV_HLS2BGR

// 样本点结构体
typedef struct SamPoints{
	vector<int>	samIndex;			// 样本点在轮廓点集中的索引
	vector<float> curvature;		// 样本点集的曲率
	vector<int>	dire;				// 样本点集向量交叉积方向
	vector<int> fgTipcIdx;			// 指尖点索引
	vector<Point> fgTipPts;			// 指尖点
	vector<float> fgTipCurva;		// 指尖点曲率
}SamPoints;
class HandGesture{
public:
	HandGesture();
	VideoCapture cap;	// 视频源
	Mat src_re;			// 缩放后的原图片

	Mat newpic;////////////临时变量
	int sample_jiange;

	Size newSize;		// 缩放尺寸
	int ImageLen;		// 图像大小
	int ImageDiagonal;	// 对角线长度

	Point palm_center;	// 掌心坐标
	Rect bound_rect;	// 轮廓边界
	int palmAxisAng;	// 掌轴角度
	float scale;		// 缩放系数
	bool if_hand;		// 是否有手
	bool if_skin_samp;	// 是否进行皮肤采样
	int cIdx;			// 主轮廓索引号
	vector<vector<Point> > contours;	// 手势轮廓点集
	vector <Point> fingerTips;			// 指尖坐标1
	//int mostFrequentFingerNumber;		// 最频繁的手指号
	//int nrOfDefects;					// 凸点数目
	//vector<vector<int> >hullI;			// 凸包点集索引
	//vector<vector<Point> >hullP;			// 凸包点集
	//vector<vector<Vec4i> > defects;		// 凸点
	
	int init(float scale,int webCam);		// 初始化
	int getSrcFrame();						// 获取源视频帧
	void makeContours(Mat src);				// 提取轮廓
	void palmCentrLoc(Mat src,Mat &dst);	// 手掌心定位函数1—通过霍夫曼圆变换
	void palmCentrLoc2(int cIdx,Mat &dst);	// 手掌心定位函数2—通过图像矩
	int palmAxisAngle(vector<Point>  fgTps);// 计算掌轴的倾斜角度
	void fingerTipLoc(vector<Point> &fg);	// 指尖点定位
	void sampleSkinColor(HDC hdc,CRect *pRect);
	bool detectIfHand();					// 检测是否有手

	void waitForPalmCover(HDC hdc,CRect *pRect);
	void average(HDC hdc,CRect *pRect);
	 // 手部区域分割
	void handSegmented(GMM gmm,bool if_use_gmm,bool if_use_cb);

	// 轮廓绘画静态函数
	static void myDrawContours(Mat src,HandGesture *hg);
	
	Mat src_thre;			// 存放二值图片
private:
	Mat YCrCb;				// YCrCb空间分割手部的mask
	Mat img_tmp;			// 中间图片
	vector<Mat> bwList;		// 二值图链表

	Point hough_center;		// 当前哈夫曼圆中心
	Point lhough_center;	// 上一个哈夫曼圆中心
	Point bound_center;		// 当前轮廓边界中心
	Point lbound_center;	// 上一个轮廓边界中心

	vector<int> fingerNumbers;

	void initColors();					 // 初始化颜色空间变量
	void hand_YCrCb(Mat src);			 // YCrCb颜色处理
	void skinColorSegHSV(Mat src,Mat &dst);// HSV空间肤色分割函数1
	void skinColorSegHistogram(Mat src,Mat &dst);// HSV空间肤色分割函数2
	int findBiggestContour();			 // 找到最大轮廓
	//void initVectors();					// 点集初始化
	//void checkForOneFinger(Mat src);		// 检测单手指
	//void computeFingerNumber();			// 计算手指数目
	//void eliminateDefects(Mat src);		// 消除
	//void addFingerNumberToVector();		// 添加手指号

	void ifingerTipLoc1(Mat src);						// 指尖定位函数1
	void ifingerTipLoc2(Mat src,int L,SamPoints &samp);	// 指尖定位函数2
	void normalizeColors();
	void curvaCompute(SamPoints &samp);					// 曲率计算
	int  len_of_L(int handRecSize);						// 确定Pi的邻居点距离L的长度
	float thred_of_curvature(int handRecSize);			// 确定曲率的阈值

	// 肤色采样变量
	int avgColor[NSAMPLES][3] ;
	int c_lower[NSAMPLES][3];
	int c_upper[NSAMPLES][3];
	int avgBGR[3];
	vector <MyROI> roi;
public:
	// 静态函数
	static	int getMedian(vector<int> val);							// 获取中位数
	static	void getAvgColor(MyROI roi,int avg[3]);					// 获取平静颜色值
	
	static	float	distanceP2P(Point a, Point b);					// 计算距离
	static	float	getAngle(Point s, Point f, Point e);			// 获取三点的夹角
	static	void	printText(Mat src, string text);				// 在原图像上打印文字
	static	void	showToClient(Mat src,HDC hdc,CRect *pRect);		// 将图像显示到面板上
	static  int		calcTilt(double mu11,double mu20,double mu02);	// 计算掌轴角度
	static  double	round(double r);								// 四舍五入函数

};


#endif