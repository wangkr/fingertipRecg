
// pptHandController1.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CpptHandController1App:
// �йش����ʵ�֣������ pptHandController1.cpp
//

class CpptHandController1App : public CWinApp
{
public:
	CpptHandController1App();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CpptHandController1App theApp;