//GMM.hpp : GMM 类声明
#ifndef _GMM_
#define _GMM_

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;
using namespace std;

//For connected components:
#define CVCONTOUR_APPROX_LEVEL 2;   // Approx.threshold - the bigger it is, the simpler is the boundary
#define CVCLOSE_ITR  1;   
// 停止标记
static int if_stop = 0;
     
//定义gmm模型用到的变量
#define GMM_MAX_COMPONT 3
#define GMM_LEARN_ALPHA 0.01    //该学习率越大的话，学习速度太快，效果不好
#define GMM_THRESHOD_SUMW 0.7    //如果取值太大了的话，则更多树的部分都被检测出来了
#define END_FRAME 100
//进度更新要用到的变量
#define PROGRESS_TRAIN 1
#define PROGRESS_LOAD  2
#define PROGRESS_SAVE  3
class GMM{
public:
	// 构造函数
	GMM();
	// 析构函数
	~GMM();
	//训练背景模型函数声明
	int Train(int webCam,float scale); 
	//运动物体检测函数声明
	void Detect(Mat img,Mat &dst);
	// 返回模型训练进度
	int getProgress(int id);
	// 进度清零函数声明
	void zeroProgress(int id);
	// 停止模型训练
	void stopTrain();
	// 返回错误标记
	bool isErr();
	// 存储GMM背景模型参数到xml文件
	bool gmmParamsWrite(const char *filename);
	// 从xml文件读取GMM背景模型参数
	bool gmmParamsRead(const char *filename);
	Mat fit_num,gmask,foreground;
	Mat output_img;

private:
	//gmm初始化函数声明
	void init(Mat img);
	//gmm第一帧初始化函数声明
	void ifirst_frame(Mat img);
	//gmm训练过程函数声明
	void itrain(Mat img);
	//对输入图像每个像素gmm选择合适的个数
	void ifit_num(Mat img);
	//gmm测试函数的声明
	void itest(Mat img);
	//连通域去噪函数声明
	void ifind_connected_components(Mat img);

	// 更新进度函数声明
	void updateProgress(int id,int step);

private:
	Mat w[GMM_MAX_COMPONT];
	Mat u[GMM_MAX_COMPONT];
	Mat sigma[GMM_MAX_COMPONT];
	// 模型训练进度——用图片数显示
	int progress_train;
	// 模型参数加载进度
	int progress_load;
	// 模型参数保存进度
	int progress_save;
	// 中间变量
	float temp_w,temp_sigma;
	unsigned char temp_u;
	// 训练过程发生错误的标记
	bool if_err;
	// 外部请求停止训练标记
	bool if_stop;
};

#endif