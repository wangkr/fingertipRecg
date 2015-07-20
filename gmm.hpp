//GMM.hpp : GMM ������
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
// ֹͣ���
static int if_stop = 0;
     
//����gmmģ���õ��ı���
#define GMM_MAX_COMPONT 3
#define GMM_LEARN_ALPHA 0.01    //��ѧϰ��Խ��Ļ���ѧϰ�ٶ�̫�죬Ч������
#define GMM_THRESHOD_SUMW 0.7    //���ȡֵ̫���˵Ļ�����������Ĳ��ֶ�����������
#define END_FRAME 100
//���ȸ���Ҫ�õ��ı���
#define PROGRESS_TRAIN 1
#define PROGRESS_LOAD  2
#define PROGRESS_SAVE  3
class GMM{
public:
	// ���캯��
	GMM();
	// ��������
	~GMM();
	//ѵ������ģ�ͺ�������
	int Train(int webCam,float scale); 
	//�˶������⺯������
	void Detect(Mat img,Mat &dst);
	// ����ģ��ѵ������
	int getProgress(int id);
	// �������㺯������
	void zeroProgress(int id);
	// ֹͣģ��ѵ��
	void stopTrain();
	// ���ش�����
	bool isErr();
	// �洢GMM����ģ�Ͳ�����xml�ļ�
	bool gmmParamsWrite(const char *filename);
	// ��xml�ļ���ȡGMM����ģ�Ͳ���
	bool gmmParamsRead(const char *filename);
	Mat fit_num,gmask,foreground;
	Mat output_img;

private:
	//gmm��ʼ����������
	void init(Mat img);
	//gmm��һ֡��ʼ����������
	void ifirst_frame(Mat img);
	//gmmѵ�����̺�������
	void itrain(Mat img);
	//������ͼ��ÿ������gmmѡ����ʵĸ���
	void ifit_num(Mat img);
	//gmm���Ժ���������
	void itest(Mat img);
	//��ͨ��ȥ�뺯������
	void ifind_connected_components(Mat img);

	// ���½��Ⱥ�������
	void updateProgress(int id,int step);

private:
	Mat w[GMM_MAX_COMPONT];
	Mat u[GMM_MAX_COMPONT];
	Mat sigma[GMM_MAX_COMPONT];
	// ģ��ѵ�����ȡ�����ͼƬ����ʾ
	int progress_train;
	// ģ�Ͳ������ؽ���
	int progress_load;
	// ģ�Ͳ����������
	int progress_save;
	// �м����
	float temp_w,temp_sigma;
	unsigned char temp_u;
	// ѵ�����̷�������ı��
	bool if_err;
	// �ⲿ����ֹͣѵ�����
	bool if_stop;
};

#endif