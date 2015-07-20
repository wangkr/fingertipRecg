#include "stdafx.h"
#include "gmm.hpp"
//Just some convienience macros
#define CV_CVX_WHITE    CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK    CV_RGB(0x00,0x00,0x00)
// ���캯��
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
// ѵ������ģ�ͺ���ʵ��
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
	// ��ȡͼ�񲢶���������š���Ԥ����
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
    fit_num=Mat(img.size(),CV_8UC1,-1);//��ʼ��Ϊ1
    gmask=Mat(img.size(),CV_8UC1,-1);
    foreground=img.clone();
	for(int i = 1;i <= END_FRAME;i++)
	{
		// Ԥ����
		cap>>src;
		resize(src,img,newSize);
		flip(img,img,1);
		// ת��Ϊ�Ҷ�ͼ
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
		// �ⲿ����ֹͣѵ��
		if(if_stop)
		{
			this->progress_train = END_FRAME;
			cap.release();
			return 2;
		}
		cvWaitKey(100);
		updateProgress(PROGRESS_TRAIN,1);// ���½���
	}
	cap.release();
	return 1;
}
// ���ش�����
bool GMM::isErr()
{
	return if_err;
}
// ����ѵ�����Ⱥ���ʵ��
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
// �������㺯��ʵ��
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
	} // ��������
}
// �˶������⺯��ʵ��
void GMM::Detect(Mat img,Mat &dst)
{
	Mat img_gray;
	cvtColor(img,img_gray,CV_BGR2GRAY);
	itest(img_gray);
	ifind_connected_components(img_gray);
	dst = gmask;
}
// ��ȡѵ�����Ⱥ���ʵ��
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
// ģ�ͳ�ʼ������ʵ��
void GMM::init(Mat img)
{
	/****initialization the three parameters ****/
    for(int j=0;j<GMM_MAX_COMPONT;j++)
    {
        w[j]=Mat(img.size(),CV_32FC1,0.0);//CV_32FC1������������������
        u[j]=Mat(img.size(),CV_8UC1,-1);//Ϊʲô���︳ֵΪ0ʱ������ľ�һֱ������ʱ����֪��ԭ���ȸ�ֵ-1����ʵ�ڲ��洢��Ҳ��0
        sigma[j]=Mat(img.size(),CV_32FC1,0.0);//float����
    }
}

// ��һ֡��ʼ����������
void GMM::ifirst_frame(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)        
        {
            w[0].at<float>(m,n)=1.0;

            //if the pixvel is gray-clever,then we should use unsigned char,not the unsigned int
            u[0].at<unsigned char>(m,n)=img.at<unsigned char>(m,n);// һ��Ҫע��������ת���������ò���Ԥ�ڵĽ��
            sigma[0].at<float>(m,n)=15.0;//opencv �Դ���gmm�������õ���15.0

            for(int k=1;k<GMM_MAX_COMPONT;k++)    
            {
                /****when assigment this,we must be very carefully****/
                w[k].at<float>(m,n)=0.0;
                u[k].at<unsigned char>(m,n)=-1;
                sigma[k].at<float>(m,n)=15.0;//��ֹ��������ʱ�з�ĸΪ0�����
            }
        }       
}

// ѵ����������ʵ��
void GMM::itrain(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            int k=0;
            int nfit=0;
            for(;k<GMM_MAX_COMPONT;k++)
            {
                int delam=abs(img.at<unsigned char>(m,n)-u[k].at<unsigned char>(m,n));//��ֹ���
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

            //��gmm������˹��������,�Ӵ�С����,��������Ϊw/sigma
            for(int kk=0;kk<GMM_MAX_COMPONT;kk++)
            {
                for(int rr=kk;rr<GMM_MAX_COMPONT;rr++)
                {
                    //������������gmm�ṹ�����������أ�
                    if(w[rr].at<float>(m,n)/sigma[rr].at<float>(m,n)>w[kk].at<float>(m,n)/sigma[kk].at<float>(m,n))
                    {
                        //Ȩֵ����
                        temp_w=w[rr].at<float>(m,n);
                        w[rr].at<float>(m,n)=w[kk].at<float>(m,n);
                        w[kk].at<float>(m,n)=temp_w;

                        //��ֵ����
                        temp_u=u[rr].at<unsigned char>(m,n);
                        u[rr].at<unsigned char>(m,n)=u[kk].at<unsigned char>(m,n);
                        u[kk].at<unsigned char>(m,n)=temp_u;

                        //�����
                        temp_sigma=sigma[rr].at<float>(m,n);
                        sigma[rr].at<float>(m,n)=sigma[kk].at<float>(m,n);
                        sigma[kk].at<float>(m,n)=temp_sigma;
                    }
                }
            }

            //****���û�����������ĸ�˹�������¿�ʼ��һ����˹�ֲ�****/
            if(nfit==GMM_MAX_COMPONT&&0==w[GMM_MAX_COMPONT-1].at<float>(m,n))//if there is no exit component fit,then start a new componen
            {
                //����дΪfor(int h=0;h<MAX_GMM_COMPONT&&((0==w[h].at<float>(m,n)));h++)����Ϊ��������h����ÿ�ζ���1
                for(int h=0;h<GMM_MAX_COMPONT;h++)
                {
                    if((0==w[h].at<float>(m,n)))
                    {
                        w[h].at<float>(m,n)=GMM_LEARN_ALPHA;//�������ĵĲ�������
                        u[h].at<unsigned char>(m,n)=(unsigned char)img.at<unsigned char>(m,n);
                        sigma[h].at<float>(m,n)=15.0;//the opencv library code is 15.0
                        for(int q=0;q<GMM_MAX_COMPONT&&q!=h;q++)
                        {
                            /****update the other unfit's weight,u and sigma remain unchanged****/
                            w[q].at<float>(m,n)*=1-GMM_LEARN_ALPHA;//normalization the weight,let they sum to 1
                        }
                        break;//�ҵ���һ��Ȩֵ��Ϊ0�ļ���
                    }                            
                }
            }
            //���GMM_MAX_COMPONT��������ֵ�������������ĸ�˹����Ȩֵ�����ĸ�˹��Ȩֵ���䣬ֻ���¾�ֵ�ͷ���
            else if(nfit==GMM_MAX_COMPONT&&w[GMM_MAX_COMPONT-1].at<float>(m,n)!=0)
            {
                u[GMM_MAX_COMPONT-1].at<unsigned char>(m,n)=(unsigned char)img.at<unsigned char>(m,n);
                sigma[GMM_MAX_COMPONT-1].at<float>(m,n)=15.0;//the opencv library code is 15.0
            }

            
        }
}

//������ͼ��ÿ������gmmѡ����ʵĸ���
void GMM::ifit_num(Mat img)
{
	//float sum_w=0.0;//���¸�ֵΪ0������һ���������ۻ�
    for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            float sum_w=0.0;//���¸�ֵΪ0������һ���������ۻ�
            //chose the fittest number fit_num
            for(unsigned char a=0;a<GMM_MAX_COMPONT;a++)
            {
                //cout<<w[a].at<float>(m,n)<<endl;
                sum_w+=w[a].at<float>(m,n);
                if(sum_w>=GMM_THRESHOD_SUMW)//�������THRESHOD_SUMW=0.6�Ļ�����ô�õ��ĸ�˹��Ŀ��Ϊ1����Ϊÿ�����ض���һ��Ȩֵ�ӽ�1
                {
                    fit_num.at<unsigned char>(m,n)=a+1;
                    break;
                }
            }
        }
}

//gmm���Ժ�����ʵ��
void GMM::itest(Mat img)
{
	for(int m=0;m<img.rows;m++)
        for(int n=0;n<img.cols;n++)
        {
            unsigned char a=0;
            for(;a<fit_num.at<unsigned char>(m,n);a++)
            {
                //�����sigmaȡ���ŵĻ�����֦����ǰ���ĸ��ʻ���󣬲����˱���������Ч��Ҳ����Щ����2��ˣ����ÿ�����Ч��������
                if(abs(img.at<unsigned char>(m,n)-u[a].at<unsigned char>(m,n))<(unsigned char)(2.5*(sigma[a].at<float>(m,n))))
                {
                    gmask.at<unsigned char>(m,n)=0;//����
                    break;
                }
            }
            if(a==fit_num.at<unsigned char>(m,n))
                gmask.at<unsigned char>(m,n)=255;//ǰ��
        }
}

//��ͨ��ȥ�뺯������
void GMM::ifind_connected_components(Mat img)
{
	morphologyEx(gmask,gmask,MORPH_OPEN,Mat());
}

// ��gmmģ�Ͳ����洢��xml�ļ����洢�ɹ�������true�����򷵻�false
bool GMM::gmmParamsWrite(const char *filename)
{
	// ����xml�ļ��洢
	updateProgress(PROGRESS_LOAD,1);
	int step = (int)(90/(3*GMM_MAX_COMPONT));
	FileStorage fs(filename,FileStorage::WRITE);
	if(!fs.isOpened()){
		if_err = true;
		return false;
	}
	char temp[5];
	// ��ʼд����
	// ����w
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_w[10] = "W";
		_itoa_s(i,temp,10);
		strcat_s(temp_w,temp);
		fs<<temp_w<<w[i];
		updateProgress(PROGRESS_LOAD,step);
	}
	// ����u
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_u[10] = "U";
		_itoa_s(i,temp,10);
		strcat_s(temp_u,temp);
		fs<<temp_u<<u[i];
		updateProgress(PROGRESS_LOAD,step);
	}
	// ����sigma
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
// ��xml�ļ���ȡgmmģ�Ͳ�������ȡ�ɹ�������true�����򷵻�false
bool GMM::gmmParamsRead(const char * filename)
{
	// ����xml�ļ��洢
	updateProgress(PROGRESS_SAVE,1);
	int step = (int)(90/(3*GMM_MAX_COMPONT));
	FileStorage fs(filename,FileStorage::READ);
	if(!fs.isOpened()){
		if_err = true;
		return false;
	}
	char temp[5];
	// ��ʼд����
	// ����w
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_w[10] = "W";
		_itoa_s(i,temp,10);
		strcat_s(temp_w,temp);
		fs[temp_w] >> w[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// ����u
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_u[10] = "U";
		_itoa_s(i,temp,10);
		strcat_s(temp_u,temp);
		fs[temp_u] >> u[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// ����sigma
	for(int i = 0;i < GMM_MAX_COMPONT;i++)
	{
		char temp_sigma[10] = "SIGMA";
		_itoa_s(i,temp,10);
		strcat_s(temp_sigma,temp);
		fs[temp_sigma] >> sigma[i];
		updateProgress(PROGRESS_SAVE,step);
	}
	// ��ȡfit_num����
	fs["FIT_NUM"]>>fit_num;
	// �ǵó�ʼ��gmask
	gmask = Mat(fit_num.size(),CV_8UC1,-1);
	this->progress_save = 100;
	fs.release();
	return true;
}
