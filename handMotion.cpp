
#include "stdafx.h"
#include "handMotion.hpp"

//int speedSamples[1000];
//int hdSizeSamples[1000];
//int spd = 0;
//int hds = 0;
int filecount = 0;

// 手势定义：对应于GESTURE的10个手势
Motion motions[10] = {{5,5},{5,5},{5,5},{5,5},{1,1},
					  {1,2},{5,0},{1,0},
					  {1,4},{1,2}};

HandMotion::HandMotion()
{

	if_FA1 = true;
	if_last_gesture = false;
	// 初始化手势变量
	motion.last_fgTipNum = 0;
	motion.cur_fgTipNum = 0;
	motion.last_handSize = 0;
	motion.cur_handSize = 0;
	motion.last_handScale = 0;
	motion.cur_handScale = 0;
	motion.cur_palmAxisAng = 0;
	motion.last_palmAxisAng = 0;
	motion.cur_palm_center = Point(0,0);
	motion.last_palm_center = Point(0,0);
	motion.is_zoom_mode = false;
	motion.if_continue = false;
	/////////
	//for(int i = 0;i < 500;i++)
	//{
	//	speedSamples[i] = 0;
	//	hdSizeSamples[i] = 0;
	//}
}

void HandMotion::init(int gestnums,int samprate)
{
	this->gestNums = gestnums;
	this->sampRate = samprate;
	waitTime = (int)(1000/samprate);
}
int HandMotion::sampGest(HandGesture &hg,GMM gmm,bool if_use_gmm,bool if_use_cb,vector<Point> &fgtipOut)
{
	Mat img;
	vector<Point> fgTipTemp;
	// 如果上一个手势存在，则赋值为第一个手势
	if(if_last_gesture){
		motion.last_handSize = motion.cur_handSize;
		motion.last_handScale = motion.cur_handScale;
		motion.last_palmAxisAng = motion.cur_palmAxisAng;
		motion.last_fgTipNum = motion.cur_fgTipNum;
		motion.last_palm_center = motion.cur_palm_center;
		motion.if_continue = true;
		lfgtips = fgtips;
	}
	//获取源图像帧
	if(hg.getSrcFrame() == -1)
		return -1;
	// 手区域分割
	hg.handSegmented(gmm,if_use_gmm,if_use_cb);
	// 轮廓提取
	hg.makeContours(hg.src_re);
	filecount++;
	if(hg.detectIfHand())
	{
		if_last_gesture = true;
		// 掌心提取
		//hg.palmCentrLoc(hg.src_re,img);
		hg.palmCentrLoc2(hg.cIdx,img);
		// 指尖提取
		hg.fingerTipLoc(fgTipTemp);
		// 手势结构赋值
		motion.cur_palmAxisAng = hg.palmAxisAngle(fgTipTemp);
		motion.cur_handSize = hg.bound_rect.area();
		motion.cur_handScale = handSizeScale(hg);
		motion.cur_palm_center = hg.palm_center;
		motion.cur_fgTipNum = fgTipTemp.size();
		//samplePalmCenter[continueFrames] = hg.palm_center;
		// 画出轮廓
		//drawContours(hg.src_re,hg.contours,hg.cIdx,CV_RGB(255,0,255),1);
		fgtips = fgTipTemp;
		fgtipOut = fgTipTemp;
	}
	else
	{
		if_last_gesture = false;
		motion.if_continue = false;
		motion.cur_handSize = 0;
		motion.cur_handScale = 0;
		motion.cur_palm_center = Point(0,0);
		motion.cur_fgTipNum = 0;
		hg.bound_rect.width = hg.bound_rect.height = 0;
	}
	return 0;
}

// 依据手心判断手的移动方向
int HandMotion::moveDirection()
{
	int direct_X,direct_Y;

	// 如果第一点和最后一点距离很近，判断为原地
	float dist = distanceP2P(motion.cur_palm_center,motion.last_palm_center);
	/////////////////////////////////////////////////
	/*if(spd < 1000)
	speedSamples[spd++] = dist;
	if(dist <= 100)			{return direction_and_speed;}
	else if(dist <= 200)	{direction_and_speed[1] = QUICK;}
	else if(dist >200)		{direction_and_speed[1] = RAPID;}*/
	Vec4f param;
	direct_X = motion.cur_palm_center.x - motion.last_palm_center.x;
	direct_Y = motion.cur_palm_center.y - motion.last_palm_center.y;

	Point change(direct_X,direct_Y);
	Point normX(1,0),normY(0,1),origin(0,0);
	float angleX = getAngle(change,origin,normX);
	float angleY = getAngle(change,origin,normY);
	/////////////////////大致方向 
	if( angleX <= 45 )
		return RIGHT;
	else if( angleX >= 135 )
		return LEFT;
	else if( angleY < 45 )
		return UP;
	else if( angleY > 135 )
		return DOWN;
}
int HandMotion::moveSpeed(HandGesture hg)
{
	float dist = distanceP2P(motion.cur_palm_center,motion.last_palm_center);
	int scale  = dist*100.0/hg.ImageDiagonal;

	if(scale <= 16)
		return COMMON;
	else if(scale <= 50)
		return QUICK;
	return -1;

}
int HandMotion::handSizeScale(HandGesture hg)
{
	int scale = hg.bound_rect.area()*100.0/hg.ImageLen;
	 if(scale <= 5)
		return S;
	else if(scale <= 10)
		return M;
	else if(scale <= 30)
		return L;
	else if(scale <= 50)
		return XL;
	return XXL;

}
// 将连续的手势组合翻译成鼠标或者键盘事件
int HandMotion::transMotion(HandGesture hg,Point &fgTip)
{
	int dirct = moveDirection();
	int speed = moveSpeed(hg);
	/////////////////////////////////////////////////
	//if(hds < 1000)
	//hdSizeSamples[hds++] = motion.cur_handSize;

	// 开启关闭缩放模式
	if(4 <= motion.last_fgTipNum&&0 == motion.cur_fgTipNum)
		motion.is_zoom_mode = true;
	else if(0 == motion.last_fgTipNum&&4 <= motion.cur_fgTipNum)
		motion.is_zoom_mode = false;
	////// 判断是不是滑动手势
	if(speed == QUICK)
	{
		return dirct;
		/*switch(dirct)
		{
		case DOWN:case LEFT:
			if(if_FA1)return PAGEDOWN;
			return PAGEUP;
		case UP:case RIGHT:
			if(if_FA1)return PAGEUP;
			return PAGEDOWN;
		default:
			break;
		}*/
	}
	// 判断缩放手势
	if(speed == COMMON&&motion.is_zoom_mode&&motion.cur_fgTipNum==0&&motion.last_fgTipNum==0){

		int scale = motion.cur_handScale - motion.last_handScale;
		// 缩放率分级别控制--不同级别缩放控制率不同
		if(scale == 0){
			double rate = motion.cur_handSize*1.0/motion.last_handSize;
			switch(motion.cur_handScale){
			case S:case XXL:
				if(rate >= 1.1)
					return ZOOMOUT;
				else if(rate <= 0.91)
					return ZOOMIN;
				break;
			case M:case XL:
				if(rate >= 1.5)
					return ZOOMOUT;
				else if(rate <= 0.67)
					return ZOOMIN;
				break;
			case L:
				if(rate >= 2.0)
					return ZOOMOUT;
				else if(rate <= 0.5)
					return ZOOMIN;
				break;
			default:
				return NONE;
			}
		}
		else {
			switch(scale)
			{
			case 1:
				return ZOOMOUT;
			case 2:case 3:case 4:
				return ZOOMOUTs;
			case -1:
				return ZOOMIN;
			case -2:case -3:case -4:
				return ZOOMINs;
			default:
				return NONE;
			}
		}
		/*double rate = motion.cur_handSize/(1.0+motion.last_handSize);
		if(rate >= 1.4&&rate < 2.5)
			return ZOOMOUT;
		else if(rate >= 2.5 && rate <= 5.0)
			return ZOOMOUTs;
		if(rate > 0.5&&rate <= 0.8)
			return ZOOMIN;
		else if(rate >= 0.3&&rate <= 0.5)
			return ZOOMINs;*/
	}
	
	// 判断是不是移动手势,如果是返回手势并赋值指尖坐标
	if(speed == COMMON&&motion.last_fgTipNum==1&&motion.cur_fgTipNum==1)
	{
		int distp2p = distanceP2P(fgtips[0],lfgtips[0]);
		if( distp2p<= 4||distp2p >= 200) // 两次距离太小或者太大视为不动
			return NONE;
		else
			fgTip = Point(fgtips[0].x - lfgtips[0].x,fgtips[0].y- lfgtips[0].y);
		return MOVE;
	}
	// 判断是否为点击
	if(speed == COMMON&&1 == motion.last_fgTipNum && 2 == motion.cur_fgTipNum)
		return LCLICK;
	return NONE;
}
// 手势搜索
int HandMotion::searchMotion(vector<Point> fg,int firNum,int lastNum)
{
	//// 判断是不是“八”或者“二”手势
	//if(firNum == 1&&lastNum==2)
	//{
	//	float angle = getAngle(fg[0],motion.cur_palm_center,fg[1]);
	//	if(angle > 40&& angle < 100)
	//		return PEN;
	//	else if(angle > 10 && angle < 40)
	//		return LCLICK;
	//}
	// 判断ESC、左键点击、右键点击手势
	if(1 == firNum && 0 == lastNum)
		return LCLICK;
	/*if(3 <= firNum&&0 == lastNum)
		motion.is_zoom_mode = true;
	else if(0 == firNum&&3 <= lastNum)
		motion.is_zoom_mode = false;*/
	/*else if(1 == firNum&&3 == lastNum)
		return LUPDOWN;*/
	/*else if(1 == firNum&&4  == lastNum)
		return RCLICK;*/
	else
	return NONE;
}
float HandMotion::getAngle(Point s, Point f, Point e){
	float l1 = distanceP2P(f,s);
	float l2 = distanceP2P(f,e);
	float dot=(s.x-f.x)*(e.x-f.x) + (s.y-f.y)*(e.y-f.y);
	float angle = acos(dot/(l1*l2));
	angle=angle*180/PI;
	return angle;
}
bool HandMotion::isContinue()
{
	return motion.if_continue;
}
float HandMotion::distanceP2P(Point a, Point b){
	float d= sqrt(fabs( pow((double)(a.x-b.x),2) + pow((double)(a.y-b.y),2) )) ;  
	return d;
}