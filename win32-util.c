/* 
   win32-util.c, utitity routines for native win32 port of newLISP 
*/

/*
//
// for function: gettimeofday() 
//
// Copyright (C) 1992-2002 Steve Adams
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2, 1991,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
*/

/*
//
// Copyright (c) 1992-2007 Lutz Mueller 
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2, 1991,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <windows.h>
#include <io.h>

#ifndef CYGWIN

#define WIN_PIPED_PROCESS 

#ifdef WINCE
#include <winsock2.h>
#endif


/* ------------------------  gettimeofday() ------------------------ */


struct timezone {
       int     tz_minuteswest;
       int     tz_dsttime;
};

int gettimeofday( struct timeval *tp, struct timezone *tzp )
{
  SYSTEMTIME lpt;
  TIME_ZONE_INFORMATION tzi;
  int result = 0;

  time_t        Now;
  Now         = time( NULL );
  tp->tv_sec  = Now;

  GetSystemTime(&lpt);
  tp->tv_usec = lpt.wMilliseconds * 1000;

  if(tzp != NULL)
  {
    result = GetTimeZoneInformation(&tzi);
    tzp->tz_minuteswest = tzi.Bias;
    tzp->tz_dsttime = tzi.DaylightBias;
  }

  return(result);
}

/* ---------------------------- pipes -------------------------------------- */

#ifdef WIN_PIPED_PROCESS
int winPipedProcess(char * cmd, int inpipe, int outpipe, int option)
{
STARTUPINFO si = { 0 };
PROCESS_INFORMATION process;
int result;
long fin, fout; 

/* GetStartupInfo(&si);  */
si.cb = sizeof(si);
si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
si.wShowWindow = option; /* SW_SHOW, SW_HIDE  get additional user option in Win32 versions */

fin = _get_osfhandle(inpipe);
fout = _get_osfhandle(outpipe);

si.hStdInput = (inpipe) ? (HANDLE)fin : GetStdHandle(STD_INPUT_HANDLE);
si.hStdOutput = (outpipe) ? (HANDLE)fout : GetStdHandle(STD_OUTPUT_HANDLE);
si.hStdError = (outpipe) ? (HANDLE)fout : GetStdHandle(STD_OUTPUT_HANDLE);

if((result = CreateProcess(NULL,cmd,NULL,NULL,TRUE,DETACHED_PROCESS,NULL,NULL,&si, &process)) == 0)
	return(0); 

/* let this be handled by parent process
CloseHandle((HANDLE)fin);
CloseHandle((HANDLE)fout);
*/

return(process.dwProcessId);
}

/*
typedef struct _PROCESS_INFORMATION { // pi  
    HANDLE hProcess; 
    HANDLE hThread; 
    DWORD dwProcessId; 
    DWORD dwThreadId; 
} PROCESS_INFORMATION; 
*/


int winPipe(UINT * inpipe, UINT * outpipe)
{
SECURITY_ATTRIBUTES sa = { 0 };
HANDLE pipe_r, pipe_w;

sa.nLength = sizeof(sa);
sa.bInheritHandle = TRUE;
sa.lpSecurityDescriptor = NULL;

if(!CreatePipe(&pipe_r, &pipe_w, &sa, 0))
	return(0);

*inpipe = _open_osfhandle((long) pipe_r, 0);
*outpipe = _open_osfhandle((long) pipe_w, 0);


return(1);
}

#endif

#endif

/* ---------------------------------- semaphores ---------------------------- */

UINT winCreateSemaphore(void)
{
SECURITY_ATTRIBUTES security = { 0 };
HANDLE hSemaphore;

security.nLength = sizeof(security);
security.bInheritHandle = TRUE;
security.lpSecurityDescriptor = NULL; /* default of caller */

hSemaphore = CreateSemaphore(&security, 0, 65536, NULL);

return((UINT)hSemaphore);
} 

UINT winWaitSemaphore(UINT hSemaphore)
{
DWORD dwWaitResult; 

dwWaitResult = WaitForSingleObject((HANDLE)hSemaphore, INFINITE);

if(dwWaitResult == WAIT_FAILED)
	return(FALSE);

return(TRUE);
}

UINT winSignalSemaphore(UINT hSemaphore, int count)
{
return(ReleaseSemaphore((HANDLE)hSemaphore, count, NULL));
}

int winDeleteSemaphore(UINT hSemaphore) 
{
return (CloseHandle((HANDLE)hSemaphore));
}

/*
See: http://www.codeguru.com/Cpp/W-P/win32/article.php/c1423/
QuerySemaphore() call
int winGetSemaphoreCount(UINT hSemaphore) {}
*/

/* ---------------------------- shared memory interface -------------------- */

UINT winSharedMemory(int size)
{
SECURITY_ATTRIBUTES sa = { 0 };
HANDLE hMemory;

sa.nLength = sizeof(sa);
sa.bInheritHandle = TRUE;
sa.lpSecurityDescriptor = NULL; /* default of caller */

hMemory = CreateFileMapping((HANDLE)0xFFFFFFFF, &sa, PAGE_READWRITE, 0, size, NULL);

return((UINT)hMemory);
}

UINT * winMapView(UINT hMemory, int size)
{
return((UINT*)MapViewOfFile((HANDLE)hMemory, FILE_MAP_WRITE, 0, 0, size));
}

/* ---------------------------- timer -------------------------------------- */


#include "newlisp.h"
#include "protos.h"

extern SYMBOL * timerEvent;
extern int milliSecTime(void);

/* 
UINT_PTR SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
BOOL KillTimer(HWND hWnd, UINT_PTR uIDEvent);
*/

void timerFunc(void * dummy);
int timerDuration = 0;
int rollover = 0;
int start;

CELL * p_timerEvent(CELL * params)
{
CELL * symCell;
double seconds;
int now;

if(params != nilCell) 
  {
  params = getSymbol(params, &timerEvent);

  if(params != nilCell)
	{
    getFloat(params, &seconds);
    timerDuration = 1000 * seconds;
	if(timerDuration > 0)
		{
    	start = milliSecTime();
    	_beginthread(timerFunc, 0, 0);
    	seconds = timerDuration/1000.0;
		}
	}
  else
    {
    /* return the elapsed time */
    seconds = ((now = milliSecTime()) < start ? 
			86400000 - start + now :
            now - start)/1000.0;  
    }
  return(stuffFloat(&seconds));    
  }
  
symCell = getCell(CELL_SYMBOL);
symCell->contents = (UINT)timerEvent;
return(symCell);
}


void timerFunc(void * dummy)
{
if(timerDuration == 0) return;

/* take care of midnight rollover */
if((rollover = start + timerDuration - 86400000) > 0)
    {
	while(milliSecTime() > start) mySleep(10); /* wait for rollover */
	while(milliSecTime() < rollover) mySleep(10);
	}
else
	while( milliSecTime() < (start + timerDuration) ) mySleep(10);

if(recursionCount)
  traceFlag |= TRACE_TIMER;
else /* if idle */
  executeSymbol(timerEvent, NULL);
}

/* ----------- relacement for UNIX realpath() for newLISP real-path------*/

CELL * p_realpath(CELL * params)
{
char * filepath;
char * realpath;

if(params != nilCell)
	getString(params, &filepath);
else
	filepath = ".";

realpath = alloca(MAX_PATH + 2);

if(GetFullPathName(filepath, MAX_PATH, realpath,NULL) == 0)
	return(nilCell);

return(stuffString(realpath));
}

/* eof */
