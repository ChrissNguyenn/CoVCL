// CoVCL.cpp : Defines the initialization routines for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "CoVCL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CCoVCLApp

BEGIN_MESSAGE_MAP(CCoVCLApp, CWinApp)
END_MESSAGE_MAP()


// CCoVCLApp construction

CCoVCLApp::CCoVCLApp()
{
	
}

CCoVCLApp::~CCoVCLApp() {
	delete gpMyPlugin;
}


// The one and only CCoVCLApp object

CCoVCLApp theApp;


// CCoVCLApp initialization

BOOL CCoVCLApp::InitInstance()
{
	CWinApp::InitInstance();
	
	return TRUE;
}

// CCoVCLApp shutdown

//---EuroScopePlugInInit-----------------------------------------------

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

		// create the instance
	* ppPlugInInstance = theApp.gpMyPlugin = new CoVCLPlugIn();
}

//---EuroScopePlugInExit-----------------------------------------------

void __declspec (dllexport) EuroScopePlugInExit(void)
{
	delete theApp;
}
