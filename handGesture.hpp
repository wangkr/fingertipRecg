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


// ������Բ��ϲ���
#define HOUGH_PARAM1 21.0
#define HOUGH_PARAM2 60.0
#define MINRADIUS	 17
#define MAXRADIUS	 38
#define PI 3.1415;
#define RECT_WIDTH 10
// �������������
#define SAMPLESTEP 20
// ������ֵ��acos(0.809)��35.9�㣩
#define CURVA_THRESHOLD 0.809
// ����
#define V_low  30
#define V_high  250
// ���Ͷ�
#define S_low  10
#define S_high  170
// ɫ��
#define H_low_max  40
#define H_high_min  100
#define if_high_light  1 //�Ƿ�߹ⲹ��

#define MopEx_value 2    // ������
#define YCrCb_lower cvScalar(46,131,119,0)
#define YCrCb_upper cvScalar(197,162,134,0)

#define NSAMPLES 6
#define ORIGCOL2COL CV_BGR2HLS
#define COL2ORIGCOL CV_HLS2BGR

// ������ṹ��
typedef struct SamPoints{
	vector<int>	samIndex;			// �������������㼯�е�����
	vector<float> curvature;		// �����㼯������
	vector<int>	dire;				// �����㼯�������������
	vector<int> fgTipcIdx;			// ָ�������
	vector<Point> fgTipPts;			// ָ���
	vector<float> fgTipCurva;		// ָ�������
}SamPoints;
class HandGesture{
public:
	HandGesture();
	VideoCapture cap;	// ��ƵԴ
	Mat src_re;			// ���ź��ԭͼƬ

	Mat newpic;////////////��ʱ����
	int sample_jiange;

	Size newSize;		// ���ųߴ�
	int ImageLen;		// ͼ���С
	int ImageDiagonal;	// �Խ��߳���

	Point palm_center;	// ��������
	Rect bound_rect;	// �����߽�
	int palmAxisAng;	// ����Ƕ�
	float scale;		// ����ϵ��
	bool if_hand;		// �Ƿ�����
	bool if_skin_samp;	// �Ƿ����Ƥ������
	int cIdx;			// ������������
	vector<vector<Point> > contours;	// ���������㼯
	vector <Point> fingerTips;			// ָ������1
	//int mostFrequentFingerNumber;		// ��Ƶ������ָ��
	//int nrOfDefects;					// ͹����Ŀ
	//vector<vector<int> >hullI;			// ͹���㼯����
	//vector<vector<Point> >hullP;			// ͹���㼯
	//vector<vector<Vec4i> > defects;		// ͹��
	
	int init(float scale,int webCam);		// ��ʼ��
	int getSrcFrame();						// ��ȡԴ��Ƶ֡
	void makeContours(Mat src);				// ��ȡ����
	void palmCentrLoc(Mat src,Mat &dst);	// �����Ķ�λ����1��ͨ��������Բ�任
	void palmCentrLoc2(int cIdx,Mat &dst);	// �����Ķ�λ����2��ͨ��ͼ���
	int palmAxisAngle(vector<Point>  fgTps);// �����������б�Ƕ�
	void fingerTipLoc(vector<Point> &fg);	// ָ��㶨λ
	void sampleSkinColor(HDC hdc,CRect *pRect);
	bool detectIfHand();					// ����Ƿ�����

	void waitForPalmCover(HDC hdc,CRect *pRect);
	void average(HDC hdc,CRect *pRect);
	 // �ֲ�����ָ�
	void handSegmented(GMM gmm,bool if_use_gmm,bool if_use_cb);

	// �����滭��̬����
	static void myDrawContours(Mat src,HandGesture *hg);
	
	Mat src_thre;			// ��Ŷ�ֵͼƬ
private:
	Mat YCrCb;				// YCrCb�ռ�ָ��ֲ���mask
	Mat img_tmp;			// �м�ͼƬ
	vector<Mat> bwList;		// ��ֵͼ����

	Point hough_center;		// ��ǰ������Բ����
	Point lhough_center;	// ��һ��������Բ����
	Point bound_center;		// ��ǰ�����߽�����
	Point lbound_center;	// ��һ�������߽�����

	vector<int> fingerNumbers;

	void initColors();					 // ��ʼ����ɫ�ռ����
	void hand_YCrCb(Mat src);			 // YCrCb��ɫ����
	void skinColorSegHSV(Mat src,Mat &dst);// HSV�ռ��ɫ�ָ��1
	void skinColorSegHistogram(Mat src,Mat &dst);// HSV�ռ��ɫ�ָ��2
	int findBiggestContour();			 // �ҵ��������
	//void initVectors();					// �㼯��ʼ��
	//void checkForOneFinger(Mat src);		// ��ⵥ��ָ
	//void computeFingerNumber();			// ������ָ��Ŀ
	//void eliminateDefects(Mat src);		// ����
	//void addFingerNumberToVector();		// �����ָ��

	void ifingerTipLoc1(Mat src);						// ָ�ⶨλ����1
	void ifingerTipLoc2(Mat src,int L,SamPoints &samp);	// ָ�ⶨλ����2
	void normalizeColors();
	void curvaCompute(SamPoints &samp);					// ���ʼ���
	int  len_of_L(int handRecSize);						// ȷ��Pi���ھӵ����L�ĳ���
	float thred_of_curvature(int handRecSize);			// ȷ�����ʵ���ֵ

	// ��ɫ��������
	int avgColor[NSAMPLES][3] ;
	int c_lower[NSAMPLES][3];
	int c_upper[NSAMPLES][3];
	int avgBGR[3];
	vector <MyROI> roi;
public:
	// ��̬����
	static	int getMedian(vector<int> val);							// ��ȡ��λ��
	static	void getAvgColor(MyROI roi,int avg[3]);					// ��ȡƽ����ɫֵ
	
	static	float	distanceP2P(Point a, Point b);					// �������
	static	float	getAngle(Point s, Point f, Point e);			// ��ȡ����ļн�
	static	void	printText(Mat src, string text);				// ��ԭͼ���ϴ�ӡ����
	static	void	showToClient(Mat src,HDC hdc,CRect *pRect);		// ��ͼ����ʾ�������
	static  int		calcTilt(double mu11,double mu20,double mu02);	// ��������Ƕ�
	static  double	round(double r);								// �������뺯��

};


#endif