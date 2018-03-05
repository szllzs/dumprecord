// DumpRecord.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include "tchar.h"
#include <windows.h>
#include <shellapi.h>
#include <Dbghelp.h>
#include <iostream>  
#include <vector>  
using namespace std; 


#pragma comment(lib, "Dbghelp.lib")


void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException)  
{  
	// 创建Dump文件  
	//  
	HANDLE hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  


	// Dump信息  
	//  
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;  
	dumpInfo.ExceptionPointers = pException;  
	dumpInfo.ThreadId = GetCurrentThreadId();  
	dumpInfo.ClientPointers = TRUE;  

	// 写入Dump文件内容  
	//  
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &dumpInfo, NULL, NULL);  


	CloseHandle(hDumpFile);  
}  


LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	return NULL;
}


BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 ==   NULL)
		return FALSE;


	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if(pOrgEntry == NULL)
		return FALSE;


	unsigned char newJump[ 100 ];
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far


	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr -  dwOrgEntryAddr;


	newJump[ 0 ] = 0xE9;  // JMP absolute
	memcpy(&newJump[ 1 ], &dwRelativeAddr, sizeof(pNewFunc));
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(),    pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
	return bRet;
}

VOID ExecuteXcopy(TCHAR *src, TCHAR *dst)
{
	TCHAR szxcopycmd[MAX_PATH] = {0};

	wsprintf(szxcopycmd, _T("/c %s %s %s /e /y"), _T("xcopy"), src, dst);
	
	ShellExecute(NULL, _T("open"), _T("cmd"),  szxcopycmd, NULL, SW_HIDE);
}

LONG WINAPI UnhandledExceptionFilterEx(struct _EXCEPTION_POINTERS *pException)
{
	TCHAR szMbsFile[MAX_PATH] = { 0 };
	::GetModuleFileName(NULL, szMbsFile, MAX_PATH);
	TCHAR* pFind = _tcsrchr(szMbsFile, '\\');
	if(pFind)
	{
		TCHAR szProcessName[MAX_PATH] = {0};
		wsprintf(szProcessName,_T("%s"), pFind + 1);
		*(pFind+1) = 0;

		TCHAR szProcessNametmp[MAX_PATH] = {0};
		wsprintf(szProcessNametmp,_T("%s"), szProcessName);
		pFind = _tcsrchr(szProcessNametmp, '.');
		*(pFind) = 0;
		
		TCHAR szLogDir[MAX_PATH] = { 0 };
		wsprintf(szLogDir, _T("%sLog"), szMbsFile);
		TCHAR szpdbDir[MAX_PATH] = {0};
		wsprintf(szpdbDir, _T("%s%s.pdb"), szMbsFile, szProcessNametmp);
		TCHAR szexeDir[MAX_PATH] = {0};
		wsprintf(szexeDir, _T("%s%s"), szMbsFile, szProcessName);

		if (IDYES != MessageBox(NULL, _T("程序崩溃,立马截取dmp吗？"), _T("致命错误"), MB_YESNO))
		{
			return 0;
		}
		 
		//拼接时间，进程ID,线程ID
		_tcscat(szMbsFile, szProcessName);//拼接进程名
		SYSTEMTIME time;
		::GetLocalTime(&time);
		TCHAR chBuf[MAX_PATH] = {0};
		TCHAR szmkdircmd[MAX_PATH] = {0};
		TCHAR szmkdir[MAX_PATH] = {0};

		wsprintf(chBuf, _T("\\%s_%u-%u-%u_%u-%u-%u-%u_PID-%u_TID-%u.dmp"), szProcessName,
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond,
			time.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());
		
		wsprintf(szmkdir, _T("%s_%u-%u-%u_%u-%u-%u-%u_PID-%u_TID-%u"),
			szMbsFile,
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond,
			time.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());

		wsprintf(szmkdircmd, _T("/c %s %s_%u-%u-%u_%u-%u-%u-%u_PID-%u_TID-%u"),
			_T("mkdir"), szMbsFile,
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond,
			time.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());
		
		//创建dmp目录
		ShellExecute(NULL, _T("open"), _T("cmd"),  szmkdircmd, NULL, SW_HIDE);

		//收集日志信息到dmp目录
		ExecuteXcopy(szLogDir, szmkdir);

		//复制环境net4的调试库到dmp目录
		ExecuteXcopy(_T("C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscordacwks.dll"), szmkdir);
		ExecuteXcopy(_T("C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscordbi.dll"), szmkdir);
		ExecuteXcopy(_T("C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll"), szmkdir);

		//复制模块pdb文件和模块文件到dmp目录
		ExecuteXcopy(szpdbDir, szmkdir);
		ExecuteXcopy(szexeDir, szmkdir);

		_tcscat(szmkdir, chBuf);
		CreateDumpFile(szmkdir, pException);
	}


	// TODO: MiniDumpWriteDump
	FatalAppExit(-1,  _T("搜集出错信息完毕!"));

	return EXCEPTION_CONTINUE_SEARCH;
}


extern "C"
{
	void _declspec(dllexport) RunCrashHandler()
	{
		SetUnhandledExceptionFilter(UnhandledExceptionFilterEx);
		PreventSetUnhandledExceptionFilter();
	}
};

