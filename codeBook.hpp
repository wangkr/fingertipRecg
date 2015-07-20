
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
// ����Ϊ�뱾��Ԫ�����ݽṹ
// ����ͼ��ʱÿ�����ض�Ӧһ���뱾,ÿ���뱾�п������ɸ���Ԫ
// ���漰һ��������,ͨ��������һЩ��ֵ�����,��Ҫ����Щ�����Ż�,��ʵ˼·���Ǽ򵥵�
typedef struct ce {
	uchar	learnHigh[CHANNELS];	// High side threshold for learning
	// ����Ԫ��ͨ���ķ�ֵ����(ѧϰ����)
	uchar	learnLow[CHANNELS];		// Low side threshold for learning
	// ����Ԫ��ͨ���ķ�ֵ����
	// ѧϰ���������һ�������ظ�ͨ��ֵx[i],���� learnLow[i]<=x[i]<=learnHigh[i],������ؿɺϲ��ڴ���Ԫ
	uchar	max[CHANNELS];			// High side of box boundary
	// ���ڴ���Ԫ�������и�ͨ�������ֵ
	uchar	min[CHANNELS];			// Low side of box boundary
	// ���ڴ���Ԫ�������и�ͨ������Сֵ
	int		t_last_update;			// This is book keeping to allow us to kill stale entries
	// ����Ԫ���һ�θ��µ�ʱ��,ÿһ֡Ϊһ����λʱ��,���ڼ���stale
	int		stale;					// max negative run (biggest period of inactivity)
	// ����Ԫ�������ʱ��,����ɾ���涨ʱ�䲻���µ���Ԫ,�����뱾
} code_element;						// ��Ԫ�����ݽṹ

typedef struct code_book {
	code_element	**cb;
	// ��Ԫ�Ķ�άָ��,���Ϊָ����Ԫָ�������ָ��,ʹ�������Ԫʱ����Ҫ���ظ�����Ԫ,ֻ��Ҫ�򵥵�ָ�븳ֵ����
	int				numEntries;
	// ���뱾����Ԫ����Ŀ
	int				t;				// count every access
	// ���뱾���ڵ�ʱ��,һ֡Ϊһ��ʱ�䵥λ
} codeBook;							// �뱾�����ݽṹ

	// ��ʼ������
	int wkrCB_init(int cam,double scale);
	// ǰ����ɫ�ָ��
	void wkrCB_frSkinSegment(Mat src,Mat &mask);
	// �������㺯������
	void wkrCB_zeroProgress();
	// ��ȡ������Ϣ
	int wkrCB_getProgress();
	// ֹͣģ��ѵ��
	void wkrCB_stopTrain();
	// ���ش�����
	bool wkrCB_isErr();
	// �ͷ��ڴ�
	void wkrCB_free();

	
	// Gray World ��ƽ�⴦����
	void grayWorld(Mat &srcImg);
	/////////////��ɫ������
	// ��ɫ�����б���
	uchar skinPixelDiff(uchar Cr,uchar Cb);
	// ��ɫ�ָ���ο��ע����
	void skinSegToRect(Mat srcImg,vector<Rect> &out,Mat &mask);
	// �ֲ����ε�ǰ���ָ��
	void frSkinSegInRect(codeBook *cb,Rect rect,Mat src,Mat &mask);
	// ��ɫǰ���ָ��
	void frSkinSeg(codeBook *cb, vector<Rect> bkSkinList,Mat src, Mat &mask);
	// �������ɫ������ǰ����ɫ���ι��˺���
	void frSkinFilter(codeBook *cb, vector<Rect> bkList,VideoCapture cap,Size sz,int &fcounter);
	/////////////////// codebook�㷨����
	// codebook���º���
	int cvupdateCodeBook(uchar *p, codeBook &c, unsigned *cbBounds, int numChannels);
	// ǰ�����������б���
	uchar cvbackgroundDiff(uchar *p, codeBook &c, int numChannels, int *minMod, int *maxMod);
	// ����¾ɵ�code
	int cvclearStaleEntries(codeBook &c);//��ÿһ���뱾���м��;
	int cvcountSegmentation(codeBook *c, Mat I, int numChannels, int *minMod, int *maxMod);
	void cvconnectedComponents(Mat mask, int poly1_hull0, float areaScale, int *num, Rect *bbs, Point *centers);
#endif