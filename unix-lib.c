/* unix-lib.c - make the newlisp shared newlisp library
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

extern void setupAllSignals(void);
extern int evalSilent;
extern int opsys;
extern SYMBOL * mainArgsSymbol;

int libInitialized = 0;

void initializeMain(void)
{
opsys += 64;

#ifdef SUPPORT_UTF8
opsys += 128;
#endif

initLocale();
initialize();

mainArgsSymbol->contents = (UINT)getCell(CELL_EXPRESSION);
setupAllSignals();

initStacks();

libInitialized = 1;
reset();
}


extern STREAM errorStream;
STREAM libStrStream;


/* ---- imported and called from a client using newlisp.so ---- */

char * newlispEvalStr(char * cmd)
{
if(!libInitialized) initializeMain();

if(setjmp(errorJump)) 
	{
	setupAllSignals(); 

	reset();
	initStacks();

	if(errorReg) 
		{
		executeSymbol(errorEvent, NULL);
		return(libStrStream.buffer);
		}
	else
	return(errorStream.buffer);
	}

openStrStream(&libStrStream, MAX_STRING, 1);
executeCommandLine(cmd, OUT_CONSOLE, NULL);

if(evalSilent) evalSilent = 0;

return(libStrStream.buffer);
}

/* eof */
