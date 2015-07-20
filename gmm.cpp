#include "stdafx.h"
#include "gmm.hpp"
//Just some convienience macros
#define CV_CVX_WHITE    CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK    CV_RGB(0x00,0x00,0x00)
// 构造函数
GMM::GMM()
{
	progress_train = 0;
	progress_load = 0;
	progress_save = 0;
	if_stop = false;
	if_err = false;
}
GMM::~GMM()
{
}
// 训练背景模型函数实现
int GMM::Train(int webCam,float scale)
{
	VideoCapture cap;
	Size newSize;
	cap = VideoCapture(webCam);
	if(!cap.isOpened())
	{
		if_err = true;
		return -1;
	}
	// 获取图像并对其进行缩放——预处理
	Mat src,img,img_gray;
	cap>>src;
	newSize.width = src.cols*scale;
	newSize.height = src.rows*scale;
	resize(src,img,newSize);

	if(img.empty())       
	{
		if_err = true;
		cap.release();
        return -2;
	}

    cvtColor(img,img_gray,CV_BGR2GRAY);//covert the colorful image to the corresponding gray-level image

    init(img_gray);
    fit_num=Mat(img.size(),CV_8UC1,-1);//初始化为1
    gmask=Mat(img.size(),CV_8UC1,-1);
    foreground=img.clone();
	for(int i = 1;i <= END_FRAME;i++)
	{
		// 预处理
		cap>>src;
		resize(src,img,newSize);
		flip(img,img,1);
		// 转换为灰度图
		cvtColor(img,img_gray,CV_BGR2GRAY);
		if(i == 1)
		{
			ifirst_frame(img_gray);
			this->progress_train = 1;
		}
		if(1 < i < END_FRAME)
		{
			itrain(img_gray);
		}
		if(i == END_FRAME)
		{
			ifit_num(img_gray);
		}
		// 外部请求停止训练
		if(if_stop)
		{
			this->progress_train = END_FRAME;
			cap.release();
			return 2;
		}
		cvWaitKey(100);
		updateProgress(PROGRESS_TRAIN,1);// 更新进度
	}
	cap.release();
	return 1;
}
// 返回错误标记
bool GMM::isErr()
{
	return if_err;
}
// 更新训练进度函数实现
void GMM::updateProgress(int id,int step)
{
	switch(id)
	{
	case PROGRESS_TRAIN:
		this->progress_train += step;
		break;
	case PROGRESS_LOAD:
		this->progress_load += step;
		break;
	case PROGRESS_SAVE:
		this->progress_save += step;
		break;
	}
}
// 进度清零函数实现
void GMM::zeroProgress(int id)
{
	switch(id)
	{
	case PROGRESS_TRAIN:
		this->progress_train = 0;
		break;
	case PROGRESS_LOAD:
		this->progress_load = 0;
		break;
	case PROGRESS_SAVE:
		this->progress_save = 0;
		break;
	} // 进度清零
}
// 运动物体检测函数实现
void GMM::Detect(Mat img,Mat &dst)
{
	Mat img_gray;
	cvtColor(img,img_gray,CV_BGR2GRAY);
	itest(img_gray);
	ifind_connected_components(img_gray);
	dst = gmask;
}
// 获取训练进度函数实现
int GMM::getProgress(int id)
{
	switch(id)
	{
	case PROGRESS_TRAIN:
		return this->progress_train;
	case PROGRESS_LOAD:
		return this->progress_load;
	case PROGRESS_SAVE:
		return this->progress_save;
	}
	return 0;
	
}
void GMM::stopTrain()
{
	if_stop = true;
}
// 模型初始化函数实现
void GMM::init(Mat img)
{
	/****initialization the three parameters ****/
    for(int j=0;j<GMM_MAX_COMPONT;j++)
    {
        w[j]=Mat(img.size(),CV_32FC1,0.0);//CV_32FC1本身体现了正负符号
        u[j]=Mat(img.size(),CV_8UC1,-1);//为什么这里赋值为0时，后面的就一直出错？暂时还不知道原因，先赋值-1，其实内部存储的也是0
        sigma[j]=Mat(img.size(),CV_32FC1,0.0);//float类型
    }
}

// 第一帧初始化函数声明
void GMM::ifirst_frame(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)        
        {
            w[0].at<float>(m,n)=1.0;

            //if the pixvel is gray-clever,then we should use unsigned char,not the unsigned int
            u[0].at<unsigned char>(m,n)=img.at<unsigned char>(m,n);// 一定要注意其类型转换，否则会得不得预期的结果
            sigma[0].at<float>(m,n)=15.0;//opencv 自带的gmm代码中用的是15.0

            for(int k=1;k<GMM_MAX_COMPONT;k++)    
            {
                /****when assigment this,we must be very carefully****/
                w[k].at<float>(m,n)=0.0;
                u[k].at<unsigned char>(m,n)=-1;
                sigma[k].at<float>(m,n)=15.0;//防止后面排序时有分母为0的情况
            }
        }       
}

// 训练函数过程实现
void GMM::itrain(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            int k=0;
            int nfit=0;
            for(;k<GMM_MAX_COMPONT;k++)
            {
                int delam=abs(img.at<unsigned char>(m,n)-u[k].at<unsigned char>(m,n));//防止溢出
                long dis=delam*delam;
                if(dis<3.0*sigma[k].at<float>(m,n))//the present pixpel is fit the component
                {
                    /****update the weight****/
                    w[k].at<float>(m,n)=w[k].at<float>(m,n)+GMM_LEARN_ALPHA*(1-w[k].at<float>(m,n));

                    /****update the average****/
                    u[k].at<unsigned char>(m,n)=u[k].at<unsigned char>(m,n)+(GMM_LEARN_ALPHA/w[k].at<float>(m,n))*delam;

                    /****update the variance****/
                    sigma[k].at<float>(m,n)=sigma[k].at<float>(m,n)+(GMM_LEARN_ALPHA/w[k].at<float>(m,n))*(dis-sigma[k].at<float>(m,n));

    //                break;
                }
                else{
                    w[k].at<float>(m,n)=w[k].at<float>(m,n)+GMM_LEARN_ALPHA*(0-w[k].at<float>(m,n));
                    nfit++;
                }        
                //        }
            }

            //对gmm各个高斯进行排序,从大到小排序,排序依据为w/sigma
            for(int kk=0;kk<GMM_MAX_COMPONT;kk++)
            {
                for(int rr=kk;rr<GMM_MAX_COMPONT;rr++)
                {
                    //怎样才能做到gmm结构体整体排序呢？
                    if(w[rr].at<float>(m,n)/sigma[rr].at<float>(m,n)>w[kk].at<float>(m,n)/sigma[kk].at<float>(m,n))
                    {
                        //权值交换
                        temp_w=w[rr].at<float>(m,n);
                        w[rr].at<float>(m,n)=w[kk].at<float>(m,n);
                        w[kk].at<float>(m,n)=temp_w;

                        //均值交换
                        temp_u=u[rr].at<unsigned char>(m,n);
                        u[rr].at<unsigned char>(m,n)=u[kk].at<unsigned char>(m,n);
                        u[kk].at<unsigned char>(m,n)=temp_u;

                        //方差交换
                        temp_sigma=sigma[rr].at<float>(m,n);
                        sigma[rr].at<float>(m,n)=sigma[kk].at<float>(m,n);
                        sigma[kk].at<float>(m,n)=temp_sigma;
                    }
                }
            }

            //****如果没有满足条件的高斯，则重新开始算一个高斯分布****/
            if(nfit==GMM_MAX_COMPONT&&0==w[GMM_MAX_COMPONT-1].at<float>(m,n))//if there is no exit component fit,then start a new componen
            {
                //不能写为for(int h=0;h<MAX_GMM_COMPONT&&((0==w[h].at<float>(m,n)));h++)，因为这样明显h不会每次都加1
                for(int h=0;h<GMM_MAX_COMPONT;h++)
                {
                    if((0==w[h].at<float>(m,n)))
                    {
                        w[h].at<float>(m,n)=GMM_LEARN_ALPHA;//按照论文的参数来的
                        u[h].at<unsigned char>(m,n)=(unsigned char)img.at<unsigned char>(m,n);
                        sigma[h].at<float>(m,n)=15.0;//the opencv library code is 15.0
                        for(int q=0;q<GMM_MAX_COMPONT&&q!=h;q++)
                        {
                            /****update the other unfit's weight,u and sigma remain unchanged****/
                            w[q].at<float>(m,n)*=1-GMM_LEARN_ALPHA;//normalization the weight,let they sum to 1
                        }
                        break;//找到第一个权值不为0的即可
                    }                            
                }
            }
            //如果GMM_MAX_COMPONT都曾经赋值过，则用新来的高斯代替权值最弱的高斯，权值不变，只更新均值和方差
            else if(nfit==GMM_MAX_COMPONT&&w[GMM_MAX_COMPONT-1].at<float>(m,n)!=0)
            {
                u[GMM_MAX_COMPONT-1].at<unsigned char>(m,n)=(unsigned char)img.at<unsigned char>(m,n);
                sigma[GMM_MAX_COMPONT-1].at<float>(m,n)=15.0;//the opencv library code is 15.0
            }

            
        }
}

//对输入图像每个像素gmm选择合适的个数
void GMM::ifit_num(Mat img)
{
	//float sum_w=0.0;//重新赋值为0，给下一个像素做累积
    for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            float sum_w=0.0;//重新赋值为0，给下一个像素做累积
            //chose the fittest number fit_num
            for(unsigned char a=0;a<GMM_MAX_COMPONT;a++)
            {
                //cout<<w[a].at<float>(m,n)<<endl;
                sum_w+=w[a].at<float>(m,n);
                if(sum_w>=GMM_THRESHOD_SUMW)//如果这里THRESHOD_SUMW=0.6的话，那么得到的高斯数目都为1，因为每个像素都有一个权值接近1
                {
                    fit_num.at<unsigned char>(m,n)=a+1;
                    break;
                }
            }
        }
}

//gmm测试函数的实现
void GMM::itest(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            unsigned char a=0;
            for(;a<fit_num.at<unsigned char>(m,n);a++)
            {
                //如果对sigma取根号的话，树枝当做前景的概率会更大，不过人被检测出来的效果也更好些；用2相乘，不用开根号效果还不错
                if(abs(img.at<unsigned char>(m,n)-u[a].at<unsigned char>(m,n))<(unsigned char)(2.5*(sigma[a].at<float>(m,n))))
                {
                    gmask.at<unsigned char>(m,n)=0;//背景
                    break;
                }
            }
            if(a==fit_num.at<unsigned char>(m,n))
                gmask.at<unsigned char>(m,n)=255;//前景
        }
}

//连通域去噪函数声明
void GMM::ifind_connected_components(Mat img)
{
	morphologyEx(gmask,gmask,MORPH_OPEN,Mat());
}

// 将gmm模型参数存储到xml文件，存储成功并返回true，否则返回false
bool GMM::gmmParamsWrite(const char *filename)
{
	// 创建xml文件存储
	updateProgress(PROGRESS_LOAD,1);
	int step = (int)(90/(3*GMM_MAX_COMPONT));
	FileStorage fs(filename,FileStorage::WRITE);
	if(!fs.isOpened()){
		if_err = true;
		return false;
	}
	char temp[5];
	// 开始写数据
	// 矩阵w
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_w[10] = "W";
		_itoa_s(i,temp,10);
		strcat_s(temp_w,temp);
		fs<<temp_w<<w[i];
		updateProgress(PROGRESS_LOAD,step);
	}
	// 矩阵u
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_u[10] = "U";
		_itoa_s(i,temp,10);
		strcat_s(temp_u,temp);
		fs<<temp_u<<u[i];
		updateProgress(PROGRESS_LOAD,step);
	}
	// 矩阵sigma
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_sigma[10] = "SIGMA";
		_itoa_s(i,temp,10);
		strcat_s(temp_sigma,temp);
		fs<<temp_sigma<<sigma[i];
		updateProgress(PROGRESS_LOAD,step);
	}
	fs<<"FIT_NUM"<<fit_num;
	this->progress_load = 100;
	fs.release();
	return true;
}
// 从xml文件读取gmm模型参数，读取成功并返回true，否则返回false
bool GMM::gmmParamsRead(const char * filename)
{
	// 创建xml文件存储
	updateProgress(PROGRESS_SAVE,1);
	int step = (int)(90/(3*GMM_MAX_COMPONT));
	FileStorage fs(filename,FileStorage::READ);
	if(!fs.isOpened()){
		if_err = true;
		return false;
	}
	char temp[5];
	// 开始写数据
	// 矩阵w
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_w[10] = "W";
		_itoa_s(i,temp,10);
		strcat_s(temp_w,temp);
		fs[temp_w] >> w[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// 矩阵u
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_u[10] = "U";
		_itoa_s(i,temp,10);
		strcat_s(temp_u,temp);
		fs[temp_u] >> u[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// 矩阵sigma
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_sigma[10] = "SIGMA";
		_itoa_s(i,temp,10);
		strcat_s(temp_sigma,temp);
		fs[temp_sigma] >> sigma[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// 读取fit_num矩阵
	fs["FIT_NUM"]>>fit_num;
	// 记得初始化gmask
	gmask = Mat(fit_num.size(),CV_8UC1,-1);
	this->progress_save = 100;
	fs.release();
	return true;
}
