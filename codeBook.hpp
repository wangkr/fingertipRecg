
#ifndef _CODEBOOK_
#define _CODEBOOK_


#include <cv.h>                // define all of the opencv classes etc.
#include <opencv.hpp>
#include <highgui.h>
#include <cxcore.h>

#define CHANNELS 3
#define MAX_FRAME_STAGE1  50
#define MAX_FRAME_STAGE2  60
#define THETA_SIN  0.5741
#define THETA_COS  -0.8187
#define PARAM_A	   644.6521
#define PARAM_B    196.8409

//////
//For learning background

//Just some convienience macros
#define CV_CVX_WHITE    CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK    CV_RGB(0x00,0x00,0x00)
using namespace cv;
using namespace std;

///////////////////////////////////////////////////////////////////////////
// 下面为码本码元的数据结构
// 处理图像时每个像素对应一个码本,每个码本中可有若干个码元
// 当涉及一个新领域,通常会遇到一些奇怪的名词,不要被这些名词吓坏,其实思路都是简单的
typedef struct ce {
	uchar	learnHigh[CHANNELS];	// High side threshold for learning
	// 此码元各通道的阀值上限(学习界限)
	uchar	learnLow[CHANNELS];		// Low side threshold for learning
	// 此码元各通道的阀值下限
	// 学习过程中如果一个新像素各通道值x[i],均有 learnLow[i]<=x[i]<=learnHigh[i],则该像素可合并于此码元
	uchar	max[CHANNELS];			// High side of box boundary
	// 属于此码元的像素中各通道的最大值
	uchar	min[CHANNELS];			// Low side of box boundary
	// 属于此码元的像素中各通道的最小值
	int		t_last_update;			// This is book keeping to allow us to kill stale entries
	// 此码元最后一次更新的时间,每一帧为一个单位时间,用于计算stale
	int		stale;					// max negative run (biggest period of inactivity)
	// 此码元最长不更新时间,用于删除规定时间不更新的码元,精简码本
} code_element;						// 码元的数据结构

typedef struct code_book {
	code_element	**cb;
	// 码元的二维指针,理解为指向码元指针数组的指针,使得添加码元时不需要来回复制码元,只需要简单的指针赋值即可
	int				numEntries;
	// 此码本中码元的数目
	int				t;				// count every access
	// 此码本现在的时间,一帧为一个时间单位
} codeBook;							// 码本的数据结构

	// 初始化函数
	int wkrCB_init(int cam,double scale);
	// 前景肤色分割函数
	void wkrCB_frSkinSegment(Mat src,Mat &mask);
	// 进度清零函数声明
	void wkrCB_zeroProgress();
	// 获取进程信息
	int wkrCB_getProgress();
	// 停止模型训练
	void wkrCB_stopTrain();
	// 返回错误标记
	bool wkrCB_isErr();
	// 释放内存
	void wkrCB_free();

	
	// Gray World 白平衡处理函数
	void grayWorld(Mat &srcImg);
	/////////////肤色处理函数
	// 肤色像素判别函数
	uchar skinPixelDiff(uchar Cr,uchar Cb);
	// 肤色分割并矩形框标注函数
	void skinSegToRect(Mat srcImg,vector<Rect> &out,Mat &mask);
	// 局部矩形的前景分割函数
	void frSkinSegInRect(codeBook *cb,Rect rect,Mat src,Mat &mask);
	// 肤色前景分割函数
	void frSkinSeg(codeBook *cb, vector<Rect> bkSkinList,Mat src, Mat &mask);
	// 背景类肤色矩形中前景肤色矩形过滤函数
	void frSkinFilter(codeBook *cb, vector<Rect> bkList,VideoCapture cap,Size sz,int &fcounter);
	/////////////////// codebook算法函数
	// codebook更新函数
	int cvupdateCodeBook(uchar *p, codeBook &c, unsigned *cbBounds, int numChannels);
	// 前景背景像素判别函数
	uchar cvbackgroundDiff(uchar *p, codeBook &c, int numChannels, int *minMod, int *maxMod);
	// 清除陈旧的code
	int cvclearStaleEntries(codeBook &c);//对每一个码本进行检查;
	int cvcountSegmentation(codeBook *c, Mat I, int numChannels, int *minMod, int *maxMod);
	void cvconnectedComponents(Mat mask, int poly1_hull0, float areaScale, int *num, Rect *bbs, Point *centers);
#endif