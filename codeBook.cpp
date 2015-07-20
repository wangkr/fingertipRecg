#include "stdafx.h"
#include "codeBook.hpp"

long int sizeofcb;

int maxMod[CHANNELS] = {25,8,8};
int minMod[CHANNELS] = {35,8,8};

unsigned cbBounds[CHANNELS] = {10,10,10};
bool	ch[CHANNELS] = {true,true,true};

codeBook *cb;

int nrows,ncols;
int nChannels;
int ImageLen;
Size newSZ;
vector<Rect> pBackSkinList;

int progress;
bool if_stop;
bool if_err;

//wkrCodeBook::wkrCodeBook()
//{
//	// 进度初始化
//	progress = 0;
//
//	if_stop = false;
//	if_err = false;
//
//	cb = NULL;
//}
//wkrCodeBook::~wkrCodeBook()
//{
//	if(cb)
//	{
//		for(int i = 0;i < ImageLen;i++)
//		{
//			for(int j = 0;j < cb[i].numEntries;j++)
//			{
//				delete[] cb[i].cb[j];
//			}
//			delete[] cb[i].cb;
//		}
//		delete[] cb;
//		cb = 0;
//	}
//}
// codebook模型初始化，即建模函数
void wkrCB_free()
{
	if(cb)
	{
		for(int i = 0;i < ImageLen;i++)
		{
			for(int j = 0;j < cb[i].numEntries;j++)
			{
				delete[] cb[i].cb[j];
			}
			delete[] cb[i].cb;
		}
		delete[] cb;
		cb = 0;
	}
}
int wkrCB_init(int cam,double scale)
{
	///////////////// 进度初始化
	progress = 0;

	if_stop = false;
	if_err = false;

	cb = NULL;
	VideoCapture cap = VideoCapture(cam);
	if(!cap.isOpened())
	{
		if_err = true;
		return -1;
	}

	Mat srcTemp,src,src1,dst,mask,yuvImg;
	uchar *pData = NULL;
	/////// 初始化类变量
	cap >> srcTemp;
	Size sz = srcTemp.size();
	//图像尺寸
	newSZ.width = sz.width*scale;newSZ.height = sz.height*scale;

	nrows = newSZ.height;ncols = newSZ.width;
	//颜色通道数
	nChannels = srcTemp.channels();
	
	if(srcTemp.isContinuous())
	{
		ncols*=nrows;
		nrows = 1;
	}
	//图像大小
	ImageLen = ncols*nrows;
	sizeofcb = 0;
	// 初始化
	cb = new codeBook[ImageLen];
	sizeofcb += sizeof(codeBook)*ImageLen;
	
	for(int f = 0; f<ImageLen; f++)
	{
		cb[f].numEntries = 0;cb[f].t = 0;
	}
	while(progress <= MAX_FRAME_STAGE2)
	{
		cap >> srcTemp;
		resize(srcTemp,src,newSZ);
		flip(src,src,1);
		grayWorld(src);
		progress ++;
		if(progress < MAX_FRAME_STAGE1)
		{
			cvtColor(src,yuvImg,CV_BGR2YCrCb);
			for(int r = 0;r < nrows;r ++)
			{
				pData = (uchar*)yuvImg.ptr<uchar>(r);
				for(int c = 0;c < ncols;c ++)
				{
					///////////////////////new 和 delete的问题亟待解决
					cvupdateCodeBook(pData,cb[r*ncols+c],cbBounds,nChannels);
					pData += 3;
				}
			}
		}
		if(progress == MAX_FRAME_STAGE1)
		{
			for(int c = 0;c < ImageLen;c++)
				cvclearStaleEntries(cb[c]);
			// 初步建立类肤色背景矩形列表
			skinSegToRect(src,pBackSkinList,dst);
		}
		if(progress > MAX_FRAME_STAGE1 && progress <= MAX_FRAME_STAGE2)
		{
			// 过滤掉前景肤色矩形框
			frSkinFilter(cb,pBackSkinList,cap,newSZ,progress);
		}
		if(if_stop)
		{
			progress = MAX_FRAME_STAGE2 + 1;
			return 1;
		}
	}
	cap.release();
	return 0;
}
// 前景肤色分割函数
void wkrCB_frSkinSegment(Mat src,Mat &mask)
{
	frSkinSeg(cb,pBackSkinList,src,mask);
}

// 获取进程信息
int wkrCB_getProgress()
{
	return progress;
}
// 进度清零函数声明
void wkrCB_zeroProgress()
{
	progress = 0;
}
// 停止模型训练
void wkrCB_stopTrain()
{
	if_stop = true;
}
// 返回错误标记
bool wkrCB_isErr()
{
	return if_err;
}


uchar skinPixelDiff(uchar Cr,uchar Cb)
{
	double temp[2];
	temp[0] = (Cb-109.38)*THETA_COS    + (Cr-152.02)*THETA_SIN;
	temp[1] = (Cb-109.38)*(-THETA_SIN) + (Cr-152.02)*THETA_COS;
	double re = pow((temp[0]-1.6),2)/PARAM_A+pow((temp[1]-2.41),2)/PARAM_B;
	if(re <= 1.0) return 255;
	else return 0;
}
void grayWorld(Mat &srcImg)
{
	/************************************************************************/
	/* 利用grayworld进行自动白平衡运算，pImg是RGB深度为24的图片             */
	/************************************************************************/
	uchar *pSrc = NULL;
	double avgB, avgG, avgR, avgGray;
	double b, g, r;
	double kB, kG, kR;
	int width, height;
	//取得平均值
	Scalar avgRGB = mean(srcImg);
	avgB = avgRGB.val[0];
	avgG = avgRGB.val[1];
	avgR = avgRGB.val[2];
	//取得平均灰度值
	avgGray = ( avgB + avgG + avgR ) / 3;
	kB = avgGray / avgB;
	kG = avgGray / avgG;
	kR = avgGray / avgR;
	
	width = srcImg.cols;
	height = srcImg.rows;
	for (int i=0; i<height; i++)
	{
		pSrc = (uchar*)srcImg.ptr(i);
		for (int j=0; j<width; j++)
		{
			b = *(pSrc+j*3),g = *(pSrc+j*3+1),r = *(pSrc+j*3+2);
			b*=kB; g*=kG; r*=kR;
			//检查是否超出范围
			b=b>255? 255: b;
			g=g>255? 255: g;
			r=r>255? 255: r;
			*(pSrc+j*3)=(int)b;
			*(pSrc+j*3+1)=(int)g;
			*(pSrc+j*3+2)=(int)r;
		}
	}	
}
// 肤色分割函数
void skinSegToRect(Mat srcImg,vector<Rect> &out,Mat &mask)
{
	Mat yuvImg;
	uchar *pMask,*pSrc;
	mask = Mat::zeros(srcImg.size(),CV_8UC1);
//	skinSegFunc(srcImg,mask);
	// 分割类肤色区域
	cvtColor(srcImg,yuvImg,CV_BGR2YCrCb);
	for(int i = 0;i < nrows;i++)
	{
		pMask = (uchar *)mask.ptr(i);
		pSrc = (uchar *)yuvImg.ptr(i);
		for(int j = 0;j < ncols;j++)
		{
			*pMask++=skinPixelDiff(*(pSrc+1),*(pSrc+2));
			pSrc += 3;
		}
	}
	vector <vector<Point>> contours;
	// 背景除噪
	//CLEAN UP RAW MASK
	Mat element = getStructuringElement(MORPH_RECT,Size(3,3));
	//开运算作用：平滑轮廓，去掉细节,断开缺口
    morphologyEx( mask, mask, CV_MOP_OPEN, element);//对输入mask进行开操作，CVCLOSE_ITR为开操作的次数，输出为mask图像
	//闭运算作用：平滑轮廓，连接缺口
	element = getStructuringElement(MORPH_RECT,Size(5,5));
    morphologyEx( mask, mask, CV_MOP_CLOSE, element);;//对输入mask进行闭操作，CVCLOSE_ITR为闭操作的次数，输出为mask图像

	findContours(mask,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
	mask = Mat::zeros(srcImg.size(),CV_8UC1);
	int ImgArea = mask.size().area();
	double AreaTH = ImgArea * 0.005;
	for(int c = 0; c < contours.size();c++)
	{
		double area = abs(contourArea(contours[c]));
		if(area > AreaTH)
		{
			Rect rect;
			vector<Point> c_new;
			approxPolyDP(contours[c],c_new,1,0);
			contours[c] = c_new;
			drawContours(mask,contours,c,CV_RGB(255,255,255),CV_FILLED,8);
			rect = boundingRect(c_new);
			out.push_back(rect);
		}
	}
}
// 局部矩形的前景分割函数
void frSkinSegInRect(codeBook *cb,Rect rect,Mat src,Mat &mask)
{
	uchar *pMask = NULL,*pSrc = NULL;
	uchar frPixel,skinPixel;
	Mat yuvImg;
	int start_row = rect.tl().y,start_col = rect.tl().x;
	int end_row = rect.br().y,end_col = rect.br().x;
	int cols = src.cols;
	Mat roi = Mat(src,rect);
	cvtColor(roi,yuvImg,CV_BGR2YCrCb);
	for(int r = start_row,r1 = 0;r < end_row;r++,r1++)
	{
		pSrc = (uchar *)yuvImg.ptr(r1);
		pMask = (uchar *)(mask.ptr(r)+ start_col);
		for(int c = start_col;c < end_col;c++)
		{
			if(*pMask)*pMask = cvbackgroundDiff(pSrc,cb[r*cols+c],nChannels,minMod,maxMod);
			pMask ++;
			pSrc += 3;
		}
	}
}
// 肤色前景分割函数
void frSkinSeg(codeBook *cb, vector<Rect> bkSkinList,Mat src, Mat &mask)
{
	vector<Rect> skinList;
	// 提取前景肤色
	skinSegToRect(src,skinList,mask);
	int isSeg = 0;
	for(int l = 0;l < skinList.size();l++)
	{
		isSeg = 0;
		for(int b = 0;b < bkSkinList.size();b++)
		{
			// 判断矩形是否相交
			int detax,detay;
			detax = skinList[l].tl().x - bkSkinList[b].tl().x;
			detay = skinList[l].tl().y - bkSkinList[b].tl().y;
			// 如果已经进行分割过了，则跳出循环，加速。
			if(isSeg == 1)
				break;
			// 如果不相交则继续
			if((detax > 0&& detax >= bkSkinList[b].width)||(detax < 0 && -detax >= skinList[l].width))
				continue;
			if((detay > 0&& detay >= bkSkinList[b].height)||(detay < 0&& -detay >= skinList[l].height))
				continue;
			frSkinSegInRect(cb,skinList[l],src,mask);
			// 标记该前景矩形已经进行前景分割了
			isSeg = 1;
		}
	}
	Mat element = getStructuringElement(MORPH_RECT,Size(5,5));
	morphologyEx( mask, mask, CV_MOP_CLOSE, element);

}
// 背景类肤色矩形中前景肤色矩形过滤函数
void frSkinFilter(codeBook *cb, vector<Rect> bkList,VideoCapture cap,Size sz,int &fcounter)
{
	Mat tempSrc,src;
	Mat yuvImg;

	int nr = sz.height,nc = sz.width;
	uchar *pData = NULL;
	// 初始化每个bkSkin矩形框中的frSkin统计数据
	vector<Vec2i> frSkinPixelstat;
	Vec2i temp;
	for(int i = 0;i < bkList.size();i++)
	{
		temp[0] = bkList[i].width*bkList[i].height;
		temp[1] = 0;
		frSkinPixelstat.push_back(temp);
	}
	
	while(fcounter  <= MAX_FRAME_STAGE2){
		cap >> tempSrc;
		resize(tempSrc,src,sz);
		grayWorld(src);
		flip(src,src,1);
		cvtColor(src,yuvImg,CV_BGR2YCrCb);
		// 利用codebook分割前景像素
		for(int r = 0;r < nr;r ++)
		{
			pData = (uchar*)yuvImg.ptr<uchar>(r);
			for(int c = 0;c < nc;c ++)
			{
				if(cvbackgroundDiff(pData,cb[r*nc+c],nChannels,minMod,maxMod))
				{
					for(int l = 0;l < bkList.size();l++)
						if(bkList[l].contains(Point(c,r)))
							frSkinPixelstat[l][1]++;
				}
				pData += 3;
			}// end for
		}// end for
		// 如果bkList中某一矩形对应的前景像素的累加值大于该矩形中总像素值得10%，则删除该矩形
		vector<Vec2i>::iterator itr2i;
		vector<Rect>::iterator itrRect;
		for(itrRect = bkList.begin(),itr2i = frSkinPixelstat.begin();\
			itrRect != bkList.end()&&itr2i != frSkinPixelstat.end();)
		{
			double rate = (*itr2i)[1]*1.0/(*itr2i)[0];
			if( rate >= 0.10){
				itrRect = bkList.erase(itrRect);
				itr2i = frSkinPixelstat.erase(itr2i);
			}
			else{
				// 统计清理
				(*itr2i)[1] = 0;
				itrRect ++;
				itr2i ++;
			}
		}// end for
		fcounter++;
		if(if_stop)
		{
			fcounter = MAX_FRAME_STAGE2+1;
			return;
		}
	}// end while

}


///////////////////////////////////////////////////////////////////////////////////
// int updateCodeBook(uchar *p, codeBook &c, unsigned cbBounds)
// Updates the codebook entry with a new data point
//
// p            Pointer to a YUV pixel
// c            Codebook for this pixel
// cbBounds        Learning bounds for codebook (Rule of thumb: 10)
// numChannels    Number of color channels we're learning
//
// NOTES:
//        cvBounds must be of size cvBounds[numChannels]
//
// RETURN
//    codebook index
int cvupdateCodeBook(uchar *p, codeBook &c, unsigned *cbBounds, int numChannels)
{

    if(c.numEntries == 0) c.t = 0;//说明每个像素如果遍历了的话至少对应一个码元
    c.t += 1;        //Record learning event，遍历该像素点的次数加1
//SET HIGH AND LOW BOUNDS
    int n;
    unsigned int high[3],low[3];
    for(n=0; n<numChannels; n++)//为该像素点的每个通道设置最大阈值和最小阈值，后面用来更新学习的高低阈值时有用
    {
        high[n] = *(p+n)+*(cbBounds+n);
        if(high[n] > 255) high[n] = 255;
        low[n] = *(p+n)-*(cbBounds+n);
        if(low[n] < 0) low[n] = 0;
    }
    int matchChannel;
    //SEE IF THIS FITS AN EXISTING CODEWORD
    int i;
    for(i=0; i<c.numEntries; i++)//需要对所有的码元进行扫描
    {
        matchChannel = 0;
        for(n=0; n<numChannels; n++)
        {
            //这个地方要非常小心，if条件不是下面表达的
//if((c.cb[i]->min[n]-c.cb[i]->learnLow[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->max[n]+c.cb[i]->learnHigh[n]))
//原因是因为在每次建立一个新码元的时候，learnHigh[n]和learnLow[n]的范围就在max[n]和min[n]上扩展了cbBounds[n]，所以说
//learnHigh[n]和learnLow[n]的变化范围实际上比max[n]和min[n]的大
            if((c.cb[i]->learnLow[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->learnHigh[n])) //Found an entry for this channel
            {
                matchChannel++;
            }
        }
        if(matchChannel == numChannels) //If an entry was found over all channels，找到了该元素此刻对应的码元
        {
            c.cb[i]->t_last_update = c.t;
            //adjust this codeword for the first channel
//更新每个码元的最大最小阈值，因为这2个阈值在后面的前景分离过程要用到
            for(n=0; n<numChannels; n++)
            {
                if(c.cb[i]->max[n] < *(p+n))//用该点的像素值更新该码元的最大值，所以max[n]保存的是实际上历史出现过的最大像素值
                {
                    c.cb[i]->max[n] = *(p+n);//因为这个for语句是在匹配成功了的条件阈值下的，所以一般来说改变后的max[n]和min[n]
//也不会过学习的高低阈值，并且学习的高低阈值也一直在缓慢变化  
                }
                else if(c.cb[i]->min[n] > *(p+n))//用该点的像素值更新该码元的最小值，所以min[n]保存的是实际上历史出现过的最小像素值
                {
                    c.cb[i]->min[n] = *(p+n);
                }
            }
            break;//一旦找到了该像素的一个码元后就不用继续往后找了，加快算法速度。因为最多只有一个码元与之对应
        }
    }

    //OVERHEAD TO TRACK POTENTIAL STALE ENTRIES
    for(int s=0; s<c.numEntries; s++)
    {
        //This garbage is to track which codebook entries are going stale
        int negRun = c.t - c.cb[s]->t_last_update;//negRun表示码元没有更新的时间间隔
        if(c.cb[s]->stale < negRun) c.cb[s]->stale = negRun;//更新每个码元的statle
    }


    //ENTER A NEW CODE WORD IF NEEDED
    if(i == c.numEntries)  //No existing code word found, make a new one，只有当该像素码本中的所有码元都不符合要求时才满足if条件
    {
        code_element **foo = new code_element* [c.numEntries+1];//创建一个新的码元序列
        for(int ii=0; ii<c.numEntries; ii++)
        {
            foo[ii] = c.cb[ii];//将码本前面所有的码元地址赋给foo
        }
        foo[c.numEntries] = new code_element;//创建一个新码元并赋给foo指针的下一个空位
		sizeofcb += sizeof(code_element);
        if(c.numEntries) {
			delete [] c.cb;//
		}
        c.cb = foo;
        for(n=0; n<numChannels; n++)//给新建立的码元结构体元素赋值
        {
            c.cb[c.numEntries]->learnHigh[n] = high[n];//当建立一个新码元时，用当前值附近cbBounds范围作为码元box的学习阈值
            c.cb[c.numEntries]->learnLow[n] = low[n];
            c.cb[c.numEntries]->max[n] = *(p+n);//当建立一个新码元时，用当前值作为码元box的最大最小边界值
            c.cb[c.numEntries]->min[n] = *(p+n);
        }
        c.cb[c.numEntries]->t_last_update = c.t;
        c.cb[c.numEntries]->stale = 0;//因为刚建立，所有为0
        c.numEntries += 1;//码元的个数加1
    }

    // SLOWLY ADJUST LEARNING BOUNDS
    for(n=0; n<numChannels; n++)//每次遍历该像素点就将每个码元的学习最大阈值变大，最小阈值变小，但是都是缓慢变化的
    {                           //如果是新建立的码元，则if条件肯定不满足
        if(c.cb[i]->learnHigh[n] < high[n]) c.cb[i]->learnHigh[n] += 1;                
        if(c.cb[i]->learnLow[n] > low[n]) c.cb[i]->learnLow[n] -= 1;
    }

    return(i);//返回所找到码本中码元的索引
}

///////////////////////////////////////////////////////////////////////////////////
// uchar cvbackgroundDiff(uchar *p, codeBook &c, int minMod, int maxMod)
// Given a pixel and a code book, determine if the pixel is covered by the codebook
//
// p        pixel pointer (YUV interleaved)
// c        codebook reference
// numChannels  Number of channels we are testing
// maxMod    Add this (possibly negative) number onto max level when code_element determining if new pixel is foreground
// minMod    Subract this (possible negative) number from min level code_element when determining if pixel is foreground
//
// NOTES:
// minMod and maxMod must have length numChannels, e.g. 3 channels => minMod[3], maxMod[3].
//
// Return
// 0 => background, 255 => foreground
uchar cvbackgroundDiff(uchar *p, codeBook &c, int numChannels, int *minMod, int *maxMod)
{
    int matchChannel;
    //SEE IF THIS FITS AN EXISTING CODEWORD
    int i;
    for(i=0; i<c.numEntries; i++)
    {
        matchChannel = 0;
        for(int n=0; n<numChannels; n++)
        {
            if((c.cb[i]->min[n] - minMod[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->max[n] + maxMod[n]))
            {
                matchChannel++; //Found an entry for this channel
            }
            else
            {
                break;//加快速度，当一个通道不满足时提前结束
            }
        }
        if(matchChannel == numChannels)
        {
            break; //Found an entry that matched all channels，加快速度，当一个码元找到时，提前结束
        }
    }
    if(i >= c.numEntries) return(255);//255代表前景，因为所有的码元都不满足条件
    return(0);//0代表背景，因为至少有一个码元满足条件
}


//UTILITES/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//int clearStaleEntries(codeBook &c)
// After you've learned for some period of time, periodically call this to clear out stale codebook entries
//
//c        Codebook to clean up
//
// Return
// number of entries cleared
int cvclearStaleEntries(codeBook &c)//对每一个码本进行检查
{
    int staleThresh = c.t>>1;//阈值设置为访问该码元的次数的一半，经验值
    int *keep = new int [c.numEntries];
    int keepCnt = 0;
    //SEE WHICH CODEBOOK ENTRIES ARE TOO STALE(陈旧)
    for(int i=0; i<c.numEntries; i++)
    {
        if(c.cb[i]->stale > staleThresh)//当在背景建模期间有一半的时间内，codebook的码元条目没有被访问，则该条目将被删除
            keep[i] = 0; //Mark for destruction
        else
        {
            keep[i] = 1; //Mark to keep，为1时，该码本的条目将被保留
            keepCnt += 1;//keepCnt记录了要保持的codebook的数目
        }
    }
    //KEEP ONLY THE GOOD
    c.t = 0;                        //Full reset on stale tracking
    code_element **foo = new code_element* [keepCnt];//重新建立一个码本的双指针
    int k=0;
    for(int ii=0; ii<c.numEntries; ii++)
    {
        if(keep[ii])
        {
            foo[k] = c.cb[ii];//要保持该码元的话就要把码元结构体复制到fook
            foo[k]->stale = 0;        //We have to refresh these entries for next clearStale，不被访问的累加器stale重新赋值0
            foo[k]->t_last_update = 0;//
            k++;
        }
		else // 释放那些stale的codebook的内存
		{
			delete[] c.cb[ii];
		}
    }
    //CLEAN UP
    delete [] keep;
    delete [] c.cb;
    c.cb = foo;
    int numCleared = c.numEntries - keepCnt;//numCleared中保存的是被删除码元的个数
    c.numEntries = keepCnt;//最后新的码元数为保存下来码元的个数
    return(numCleared);//返回被删除的码元个数
}

/////////////////////////////////////////////////////////////////////////////////
//int countSegmentation(codeBook *c, Mat I)
//
//Count how many pixels are detected as foreground
// c    Codebook
// I    Image (yuv, 24 bits)
// numChannels  Number of channels we are testing
// maxMod    Add this (possibly negative) number onto max level when code_element determining if new pixel is foreground
// minMod    Subract this (possible negative) number from min level code_element when determining if pixel is foreground
//
// NOTES:
// minMod and maxMod must have length numChannels, e.g. 3 channels => minMod[3], maxMod[3].
//
//Return
// Count of fg pixels
//
int cvcountSegmentation(codeBook *c, Mat I, int numChannels, int *minMod, int *maxMod)
{
    int count = 0,i;
    uchar *pColor;
	int imageLen = I.size().width*I.size().height;

    //GET BASELINE NUMBER OF FG PIXELS FOR Iraw
	pColor = (uchar *)(I.data);
    for(i=0; i<imageLen; i++)
    {
        if(cvbackgroundDiff(pColor, c[i], numChannels, minMod, maxMod))//对每一个像素点都要检测其是否为前景，如果是的话，计数器count就加1
            count++;
        pColor += 3;
    }
    return(count);//返回图像I的前景像素点的个数
}


///////////////////////////////////////////////////////////////////////////////////////////
//void cvconnectedComponents(Mat mask, int poly1_hull0, float perimScale, int *num, Rect *bbs, Point *centers)
// This cleans up the forground segmentation mask derived from calls to cvbackgroundDiff
//
// mask            Is a grayscale (8 bit depth) "raw" mask image which will be cleaned up
//
// OPTIONAL PARAMETERS:
// poly1_hull0    If set, approximate connected component by (DEFAULT) polygon, or else convex hull (0)
// perimScale     Len = image (width+height)/perimScale.  If contour len < this, delete that contour (DEFAULT: 4)
// num            Maximum number of rectangles and/or centers to return, on return, will contain number filled (DEFAULT: NULL)
// bbs            Pointer to bounding box rectangle vector of length num.  (DEFAULT SETTING: NULL)
// centers        Pointer to contour centers vectore of length num (DEFULT: NULL)
//
void cvconnectedComponents(Mat mask, int poly1_hull0, float areaScale, int *num, Rect *bbs, Point *centers)
{
	vector<vector<Point> > contours;
	Mat element = getStructuringElement(MORPH_RECT,Size(3,3));
//CLEAN UP RAW MASK
//开运算作用：平滑轮廓，去掉细节,断开缺口
    morphologyEx( mask, mask, CV_MOP_OPEN, element);//对输入mask进行开操作，CVCLOSE_ITR为开操作的次数，输出为mask图像
//闭运算作用：平滑轮廓，连接缺口
	element = getStructuringElement(MORPH_RECT,Size(5,5));
    morphologyEx( mask, mask, CV_MOP_CLOSE, element);;//对输入mask进行闭操作，CVCLOSE_ITR为闭操作的次数，输出为mask图像

//FIND CONTOURS AROUND ONLY BIGGER REGIONS

    //CV_RETR_EXTERNAL=0是在types_c.h中定义的，CV_CHAIN_APPROX_SIMPLE=2也是在该文件中定义的
	findContours(mask,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
    int numCont = 0;
	// 图片面积
	int ImageArea = mask.size().area();

	int *conIdx = new int[contours.size()];
	for(int i = 0;i < contours.size();i++){
		double area = abs(contourArea(contours[i]));
		conIdx[i] = 0;
		//Smooth it's edges if it's large enough
		if(area >= ImageArea*areaScale)
		{
			conIdx[i] = 1;
			if(poly1_hull0){
				vector<Point> c_new;
				approxPolyDP(contours[i],c_new,1,0);
				contours[i] = c_new;
			}
		}
	}

// PAINT THE FOUND REGIONS BACK INTO THE IMAGE
	mask.zeros(mask.size(),mask.type());
    Mat masktemp;
    //CALC CENTER OF MASS AND OR BOUNDING RECTANGLES
    if(num != NULL)
    {
        int N = *num, numFilled = 0, i=0;
        Moments mms;
		masktemp = mask.clone();
		int ii = 0;
		for(i = 0; i < contours.size();i++ )
        {
            if(i < N &&conIdx[i] > 0) //Only progress up to *num of them
            {
                drawContours(masktemp,contours,i,CV_CVX_WHITE,CV_FILLED,8);
                //Find the center of each contour
                if(centers != NULL)
                {
					mms = moments(masktemp,1);
					centers[ii].x = (int)(mms.m10/mms.m00);
					centers[ii].y = (int)(mms.m01/mms.m00);
                }
                //Bounding rectangles around blobs
                if(bbs != NULL)
                {
                    bbs[ii] = boundingRect(contours[i]);
                }
				masktemp.zeros(masktemp.size(),masktemp.type());
				ii++;
                numFilled++;
            }
            //Draw filled contours into mask
			if(conIdx[i] > 0)
				drawContours(mask,contours,i,CV_CVX_WHITE,CV_FILLED,8); //draw to central mask
        } //end looping over contours
        *num = numFilled;
    }
    //ELSE JUST DRAW progressED CONTOURS INTO THE MASK
    else
    {
		for(int i = 0;i < contours.size();i++)
        {
			if(conIdx[i] == 1)
            drawContours(mask,contours,i,CV_CVX_WHITE,CV_FILLED,8);
        }
    }
	delete[] conIdx;
}