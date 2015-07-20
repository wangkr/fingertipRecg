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
//	// ���ȳ�ʼ��
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
// codebookģ�ͳ�ʼ��������ģ����
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
	///////////////// ���ȳ�ʼ��
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
	/////// ��ʼ�������
	cap >> srcTemp;
	Size sz = srcTemp.size();
	//ͼ��ߴ�
	newSZ.width = sz.width*scale;newSZ.height = sz.height*scale;

	nrows = newSZ.height;ncols = newSZ.width;
	//��ɫͨ����
	nChannels = srcTemp.channels();
	
	if(srcTemp.isContinuous())
	{
		ncols*=nrows;
		nrows = 1;
	}
	//ͼ���С
	ImageLen = ncols*nrows;
	sizeofcb = 0;
	// ��ʼ��
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
					///////////////////////new �� delete������ؽ�����
					cvupdateCodeBook(pData,cb[r*ncols+c],cbBounds,nChannels);
					pData += 3;
				}
			}
		}
		if(progress == MAX_FRAME_STAGE1)
		{
			for(int c = 0;c < ImageLen;c++)
				cvclearStaleEntries(cb[c]);
			// �����������ɫ���������б�
			skinSegToRect(src,pBackSkinList,dst);
		}
		if(progress > MAX_FRAME_STAGE1 && progress <= MAX_FRAME_STAGE2)
		{
			// ���˵�ǰ����ɫ���ο�
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
// ǰ����ɫ�ָ��
void wkrCB_frSkinSegment(Mat src,Mat &mask)
{
	frSkinSeg(cb,pBackSkinList,src,mask);
}

// ��ȡ������Ϣ
int wkrCB_getProgress()
{
	return progress;
}
// �������㺯������
void wkrCB_zeroProgress()
{
	progress = 0;
}
// ֹͣģ��ѵ��
void wkrCB_stopTrain()
{
	if_stop = true;
}
// ���ش�����
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
	/* ����grayworld�����Զ���ƽ�����㣬pImg��RGB���Ϊ24��ͼƬ             */
	/************************************************************************/
	uchar *pSrc = NULL;
	double avgB, avgG, avgR, avgGray;
	double b, g, r;
	double kB, kG, kR;
	int width, height;
	//ȡ��ƽ��ֵ
	Scalar avgRGB = mean(srcImg);
	avgB = avgRGB.val[0];
	avgG = avgRGB.val[1];
	avgR = avgRGB.val[2];
	//ȡ��ƽ���Ҷ�ֵ
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
			//����Ƿ񳬳���Χ
			b=b>255? 255: b;
			g=g>255? 255: g;
			r=r>255? 255: r;
			*(pSrc+j*3)=(int)b;
			*(pSrc+j*3+1)=(int)g;
			*(pSrc+j*3+2)=(int)r;
		}
	}	
}
// ��ɫ�ָ��
void skinSegToRect(Mat srcImg,vector<Rect> &out,Mat &mask)
{
	Mat yuvImg;
	uchar *pMask,*pSrc;
	mask = Mat::zeros(srcImg.size(),CV_8UC1);
//	skinSegFunc(srcImg,mask);
	// �ָ����ɫ����
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
	// ��������
	//CLEAN UP RAW MASK
	Mat element = getStructuringElement(MORPH_RECT,Size(3,3));
	//���������ã�ƽ��������ȥ��ϸ��,�Ͽ�ȱ��
    morphologyEx( mask, mask, CV_MOP_OPEN, element);//������mask���п�������CVCLOSE_ITRΪ�������Ĵ��������Ϊmaskͼ��
	//���������ã�ƽ������������ȱ��
	element = getStructuringElement(MORPH_RECT,Size(5,5));
    morphologyEx( mask, mask, CV_MOP_CLOSE, element);;//������mask���бղ�����CVCLOSE_ITRΪ�ղ����Ĵ��������Ϊmaskͼ��

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
// �ֲ����ε�ǰ���ָ��
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
// ��ɫǰ���ָ��
void frSkinSeg(codeBook *cb, vector<Rect> bkSkinList,Mat src, Mat &mask)
{
	vector<Rect> skinList;
	// ��ȡǰ����ɫ
	skinSegToRect(src,skinList,mask);
	int isSeg = 0;
	for(int l = 0;l < skinList.size();l++)
	{
		isSeg = 0;
		for(int b = 0;b < bkSkinList.size();b++)
		{
			// �жϾ����Ƿ��ཻ
			int detax,detay;
			detax = skinList[l].tl().x - bkSkinList[b].tl().x;
			detay = skinList[l].tl().y - bkSkinList[b].tl().y;
			// ����Ѿ����зָ���ˣ�������ѭ�������١�
			if(isSeg == 1)
				break;
			// ������ཻ�����
			if((detax > 0&& detax >= bkSkinList[b].width)||(detax < 0 && -detax >= skinList[l].width))
				continue;
			if((detay > 0&& detay >= bkSkinList[b].height)||(detay < 0&& -detay >= skinList[l].height))
				continue;
			frSkinSegInRect(cb,skinList[l],src,mask);
			// ��Ǹ�ǰ�������Ѿ�����ǰ���ָ���
			isSeg = 1;
		}
	}
	Mat element = getStructuringElement(MORPH_RECT,Size(5,5));
	morphologyEx( mask, mask, CV_MOP_CLOSE, element);

}
// �������ɫ������ǰ����ɫ���ι��˺���
void frSkinFilter(codeBook *cb, vector<Rect> bkList,VideoCapture cap,Size sz,int &fcounter)
{
	Mat tempSrc,src;
	Mat yuvImg;

	int nr = sz.height,nc = sz.width;
	uchar *pData = NULL;
	// ��ʼ��ÿ��bkSkin���ο��е�frSkinͳ������
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
		// ����codebook�ָ�ǰ������
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
		// ���bkList��ĳһ���ζ�Ӧ��ǰ�����ص��ۼ�ֵ���ڸþ�����������ֵ��10%����ɾ���þ���
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
				// ͳ������
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

    if(c.numEntries == 0) c.t = 0;//˵��ÿ��������������˵Ļ����ٶ�Ӧһ����Ԫ
    c.t += 1;        //Record learning event�����������ص�Ĵ�����1
//SET HIGH AND LOW BOUNDS
    int n;
    unsigned int high[3],low[3];
    for(n=0; n<numChannels; n++)//Ϊ�����ص��ÿ��ͨ�����������ֵ����С��ֵ��������������ѧϰ�ĸߵ���ֵʱ����
    {
        high[n] = *(p+n)+*(cbBounds+n);
        if(high[n] > 255) high[n] = 255;
        low[n] = *(p+n)-*(cbBounds+n);
        if(low[n] < 0) low[n] = 0;
    }
    int matchChannel;
    //SEE IF THIS FITS AN EXISTING CODEWORD
    int i;
    for(i=0; i<c.numEntries; i++)//��Ҫ�����е���Ԫ����ɨ��
    {
        matchChannel = 0;
        for(n=0; n<numChannels; n++)
        {
            //����ط�Ҫ�ǳ�С�ģ�if���������������
//if((c.cb[i]->min[n]-c.cb[i]->learnLow[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->max[n]+c.cb[i]->learnHigh[n]))
//ԭ������Ϊ��ÿ�ν���һ������Ԫ��ʱ��learnHigh[n]��learnLow[n]�ķ�Χ����max[n]��min[n]����չ��cbBounds[n]������˵
//learnHigh[n]��learnLow[n]�ı仯��Χʵ���ϱ�max[n]��min[n]�Ĵ�
            if((c.cb[i]->learnLow[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->learnHigh[n])) //Found an entry for this channel
            {
                matchChannel++;
            }
        }
        if(matchChannel == numChannels) //If an entry was found over all channels���ҵ��˸�Ԫ�ش˿̶�Ӧ����Ԫ
        {
            c.cb[i]->t_last_update = c.t;
            //adjust this codeword for the first channel
//����ÿ����Ԫ�������С��ֵ����Ϊ��2����ֵ�ں����ǰ���������Ҫ�õ�
            for(n=0; n<numChannels; n++)
            {
                if(c.cb[i]->max[n] < *(p+n))//�øõ������ֵ���¸���Ԫ�����ֵ������max[n]�������ʵ������ʷ���ֹ����������ֵ
                {
                    c.cb[i]->max[n] = *(p+n);//��Ϊ���for�������ƥ��ɹ��˵�������ֵ�µģ�����һ����˵�ı���max[n]��min[n]
//Ҳ�����ѧϰ�ĸߵ���ֵ������ѧϰ�ĸߵ���ֵҲһֱ�ڻ����仯  
                }
                else if(c.cb[i]->min[n] > *(p+n))//�øõ������ֵ���¸���Ԫ����Сֵ������min[n]�������ʵ������ʷ���ֹ�����С����ֵ
                {
                    c.cb[i]->min[n] = *(p+n);
                }
            }
            break;//һ���ҵ��˸����ص�һ����Ԫ��Ͳ��ü����������ˣ��ӿ��㷨�ٶȡ���Ϊ���ֻ��һ����Ԫ��֮��Ӧ
        }
    }

    //OVERHEAD TO TRACK POTENTIAL STALE ENTRIES
    for(int s=0; s<c.numEntries; s++)
    {
        //This garbage is to track which codebook entries are going stale
        int negRun = c.t - c.cb[s]->t_last_update;//negRun��ʾ��Ԫû�и��µ�ʱ����
        if(c.cb[s]->stale < negRun) c.cb[s]->stale = negRun;//����ÿ����Ԫ��statle
    }


    //ENTER A NEW CODE WORD IF NEEDED
    if(i == c.numEntries)  //No existing code word found, make a new one��ֻ�е��������뱾�е�������Ԫ��������Ҫ��ʱ������if����
    {
        code_element **foo = new code_element* [c.numEntries+1];//����һ���µ���Ԫ����
        for(int ii=0; ii<c.numEntries; ii++)
        {
            foo[ii] = c.cb[ii];//���뱾ǰ�����е���Ԫ��ַ����foo
        }
        foo[c.numEntries] = new code_element;//����һ������Ԫ������fooָ�����һ����λ
		sizeofcb += sizeof(code_element);
        if(c.numEntries) {
			delete [] c.cb;//
		}
        c.cb = foo;
        for(n=0; n<numChannels; n++)//���½�������Ԫ�ṹ��Ԫ�ظ�ֵ
        {
            c.cb[c.numEntries]->learnHigh[n] = high[n];//������һ������Ԫʱ���õ�ǰֵ����cbBounds��Χ��Ϊ��Ԫbox��ѧϰ��ֵ
            c.cb[c.numEntries]->learnLow[n] = low[n];
            c.cb[c.numEntries]->max[n] = *(p+n);//������һ������Ԫʱ���õ�ǰֵ��Ϊ��Ԫbox�������С�߽�ֵ
            c.cb[c.numEntries]->min[n] = *(p+n);
        }
        c.cb[c.numEntries]->t_last_update = c.t;
        c.cb[c.numEntries]->stale = 0;//��Ϊ�ս���������Ϊ0
        c.numEntries += 1;//��Ԫ�ĸ�����1
    }

    // SLOWLY ADJUST LEARNING BOUNDS
    for(n=0; n<numChannels; n++)//ÿ�α��������ص�ͽ�ÿ����Ԫ��ѧϰ�����ֵ�����С��ֵ��С�����Ƕ��ǻ����仯��
    {                           //������½�������Ԫ����if�����϶�������
        if(c.cb[i]->learnHigh[n] < high[n]) c.cb[i]->learnHigh[n] += 1;                
        if(c.cb[i]->learnLow[n] > low[n]) c.cb[i]->learnLow[n] -= 1;
    }

    return(i);//�������ҵ��뱾����Ԫ������
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
                break;//�ӿ��ٶȣ���һ��ͨ��������ʱ��ǰ����
            }
        }
        if(matchChannel == numChannels)
        {
            break; //Found an entry that matched all channels���ӿ��ٶȣ���һ����Ԫ�ҵ�ʱ����ǰ����
        }
    }
    if(i >= c.numEntries) return(255);//255����ǰ������Ϊ���е���Ԫ������������
    return(0);//0����������Ϊ������һ����Ԫ��������
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
int cvclearStaleEntries(codeBook &c)//��ÿһ���뱾���м��
{
    int staleThresh = c.t>>1;//��ֵ����Ϊ���ʸ���Ԫ�Ĵ�����һ�룬����ֵ
    int *keep = new int [c.numEntries];
    int keepCnt = 0;
    //SEE WHICH CODEBOOK ENTRIES ARE TOO STALE(�¾�)
    for(int i=0; i<c.numEntries; i++)
    {
        if(c.cb[i]->stale > staleThresh)//���ڱ�����ģ�ڼ���һ���ʱ���ڣ�codebook����Ԫ��Ŀû�б����ʣ������Ŀ����ɾ��
            keep[i] = 0; //Mark for destruction
        else
        {
            keep[i] = 1; //Mark to keep��Ϊ1ʱ�����뱾����Ŀ��������
            keepCnt += 1;//keepCnt��¼��Ҫ���ֵ�codebook����Ŀ
        }
    }
    //KEEP ONLY THE GOOD
    c.t = 0;                        //Full reset on stale tracking
    code_element **foo = new code_element* [keepCnt];//���½���һ���뱾��˫ָ��
    int k=0;
    for(int ii=0; ii<c.numEntries; ii++)
    {
        if(keep[ii])
        {
            foo[k] = c.cb[ii];//Ҫ���ָ���Ԫ�Ļ���Ҫ����Ԫ�ṹ�帴�Ƶ�fook
            foo[k]->stale = 0;        //We have to refresh these entries for next clearStale���������ʵ��ۼ���stale���¸�ֵ0
            foo[k]->t_last_update = 0;//
            k++;
        }
		else // �ͷ���Щstale��codebook���ڴ�
		{
			delete[] c.cb[ii];
		}
    }
    //CLEAN UP
    delete [] keep;
    delete [] c.cb;
    c.cb = foo;
    int numCleared = c.numEntries - keepCnt;//numCleared�б�����Ǳ�ɾ����Ԫ�ĸ���
    c.numEntries = keepCnt;//����µ���Ԫ��Ϊ����������Ԫ�ĸ���
    return(numCleared);//���ر�ɾ������Ԫ����
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
        if(cvbackgroundDiff(pColor, c[i], numChannels, minMod, maxMod))//��ÿһ�����ص㶼Ҫ������Ƿ�Ϊǰ��������ǵĻ���������count�ͼ�1
            count++;
        pColor += 3;
    }
    return(count);//����ͼ��I��ǰ�����ص�ĸ���
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
//���������ã�ƽ��������ȥ��ϸ��,�Ͽ�ȱ��
    morphologyEx( mask, mask, CV_MOP_OPEN, element);//������mask���п�������CVCLOSE_ITRΪ�������Ĵ��������Ϊmaskͼ��
//���������ã�ƽ������������ȱ��
	element = getStructuringElement(MORPH_RECT,Size(5,5));
    morphologyEx( mask, mask, CV_MOP_CLOSE, element);;//������mask���бղ�����CVCLOSE_ITRΪ�ղ����Ĵ��������Ϊmaskͼ��

//FIND CONTOURS AROUND ONLY BIGGER REGIONS

    //CV_RETR_EXTERNAL=0����types_c.h�ж���ģ�CV_CHAIN_APPROX_SIMPLE=2Ҳ���ڸ��ļ��ж����
	findContours(mask,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
    int numCont = 0;
	// ͼƬ���
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