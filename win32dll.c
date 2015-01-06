/* win32dll.c - make the newlisp.exe usable as a DLL 
//
// Copyright (C) 1992-2007 Lutz Mueller <lutz@nuevatec.com>
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

#include "newlisp.h"
#include "protos.h"

/* note that DLL compile with MinGW is still in development */
#ifdef MINGW
#define EXPORT __declspec(dllexport) __stdcall
#define DLLCALL EXPORT
#endif

#ifdef WINCE
#define DLLCALL __declspec(dllexport) __stdcall
#endif

#include <winsock2.h>

extern void loadStartup(char *name);
extern LPSTR getLibname(void);
extern int evalSilent;
extern int opsys;
extern SYMBOL * mainArgsSymbol;


int dllInitialized = 0;
WSADATA WSAData;

char libName[MAX_LINE] = "newlisp.dll";

void initializeMain(void)
{
#ifndef WINCE
char name[MAX_LINE + 1];
#endif

WSAStartup(MAKEWORD(1,1), &WSAData);
#ifndef MINGW
#ifndef WINCE
_control87(0x033e, 0xffff); 
#endif
#endif

opsys += 64;

#ifdef SUPPORT_UTF8
opsys += 128;
#endif

initLocale();
initialize();
mainArgsSymbol->contents = (UINT)getCell(CELL_EXPRESSION);
initStacks();

/* printf("libname %s\n", libName); */

#ifndef WINCE
GetModuleFileName(GetModuleHandle(libName), name, MAX_LINE);

loadStartup(name);
#endif

dllInitialized = 1;
reset();
}

/* ------------ initialize DLL (gets called by Windows) ----------------- */

extern STREAM errorStream;

STREAM libStrStream;

int CALLBACK LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
return 1;
}


/* called automatically from nl-import when DLL is loaded from newLISP */

int DLLCALL dllName(LPSTR name)
{
strncpy(libName, name, MAX_LINE);
return(1);
}


/* ---- imported and called from a client using newlisp.dll ---- */

LPSTR DLLCALL newlispEvalStr(LPSTR cmd)
{
if(!dllInitialized) initializeMain();

if(setjmp(errorJump)) 
	{
	reset();
	initStacks();
	if(errorReg) 
		{
		executeSymbol(errorEvent, NULL);
		return((LPSTR)libStrStream.buffer);
		}
	else
	return((LPSTR)errorStream.buffer);
	}

openStrStream(&libStrStream, MAX_STRING, 1);
executeCommandLine(cmd, (UINT)&libStrStream, NULL);

if(evalSilent) evalSilent = 0;

return((LPSTR)libStrStream.buffer);
}


LPSTR DLLCALL dllEvalStr(LPSTR cmd)
{
return(newlispEvalStr(cmd));
}



/* ------------ called from Windows when unloading DLL ------------------ */

int DLLCALL WEP (int bSystemExit)
{
return(1);
}

/* eof */
