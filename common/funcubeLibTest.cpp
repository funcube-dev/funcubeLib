// FuncubeLibTest.cpp : Defines the entry point for the console application.
//
#include "targetver.h"
#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#endif

#include "gmock/gmock.h" 
#include "gtest/gtest.h"
#include "stdio.h"
#include "tchar.h"
#include <stdlib.h>

#define NEG_FILTER "-"
#define INTERACTIVE_FILTER "*Interactive*"
#define WIN_7_FILTER "*Win7*"
#define WIN_XP_FILTER "*WinXP*"

using ::testing::AtLeast; 
using ::testing::NiceMock;

int _tmain(int argc, _TCHAR* argv[])
{	
	DWORD dwMajorVersion = (DWORD)LOBYTE(LOWORD(GetVersion()));
	DWORD dwMinorVersion = (DWORD)HIBYTE(LOWORD(GetVersion()));

	// Exclude interactive tests by default, to overide pass in
	// filter on command line i.e. PluginLibTest.exe --gtest_filter=*
	
	// Exclude tests that will not pass on the current operating system,
	// Only major version checked so vista win7 win8 are all lumped together
	// just use Win7 for short
	switch(dwMajorVersion)
	{		
	case 6: // running on Vista/Win7 exclude XP tests
		::testing::GTEST_FLAG(filter) = 
			NEG_FILTER WIN_XP_FILTER ":" INTERACTIVE_FILTER;
		break;
	case 5: // running on W2k/XP exclude Win7 tests
		::testing::GTEST_FLAG(filter) = 
			NEG_FILTER WIN_7_FILTER ":" INTERACTIVE_FILTER;
		break;
	}

  // The following line must be executed to initialize Google Mock   
  // (and Google Test) before running the tests.   
  ::testing::InitGoogleMock(&argc, argv);   
  int result = 1;
  try
  {
   result = RUN_ALL_TESTS();
  }
  catch(...)
  {
	  printf("\nException running tests");
  }
    
  return result;

}