#include "stdafx.h"
#include "handGesture.hpp"

struct dim{int w; int h;}boundingDim;
Mat edges;
MyROI roi1, roi2,roi3,roi4,roi5,roi6;


vector <KalmanFilter> kf;
vector <Mat_<float> > measurement;
HandGesture::HandGesture()
{
	if_hand = false;
	if_skin_samp = false;
	palm_center = Point(0,0);
	bound_rect.width = bound_rect.height = 0;
	palmAxisAng = 0;

	sample_jiange = 10;
}

int HandGesture::init(float scale,int webCam)
{
	cap = VideoCapture(webCam);
	if(!cap.isOpened())
		return -1;
	this->scale = scale;
	Mat src1;
	cap>>src1;
	Size sz = src1.size();

	// 初始化类变量
	newSize.width  = (int)sz.width*scale;
	newSize.height = (int)sz.height*scale;

	ImageLen = newSize.area();
	ImageDiagonal = (int)sqrt((double)(newSize.width*newSize.width+newSize.height*newSize.height));

	src_re = Mat(newSize,src1.type());

	palm_center = Point(0,0);
	hough_center = Point(0,0);
	lhough_center = Point(0,0);
	bound_center = Point(0,0);
	lbound_center = Point(0,0);
	return 0;
}
void HandGesture::initColors()
{
	for(int i=0;i<NSAMPLES;i++){
		c_lower[i][0]=12;
		c_upper[i][0]=7;
		c_lower[i][1]=30;
		c_upper[i][1]=40;
		c_lower[i][2]=80;
		c_upper[i][2]=80;
	}
}
int HandGesture::getMedian(vector<int> val){
  int median;
  size_t size = val.size();
  sort(val.begin(), val.end());
  if (size  % 2 == 0)  {
      median = val[size / 2 - 1] ;
  } else{
      median = val[size / 2];
  }
  return median;
}

void HandGesture::getAvgColor(MyROI roi,int avg[3]){
	Mat r;
	roi.roi_ptr.copyTo(r);
	vector<int>hm;
	vector<int>sm;
	vector<int>lm;
	// generate vectors
	for(int i=2; i<r.rows-2; i++){
    	for(int j=2; j<r.cols-2; j++){
    		hm.push_back(r.data[r.channels()*(r.cols*i + j) + 0]);
        	sm.push_back(r.data[r.channels()*(r.cols*i + j) + 1]);
        	lm.push_back(r.data[r.channels()*(r.cols*i + j) + 2]);
   		}
	}
	avg[0]=getMedian(hm);
	avg[1]=getMedian(sm);
	avg[2]=getMedian(lm);
}

// 求肤色像素采样平均值
void HandGesture::average(HDC hdc,CRect *pRect){
	getSrcFrame();
	for(int i=0;i<30;i++){
		getSrcFrame();
		cvtColor(src_re,src_re,ORIGCOL2COL);
		for(int j=0;j<NSAMPLES;j++){
			getAvgColor(roi[j],avgColor[j]);
			roi[j].draw_rectangle(src_re);
		}	
		cvtColor(src_re,src_re,COL2ORIGCOL);
		string imgText=string("Finding average color of hand...");
		printText(src_re,imgText);	
		// 显示图像
		showToClient(src_re,hdc,pRect);
        if(cv::waitKey(30) >= 0) break;
	}
}

// 在视频中画出手掌的采样位置
void HandGesture::waitForPalmCover(HDC hdc,CRect *pRect){
	vector<MyROI>().swap(roi);
	roi.push_back(MyROI(Point((int)(newSize.width/3), (int)(newSize.height/6)),Point((int)(newSize.width/3)+RECT_WIDTH,(int)(newSize.height/6)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/4), (int)(newSize.height/2)),Point((int)(newSize.width/4)+RECT_WIDTH,(int)(newSize.height/2)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/3), (int)(newSize.height/1.5)),Point((int)(newSize.width/3)+RECT_WIDTH,(int)(newSize.height/1.5)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/2), (int)(newSize.height/2)),Point((int)(newSize.width/2)+RECT_WIDTH,(int)(newSize.height/2)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/2.5),(int)(newSize.height/2.5)),Point((int)(newSize.width/2.5)+RECT_WIDTH,(int)(newSize.height/2.5)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/2),  (int)(newSize.height/1.5)),Point((int)(newSize.width/2)+RECT_WIDTH,(int)(newSize.height/1.5)+RECT_WIDTH),src_re));
	roi.push_back(MyROI(Point((int)(newSize.width/2.5),(int)(newSize.height/1.8)),Point((int)(newSize.width/2.5)+RECT_WIDTH,(int)(newSize.height/1.8)+RECT_WIDTH),src_re));
	// 等待时间50*30ms
	for(int i =0;i<50;i++){
    	getSrcFrame();
		for(int j=0;j<NSAMPLES;j++){
			roi[j].draw_rectangle(src_re);
		}
		string imgText=string("Cover rectangles with palm");
		printText(src_re,imgText);	
		// 显示图像
		showToClient(src_re,hdc,pRect);
        if(cv::waitKey(30) >= 0) break;
	}
}
void HandGesture::normalizeColors(){

	for(int i=1;i<NSAMPLES;i++){
		for(int j=0;j<3;j++){
			c_lower[i][j]=c_lower[0][j];	
			c_upper[i][j]=c_upper[0][j];	
		}	
	}
	
	for(int i=0;i<NSAMPLES;i++){
		if((avgColor[i][0]-c_lower[i][0]) <0){
			c_lower[i][0] = avgColor[i][0] ;
		}if((avgColor[i][1]-c_lower[i][1]) <0){
			c_lower[i][1] = avgColor[i][1] ;
		}if((avgColor[i][2]-c_lower[i][2]) <0){
			c_lower[i][2] = avgColor[i][2] ;
		}if((avgColor[i][0]+c_upper[i][0]) >255){ 
			c_upper[i][0] = 255-avgColor[i][0] ;
		}if((avgColor[i][1]+c_upper[i][1]) >255){
			c_upper[i][1] = 255-avgColor[i][1] ;
		}if((avgColor[i][2]+c_upper[i][2]) >255){
			c_upper[i][2] = 255-avgColor[i][2] ;
		}
	}
}
// 肤色分割-直方图方法
void HandGesture::skinColorSegHistogram(Mat src,Mat &dst){	
	Scalar lowerBound;
	Scalar upperBound;
	vector<Mat>().swap(bwList);
	Mat foo;
	for(int i=0;i<NSAMPLES;i++){
		normalizeColors();
		lowerBound=Scalar( avgColor[i][0] - c_lower[i][0] , avgColor[i][1] - c_lower[i][1], avgColor[i][2] - c_lower[i][2] );
		upperBound=Scalar( avgColor[i][0] + c_upper[i][0] , avgColor[i][1] + c_upper[i][1], avgColor[i][2] + c_upper[i][2] );
		bwList.push_back(Mat(src.rows,src.cols,CV_8U));
		inRange(src,lowerBound,upperBound,bwList[i]);
	}
	bwList[0].copyTo(dst);
	for(int i=1;i<NSAMPLES;i++){
		dst+=bwList[i];
	}
	medianBlur(dst, dst,7);
}
// 肤色采样
void HandGesture::sampleSkinColor(HDC hdc,CRect *pRect)
{
	waitForPalmCover(hdc,pRect);
	average(hdc,pRect);
	initColors();
}

// 获取原视频帧并进行预处理
int HandGesture::getSrcFrame()
{
	Mat src;
	if(!cap.read(src))
		return -1;
	cap >> src;
	resize(src,src_re,newSize);
	grayWorld(src_re);
	flip(src_re,src_re,1);
	return 0;

}
//void HandGesture::hand_YCrCb(Mat src)
//{
//	Mat img_YCrCb = Mat(newSize,CV_8UC3);
//	Mat YCrCb_mask = Mat(newSize,CV_8UC1);
//
//	//最终的图片
//	YCrCb = Mat(newSize,CV_8UC3);
//	// 肤色的Y,Cr,Cb颜色范围,分别是它们的最低和最高值
//	Scalar scalar[3][2] = {{CV_RGB(0,0,46),CV_RGB(0,0,197)},{CV_RGB(0,0,131),CV_RGB(0,0,162)},{ CV_RGB(0,0,119),CV_RGB(0,0,134)}};
//	
//	//三通道
//	Mat channels[3],channels_tmp[3];
//	for(int i = 0;i< 3;i++){
//		channels[i] = Mat(newSize,CV_8UC1);
//		channels_tmp[i ] =Mat(newSize,CV_8UC1);
//	}
//	//转换到YCrBr
//	cvtColor(src,img_YCrCb,CV_RGB2GRAY);
//
//	//分割到Y,Cr,Cb
//	split(img_YCrCb,channels);
//
//	//将Y_channel的位于 Y_lower 和 Y_upper 之间的元素复制到 Y_tmp中
//	for(int j = 0;j< 3;j++)
//		inRange(channels[j],scalar[j][0],scalar[j][1],channels_tmp[j]);
//
//	//合并Y,Cr,Cb通道到YCrCb中
//	merge(channels_tmp,3,YCrCb);
//}

void HandGesture::skinColorSegHSV(Mat src,Mat &dst)
{
	Mat hsv_image = Mat(newSize,CV_8UC3);
	Scalar hsv_min = Scalar(0, 20, 20, 0);
	Scalar hsv_max = Scalar(20, 250, 255, 0);
	//hsv_mask->origin = 1;

	//方法2: 单独处理各个通道
	Mat HSV_img[3];
	for(int i = 0;i < 3;i++)HSV_img[i] = Mat(newSize,CV_8UC1);

	Mat H_mask = Mat(newSize,CV_8UC1);
	Mat H_mask1 = Mat(newSize,CV_8UC1);
	Mat S_mask = Mat(newSize,CV_8UC1);
	Mat V_mask = Mat(newSize,CV_8UC1);
	Mat V_mask1 = Mat(newSize,CV_8UC1);

	cvtColor(src, hsv_image, CV_BGR2HSV);

	//方法2: 单独处理各个通道
	split(hsv_image,HSV_img);

	//色相
	inRange(HSV_img[0],Scalar(0,0,0,0),Scalar(H_low_max,0,0,0),H_mask);			 //红色区
	inRange(HSV_img[0],Scalar(256 - H_high_min,0,0,0),Scalar(256,0,0,0),H_mask1);//紫色区

	//饱和度
	inRange(HSV_img[1],Scalar(S_low,0,0,0),Scalar(S_high,0,0,0),S_mask);		 //中间区

	//亮度
	inRange(HSV_img[2],Scalar(V_high,0,0,0),Scalar(256,0,0,0),V_mask);			 //高亮区
	inRange(HSV_img[2],Scalar(V_low,0,0,0),Scalar(V_high,0,0,0),V_mask1);		 //中间区
		
	//红黄, 和蓝紫的混合
	H_mask |= H_mask1;

	//消除饱和度过低区域
	H_mask &= S_mask;

	//消去过亮过暗区域
	H_mask &= V_mask1;

	//补偿过亮区域
	if(if_high_light) H_mask |= V_mask;;
	// 形态学处理
	morphologyEx(H_mask,dst, CV_MOP_CLOSE, NULL,Point(-1,-1),MopEx_value);
}

// 找到最大的轮廓也即轮廓点数目最多的轮廓
int HandGesture::findBiggestContour(){
    int indexOfBiggestContour = -1;
    int sizeOfBiggestContour = 0;
    for (int i = 0; i < contours.size(); i++){
        if(contours[i].size() > sizeOfBiggestContour){
            sizeOfBiggestContour = contours[i].size();
            indexOfBiggestContour = i;
        }
    }
	// 轮廓面积太小的排除
	int ImageArea = newSize.area();
	if(indexOfBiggestContour!=-1&&abs(contourArea(contours[indexOfBiggestContour])) < ImageArea*0.01)
		return -1;
    return indexOfBiggestContour;
}
//void HandGesture::initVectors(){
//	hullI=vector<vector<int> >(contours.size());
//	hullP=vector<vector<Point> >(contours.size());
//	defects=vector<vector<Vec4i> > (contours.size());	
//}
// 移除那些距离挨得太近的指尖点
//void HandGesture::removeRedundantFingerTips(){
//	vector<Point> newFingers;
//	for(int i=0;i<fingerTips.size();i++){
//		for(int j=i;j<fingerTips.size();j++){
//			// 距离阈值为10个像素
//			if(distanceP2P(fingerTips[i],fingerTips[j])<10 && i!=j){
//			}else{
//				newFingers.push_back(fingerTips[i]);	
//				break;
//			}	
//		}	
//	}
//	fingerTips.swap(newFingers);
//}
// 凸点过滤
//void HandGesture::eliminateDefects(Mat src){
//	int tolerance =  bound_rect.height/5;
//	float angleTol=95;
//	vector<Vec4i> newDefects;
//	int startidx, endidx, faridx;
//	vector<Vec4i>::iterator d=defects[cIdx].begin();
//	while( d!=defects[cIdx].end() ) {
//   	    Vec4i& v=(*d);
//	    startidx=v[0]; Point ptStart(contours[cIdx][startidx] );
//   		endidx=v[1]; Point ptEnd(contours[cIdx][endidx] );
//  	    faridx=v[2]; Point ptFar(contours[cIdx][faridx] );
//		if(distanceP2P(ptStart, ptFar) > tolerance && distanceP2P(ptEnd, ptFar) > tolerance && getAngle(ptStart, ptFar, ptEnd  ) < angleTol ){
//			if( ptEnd.y > (bound_rect.y + bound_rect.height -bound_rect.height/4 ) ){
//			}else if( ptStart.y > (bound_rect.y + bound_rect.height -bound_rect.height/4 ) ){
//			}else {
//				newDefects.push_back(v);		
//			}
//		}	
//		d++;
//	}
//	nrOfDefects=newDefects.size();
//	defects[cIdx].swap(newDefects);
//	removeRedundantEndPoints(defects[cIdx]);
//}

bool HandGesture::detectIfHand(){
	double h = bound_rect.height; 
	double w = bound_rect.width;
	int br_x = bound_rect.br().x,br_y = bound_rect.br().y;
	int tl_x = bound_rect.tl().x,tl_y = bound_rect.tl().y;
	if_hand=true;
	if(cIdx == -1){
		if_hand = false;
	}else if(h==0 || w == 0){
		if_hand=false;
	}else if(h/w > 4 || w/h >4){
		if_hand=false;
	}else if(br_x >= newSize.width - 1|| br_y >= newSize.height - 1||tl_x <= 0||tl_y <= 0)
		if_hand = false;
	return if_hand;
}

float HandGesture::getAngle(Point s, Point f, Point e){
	float l1 = distanceP2P(f,s);
	float l2 = distanceP2P(f,e);
	float dot=(s.x-f.x)*(e.x-f.x) + (s.y-f.y)*(e.y-f.y);
	float angle = acos(dot/(l1*l2));
	angle=angle*180/PI;
	return angle;
}

float HandGesture::distanceP2P(Point a, Point b){
	float d= sqrt(fabs( pow((double)(a.x-b.x),2) + pow((double)(a.y-b.y),2) )) ;  
	return d;
}
// 在图像上打印文字
void HandGesture::printText(Mat src, string text){
	int fontFace = FONT_HERSHEY_PLAIN;
	putText(src,text,Point(10, src.rows/10),fontFace, 2,Scalar(0,0,250),1);
}
// 移除那些在同一个指尖点的凸点
//void HandGesture::removeRedundantEndPoints(vector<Vec4i> newDefects){
//	Vec4i temp;
//	float avgX, avgY;
//	float tolerance=bound_rect.width/6;
//	int startidx, endidx, faridx;
//	int startidx2, endidx2;
//	for(int i=0;i<newDefects.size();i++){
//		for(int j=i;j<newDefects.size();j++){
//	    	startidx=newDefects[i][0]; Point ptStart(contours[cIdx][startidx] );
//	   		endidx=newDefects[i][1]; Point ptEnd(contours[cIdx][endidx] );
//	    	startidx2=newDefects[j][0]; Point ptStart2(contours[cIdx][startidx2] );
//	   		endidx2=newDefects[j][1]; Point ptEnd2(contours[cIdx][endidx2] );
//			if(distanceP2P(ptStart,ptEnd2) < tolerance ){
//				contours[cIdx][startidx]=ptEnd2;
//				break;
//			}if(distanceP2P(ptEnd,ptStart2) < tolerance ){
//				contours[cIdx][startidx2]=ptEnd;
//			}
//		}
//	}
//}
// 提取轮廓，检测凸点，进行多边形匹配
void HandGesture::makeContours(Mat src){
	Mat aBw;
	//pyrUp(src_thre, src_thre);
	src_thre.copyTo(aBw);
	findContours(aBw,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
	//initVectors(); 
	cIdx=findBiggestContour();
	////////////////////
	//Mat src123;
	//src123 = Mat::zeros(src.size(),src.type());
	//drawContours(src123,contours,cIdx,CV_RGB(255,255,255),1);
	//imwrite("testpic\\contour.bmp",src123);
	////////////////////
	if(cIdx!=-1){
		bound_rect=boundingRect(Mat(contours[cIdx]));	
		/*
		// 凸形状检测
		convexHull(Mat(contours[cIdx]),hullP[cIdx],false,true);
		convexHull(Mat(contours[cIdx]),hullI[cIdx],false,false);
		// 凸多边形拟合
		approxPolyDP( Mat(hullP[cIdx]), hullP[cIdx], 18, true );
		if(contours[cIdx].size()>3 ){
			convexityDefects(contours[cIdx],hullI[cIdx],defects[cIdx]);
			eliminateDefects(src);
		}*/
		if_hand=detectIfHand();
	}
}
// 画出指尖位置——用圆替代点
//void HandGesture::drawFingerTips(Mat src){
//	Point p;
//	int k=0;
//	for(int i=0;i<fingerTips.size();i++){
//		p=fingerTips[i];
//   		circle(src,p,  5, Scalar(100,255,100), 4 );
//   	 }
//}
// 计算手指数量
//void HandGesture::computeFingerNumber(){
//	std::sort(fingerNumbers.begin(), fingerNumbers.end());
//	int frequentNr;	
//	int thisNumberFreq=1;
//	int highestFreq=1;
//	frequentNr=fingerNumbers[0];
//	for(int i=1;i < fingerNumbers.size(); i++){
//		if(fingerNumbers[i-1]!=fingerNumbers[i]){
//			if(thisNumberFreq>highestFreq){
//				frequentNr=fingerNumbers[i-1];	
//				highestFreq=thisNumberFreq;
//			}
//			thisNumberFreq=0;	
//		}
//		thisNumberFreq++;	
//	}
//	if(thisNumberFreq>highestFreq){
//		frequentNr=fingerNumbers[fingerNumbers.size()-1];	
//	}
//	mostFrequentFingerNumber=frequentNr;	
//}

//void HandGesture::addFingerNumberToVector(){
//	int i=fingerTips.size();	
//	fingerNumbers.push_back(i);
//}
// 指尖定位子函数1—通过凸点判断的结果，来获取指尖位置
//void HandGesture::ifingerTipLoc1(Mat src){
//	fingerTips.clear();
//	int i=0;
//	// 定义一个迭代器，相当于指针
//	vector<Vec4i>::iterator d=defects[cIdx].begin();
//	while( d!=defects[cIdx].end() ) {
//   	    Vec4i& v=(*d);
//	    int startidx=v[0]; Point ptStart(contours[cIdx][startidx] );
//   		int endidx=v[1]; Point ptEnd(contours[cIdx][endidx] );
//  	    int faridx=v[2]; Point ptFar(contours[cIdx][faridx] );
//		if(i==0){
//			fingerTips.push_back(ptStart);
//			i++;
//		}
//		fingerTips.push_back(ptEnd);
//		// 指针加一
//		d++;
//		i++;
//   	}
//	if(fingerTips.size()==0){
//		checkForOneFinger(src);
//	}
//}
// 当手部区域不完整时，判断是否只有一根手指（很好的函数）
//void HandGesture::checkForOneFinger(Mat src){
//	int yTol=bound_rect.height/6;
//	Point highestP;
//	highestP.y=src.rows;
//	vector<Point>::iterator d=contours[cIdx].begin();
//	while( d!=contours[cIdx].end() ) {
//   	    Point v=(*d);
//		if(v.y<highestP.y){
//			highestP=v;
//		}
//		d++;	
//	}
//	int n=0;
//	d=hullP[cIdx].begin();
//	while( d!=hullP[cIdx].end() ) {
//   	    Point v=(*d);
//		if(v.y<highestP.y+yTol && v.y!=highestP.y && v.x!=highestP.x){
//			n++;
//		}
//		d++;
//	}if(n==0){
//		fingerTips.push_back(highestP);
//	}
//}

// 计算曲率来定位指尖
void HandGesture::curvaCompute(SamPoints &samp)
{
	Point pt1,pt2,pt3,pt1_temp,pt2_temp;
	double temp1,temp2;
	int vector1[2],vector2[2];
	int samSize = samp.samIndex.size();
	// 计算每一个采样点的曲率
	for(int i = 0;i < samSize;i++)
	{
		pt1 = contours[cIdx][samp.samIndex[(i-3+samSize)%samSize]];
		pt2 = contours[cIdx][samp.samIndex[i]];
		pt3 = contours[cIdx][samp.samIndex[(i+3)%samSize]];
		vector1[0] = pt1.x - pt2.x; vector1[1] = pt1.y - pt2.y;
		vector2[0] = pt3.x - pt2.x; vector2[1] = pt3.y - pt2.y;
		temp1 = vector1[0]*vector2[0] + vector1[1] * vector2[1]; // 向量数量积
		temp2 = sqrt(1.0*(vector1[0]*vector1[0] + vector1[1]*vector1[1]))*sqrt(1.0*(vector2[0]*vector2[0] + vector2[1]*vector2[1])); // 向量模相乘
		// 计算曲率
		samp.curvature[i] = (temp1*1.0)/temp2;
		samp.dire[i] = vector1[0]*vector2[1] - vector1[1]*vector2[0]; // 向量叉乘积方向
		pt1 = pt2;
		pt2 = pt3;
	}
	// 指尖定位
	Point pt_tmp;
	for(int i = 0;i < samSize;i++)
	{
		// 排除曲率小于阈值且矢量积为负的点
		if(samp.curvature[i] >= thred_of_curvature(bound_rect.area()) &&samp.dire[i] > 0)
		{
			pt_tmp = contours[cIdx][samp.samIndex[i]];
			samp.fgTipcIdx.push_back(samp.samIndex[i]);
				samp.fgTipPts.push_back(contours[cIdx][samp.samIndex[i]]);
				samp.fgTipCurva.push_back(samp.curvature[i]);
			// 排除那些离掌心很近的点
			float dist = distanceP2P(pt_tmp,palm_center);
			if(dist >= bound_rect.height/3&&(dist < bound_rect.width||dist < bound_rect.height)){
				samp.fgTipcIdx.push_back(samp.samIndex[i]);
				samp.fgTipPts.push_back(contours[cIdx][samp.samIndex[i]]);
				samp.fgTipCurva.push_back(samp.curvature[i]);
			}
		}
	}
	// 删除隔得很近的点
	int fgsize = samp.fgTipcIdx.size();
	float minFgDist = bound_rect.width*1.0/8;
	Point pt_temp1,pt_temp2;
	for(int i = 0;i < samp.fgTipPts.size();i++)
		for(int j = i+1;j < samp.fgTipPts.size();j++)
		{
			pt_temp1 = samp.fgTipPts[i];
			pt_temp2 = samp.fgTipPts[j];
			if(distanceP2P(pt_temp1,pt_temp2) < minFgDist){
				samp.fgTipcIdx[j] = -1;
			}
		}
	// 排除那些靠近图像边缘上的点
	for(int i = 0;i < samp.fgTipcIdx.size();i++)
	{
		if(samp.fgTipcIdx[i] >= 0){
			int x = samp.fgTipPts[i].x;
			int y = samp.fgTipPts[i].y;
			if((x >= 0&& x <= 5 )||(x <= newSize.width&&x >= newSize.width - 5)||
				(y >= 0&&y <= 5)||( y <= newSize.height&& y>= newSize.height - 5 ))
				samp.fgTipcIdx[i] = -1;
		}
	}

}

//指尖定位子函数2—通过计算曲率来定位指尖,L为采样距离
void HandGesture::ifingerTipLoc2(Mat src,int L,SamPoints &samp)
{
	Point pt;
	int sampPointsNum,tempi = 0;
	int allPointsNum = contours[cIdx].size();
	if(allPointsNum==0)return;

	int if_add_point;
	sampPointsNum = allPointsNum/L + (if_add_point=allPointsNum%L>= cvFloor(L/2.0)?1:0);

	samp.samIndex = vector<int>(sampPointsNum);
	samp.dire = vector<int>(sampPointsNum);
	samp.curvature = vector<float>(sampPointsNum);
	for(int i = 0;i < sampPointsNum;i++)
	{
		// 开始采样
		if(i != sampPointsNum - 1) // 采样间隔为 L
		{
			samp.samIndex[i] = i*L;
		}
		// 判断是否到达最后一个采样点
		else if(i == sampPointsNum - 1)
		{
			// 最后的采样点
			samp.samIndex[i] = (i-1)*L + (allPointsNum - (i-1)*L)/2;
		}
	}
	//////////////////////////////////////////////
	//Mat src123;
	//src123 = Mat::zeros(src.size(),src.type());
	//for(int i = 0;i < sampPointsNum;i++)
	//{
	//	circle(src123,contours[cIdx][samp.samIndex[i]],1,RGB(255,255,255),2);
	//}
	//imwrite("testpic\\sampPoints.bmp",src123);
	/////////////////////////////////////////////
	// 指尖定位
	curvaCompute(samp);

}
// 指尖点定位函数
void HandGesture::fingerTipLoc(vector<Point> &fg)
{
	if(!if_hand||contours.size()==0)
	{
		fg = vector<Point>(0);
		return;
	}
	SamPoints samp;
	ifingerTipLoc2(src_re,len_of_L(bound_rect.area()),samp);
	for(int i = 0;i < samp.fgTipcIdx.size();i++)
	{
		if(samp.fgTipcIdx[i] >= 0)
			fg.push_back(samp.fgTipPts[i]);
	}
}
// 确定Pi的邻居点距离L的长度
/* 输入handRecScale—手势矩形区域占图像的比例
*/
int HandGesture::len_of_L(int handRecSize)
{
	int Imgscale = round((handRecSize*100.0/ImageLen));
	int L;
	if(Imgscale <= 5)
		L = 3;
	else if(Imgscale <= 10)
		L = 5;
	else if(Imgscale <= 20)
		L = 8;
	else if(Imgscale <= 40)
		L = 11;
	else
		L = 13;
	return L;
}
// 根据手势矩形区域大小计算曲率阈值
float HandGesture::thred_of_curvature(int handRecSize)
{
	int Imgscale = round((handRecSize*100.0/ImageLen));
	float ct;
	if(Imgscale <= 5)
		ct = 0.55;
	else if(Imgscale <= 15)
		ct = 0.70;
	else if(Imgscale <= 20)
		ct = 0.78;
	else
		ct = 0.81;
	return ct;
}
// 计算掌轴角度
int HandGesture::calcTilt(double mu11,double mu20,double mu02)
{
	// 使用手指之间的坐标信息来判断 
	double diff = mu20 - mu02;
	if (diff == 0) {
		if (mu11 == 0)
			return 0;
		else if (mu11 > 0)
			return 45;
		else   // m11 < 0
			return -45;
  }

  double theta = 0.5 * atan2(2*mu11, diff);
  int tilt = (int) round(theta *180/3.1416);

  if ((diff > 0) && (mu11 == 0))
    return 0;
  else if ((diff < 0) && (mu11 == 0))
    return -90;
  else if ((diff > 0) && (mu11 > 0))  // 0 to 45 degrees
    return tilt;
  else if ((diff > 0) && (mu11 < 0))  // -45 to 0
    return (180 + tilt);   // change to counter-clockwise angle
  else if ((diff < 0) && (mu11 > 0))  // 45 to 90
    return tilt;
  else if ((diff < 0) && (mu11 < 0))  // -90 to -45
    return (180 + tilt);  // change to counter-clockwise angle

  return 0;
}
// 四舍五入函数
double HandGesture::round(double r)
{
	return (r>0.0)?floor(r+0.5):ceil(r-0.5);
}
// 计算手掌倾斜度函数
int HandGesture::palmAxisAngle(vector<Point> fgTps)
{
	Moments m = moments(contours[cIdx],1);
	int contourAxisAngle = 0;
	// 中心矩
	double mu11 = m.mu11;
	double mu20 = m.mu20;
	double mu02 = m.mu02;
	// 处理手指向下的情况
	contourAxisAngle = calcTilt(mu11,mu20,mu02);

	if (fgTps.size() > 0){
		int yTotal = 0;
		for(int i = 0;i < fgTps.size();i++)
			yTotal += fgTps[i].y;
		int avgYFinger = yTotal/fgTps.size();
		if(avgYFinger > palm_center.y)   // fingers below COG
			contourAxisAngle -= 180;
	}
	palmAxisAng = contourAxisAngle;

	return contourAxisAngle;
}
// 掌心定位函数2，cIdx最大轮廓索引，dst目的文件
/**
* 通过图像矩计算掌心(COG)和掌轴
*/
void HandGesture::palmCentrLoc2(int cIdx,Mat &dst)
{
	Moments m = moments(contours[cIdx],1);
	// 图像的重心（COG）
	// 图像的标准矩
	double m00 = m.m00;
	double m01 = m.m01;
	double m10 = m.m10;
	if(m00 !=0)
	{
		int xCenter = (int)round(m10/m00);
		int yCenter = (int)round(m01/m00);
		palm_center.x = xCenter;
		palm_center.y = yCenter;
	}
}
// 掌心定位函数，src源文件，dst目的文件
/**
* 通过霍夫圆变换计算掌心
*/
void HandGesture::palmCentrLoc(Mat src,Mat &dst)
{
	//如果没有手, 则清除结果
	if(!if_hand||contours.size()==0){
		 bound_center  = cvPoint(0,0); 
		 hough_center = cvPoint(0,0);
		 lhough_center = cvPoint(0,0);
		 lbound_center = cvPoint(0,0);
		return;
	}
	
	lbound_center = bound_center;

	//获得中心点
	bound_center.x = bound_rect.x + bound_rect.width/2;
	bound_center.y = bound_rect.y + bound_rect.height/2;

	///////////////////////////////////////////////////////
	// 设置轮廓边界区域为感兴趣区域
	Mat roi = src(bound_rect);
	
	Mat src_gray;
	cvtColor(roi,src_gray,CV_RGB2GRAY,0);
	vector<Vec3f> results;
	HoughCircles(src_gray,results,CV_HOUGH_GRADIENT,2,src_gray.cols,HOUGH_PARAM1,HOUGH_PARAM2,MINRADIUS,MAXRADIUS);
	// 如果没有检测到圆，则以轮廓边界中心为手掌中心
	if(results.size() == 0)palm_center = bound_center;
	for(int i = 0;i < results.size(); i++)
	{
		Vec3f circles;
		circles = results[i];
		lhough_center = hough_center;
		hough_center = Point(bound_rect.x+cvRound(circles[0]),bound_rect.y+cvRound(circles[1]));
		/*circle(src_re,hough_center,2,CV_RGB(0xff,0,0),2);
		circle(src_re,hough_center,cvRound(circles[2]),CV_RGB(0xff,0,0),2);*/
		// 手掌中心坐标校正
		if(distanceP2P(hough_center,bound_center)<=bound_rect.width/2){
			//首先比较轮廓边界中心的变化量，
			//如果轮廓中心的变化量远小于哈夫曼圆中心的变化量，则以轮廓中心坐标为准，否则求两者的平均
			double deta = abs(distanceP2P(lhough_center,hough_center) - distanceP2P(bound_center,lbound_center));
			double deta_r = deta/distanceP2P(bound_center,lbound_center);
			if(deta_r >=0.8 ){
				palm_center.x = (bound_center.x*0.8 + hough_center.x*0.2),palm_center.y = (bound_center.y*0.8 + hough_center.y*0.2);
				break;
			}
			else{
				palm_center.x = (bound_center.x*0.5 + hough_center.x*0.5),palm_center.y = (bound_center.y*0.5 + hough_center.y*0.5);
				break;
			}
		}
		else 
			palm_center = bound_center;
	}
	//circle(dst,palm_center,2,CV_RGB(0xff,0xff,0xff),2);
}
void HandGesture::handSegmented(GMM gmm,bool if_use_gmm,bool if_use_cb)
{
	// 混合高斯模型背景减法
	Mat img,src_re1;
	if(if_use_gmm&&!if_use_cb)
	{
		Mat src_cln = src_re.clone();
		gmm.Detect(src_cln,img);
		src_cln.copyTo(src_re1,img);
		/*namedWindow("背景减法",0);
		imshow("背景减法",img);*/
	}
	else
	{
		src_re1 = src_re.clone();
	}

	if(if_skin_samp)
	{
		blur(src_re1,img_tmp,Size(3,3));
		cvtColor(img_tmp,img_tmp,ORIGCOL2COL);
		skinColorSegHistogram(img_tmp,src_thre);
	}
	else if(if_use_cb)
	{
		Mat mask;
		wkrCB_frSkinSegment(src_re1,mask);
		mask.copyTo(src_thre);
	}
	else if(if_use_gmm)
	{
		// 首先进行高斯平滑
		GaussianBlur(src_re1,img_tmp,Size(5,5),0,0);
		skinColorSegHSV(img_tmp,src_thre);
	}
}

void HandGesture::showToClient(Mat src,HDC hdc,CRect *pRect)
{
	IplImage src1 = src;
	CvvImage m_CvvImage;
	m_CvvImage.CopyOf(&src1,1);
	m_CvvImage.DrawToHDC(hdc,pRect);
}