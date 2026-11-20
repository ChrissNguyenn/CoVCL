// CoVCL.h : main header file for the CoVCL DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "CoVCLPlugIn.h"

// CCoVCLApp
// See CoVCL.cpp for the implementation of this class
//

class CCoVCLApp : public CWinApp
{
public:
	CCoVCLApp();
	~CCoVCLApp();

// Overrides
public:
	virtual BOOL InitInstance();

	CoVCLPlugIn* gpMyPlugin = NULL;

	DECLARE_MESSAGE_MAP()
};
