/* newlisp.c --- enrty point and main functions for newLISP
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
#include "pcre.h"
#include "protos.h"
#include "primes.h"

#ifdef MINGW
#include <winsock2.h>
#endif

#ifdef WINCE
#include <winsock2.h>
#define _finite finite
#endif

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef SUPPORT_UTF8
#include <wctype.h>
#endif

#define freeMemory free

#ifdef WINCC
#define INIT_FILE "init.lsp"
#define fprintf win32_fprintf
#define fgets win32_fgets
#define fclose win32_fclose
#else
#define INIT_FILE "/usr/share/newlisp/init.lsp"
#endif

#ifdef LIBRARY
extern STREAM libStrStream;
#endif

#ifdef LINUX
int opsys = 1;
char ostype[]="Linux";
#endif
#ifdef _BSD
int opsys = 2;
char ostype[]="BSD";
#endif
#ifdef MAC_OSX
int opsys = 3;
char ostype[]="OSX";
#endif

#ifdef SOLARIS
#ifdef TRU64
int opsys = 9;
char ostype[]="Tru64Unix";
#else
int opsys = 4;
char ostype[]="Solaris";
#endif
#endif

#ifdef CYGWIN
int opsys = 5;
char ostype[]="Win32 Cygwin";
#endif
#ifdef MINGW
int opsys = 6;
char ostype[]="Win32";
#endif
#ifdef OS2 
int opsys = 7; 
char ostype[]="OS/2"; 
#endif 
#ifdef WINCE
int opsys = 8;
char ostype[]="WinCE";
#endif

int version = 9100;

char copyright[]=
"\nnewLISP v.9.1.0 Copyright (c) 2007 Lutz Mueller. All rights reserved.\n\n%s\n\n";

#ifndef NEWLISP64
#ifdef SUPPORT_UTF8
char banner[]=
"newLISP v.9.1.0 on %s UTF-8%s\n\n";
#else
char banner[]=
"newLISP v.9.1.0 on %s%s\n\n";
#endif
#else
#ifdef SUPPORT_UTF8
char banner[]=
"newLISP v.9.1.0 64-bit on %s UTF-8%s\n\n";
#else
char banner[]=
"newLISP v.9.1.0 64-bit on %s%s\n\n";
#endif 
#endif

char banner2[]=
", execute 'newlisp -h' for more info.";

char linkOffset[] = "@@@@@@@@";

void printHelpText(void);

/* --------------------- globals -------------------------------------- */

/* interactive command line */

int commandLineFlag = TRUE;
int isTTY = FALSE;
int demonMode = 0;

int noPromptMode = 0;
int forcePromptMode = 0;
int httpMode = 0;

FILE * IOchannel;
int IOport = 0;
int logTraffic = 0;
#define LOG_LESS 1
#define LOG_MORE 2

/* initialization */
int MAX_CPU_STACK = 0x800;
int MAX_ENV_STACK;
int MAX_RESULT_STACK;
#ifndef NEWLISP64
long MAX_CELL_COUNT = 0x10000000;
#else
long MAX_CELL_COUNT = 0x800000000000000LL;
#endif

CELL * firstFreeCell = NULL;

CELL * nilCell;
CELL * trueCell;
CELL * lastCellCopied;
SYMBOL * nilSymbol;
SYMBOL * trueSymbol;
SYMBOL * starSymbol;
SYMBOL * plusSymbol;
SYMBOL * questionSymbol;
SYMBOL * atSymbol;
SYMBOL * currentFunc;
SYMBOL * argsSymbol;
SYMBOL * mainArgsSymbol;
SYMBOL * dolistIdxSymbol;

SYMBOL * sysSymbol[MAX_REGEX_EXP];

SYMBOL * errorEvent;
SYMBOL * currentContext = NULL;
SYMBOL * mainContext = NULL;
SYMBOL * demonRequest;
SYMBOL * timerEvent;

SYMBOL * symHandler[32];
int currentSignal = 0;

jmp_buf errorJump;

char lc_decimal_point;

/* error and exception handling */

#define EXCEPTION_THROW -1
int errorReg = 0;
CELL * throwResult;
int errnoSave;

/* buffer for read-line */
STREAM readLineStream;

/* compiler */

size_t cellCount = 0;
size_t symbolCount = 0;

int parStackCounter = 0;

/* expression evaluation */

static CELL * (*evalFunc)(CELL *) = NULL;
UINT * envStack = NULL;
UINT * resultStack = NULL;
UINT * lambdaStack = NULL;
int envStackIdx, resultStackIdx, lambdaStackIdx;
int evalSilent = 0;

extern PRIMITIVE primitive[];

int traceFlag = 0;
int evalCatchFlag = 0;
int recursionCount = 0;
int symbolProtectionLevel = 0; 

int prettyPrintPars = 0;
int prettyPrintCurrent = 0;
int prettyPrintFlags = 0;
int prettyPrintLength = 0;
char * prettyPrintTab = " ";
#define MAX_PRETTY_PRINT_LENGTH 64
UINT prettyPrintMaxLength =  MAX_PRETTY_PRINT_LENGTH;
int stringOutputRaw = TRUE;

#define pushLambda(A) (*(lambdaStack + lambdaStackIdx++) = (UINT)(A))
#define popLambda() ((CELL *)*(lambdaStack + --lambdaStackIdx))

int pushResultFlag = TRUE;

char startupDir[PATH_MAX]; /* start up directory, if defined via -w */
char logFile[PATH_MAX]; /* logFile, is define with -l, -L */

/* ============================== MAIN ================================ */

/*
void setupSignalHandler(int sig, void (* handler)(int))
{
static struct sigaction sig_act;
sig_act.sa_handler = handler;
sigemptyset(&sig_act.sa_mask);
sig_act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
if(sigaction(sig, &sig_act, 0) != 0)
	printf("Error setting signal:%d handler\n", sig);
}
*/

void setupSignalHandler(int sig, void (* handler)(int))
{
if(signal(sig, handler) == SIG_ERR)
	printf("Error setting signal:%d handler\n", sig);
}

#ifdef SOLARIS
void sigpipe_handler(int sig)
{
setupSignalHandler(SIGPIPE, sigpipe_handler);
}

void sigchld_handler(int sig)
{
waitpid(-1, (int *)0, WNOHANG);
}

void ctrlC_handler(int sig) 
{
char chr; 

setupSignalHandler(SIGINT, ctrlC_handler);

if(commandLineFlag != TRUE) return;

traceFlag |= TRACE_SIGINT;

printErrorMessage(ERR_SIGINT, NULL, 0);
printf("(c)ontinue, e(x)it, (r)eset:");
fflush(NULL);
chr = getchar();
if(chr == 'x') exit(1);
if(chr == 'c') traceFlag &= ~TRACE_SIGINT;
}


void sigalrm_handler(int sig)
{
setupSignalHandler(sig, sigalrm_handler);
/* check if not sitting idle */
if(recursionCount)
  traceFlag |= TRACE_TIMER;
else /* if idle */
  executeSymbol(timerEvent, NULL);
}

#endif /* solaris */


void setupAllSignals(void)
{
#ifdef SOLARIS
setupSignalHandler(SIGINT,ctrlC_handler);
#else
setupSignalHandler(SIGINT, signal_handler);
#endif

#ifndef WIN_32

#ifdef SOLARIS
setupSignalHandler(SIGALRM, sigalrm_handler);
setupSignalHandler(SIGVTALRM, sigalrm_handler);
setupSignalHandler(SIGPROF, sigalrm_handler);
setupSignalHandler(SIGPIPE, sigpipe_handler);
setupSignalHandler(SIGCHLD, sigchld_handler);
#else
setupSignalHandler(SIGALRM, signal_handler);
setupSignalHandler(SIGVTALRM, signal_handler);
setupSignalHandler(SIGPROF, signal_handler);
setupSignalHandler(SIGPIPE, signal_handler);
setupSignalHandler(SIGCHLD, signal_handler);
#endif

#endif
}

void signal_handler(int sig)
{
#ifndef WINCC
char chr; 
#endif

/* printf("SIG: %d\n", sig); */

if(sig > 32 || sig < 1) return;


#ifdef SOLARIS
switch(sig)
  {
  case SIGALRM:
  case SIGVTALRM:
  case SIGPROF:
    setupSignalHandler(sig, sigalrm_handler);
    break;
  case SIGPIPE:
    setupSignalHandler(SIGPIPE, sigpipe_handler);
    break;
  case SIGCHLD:
    setupSignalHandler(SIGCHLD, sigchld_handler);
    break;
  }
#else
setupSignalHandler(sig, signal_handler);
#endif

if(symHandler[sig - 1] != nilSymbol)
    {
    if(recursionCount)
        {
        currentSignal = sig;
        traceFlag |= TRACE_SIGNAL;
        return;
        }
    else
        {
        executeSymbol(symHandler[sig-1], stuffInteger(sig));
        return;
        }
    }

  
switch(sig)
    {
    case SIGINT:
        if(commandLineFlag != TRUE) return;

        printErrorMessage(ERR_SIGINT, NULL, 0);

#ifdef WIN_32
        traceFlag |= TRACE_SIGINT;
#else
        printf("\n(c)ontinue, (d)ebug, e(x)it, (r)eset:");
        fflush(NULL);
        chr = getchar();
        if(chr == 'x') exit(1);
        if(chr == 'd') 
            {
            traceFlag &= ~TRACE_SIGINT;
            openTrace();
            }
        if(chr == 'r') traceFlag |= TRACE_SIGINT;
        break;

    case SIGPIPE:
        break;
        
    case SIGALRM:
    case SIGVTALRM:
    case SIGPROF:
        /* check if not sitting idle */
        if(recursionCount)
            traceFlag |= TRACE_TIMER;
        else /* if idle */
            executeSymbol(timerEvent, NULL);
        break;

    case SIGCHLD:
        waitpid(-1, (int *)0, WNOHANG);
#endif
        break;
    
    default:
        return;
    }	

}
 

void loadStartup(char * name)
{
#ifdef WINCC
#ifndef LIBRARY
char * ptr;
char EXEName[MAX_LINE];
char initFile[MAX_LINE];

GetModuleFileName(NULL, EXEName, MAX_LINE);
name = EXEName;
#endif 
#endif

if(strncmp(linkOffset, "@@@@", 4) == 0)
        {
#ifdef WINCC
#ifndef LIBRARY
        ptr = name + strlen(name) - 1;
        while(ptr != name)
          {
          if(*ptr == '/' || *ptr == '\\') break;
          ptr--;
          }
        *ptr = 0;
        strncpy(initFile, name, MAX_LINE - 9);
        strcat(initFile, "/");
        strcat(initFile, INIT_FILE);
        loadFile(initFile, 0, 0, mainContext);
#else
        loadFile(INIT_FILE, 0, 0, mainContext);
#endif
#else        
        loadFile(INIT_FILE, 0, 0, mainContext);
#endif
        }
else    /* load encrypted part at offset */ 
	loadFile(name, *(UINT*)linkOffset, 1, mainContext);
}


#ifdef _BSD
struct lconv    *localeconv(void);
char            *setlocale(int, const char *);  
#endif

void initLocale(void)
{
struct lconv * lc;
char * locale;

#ifndef SUPPORT_UTF8
locale = setlocale(LC_ALL, "C");
#else
locale = setlocale(LC_ALL, "");
#endif

if (locale != NULL)
  stringOutputRaw = (strcmp(locale, "C") == 0);

lc = localeconv();
lc_decimal_point = *lc->decimal_point;
}


#ifndef  LIBRARY
char * getArg(char * * arg, int argc, int * index)
{
if(strlen(arg[*index]) > 2)
	return(arg[*index] + 2);

if(*index >= (argc - 1))
	{
	printf("missing parameter for %s\n", arg[*index]);
	exit(-1);
	}

*index = *index + 1;

return(arg[*index]);
}

#ifndef WINCC
char ** MainArgs;
#endif 

CELL * getMainArgs(char * mainArgs[])
{
CELL * argList;
#ifndef LIBRARY
CELL * lastEntry;
int idx = 0;
#endif

#ifndef WINCC
MainArgs = mainArgs;
#endif

argList = getCell(CELL_EXPRESSION);

#ifndef LIBRARY
lastEntry = NULL;
while(mainArgs[idx] != NULL)
	{
	if(lastEntry == NULL)
		{
		lastEntry = stuffString(mainArgs[idx]);
		argList->contents = (UINT)lastEntry;
		}
	else
		{
		lastEntry->next = stuffString(mainArgs[idx]);
		lastEntry = lastEntry->next;
		}
	idx++;
	}
#endif 

return(argList);
}

    
int main(int argc, char * argv[])
{
char command[MAX_LINE];
STREAM cmdStream;
int idx;
#ifdef READLINE
char * cmd;
#endif

#ifdef WINCC
WSADATA WSAData;
WSAStartup(MAKEWORD(1,1), &WSAData);
#endif

#ifdef SUPPORT_UTF8
opsys += 128;
#endif

memset(&cmdStream, 0, sizeof(STREAM));

initLocale();
IOchannel = stdin;

initialize();
initStacks();

mainArgsSymbol->contents = (UINT)getMainArgs(argv);

if((errorReg = setjmp(errorJump)) != 0) 
    {
    if(errorReg && (errorEvent != nilSymbol)) 
        executeSymbol(errorEvent, NULL);
    else exit(-1);
    goto AFTER_ERROR_ENTRY;
    }

setupAllSignals();

loadStartup(argv[0]);
errno = 0;

#ifndef WINCC
realpath(".", startupDir);
#else
GetFullPathName(".", MAX_PATH, startupDir, NULL);
#endif

for(idx = 1; idx < argc; idx++)
	{
#ifndef  NOCMD
	if(strncmp(argv[idx], "-c", 2) == 0)
		noPromptMode = TRUE;

	if(strncmp(argv[idx], "-C", 2) == 0)
		forcePromptMode = TRUE;

	if(strncmp(argv[idx], "-http", 5) == 0)
		{
		noPromptMode = TRUE;
		httpMode = TRUE;
		}

	if(strncmp(argv[idx], "-s", 2) == 0)
		{
		MAX_CPU_STACK = atoi(getArg(argv, argc, &idx));

		if(MAX_CPU_STACK < 1024) MAX_CPU_STACK = 1024;
		initStacks();
		continue;
		}

	if(strncmp(argv[idx], "-p", 2) == 0 || strncmp(argv[idx], "-d", 2) == 0  )
		{
		if(strncmp(argv[idx], "-d", 2) == 0)
			demonMode = TRUE;

		IOport = atoi(getArg(argv, argc, &idx));

		setupServer(0);
		continue;
		}

	if(strncmp(argv[idx], "-e", 2) == 0)
		{
		executeCommandLine(getArg(argv, argc, &idx), OUT_CONSOLE, &cmdStream);
		exit(0);
		}		

	if(strncmp(argv[idx], "-l", 2) == 0 || strncmp(argv[idx], "-L", 2) == 0)
		{
		logTraffic = (strncmp(argv[idx], "-L", 2) == 0) ? LOG_MORE : LOG_LESS;
#ifndef WINCC
		realpath(getArg(argv, argc, &idx), logFile);
#else
		GetFullPathName(getArg(argv, argc, &idx) , MAX_PATH, logFile, NULL);
#endif
		continue;
		}

	if(strncmp(argv[idx], "-m", 2) == 0)
		{
#ifndef NEWLISP64
		MAX_CELL_COUNT =  abs(0x0010000 * atoi(getArg(argv, argc, &idx)));
#else
		MAX_CELL_COUNT =  abs(0x0008000 * atoi(getArg(argv, argc, &idx)));
#endif
		continue;
		}

	if(strncmp(argv[idx], "-w", 2) == 0)
		{
#ifndef WINCC
		realpath(getArg(argv, argc, &idx), startupDir);
#else
		GetFullPathName(getArg(argv, argc, &idx) , MAX_PATH, startupDir, NULL);
#endif
		chdir(startupDir);	
		continue;
		}	

	if(strcmp(argv[idx], "-h") == 0)
		{
		printHelpText();
		exit(0);
		}
#endif
	
	loadFile(argv[idx], 0, 0, mainContext);
	}

AFTER_ERROR_ENTRY:

if(isatty(fileno(IOchannel)))
	{
	isTTY = TRUE;
	if(!noPromptMode) 	
		varPrintf(OUT_CONSOLE, banner, ostype, banner2);
	}
else
	{
#ifdef WINCC
	/* its a faked FILE struct, see win32_fdopen() in nl-sock.c */
        if(!isSocketStream(IOchannel))
#endif
        setbuf(IOchannel,0);
	}


errorReg = setjmp(errorJump);

setupAllSignals();
reset();
initStacks();

if(errorReg) executeSymbol(errorEvent, NULL);

while(TRUE)
	{
	if(commandLineFlag == TRUE)
		{
#ifdef READLINE
		if(isTTY) 
			{
			errnoSave = errno;
			if((cmd = readline(prompt())) == NULL) exit(0);
			errno = errnoSave; /* reset errno, set by readline() */
			if(strlen(cmd) > 0) add_history(cmd);
			executeCommandLine(cmd, OUT_CONSOLE, &cmdStream);
			free(cmd);
			continue;
			}

		if(IOchannel != stdin || forcePromptMode) 
			varPrintf(OUT_CONSOLE, prompt());
#endif
#ifndef READLINE
		if(isTTY || IOchannel != stdin || forcePromptMode) 
			varPrintf(OUT_CONSOLE, prompt());
#endif	
		if(IOchannel == NULL || fgets(command, MAX_LINE - 1, IOchannel) == NULL)
			{
			if(!demonMode)  exit(1);
			if(IOchannel != NULL) fclose(IOchannel);
			setupServer(1);
			continue;
			}

		executeCommandLine(command, OUT_CONSOLE, &cmdStream);
        }
	}

#ifndef WINCC
return 0;
#endif
}
#endif


void printHelpText(void)
{
varPrintf(OUT_CONSOLE, copyright, 
		"usage: newlisp [file ...] [options ...] [file ...]\n\noptions:\n");
varPrintf(OUT_CONSOLE, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n%s\n\n",
	" -h this help",
	" -s <stacksize>",
	" -m <max-mem-megabyte>",
	" -l log connections only",
	" -L log all",
	" -p <port-number>",
	" -d <port-number>",
	" -e <quoted lisp expression>",
	" -c no prompts, HTTP",
	" -C force prompts",
	" -http HTTP only",
	" -w <working-directory>",
	"more information at http://newlisp.org");
}


void setupServer(int reconnect)
{
if(IOport == 0 || (IOchannel  = serverFD(IOport, reconnect)) == NULL)
	{
	printf("newLISP server setup on port %d failed.\n", IOport);
	exit(1);
	}

#ifdef WINCC
	if(!isSocketStream(IOchannel))
#endif
        setbuf(IOchannel,0);

if(!reconnect && !noPromptMode)
	varPrintf(OUT_CONSOLE, banner, ostype, ".");
}


char * prompt(void)
{
char * context;
static char string[32];

if(evalSilent || noPromptMode) 
	{
	evalSilent = 0;
	return("");
	}
	
if(currentContext != mainContext)
	context = currentContext->name;
else context = "";

if(traceFlag & TRACE_SIGINT) 
	{
	traceFlag &= ~TRACE_SIGINT;
	longjmp(errorJump, errorReg);
	}
	
if(traceFlag)
	snprintf(string, 31, "%s %d> ", context, recursionCount);
else
	snprintf(string, 31, "%s> ", context);

return(string);
}


void reset()
{
recoverEnvironment(0);

collectGarbage();

if(printDevice) close((int)printDevice);
printDevice = recursionCount = resultStackIdx = envStackIdx = lambdaStackIdx = 0;
symbolProtectionLevel = traceFlag = prettyPrintFlags = 0;
evalFunc = NULL;
pushResultFlag = commandLineFlag = TRUE;
currentContext = mainContext;
}


void recoverEnvironment(int index)
{
SYMBOL * symbol;
CELL * cell;

while(envStackIdx > index)
	{
	symbol = (SYMBOL *)popEnvironment();
	cell = (CELL*)popEnvironment();
	if(cell != (CELL*)symbol->contents)
		{
		deleteList((CELL*)symbol->contents);
		symbol->contents = (UINT)cell;
		if(isProtected(symbol->flags))
			symbol->flags &= ~SYMBOL_PROTECTED;
		}
	}
}


void executeCommandLine(char * command, int outDevice, STREAM * cmdStream)
{
STREAM stream;
char buff[MAX_LINE];

if(strlen(command) == 0 || *command == '\n') return;

if(noPromptMode) 
	{
	if(logTraffic == LOG_MORE) 
		writeLog(command, 0);
	if(strncmp(command, "GET /", 5) == 0) 
		{
		executeHTTPrequest(command + 5, HTTP_GET_URL);
		return;
		}
	else if(strncmp(command, "HEAD /", 6) == 0)
		{
		executeHTTPrequest(command + 5, HTTP_GET_HEAD);
		return;
		}
	else if(strncmp(command, "PUT /", 5) == 0)
		{
		executeHTTPrequest(command + 5, HTTP_PUT_URL);
		return;
		}
	else if(strncmp(command, "POST /", 6) == 0)
		{
		executeHTTPrequest(command + 6, HTTP_POST_URL);
		return;
		}
	else if(strncmp(command, "DELETE /", 8) == 0)
		{
		executeHTTPrequest(command + 8, HTTP_DELETE_URL);
		return;
		}

	if(httpMode) return;
	}

if(*command == '!' && *(command + 1) != ' ' && strlen(command) > 2)
	{
	system((command + 1));
	return;
	}
	
if(cmdStream != NULL && strncmp(command, "[cmd]", 5) == 0)
	{
	openStrStream(cmdStream, 1024, TRUE);	
	while(fgets(buff, MAX_LINE - 1, IOchannel) != NULL)
		{
		if(strncmp(buff, "[/cmd]", 6) == 0)
			{
			/* modify stream for evaluation */
			if(logTraffic) writeLog(cmdStream->buffer, TRUE);
			makeStreamFromString(&stream, cmdStream->buffer);
			evaluateStream(&stream, OUT_CONSOLE, 0);
			closeStrStream(cmdStream);
			return;
			}
		writeStreamStr(cmdStream, buff, 0);
		}
	closeStrStream(cmdStream);
	if(!demonMode)  exit(1);
	if(IOchannel != NULL) fclose(IOchannel);
	setupServer(1);
	return;
	}

if(logTraffic) writeLog(command, TRUE);
prettyPrintLength = 0;

makeStreamFromString(&stream, command);
evaluateStream(&stream, outDevice, 0);

return;
}


CELL * evaluateStream(STREAM * stream, UINT outDevice, int flag)
{
CELL * program;
CELL * eval = nilCell;
int resultIdxSave;
int result;

result = TRUE;
resultIdxSave = resultStackIdx;
while(result)
	{
	pushResult(program = getCell(CELL_QUOTE));
	result = compileExpression(stream, program);
	if(result)
		{
		if(flag && eval != nilCell) deleteList(eval);
		eval = evaluateExpression((CELL *)program->contents);
		if(outDevice != 0 && !evalSilent) 
		    {
            printCell(eval, TRUE, outDevice);
            varPrintf(outDevice, "\n");
            if(logTraffic == LOG_MORE)
            	{
                printCell(eval, TRUE, OUT_LOG);
                writeLog("", TRUE);
                }
            }                      
           if(flag) eval = copyCell(eval);
		}
	cleanupResults(resultIdxSave);
	}

if(flag) return(eval);
return(NULL);
}

long executeSymbol(SYMBOL * symbol, CELL * params)
{
CELL * program;
CELL * cell;
int resultIdxSave;

if(symbol == nilSymbol || symbol == NULL) return(0);
resultIdxSave = resultStackIdx;
pushResult(program = getCell(CELL_EXPRESSION));
cell = getCell(CELL_SYMBOL);
program->contents = (UINT)cell;
cell->contents = (UINT)symbol;
if(params != NULL) cell->next = params;
cell = evaluateExpression(program);
cleanupResults(resultIdxSave);

return(cell->contents);
}


void initialize()
{
int i;
SYMBOL * symbol;
CELL * pCell;
char  symName[8];

/* build true and false cells */

nilCell = getCell(CELL_NIL);
trueCell = getCell(CELL_TRUE);
nilCell->contents = (UINT)nilCell;
trueCell->contents = (UINT)trueCell;
nilCell->next = trueCell->next = nilCell;

/* build first symbol and context MAIN */
mainContext = currentContext = translateCreateSymbol("MAIN", CELL_CONTEXT, NULL, TRUE);
makeContextFromSymbol(mainContext, mainContext);

/* build symbols for primitives */

for(i = 0; primitive[i].name != NULL; i++)
	{
	pCell = getCell(CELL_PRIMITIVE);
	symbol = translateCreateSymbol(
		primitive[i].name, CELL_PRIMITIVE, mainContext, TRUE);
	symbol->contents = (UINT)pCell;
	symbol->flags = primitive[i].prettyPrint | SYMBOL_PROTECTED | SYMBOL_GLOBAL | SYMBOL_BUILTIN;
	pCell->contents = (UINT)primitive[i].function;
	pCell->aux = (UINT)symbol->name;
	}

/* build true, nil, * and ? symbols */

trueSymbol = translateCreateSymbol("true", CELL_TRUE, mainContext, TRUE);
trueSymbol->contents = (UINT)trueCell;
nilSymbol = translateCreateSymbol("nil", CELL_NIL, mainContext, TRUE);
nilSymbol->contents = (UINT)nilCell;
starSymbol = translateCreateSymbol("*", CELL_PRIMITIVE, mainContext, TRUE);
plusSymbol = translateCreateSymbol("+", CELL_PRIMITIVE, mainContext, TRUE);
questionSymbol = translateCreateSymbol("?", CELL_NIL, mainContext, TRUE);
atSymbol = translateCreateSymbol("@", CELL_NIL, mainContext, TRUE);
argsSymbol = translateCreateSymbol("$args", CELL_NIL, mainContext, TRUE);
mainArgsSymbol = translateCreateSymbol("$main-args", CELL_NIL, mainContext, TRUE);
dolistIdxSymbol = translateCreateSymbol("$idx", CELL_NIL, mainContext, TRUE);

for(i = 0; i < MAX_REGEX_EXP; i++)
	{
	snprintf(symName, 8, "$%d", i);
	sysSymbol[i] = translateCreateSymbol(symName, CELL_NIL, mainContext, TRUE);
	sysSymbol[i]->flags |= SYMBOL_GLOBAL | SYMBOL_BUILTIN;
	}

currentFunc = errorEvent = timerEvent  = nilSymbol;

trueSymbol->flags |= SYMBOL_PROTECTED | SYMBOL_GLOBAL;
nilSymbol->flags |= SYMBOL_PROTECTED | SYMBOL_GLOBAL;
questionSymbol->flags |= SYMBOL_PROTECTED | SYMBOL_GLOBAL;
atSymbol->flags |=  SYMBOL_GLOBAL | SYMBOL_BUILTIN;
argsSymbol->flags |= SYMBOL_GLOBAL | SYMBOL_BUILTIN | SYMBOL_PROTECTED;
mainArgsSymbol->flags |= SYMBOL_GLOBAL | SYMBOL_BUILTIN | SYMBOL_PROTECTED;
dolistIdxSymbol->flags |= SYMBOL_GLOBAL | SYMBOL_BUILTIN | SYMBOL_PROTECTED;
argsSymbol->contents = (UINT)getCell(CELL_EXPRESSION);

symbol = translateCreateSymbol("ostype", CELL_STRING, mainContext, TRUE);
symbol->contents = (UINT)stuffString(ostype);
symbol->flags |= SYMBOL_GLOBAL | SYMBOL_BUILTIN | SYMBOL_PROTECTED;

/* init signal handlers */
for(i = 0; i < 32; i++)
  symHandler[i] = nilSymbol;

/* init readLineStream */
openStrStream(&readLineStream, 16, 0);
}


void initStacks()
{
MAX_ENV_STACK = (MAX_CPU_STACK * 8 * 2);
MAX_RESULT_STACK = (MAX_CPU_STACK * 2);
if(envStack != NULL) freeMemory(envStack);
if(resultStack != NULL) freeMemory(resultStack);
if(lambdaStack != NULL) freeMemory(lambdaStack);
envStack = (UINT *)allocMemory((MAX_ENV_STACK + 16) * sizeof(UINT));
resultStack = (UINT *)allocMemory((MAX_RESULT_STACK + 16) * sizeof(UINT));
lambdaStack = (UINT *)allocMemory((MAX_RESULT_STACK + 16) * sizeof(UINT));
envStackIdx = resultStackIdx = lambdaStackIdx = 0;
}

	
/* ------------------------- evaluate s-expression --------------------- */


CELL * evaluateExpression(CELL * cell)
{
CELL * result;
CELL * args = NULL;
CELL * pCell = NULL;
SYMBOL * newContext = NULL;
SYMBOL * sPtr;
int resultIdxSave = 0;

if(cell->type & EVAL_SELF_TYPE_MASK) return cell;
switch(cell->type)
	{
	case CELL_SYMBOL:
	case CELL_CONTEXT:
		return((CELL*)((SYMBOL *)cell->contents)->contents);

	case CELL_QUOTE:
		return((CELL *)cell->contents);

	case CELL_EXPRESSION:
		args = (CELL *)cell->contents;
		resultIdxSave = resultStackIdx;                
		
		if(++recursionCount > (int)MAX_CPU_STACK)
			fatalError(ERR_OUT_OF_CALL_STACK, args, 0);
		
		if(args->type == CELL_SYMBOL) /* precheck for speedup */
			pCell =  (CELL*)((SYMBOL *)args->contents)->contents;
		else
			pCell = evaluateExpression(args);

		if(traceFlag) traceEntry(cell, pCell, args);

		if(pCell->type == CELL_PRIMITIVE)
			{
			evalFunc = (CELL *(*)(CELL*))pCell->contents;
			result = (*evalFunc)(args->next);
			evalFunc = NULL;
			break;
			}
		
		if(pCell->type == CELL_LAMBDA)
			{ 
			pushLambda(args);
			if(args->type == CELL_SYMBOL)
			    newContext = ((SYMBOL *)args->contents)->context;
            else
                newContext = currentContext;
			result = evaluateLambda((CELL *)pCell->contents, args->next, newContext); 
			--lambdaStackIdx; 
			break; 
			}
		
		if(pCell->type == CELL_MACRO)
			{ 
			if(args->type == CELL_SYMBOL)
			    newContext = ((SYMBOL *)args->contents)->context;
             else
                newContext = currentContext;
			result = evaluateMacro((CELL *)pCell->contents, args->next, newContext);
			break;
			}

		if(pCell->type == CELL_IMPORT_CDECL
#ifdef WINCC
		   || pCell->type == CELL_IMPORT_DLL
#endif
			)
			{
			result = executeLibfunction(pCell, args->next);  
 			break;
			}

			/* check for 'default' functor
			* allow function call with context name, i.e: (ctx)
			* assumes that a ctx:ctx contains a function
			*/
		if(pCell->type == CELL_CONTEXT)
			{
			newContext = (SYMBOL *)pCell->contents;
			sPtr= translateCreateSymbol(newContext->name, CELL_NIL, newContext, TRUE);
			pCell = (CELL *)sPtr->contents;

			if(pCell->type == CELL_PRIMITIVE)
				{
				evalFunc = (CELL *(*)(CELL*))pCell->contents;
				result = (*evalFunc)(args->next);
				evalFunc = NULL;
				break;
				}

			else if(pCell->type == CELL_LAMBDA)
				{
				pushLambda(args);
				result = evaluateLambda((CELL *)pCell->contents, args->next, newContext); 
				--lambdaStackIdx; 
				break; 
				}

			else if(pCell->type  == CELL_MACRO)
				{
				result = evaluateMacro((CELL *)pCell->contents, args->next, newContext); 
				break; 
				}

			}
			

		/* allow 'implicit indexing' if pCell is a list, array, string or number:
                   (pCell idx1 idx2 ...) 
		*/
                
		if(args->next != nilCell)
			{
			if(pCell->type == CELL_EXPRESSION)
				result = copyCell(implicitIndexList(pCell, args->next));

			else if(pCell->type == CELL_ARRAY)
				result = copyCell(implicitIndexArray(pCell, args->next));

			else if(pCell->type == CELL_STRING)
				result = implicitIndexString(pCell, args->next);
                              
			else if(isNumber(pCell->type))
				result = implicitNrestSlice(pCell, args->next);
                              
			else result = errorProcExt(ERR_INVALID_FUNCTION, cell);                              
			}
		else 
			result = errorProcExt(ERR_INVALID_FUNCTION, cell);
            break;

	case CELL_DYN_SYMBOL:
		return((CELL*)(getDynamicSymbol(cell))->contents);
		
	default:
		result = nilCell;
	}

while(resultStackIdx > resultIdxSave)
	deleteList(popResult());

if(pushResultFlag) 
	{
	if(resultStackIdx > MAX_RESULT_STACK)
		fatalError(ERR_OUT_OF_CALL_STACK, pCell, 0);
	pushResult(result);
	}
else pushResultFlag = TRUE;

if(traceFlag) traceExit(result, cell, pCell, args);
--recursionCount;
return(result);
}


CELL *  evaluateExpressionSafe(CELL * cell, int * errNo)
{
jmp_buf errorJumpSave;
CELL * result;

memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));
if((*errNo = setjmp(errorJump)) != 0)
	{
	memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
	return(NULL);
	}

result = evaluateExpression(cell);
memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
return(result);
}


/* a symbol belonging to a dynamic context */
/* the parent context symbol points to the real context */
/* cell->contents -> name str of this symbol */
/* cell->aux -> symbol var which holds context (dynamic) */
/* ((SYMBOL*)cell->aux)->contents -> context cell */
SYMBOL * getDynamicSymbol(CELL * cell)
{
CELL * contextCell;

contextCell = (CELL *)((SYMBOL *)cell->aux)->contents;
if(contextCell->type != CELL_CONTEXT)
	fatalError(ERR_CONTEXT_EXPECTED, stuffSymbol((SYMBOL*)cell->aux), TRUE);

return(translateCreateSymbol( 
		(char*)cell->contents,		/* name of dyn symbol */
		CELL_NIL,
		(SYMBOL*)contextCell->contents,	/* contextPtr */
		TRUE));
}


CELL * evalCheckProtected(CELL * cell, CELL * * flagPtr)
{
CELL * result;
SYMBOL * sPtr;

if(isSymbol(cell->type))
	{
	if(cell->type == CELL_SYMBOL)
		sPtr = (SYMBOL *)cell->contents;
	else
		sPtr = getDynamicSymbol(cell);

   	if(isProtected(sPtr->flags))
		return(errorProcExt(ERR_SYMBOL_PROTECTED, cell));

	return((CELL *)sPtr->contents);
	}

symbolProtectionLevel = recursionCount;
result = evaluateExpression(cell);
if(symbolProtectionLevel == 0xFFFFFFFF)
	{
	if(flagPtr == NULL)
		return(errorProcExt(ERR_SYMBOL_PROTECTED, cell));
	else *flagPtr = cell;
	}

symbolProtectionLevel = 0;
return(result);
}


/* -------------------- evaluate lambda function ----------------------- */

CELL * evaluateLambda(CELL * localLst, CELL * arg, SYMBOL * newContext)
{
CELL * local;
CELL * result = nilCell;
CELL * cell;
SYMBOL * symbol;
SYMBOL * contextSave;
int localCount = 0;

if(envStackIdx > (UINT)MAX_ENV_STACK)
	return(errorProc(ERR_OUT_OF_ENV_STACK));

if(localLst->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_INVALID_LAMBDA, localLst));

/* evaluate arguments */
if(arg != nilCell)
	{
	/* this symbol precheck does 10% speed improvment on lambdas  */
	if(arg->type == CELL_SYMBOL)
		cell = result = copyCell((CELL*)((SYMBOL *)arg->contents)->contents);
	else
		cell = result = copyCell(evaluateExpression(arg));
       
	while((arg = arg->next) != nilCell)
		{
		if(arg->type == CELL_SYMBOL)
			cell->next = copyCell((CELL*)((SYMBOL *)arg->contents)->contents);
		else
			cell->next = copyCell(evaluateExpression(arg));

		cell = cell->next;
		}
	}

/* change to new context */
contextSave = currentContext;
currentContext = newContext;

/* save environment and get parameters */
local = (CELL*)localLst->contents;
GET_LOCAL:
	{
	if(local->type == CELL_SYMBOL)
		symbol = (SYMBOL *)local->contents;
	/* get default parameters */
	else if(local->type == CELL_EXPRESSION)
		{
		if(((CELL*)local->contents)->type == CELL_SYMBOL)
			{
			cell = (CELL *)local->contents;
			if(cell->type == CELL_SYMBOL)
				{
				symbol = (SYMBOL *)cell->contents;
				if(result == nilCell)
					result = copyCell(evaluateExpression(cell->next));
				}
			else goto GOT_LOCALS;
			}
		else goto GOT_LOCALS;
		}
	else goto GOT_LOCALS;

	if(isProtected(symbol->flags))
		return(errorProcExt(ERR_SYMBOL_PROTECTED, local));

	/* save symbol environment */
	pushEnvironment(symbol->contents);
	pushEnvironment((UINT)symbol);

	/* fill local symbols */
	symbol->contents = (UINT)result;
	cell = result;
	result = result->next;

	/* unlink list */
	cell->next = nilCell;

	local = local->next;
	localCount++;
	}
goto GET_LOCAL;

GOT_LOCALS:
/* put unassigned args in $args */
pushEnvironment(argsSymbol->contents);
pushEnvironment((UINT)argsSymbol);
argsSymbol->contents = (UINT)getCell(CELL_EXPRESSION);
if(result != nilCell)
  ((CELL*)argsSymbol->contents)->contents = (UINT)result;
++localCount;

/* evaluate body expressions */
cell = localLst->next;
result = nilCell;
while(cell != nilCell)
	{
	result = evaluateExpression(cell);
	cell = cell->next;
	}
result = copyCell(result);

/* recover environment of local symbols */
while(localCount--)
	{
	symbol = (SYMBOL *)popEnvironment();
	if(isProtected(symbol->flags) && (symbol != argsSymbol))
		symbol->flags &= ~SYMBOL_PROTECTED;
	deleteList((CELL *)symbol->contents);
	symbol->contents = popEnvironment();
	}

currentContext = contextSave;
return(result);
}


CELL * evaluateMacro(CELL * localLst, CELL * arg, SYMBOL * newContext)
{
CELL * local;
CELL * result;
CELL * cell;
SYMBOL * symbol;
SYMBOL * contextSave;
int localCount;

if(envStackIdx > (UINT)MAX_ENV_STACK)
	return(errorProc(ERR_OUT_OF_ENV_STACK));

if(localLst->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_INVALID_MACRO, localLst));
local = (CELL *)localLst->contents;

contextSave = currentContext;
currentContext = newContext;

/* save environment and get parameters */
localCount = 0;
GET_ARGS:
  {
  if(local->type == CELL_SYMBOL)
  	symbol = (SYMBOL *)local->contents;
  /* get default parameters */
  else if(local->type == CELL_EXPRESSION)
	{
	if(((CELL*)local->contents)->type == CELL_SYMBOL)
		{
		cell = (CELL *)local->contents;
		if(cell->type == CELL_SYMBOL)
			{
			symbol = (SYMBOL *)cell->contents;
			if(arg == nilCell)
				arg = evaluateExpression(cell->next);
			}
		else goto GOT_ARGS;
		}
	else goto GOT_ARGS;
	}
  else goto GOT_ARGS;

  if(isProtected(symbol->flags))
    return(errorProcExt(ERR_SYMBOL_PROTECTED, local));

  pushEnvironment(symbol->contents);
  pushEnvironment((UINT)symbol);
  symbol->contents = (UINT)copyCell(arg);
  local = local->next;
  arg = arg->next;
  localCount++;
  }
goto GET_ARGS;

GOT_ARGS:

pushEnvironment(argsSymbol->contents);
pushEnvironment((UINT)argsSymbol);
argsSymbol->contents = (UINT)getCell(CELL_EXPRESSION);
if(arg != nilCell)
    ((CELL*)argsSymbol->contents)->contents = (UINT)copyList(arg);
++localCount;

arg = localLst->next;
result = nilCell;

while(arg != nilCell)
	{
	result = evaluateExpression(arg);
	arg = arg->next;
	}
result = copyCell(result);

while(localCount--)
	{
	symbol = (SYMBOL *)popEnvironment();
	if(isProtected(symbol->flags) && (symbol != argsSymbol))
		symbol->flags &= ~SYMBOL_PROTECTED;
	deleteList((CELL *)symbol->contents);
	symbol->contents = popEnvironment();
	}

currentContext = contextSave;	
return(result);
}


/* -------------- list/cell creation/deletion routines ---------------- */

CELL * stuffInteger(UINT contents)
{
CELL * cell;

cell = getCell(CELL_LONG);
cell->contents = (UINT) contents;
return(cell);
}

#ifndef NEWLISP64
CELL * stuffInteger64(INT64 contents)
{
CELL * cell;

cell = getCell(CELL_INT64);
*(INT64 *)&cell->aux = contents;
return(cell);
}
#endif


CELL * stuffIntegerList(int argc, ...)
{
CELL * cell;
CELL * list;
va_list ap;

va_start(ap, argc);

list = getCell(CELL_EXPRESSION);
list->contents = (UINT)stuffInteger(va_arg(ap, UINT));
cell = (CELL *)list->contents;

while(--argc)
	{
	cell->next = stuffInteger(va_arg(ap, UINT));
	cell = cell->next;
	}
va_end(ap);

return(list);
}


CELL * stuffString(char * string)
{
CELL * cell;

cell = getCell(CELL_STRING);
cell->aux = strlen(string) + 1;
cell->contents = (UINT)allocMemory((UINT)cell->aux);
memcpy((void *)cell->contents, string, (UINT)cell->aux);
return(cell);
}


CELL * stuffStringN(char * string, int len)
{
CELL * cell;

cell = getCell(CELL_STRING);
cell->aux = len + 1;
cell->contents = (UINT)allocMemory((UINT)cell->aux);
memcpy((void *)cell->contents, string, len);
*(char*)(cell->contents + len) = 0;
return(cell);
}

CELL * stuffFloat(double * floatPtr)
{
CELL * cell;

cell = getCell(CELL_FLOAT);
#ifndef NEWLISP64
*(double *)&cell->aux = *floatPtr;
#else
*(double *)&cell->contents = *floatPtr;
#endif
return(cell);
}


CELL * stuffSymbol(SYMBOL * sPtr)
{
CELL * cell;

cell = getCell(CELL_SYMBOL);
cell->contents = (UINT)sPtr;
return(cell);
}

ssize_t convertNegativeOffset(ssize_t offset, CELL * list)
{
int len=0;

while(list != nilCell)
	{
	++len;
	list = list->next;
	}
offset = len + offset;
if(offset < 0) offset = 0;
return(offset);
}

/* ------------------------ creating and freeing cells ------------------- */

CELL * getCell(int type)
{
CELL * cell;

if(firstFreeCell == NULL) allocBlock();
cell = firstFreeCell;
firstFreeCell = cell->next;
++cellCount;

cell->type = type;
cell->next = nilCell;
cell->aux = (UINT)nilCell;
cell->contents = (UINT)nilCell;

return(cell);
}


CELL * copyCell(CELL * cell)
{
CELL * newCell;
UINT len;

if(firstFreeCell == NULL) allocBlock();
newCell = firstFreeCell;
firstFreeCell = newCell->next;
++cellCount;

newCell->type = cell->type;
newCell->next = nilCell;
newCell->aux = cell->aux;
newCell->contents = cell->contents;

if(isEnvelope(cell->type))
	{
	if(cell->type == CELL_ARRAY)
		newCell->contents = (UINT)copyArray(cell);
	else
	    {
		newCell->contents = (UINT)copyList((CELL *)cell->contents);
		newCell->aux = (UINT)lastCellCopied; 
		}
	}
else if(cell->type == CELL_STRING)
	{
	newCell->contents = (UINT)allocMemory((UINT)cell->aux);
	memcpy((void *)newCell->contents,
		(void*)cell->contents, (UINT)cell->aux);
	}
else if(cell->type == CELL_DYN_SYMBOL)
	{
	len = strlen((char *)cell->contents);
	newCell->contents = (UINT)allocMemory(len + 1);
	memcpy((char *)newCell->contents, (char *)cell->contents, len + 1);
	}

return(newCell);
}


/* this routine must be called with the list head
   if copying with envelope call copyCell() instead */
CELL * copyList(CELL * cell)
{
CELL * firstCell;
CELL * newCell;

if(cell == nilCell || cell == trueCell) return(lastCellCopied = cell);
firstCell = newCell = copyCell(cell);

while((cell = cell->next) != nilCell)
	{
	newCell->next = copyCell(cell);
	newCell = newCell->next;
	}
	
lastCellCopied = newCell;
return(firstCell);
}


/* for deleting lists _and_ cells */
void deleteList(CELL * cell)
{
CELL * next;

while(cell != nilCell)
	{
	if(isEnvelope(cell->type))
		{
		if(cell->type == CELL_ARRAY)
			deleteArray(cell);
		else
			deleteList((CELL *)cell->contents);
		}

	else if(cell->type == CELL_STRING || cell->type == CELL_DYN_SYMBOL) 
		freeMemory( (void *)cell->contents);

	next = cell->next;
	
	/* free cell */
	if(cell == trueCell) 
		{
		cell = next;
		continue;
		}

	cell->type = CELL_FREE;
	cell->next = firstFreeCell;
	firstFreeCell = cell;
	--cellCount;
	
	cell = next;
	}
}

/* --------------- cell / memory allocation and deallocation ------------- */

CELL * cellMemory = NULL;
CELL * cellBlock = NULL;

void allocBlock()
{
int i;

if(cellCount > MAX_CELL_COUNT) fatalError(ERR_NOT_ENOUGH_MEMORY, NULL, 0);

if(cellMemory == NULL)
	{
	cellMemory = (CELL *)allocMemory((MAX_BLOCK + 1) * sizeof(CELL));
	cellBlock = cellMemory;
	}
else
	{
	(cellBlock + MAX_BLOCK)->next = 
		(CELL *)allocMemory((MAX_BLOCK + 1) * sizeof(CELL));
	cellBlock = (cellBlock + MAX_BLOCK)->next;
	}

for(i = 0; i < MAX_BLOCK; i++)
	{
	(cellBlock + i)->type = CELL_FREE;
	(cellBlock + i)->next = (cellBlock + i + 1);
	}
(cellBlock + MAX_BLOCK - 1)->next = NULL;
(cellBlock + MAX_BLOCK)->next = NULL;
firstFreeCell = cellBlock;
}


void * allocMemory(size_t nbytes)
{
void * ptr;

if( (ptr = (void *)malloc(nbytes)) == NULL)
	fatalError(ERR_NOT_ENOUGH_MEMORY, NULL, 0);

return(ptr);
}

void * callocMemory(size_t nbytes)
{
void * ptr;

if( (ptr = (void *)calloc(nbytes, 1)) == NULL)
	fatalError(ERR_NOT_ENOUGH_MEMORY, NULL, 0);

return(ptr);
}

void * reallocMemory(void * prevPtr, UINT size)
{
void * ptr;

if( (ptr = realloc(prevPtr, size)) == NULL)
	fatalError(ERR_NOT_ENOUGH_MEMORY, NULL, 0);

return(ptr);
}

/* ----------- garbage collection , only required on error --------------- */

void markReferences(SYMBOL * sPtr);
void markList(CELL * cell);
void sweepGarbage(void);
void relinkCells(void);


void collectGarbage()
{
resultStackIdx = 0;
nilCell->type |= (UINT)0x00008000; 
markReferences((SYMBOL *)((CELL *)mainContext->contents)->aux);
sweepGarbage();
relinkCells();
}


void markReferences(SYMBOL * sPtr)
{
CELL * content;

if(sPtr != NIL_SYM && sPtr != NULL)
	{
	markReferences(sPtr->left);
	markList((CELL *)sPtr->contents);
	if((symbolType(sPtr) & 0xFF) == CELL_CONTEXT && sPtr != mainContext)
		{
		content = (CELL *)sPtr->contents;
		if((SYMBOL*)content->contents != mainContext && (SYMBOL*)content->contents == sPtr)
			markReferences((SYMBOL *)content->aux);
		}
	markReferences(sPtr->right);
	}
}


void markList(CELL * cell)
{
while(cell != nilCell)
	{
	cell->type |= (UINT)0x00008000;
	if(isEnvelope(cell->type & RAW_TYPE_MASK)) 
		{
		if((RAW_TYPE_MASK & cell->type) == CELL_ARRAY)
			markArray(cell);
		else
			markList((CELL *)cell->contents);
		}
	cell = cell->next;
	}
}

			     
void sweepGarbage()
{
CELL * blockPtr;
CELL * lastBlockPtr;
CELL * memPtr;
int i, freed;

lastBlockPtr = blockPtr = cellMemory;
while(blockPtr != NULL)
	{
	for(i = freed = 0; i < MAX_BLOCK; i++)
		{
		if(*(UINT *)blockPtr != CELL_FREE)
			{
			if( *(UINT *)blockPtr & (UINT)0x00008000)
				*(UINT *)blockPtr &= (UINT)0x00007FFF;
			else 
				{
				blockPtr->type = CELL_FREE;
				--cellCount;
				freed++;
				}
			}
		else freed++;
		blockPtr++;
		}
	if(freed == MAX_BLOCK)
		{
		memPtr = blockPtr->next;
		freeMemory(lastBlockPtr->next);
		lastBlockPtr->next = memPtr;
		blockPtr = memPtr;
		}
	else 
		{
		lastBlockPtr = blockPtr;
		blockPtr = blockPtr->next;
		}
	}
}


void relinkCells(void)
{
CELL * blockPtr;
CELL * lastFreeCell = NULL;
int i;

cellBlock = blockPtr = cellMemory;
firstFreeCell = NULL;
while(blockPtr != NULL)
	{
	cellBlock = blockPtr;
	for(i = 0; i <  MAX_BLOCK; i++)
		{
		if(*(UINT *)blockPtr == CELL_FREE)
			{
			if(firstFreeCell == NULL)
				firstFreeCell = lastFreeCell = blockPtr;
			else
				{
				lastFreeCell->next = blockPtr;
				lastFreeCell = blockPtr;
				}
			}
		++blockPtr;
		}
	blockPtr = blockPtr->next;
	}
lastFreeCell->next = NULL;
}


void cleanupResults(int from)
{
while(resultStackIdx > from)
	deleteList(popResult());
}

/* -------------------------- I/O routines ------------------------------ */

UINT printDevice;
STREAM errorStream;
void prettyPrint(UINT device);


void varPrintf(UINT device, char * format, ...)
{
char * buffer;
va_list argptr;
 
va_start(argptr,format);

/* new in 7201 , defined in nl-filesys.c if not in libc */
vasprintf(&buffer, format, argptr); 

prettyPrintLength += strlen(buffer);
switch(device)
	{
	case OUT_NULL:
		return;
	case OUT_DEVICE:
		if(printDevice != 0)
			{
			write(printDevice, buffer, strlen(buffer));
			break;
			}
	case OUT_CONSOLE:
#ifdef LIBRARY
            	writeStreamStr(&libStrStream, buffer, 0);
		return;
#else
		if(IOchannel == stdin)
		        {
			printf("%s", buffer);
			if(!isTTY) fflush(NULL);
			}
		else
			{
			if(IOchannel != NULL) 
#ifndef WIN32
				fprintf(IOchannel, "%s", buffer);
#else
				fprintf(IOchannel, buffer);
#endif
			}
		break;
#endif
        case OUT_LOG:
            writeLog(buffer, 0);
            break;
	default:
            writeStreamStr((STREAM *)device, buffer, 0);
            break;
	}

freeMemory(buffer);

va_end(argptr);
}


int printCell(CELL * cell, UINT printFlag, UINT device)
{
SYMBOL * sPtr;
SYMBOL * sp;

switch(cell->type)
	{
	case CELL_NIL:
		varPrintf(device, "nil"); break;

	case CELL_TRUE:
		varPrintf(device, "true"); break;
	
	case CELL_LONG:
		varPrintf(device,"%ld", cell->contents); break;

#ifndef NEWLISP64
    case CELL_INT64:
#ifdef TRU64
        varPrintf(device,"%ld", *(INT64 *)&cell->aux); break;
#else
#ifdef WINCC
        varPrintf(device,"%I64d", *(INT64 *)&cell->aux); break;
#else
        varPrintf(device,"%lld", *(INT64 *)&cell->aux); break;
#endif
#endif
#endif
    case CELL_FLOAT:
#ifndef NEWLISP64
        varPrintf(device,"%1.10g",*(double *)&cell->aux);
#else
        varPrintf(device,"%1.10g",*(double *)&cell->contents);
#endif
        break;

	case CELL_STRING:
		if(printFlag)
			printString((char *)cell->contents, device, cell->aux - 1);
		else
			varPrintf(device,"%s",cell->contents);
		break;
	
	case CELL_SYMBOL:
	case CELL_CONTEXT:
		sPtr = (SYMBOL *)cell->contents;
		if(sPtr->context != currentContext  
			/* if not global or global overwritten in current context */
			&& (!(sPtr->flags & SYMBOL_GLOBAL) || (lookupSymbol(sPtr->name, currentContext)))
			&& (symbolType(sPtr) != CELL_CONTEXT || 
				(SYMBOL *)((CELL*)sPtr->contents)->contents != sPtr)) /* context var */
			{
			varPrintf(device,"%s:%s", (char*)((SYMBOL*)sPtr->context)->name, sPtr->name);
			break;
			}
		/* overwriting global in MAIN */
		if(sPtr->context == currentContext
			&& currentContext != mainContext
			&& ((sp = lookupSymbol(sPtr->name, mainContext)) != NULL)
			&& (sp->flags & SYMBOL_GLOBAL) )
			{
			varPrintf(device,"%s:%s", currentContext->name, sPtr->name);
			break;
			}

		varPrintf(device,"%s",sPtr->name);

		break;
	
	case CELL_PRIMITIVE:
	case CELL_IMPORT_CDECL:
#ifdef WINCC
	case CELL_IMPORT_DLL:
#endif
		varPrintf(device,"%s <%lX>", (char *)cell->aux,
			cell->contents);
		break;
	
	case CELL_QUOTE:
		varPrintf(device, "'");
		prettyPrintFlags |= PRETTYPRINT_DOUBLE;
		printCell((CELL *)cell->contents, printFlag, device);
		break;
	
	case CELL_EXPRESSION:
	case CELL_LAMBDA:
	case CELL_MACRO:
		printExpression(cell, device);
		break;

	case CELL_DYN_SYMBOL:
		varPrintf(device, "%s:%s", ((SYMBOL*)cell->aux)->name, (char*)cell->contents);
		break;                                                                                                                                                                             
	case CELL_ARRAY:
		printArray(cell, device);
		break;

	default:
		varPrintf(device,"?");
	}

prettyPrintFlags &= ~PRETTYPRINT_DOUBLE;
return(1);
}


void printString(char * str, UINT  device, int size)
{
char chr;

if(size >= MAX_STRING)
    {
    varPrintf(device, "[text]");
    while(size--) varPrintf(device, "%c", *str++);
    varPrintf(device, "[/text]");
    return;
    }

varPrintf(device,"\"");
while(size--)
	{
	switch(chr = *str++)
		{
		case '\n': varPrintf(device,"\\n"); break;
		case '\r': varPrintf(device,"\\r"); break;
		case '\t': varPrintf(device,"\\t"); break;
		case '\\': varPrintf(device,"\\\\"); break;
		case '"': varPrintf(device,"\\%c",'"'); break;
		default: 
			if((unsigned char)chr < 32 || (stringOutputRaw && (unsigned char)chr > 126))
                            varPrintf(device,"\\%03u", (unsigned char)chr);
                        else
			    varPrintf(device,"%c",chr); break;
		}
	}
varPrintf(device,"\"");
}


int printExpression(CELL * cell, UINT device)
{
CELL * item;
int i, pFlags;

item = (CELL *)cell->contents;


if(prettyPrintPars <= prettyPrintCurrent || 
	prettyPrintLength > prettyPrintMaxLength)
	prettyPrint(device);

if(cell->type == CELL_LAMBDA) 
	{
	varPrintf(device, "(lambda ");
	++prettyPrintPars;
	}
else if(cell->type == CELL_MACRO) 
	{
	varPrintf(device, "(lambda-macro ");
	++prettyPrintPars;
	}
else 
	{
	if(isSymbol(item->type))
		{
		if(item->type == CELL_SYMBOL)
			 pFlags = ((SYMBOL *)item->contents)->flags;
		else
			 pFlags = 0;

		if((pFlags & PRINT_TYPE_MASK) != 0)
			{
			prettyPrint(device);
			varPrintf(device, "(");
			++prettyPrintPars;
			for(i = 0; i < (pFlags & PRINT_TYPE_MASK); i++)
				{
				if(item == nilCell) 
					{prettyPrintFlags |= PRETTYPRINT_DOUBLE; break;}
				printCell(item, TRUE, device);
				item = item->next;
				if(item != nilCell) varPrintf(device," ");
				else prettyPrintFlags |= PRETTYPRINT_DOUBLE;
				}
			prettyPrint(device);
			}
		else 
			{
			varPrintf(device, "(");
			++prettyPrintPars;
			}
		}
	else 
		{
		varPrintf(device, "(");
		++prettyPrintPars;
		}
	}


while(item != nilCell)
	{
	if(prettyPrintLength > prettyPrintMaxLength) prettyPrint(device);
	if(printCell(item, TRUE, device) == 0) return(0);
	item = item->next;
	if(item != nilCell) varPrintf(device," ");
	}

varPrintf(device,")");
--prettyPrintPars;

return(TRUE);
}


void prettyPrint(UINT device)
{
int i;

if(prettyPrintFlags) return;

if(prettyPrintPars > 0) 
	varPrintf(device, LINE_FEED);
/* varPrintf(device, LINE_FEED);  before 7106 */

for(i = 0; i < prettyPrintPars; i++) 
	varPrintf(device, prettyPrintTab);
prettyPrintLength = prettyPrintCurrent = prettyPrintPars;
prettyPrintFlags |= PRETTYPRINT_DOUBLE;
}


void printSymbol(SYMBOL * sPtr, UINT device)
{
CELL * cell;
CELL * list = NULL;
char * setStr;

prettyPrintCurrent = prettyPrintPars = 1;
prettyPrintLength = 0;
prettyPrintFlags &= !PRETTYPRINT_DOUBLE;

if(sPtr->flags & SYMBOL_PROTECTED)
	setStr = "(constant ";
else
	setStr = "(set ";

switch(symbolType(sPtr))
	{
	case CELL_PRIMITIVE:
	case CELL_IMPORT_CDECL:
#ifdef WINCC 
	case CELL_IMPORT_DLL:
#endif
		break;
	case CELL_SYMBOL:
	case CELL_DYN_SYMBOL:
		varPrintf(device, setStr);
		printSymbolNameExt(device, sPtr);
		varPrintf(device,"'");
		printCell((CELL *)sPtr->contents, TRUE, device);
		varPrintf(device, ")");
		break;
	case CELL_ARRAY:
	case CELL_EXPRESSION:
		varPrintf(device, setStr);
		printSymbolNameExt(device, sPtr);
		cell = (CELL *)sPtr->contents;

		if(symbolType(sPtr) == CELL_ARRAY)
			{
			varPrintf(device, "(array ");
			printArrayDimensions(cell, device);
			varPrintf(device, "(flat ");
			list = cell = arrayList(cell);
			}

		cell = (CELL *)cell->contents;

		varPrintf(device,"'(");
		prettyPrintPars = 2;
		if(cell->type == CELL_EXPRESSION) prettyPrint(device);
		while(cell != nilCell)
			{
			if(prettyPrintLength > prettyPrintMaxLength) 
					prettyPrint(device);
			printCell(cell, TRUE, device);
			cell = cell->next;
			if(cell != nilCell) varPrintf(device, " ");
			}
		varPrintf(device, "))");
		if(symbolType(sPtr) == CELL_ARRAY)
			{
			deleteList(list);
			varPrintf(device ,"))");
			}
		break;
	case CELL_LAMBDA:
	case CELL_MACRO:
		if(isProtected(sPtr->flags))
			{
			varPrintf(device, "%s%s%s", LINE_FEED, LINE_FEED, setStr);
			printSymbolNameExt(device, sPtr);
			printExpression((CELL *)sPtr->contents, device);
			varPrintf(device, ")");
			}
		else if (isGlobal(sPtr->flags))
			{
			printLambda(sPtr, device);
			varPrintf(device, "%s%s", LINE_FEED, LINE_FEED);
			printSymbolNameExt(device, sPtr);
			}
		else printLambda(sPtr, device);
		break;
	default:
		varPrintf(device, setStr);
		printSymbolNameExt(device, sPtr);
		printCell((CELL *)sPtr->contents, TRUE, device);
		varPrintf(device, ")");
		break;
	}

varPrintf(device, "%s%s", LINE_FEED, LINE_FEED);

prettyPrintLength = prettyPrintPars = 0;
}


void printLambda(SYMBOL * sPtr, UINT device)
{
CELL * lambda;
CELL * cell;

lambda = (CELL *)sPtr->contents;
cell = (CELL *)lambda->contents;
if(cell->type == CELL_EXPRESSION)
	cell = (CELL *)cell->contents;

if(!isLegalSymbol(sPtr->name))
        {
        varPrintf(device, "(set (sym ");
        printString(sPtr->name, device, strlen(sPtr->name));
        varPrintf(device, " %s) ", ((SYMBOL*)sPtr->context)->name);
        printExpression((CELL *)sPtr->contents, device);
        varPrintf(device, ")");
        return;
        }
	
if(symbolType(sPtr) == CELL_LAMBDA)
	varPrintf(device, "(define (");
else 
	varPrintf(device, "(define-macro (");
prettyPrintPars += 2;

printSymbolName(device, sPtr);
varPrintf(device, " ");

while(cell != nilCell)
	{
	printCell(cell, TRUE, device);
	cell = cell->next;
	if(cell != nilCell) varPrintf(device, " ");
	}
varPrintf(device, ")");
--prettyPrintPars;
prettyPrint(device);

cell = (CELL *)lambda->contents;
while((cell = cell->next) != nilCell)
	{
	if(prettyPrintLength > prettyPrintMaxLength) prettyPrint(device);
	printCell(cell, TRUE, device);
	if(!(cell->type & ENVELOPE_TYPE_MASK) && cell->next != nilCell) varPrintf(device, " ");
	}

varPrintf(device, ")");
--prettyPrintPars;
}


void printSymbolName(UINT device, SYMBOL * sPtr)
{
SYMBOL * sp;

if(sPtr->context == currentContext)
	{
	if(*sPtr->name == *currentContext->name && strcmp(sPtr->name, currentContext->name) == 0)
		varPrintf(device, "%s:%s", sPtr->name, sPtr->name);

	else if(currentContext != mainContext 
		&& ((sp = lookupSymbol(sPtr->name, mainContext)) != NULL)
		&& (sp->flags &  SYMBOL_GLOBAL) )
		varPrintf(device, "%s:%s", currentContext->name, sPtr->name);
	else
		varPrintf(device,"%s", sPtr->name);
	}
else
	varPrintf(device,"%s:%s", 
		(char *)((SYMBOL*)sPtr->context)->name, sPtr->name);
}


void printSymbolNameExt(UINT device, SYMBOL * sPtr)
{
if(isGlobal(sPtr->flags))
	{
	varPrintf(device, "(global '");
	printSymbolName(device, sPtr);
	if(symbolType(sPtr) == CELL_LAMBDA || symbolType(sPtr) == CELL_MACRO)
		varPrintf(device, ")");
	else varPrintf(device, ") ");
	}
else 
	{
	if(!isLegalSymbol(sPtr->name))
            {
            varPrintf(device, " (sym ");
            printString(sPtr->name, device, strlen(sPtr->name));
            varPrintf(device, " %s) ", ((SYMBOL*)sPtr->context)->name);
            }
        else
            {
	    varPrintf(device, "'");
            printSymbolName(device, sPtr);
            }
	varPrintf(device, " ");
	}
}


CELL * p_prettyPrint(CELL * params)
{
CELL * result;
char * str;
size_t len;

if(params != nilCell)
	params = getInteger(params, &prettyPrintMaxLength);
if(params != nilCell)
	{
	getStringSize(params, &str, &len, TRUE);
	prettyPrintTab = allocMemory(len + 1);
	memcpy(prettyPrintTab, str, len + 1);
	}

result = getCell(CELL_EXPRESSION);
result->contents = (UINT)stuffInteger(prettyPrintMaxLength);
((CELL *)result->contents)->next = stuffString(prettyPrintTab);

return(result);
}



/* -------------------------- error handling --------------------------- */

char * errorMessage[] =
	{
	"",                             /* 0 */
	"not enough memory",            /* 1 */
	"environment stack overflow",   /* 2 */
	"call stack overflow",          /* 3 */
	"problem accessing file",       /* 4 */
	"not an expression",            /* 5 */
	"missing parenthesis",          /* 6 */
	"string token too long",        /* 7 */
	"missing argument",             /* 8 */
	"number or string expected",    /* 9 */
	"value expected",               /* 10 */
	"string expected",              /* 11 */
	"symbol expected",              /* 12 */
	"context expected",             /* 13 */
	"symbol or context expected",   /* 14 */
	"list expected",                /* 15 */
	"list or symbol expected",      /* 16 */
	"list or string expected",      /* 17 */
	"list or number expected",      /* 18 */
	"array expected",               /* 19 */
	"array, list or string expected", /* 20 */
	"lambda expected",              /* 21 */
	"lambda-macro expected",        /* 22 */
	"invalid function",             /* 23 */
	"invalid lambda expression",    /* 24 */
	"invalid macro expression",     /* 25 */
	"invalid let parameter list",   /* 26 */
	"problem saving file",          /* 27 */
	"division by zero",             /* 28 */
	"matrix expected",              /* 29 */ 
	"wrong dimensions",             /* 30 */
	"matrix is singular",           /* 31 */
	"syntax in regular expression", /* 32 */
	"throw without catch",			/* 33 */
	"problem loading library",      /* 34 */
	"import function not found",    /* 35 */
	"symbol is protected",          /* 36 */
	"number out of range",          /* 37 */
	"regular expression",           /* 38 */
	"missing end of text [/text]",  /* 39 */
	"mismatch in number of arguments",  /* 40 */
	"problem in format string",     /* 41 */
	"data type and format don't match", /* 42 */
	"invalid parameter: 0.0",	 	/* 43 */
	"invalid parameter: NaN",	 	/* 44 */
	"illegal parameter type",	 	/* 45 */
	"symbol not in MAIN context",	/* 46 */
	"symbol not in current context", /* 47 */
	"target cannot be MAIN",		/* 48 */
	"array index out of bounds",	/* 49 */
	"nesting level to deep",		/* 50 */
	"user error",	                /* 51 */
	"user reset -",		 	 		/* 52 */
	"received SIGINT -"		 		/* 53 */
	};


void errorMissingPar(STREAM * stream)
{
char str[64]; 
snprintf(str, 40, "...%-40s", ((char *)((stream->ptr - stream->buffer) > 40 ? stream->ptr - 40 : stream->buffer)));
errorProcExt2(ERR_MISSING_PAR, stuffString(str));
}

CELL * errorProcAll(int errorNumber, CELL * expr, int deleteFlag)
{
if(!traceFlag) fatalError(errorNumber, expr, deleteFlag);
printErrorMessage(errorNumber, expr, deleteFlag);
openTrace();
return(nilCell);
}

CELL * errorProc(int errorNumber)
{
return(errorProcAll(errorNumber, NULL, 0));
}

/* extended error info in expr */
CELL * errorProcExt(int errorNumber, CELL * expr)
{
return(errorProcAll(errorNumber, expr, 0));
}

/* extended error info in expr, which has to be discarded after printing */
CELL * errorProcExt2(int errorNumber, CELL * expr)
{
return(errorProcAll(errorNumber, expr, 1));
}

CELL * errorProcArgs(int errorNumber, CELL * expr)
{
if(expr == nilCell) 
	return(errorProcExt(ERR_MISSING_ARGUMENT, NULL));

return(errorProcExt(errorNumber, expr));
}

void fatalError(int errorNumber, CELL * expr, int deleteFlag)
{
printErrorMessage(errorNumber, expr, deleteFlag);
closeTrace();
longjmp(errorJump, errorReg);
}


void printErrorMessage(UINT errorNumber, CELL * expr, int deleteFlag)
{
CELL * lambdaFunc;
UINT lambdaStackIdxSave;
SYMBOL * context;
int i;

if(errorNumber == EXCEPTION_THROW)
	errorNumber = ERR_THROW_WO_CATCH;

errorReg = errorNumber;

if(!errorNumber) return;

openStrStream(&errorStream, MAX_STRING, 1);
if(traceFlag & ~TRACE_SIGINT) writeStreamStr(&errorStream, "ERR:", 4);
writeStreamStr(&errorStream, errorMessage[errorReg], 0);

for(i = 0; primitive[i].name != NULL; i++)
	{
	if(evalFunc == primitive[i].function)
		{
		writeStreamStr(&errorStream, " in function ", 0);
		writeStreamStr(&errorStream, primitive[i].name, 0);
		break;
		}
	}

if(expr != NULL)
	{
	writeStreamStr(&errorStream, " : ", 3);
	printCell(expr, (errorNumber != ERR_USER_ERROR), (UINT)&errorStream);
	if(deleteFlag) deleteList(expr);
	}

lambdaStackIdxSave = lambdaStackIdx;
while(lambdaStackIdx)
	{
	lambdaFunc = popLambda();
	if(lambdaFunc->type == CELL_SYMBOL)
		{
		writeStreamStr(&errorStream, LINE_FEED, 0);
		writeStreamStr(&errorStream, "called from user defined function ", 0);
		context = ((SYMBOL *)lambdaFunc->contents)->context;
		if(context != mainContext)
		  {
		  writeStreamStr(&errorStream, context->name, 0);
		  writeStreamStr(&errorStream, ":", 0);
		  }
		writeStreamStr(&errorStream, ((SYMBOL *)lambdaFunc->contents)->name, 0);
		}
	}
lambdaStackIdx = lambdaStackIdxSave;


if(!(traceFlag & TRACE_SIGINT)) evalFunc = NULL; 
parStackCounter = prettyPrintPars = 0;

if(evalCatchFlag && !(traceFlag & TRACE_SIGINT)) return;

if(errorEvent == nilSymbol)
	{
	if(errorNumber == ERR_SIGINT)
		printf(errorStream.buffer);
	else
		varPrintf(OUT_CONSOLE, "\n%.1024s\n", errorStream.buffer);
	}
}


/* --------------------------- load source file ------------------------- */


CELL * loadFile(char * fileName, UINT offset, int encryptFlag, SYMBOL * context)
{
CELL * result;
STREAM stream;
int errNo, dataLen;
jmp_buf errorJumpSave;
SYMBOL * contextSave;
char key[16];
#ifdef LOAD_DEBUG
int i;
#endif

contextSave = currentContext;
currentContext = context;
if(encryptFlag)
	{
	dataLen = *((int *) (linkOffset + 4));
	snprintf( key, 15, "%d", dataLen);
	}
else dataLen = MAX_FILE_BUFFER;

if(makeStreamFromFile(&stream, fileName, dataLen + 4 * MAX_STRING, offset) == 0) 
	return(NULL);

if(encryptFlag)
	encryptPad(stream.buffer, stream.buffer, key, dataLen, strlen(key));

memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));
if((errNo = setjmp(errorJump)) != 0)
	{
	closeStrStream(&stream);
	memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
	currentContext = contextSave;
	longjmp(errorJump, errNo);
	}
	
#ifdef LOAD_DEBUG
for(i = 0; i<recursionCount; i++) printf("  ");	
printf("load: %s\n", fileName);
#endif

result = evaluateStream(&stream, 0, TRUE);
currentContext = contextSave;

#ifdef LOAD_DEBUG
for(i = 0; i<recursionCount; i++) printf("  ");	
printf("finish load: %s\n", fileName);
#endif

memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
closeStrStream(&stream);
return(result);
}

/* -------------------------- parse / compile ----------------------------- */


int compileExpression(STREAM * stream, CELL * cell)
{
char token[MAX_STRING + 4];
double floatNumber;
CELL * newCell;
CELL * contextCell;
SYMBOL * contextPtr;
int listFlag, tklen;
char * lastPtr;
#if experimental
SYMBOL * saveContext;
int defaultSymbolLevel = 0;

saveContext = currentContext;
#endif

listFlag = TRUE; /* assumes we just entered from an envelope cell ! */

GETNEXT:
lastPtr = stream->ptr;
switch(getToken(stream, token, &tklen))
	{
	case TKN_ERROR:
		errorProcExt2(ERR_EXPRESSION, stuffStringN(lastPtr, 
			(strlen(lastPtr) < 60) ? strlen(lastPtr) : 60));
		return(0);

	case TKN_EMPTY:
		if(parStackCounter != 0) errorMissingPar(stream);
		return(0);

	case TKN_CHARACTER:
		newCell = stuffInteger((UINT)token[0]);
		break;

    case TKN_HEX:
#ifndef NEWLISP64
        newCell = stuffInteger64((INT64)strtoull(token,NULL,0));
#else
        newCell = stuffInteger(strtoull(token,NULL,0));
#endif
        break;

    case TKN_DECIMAL:
#ifndef NEWLISP64
        newCell = stuffInteger64(strtoll(token,NULL,0));
#else
        newCell = stuffInteger(strtoll(token,NULL,0));
#endif
        break;

	case TKN_FLOAT:
		floatNumber = (double)atof(token);
		newCell = stuffFloat(&floatNumber);
		break;

	case TKN_STRING:
		newCell = stuffStringN(token, tklen);
		break;

	case TKN_SYMBOL:
		if(strcmp(token, "lambda") == 0 || strcmp(token, "fn") == 0)
			{
			if(cell->type != CELL_EXPRESSION)
				{
				errorProcExt2(ERR_INVALID_LAMBDA, stuffString(lastPtr));
				return(0);
				}
			cell->type =  CELL_LAMBDA;
			cell->aux = (UINT)nilCell;
			goto GETNEXT;
			}
		else if(strcmp(token, "lambda-macro") == 0 || strcmp(token, "fn-macro") == 0)
			{
			if(cell->type != CELL_EXPRESSION)
				{
				errorProcExt2(ERR_INVALID_LAMBDA, stuffString(lastPtr));
				return(0);
				}
			cell->type =  CELL_MACRO;
			cell->aux = (UINT)nilCell;
			goto GETNEXT;
			}

		else if(*(unsigned int *)token == *(unsigned int *)"[tex" && *(INT16*)(token +4) == *(INT16*)"t]") 
/*		else if(strncmp(token, "[text]", 6) == 0) */
			{
			newCell = getCell(CELL_STRING);
			newCell->contents =  (UINT)readStreamText(stream, "[/text]");
            if(newCell->contents == 0)
				{
				deleteList(newCell); 
				errorProc(ERR_MISSING_TEXT_END);
				}
			newCell->aux = strlen((char *)newCell->contents) + 1;
			newCell->type = CELL_STRING;
			break;
			}
		newCell = getCell(CELL_SYMBOL);
		if(*token == '$')
			newCell->contents = (UINT)translateCreateSymbol(
				token, CELL_NIL, mainContext, TRUE);
		else
			newCell->contents = (UINT)translateCreateSymbol(
				token, CELL_NIL, currentContext, 0);
		break;

	case TKN_CONTEXT:
		contextPtr = NULL; /* since 7.5.1 dyna vars inside contexts */
		if(currentContext != mainContext)
			{
			if(strcmp(currentContext->name, token) == 0)
				contextPtr = currentContext;
			else
				contextPtr = lookupSymbol(token, currentContext);
			}

		if(contextPtr == NULL)
			contextPtr = translateCreateSymbol(
				token, CELL_CONTEXT, mainContext, TRUE);

		contextCell = (CELL *)contextPtr->contents;

		if(getToken(stream, token, &tklen) != TKN_SYMBOL)
			errorProcExt2(ERR_SYMBOL_EXPECTED, stuffString(lastPtr));
		
		/* context does not exist */
		if(contextCell->type != CELL_CONTEXT 
		   || contextPtr != (SYMBOL*)contextCell->contents)
			{
			newCell = getCell(CELL_DYN_SYMBOL);
			newCell->aux = (UINT)contextPtr;
			newCell->contents = (UINT)allocMemory(tklen + 1);
			strncpy((char *)newCell->contents, token, tklen + 1);
			break;
			}

		/* context exists make a symbol for it */
		newCell = getCell(CELL_SYMBOL);
		newCell->contents = (UINT)translateCreateSymbol(
				token, CELL_NIL, contextPtr, TRUE);
		break;

	case TKN_QUOTE:
		newCell = getCell(CELL_QUOTE);
		linkCell(cell, newCell, listFlag);
		compileExpression(stream, newCell);
		break;

	case TKN_LEFT_PAR:
		++parStackCounter;
		newCell = getCell(CELL_EXPRESSION);
		linkCell(cell, newCell, listFlag);
		compileExpression(stream, newCell);
		break;

	case TKN_RIGHT_PAR:
		if(parStackCounter == 0) errorMissingPar(stream);
		--parStackCounter;
		cell->next = nilCell;
		return(TRUE);

	default:
		errorProcExt2(ERR_EXPRESSION, stuffString(lastPtr));
		return(0);

	}

linkCell(cell, newCell, listFlag);

if(cell->type == CELL_QUOTE && listFlag == TRUE)
	return(TRUE);

listFlag = 0;
cell = newCell;

if(parStackCounter != 0)
	{
	if(*(stream->ptr) != 0) goto GETNEXT;
	else errorMissingPar(stream);
	}

return(0);
}


void linkCell(CELL * left, CELL * right, int linkFlag)
{
if(linkFlag == 0)
	left->next = right;
else left->contents = (UINT)right;
}

int getToken(STREAM * stream, char * token, int * ptr_len)
{
char *tkn;
char chr;
int tknLen;
int floatFlag;
int bracketBalance;
char buff[4];

tkn = token;
tknLen = floatFlag = 0;
*tkn = 0;

STRIP:
if(stream->ptr > (stream->buffer + stream->size - 4 * MAX_STRING))
	{
	if(stream->handle == 0)
		{
        /* coming from commmand line or p_evalString */
		stream->buffer = stream->ptr;
		}
	else
		{
		stream->position += (stream->ptr - stream->buffer);
        		lseek((int)stream->handle, stream->position, SEEK_SET);
		memset(stream->buffer, 0, stream->size + 1);

		if(read(stream->handle, stream->buffer, stream->size) > 0)

	       	stream->ptr = stream->buffer;
		else
			{
			*stream->ptr = 0;
			return(TKN_EMPTY);
			}
		}
	}

while((unsigned char)*stream->ptr <= ' ' && (unsigned char)*stream->ptr != 0)
	++stream->ptr;

if(*stream->ptr == 0) return(TKN_EMPTY);

/* check for comments */
if(*stream->ptr == ';' || *stream->ptr == '#')
	{
	stream->ptr++;
	for(;;)
		{
		if(*stream->ptr == 0) return(TKN_EMPTY);
		if(*stream->ptr == '\n' || *stream->ptr == '\r')
			break;
		stream->ptr++;
		}
	stream->ptr++;
	goto STRIP;
	}


if( *stream->ptr == '-' || *stream->ptr == '+')
	{
	if(isDigit((unsigned char)*(stream->ptr + 1)) )
		*(tkn++) = *(stream->ptr++), tknLen++;
	}

	
if(isDigit((unsigned char)*stream->ptr) || 
                (*stream->ptr == lc_decimal_point && 
                isDigit((unsigned char)*(stream->ptr + 1))))
	{
	if(*stream->ptr == '0' && isDigit((unsigned char)*(stream->ptr + 1)))
		{
		*(tkn++) = *(stream->ptr++), tknLen++;
		while(*stream->ptr < '8' && *stream->ptr >= '0' && *stream->ptr != 0)
			*(tkn++) = *(stream->ptr++), tknLen++;
		*tkn = 0;
		return(TKN_DECIMAL);
		}
		
	while(isDigit((unsigned char)*stream->ptr) && tknLen < MAX_SYMBOL)
		*(tkn++) = *(stream->ptr++), tknLen++;
	
	if(toupper(*stream->ptr) == 'X' && token[0] == '0')
		{
		*(tkn++) = *(stream->ptr++), tknLen++;
		while(isxdigit((unsigned char)*stream->ptr) && tknLen < MAX_SYMBOL)
			*(tkn++) = *(stream->ptr++), tknLen++;
		*tkn = 0;
		return(TKN_HEX);
		}

	if(*stream->ptr == lc_decimal_point)
		{
		*(tkn++) = *(stream->ptr++), tknLen++;
		while(isDigit((unsigned char)*stream->ptr) && tknLen < MAX_SYMBOL)
			*(tkn++) = *(stream->ptr++), tknLen++;
		floatFlag = TRUE;
		}
	else if(toupper(*stream->ptr) != 'E')
		{
		*tkn = 0;
		return(TKN_DECIMAL);
		}
	
	if(toupper(*stream->ptr) == 'E') 
		{
		if(isDigit((unsigned char)*(stream->ptr+2))
		&& ( *(stream->ptr+1) == '-' || *(stream->ptr+1) == '+') )
			*(tkn++) = *(stream->ptr++), tknLen++;
		if(isDigit((unsigned char)*(stream->ptr+1)))
			{
			*(tkn++) = *(stream->ptr++), tknLen++;
			while(isDigit((unsigned char)*stream->ptr) && tknLen < MAX_SYMBOL)
				*(tkn++) = *(stream->ptr++), tknLen++;
			}
		else 
			{
			*tkn = 0;
			if(floatFlag == TRUE) return(TKN_FLOAT);
			else return(TKN_DECIMAL);
			}
		}
	*tkn = 0;
	return(TKN_FLOAT);
	}
else
	{
	chr = *stream->ptr;
	*(tkn++) = *(stream->ptr++), tknLen++;
	switch(chr)
	 {
	 case '"':
		--tkn; --tknLen;
		while(*stream->ptr != '"' && *stream->ptr != 0 
					  && tknLen < MAX_STRING) 
 			{
			if(*stream->ptr == '\\')
				{
				stream->ptr++;
				if(isDigit((unsigned char)*stream->ptr) && 
				          isDigit((unsigned char)*(stream->ptr+1)) && 
				          isDigit((unsigned char)*(stream->ptr+2)))
					{
					memcpy(buff, stream->ptr, 3);
					buff[3] = 0;
					*(tkn++) = atoi(buff);
                    tknLen++;
					stream->ptr += 3;
					continue;
					}

				switch(*stream->ptr)
					{
					case 0:
					    *tkn = 0;
					    errorProcExt2(ERR_STRING_TOO_LONG, stuffString(token));
					    break;
					case 'n':
					    *(tkn++) = '\n'; break;
					case '\\':
					    *(tkn++) = '\\'; break;
					case 'r':
					    *(tkn++) = '\r'; break;
					case 't':
					    *(tkn++) = '\t'; break;
					case '"':
					    *(tkn++) = '"';  break;
					case 'x':
						if(isxdigit((unsigned char)*(stream->ptr + 1)) &&
						   isxdigit((unsigned char)*(stream->ptr + 2)))
							{
							buff[0] = '0';
							buff[1] = (unsigned char)*(stream->ptr + 1);
							buff[2] = (unsigned char)*(stream->ptr + 2);
							buff[3] = 0;
							*(tkn++) = strtol(buff, NULL, 16);
							stream->ptr += 2;
							break;
							}
					default:
					    *(tkn++) = *stream->ptr;
					}
				stream->ptr++;
                tknLen++;
				}
			else *(tkn++) = *(stream->ptr++), tknLen++;
			}
		if(*stream->ptr == '\"')
			{
			*tkn = 0;
			stream->ptr++;
                        *ptr_len = tknLen;
			return(TKN_STRING);
			}
		else
			{
			*tkn = 0;
			errorProcExt2(ERR_STRING_TOO_LONG, 
				stuffStringN(token, strlen(token) < 40 ? strlen(token) : 40));
			}
		break;

	 case '\'':
	 case '(':
	 case ')':
		*tkn = 0;
		return(chr);
	 case '{':
		--tkn; --tknLen;
		bracketBalance = 1;
		while(*stream->ptr != 0  && tknLen < MAX_STRING) 
 			{
			if(*stream->ptr == '{') ++bracketBalance;
			if(*stream->ptr == '}') --bracketBalance;
			if(bracketBalance == 0) break;

			*(tkn++) = *(stream->ptr++), tknLen++;
			}
		if(*stream->ptr == '}')
			{
			*tkn = 0;
			stream->ptr++;
                        *ptr_len = tknLen;
			return(TKN_STRING);
			}
		else
			{
			*tkn = 0;
			errorProcExt2(ERR_STRING_TOO_LONG, stuffStringN(token, 40));
			}
		break;

		
	 case ',':
	 case ':':
		*tkn = 0;
		return(TKN_SYMBOL);

         case '[':
                while( tknLen < MAX_SYMBOL && *stream->ptr != 0 && *stream->ptr != ']')
                   *(tkn++) = *(stream->ptr++), tknLen++;
                *tkn++ = ']';
                *tkn = 0;
                stream->ptr++;

                return(TKN_SYMBOL);

	 default:
		while(  tknLen < MAX_SYMBOL
			&& (unsigned char)*stream->ptr > ' ' 
			&& *stream->ptr != '"' && *stream->ptr != '\''
			&& *stream->ptr != '(' && *stream->ptr != ')'
			&& *stream->ptr != ':' && *stream->ptr != ','
                        && *stream->ptr != 0)
				*(tkn++) = *(stream->ptr++), tknLen++;
		*tkn = 0;
                *ptr_len = tknLen;
		if(*stream->ptr == ':') 
			{
			stream->ptr++;
			return(TKN_CONTEXT);
			}
		return(TKN_SYMBOL);
	 }
	}
*tkn=0;
return(TKN_ERROR);
}

/* -------------------------- utilities ------------------------------------ */

size_t listlen(CELL * listHead)
{
size_t len = 0;

while(listHead != nilCell)
  {
  len++;
  listHead = listHead->next;
  }
  
return(len);
}

/* -------------------------- functions to get parameters ------------------ */

void collectSymbols(SYMBOL * sPtr);

int getFlag(CELL * params)
{
params = evaluateExpression(params);
return(!isNil(params));
}

CELL * getInteger(CELL * params, UINT * number)
{
CELL * cell;
	
cell = evaluateExpression(params);

#ifndef NEWLISP64
if(cell->type == CELL_INT64)
	{
	if(*(INT64 *)&cell->aux >  0xFFFFFFFF) *number = 0xFFFFFFFF;
	else if(*(INT64 *)&cell->aux < INT32_MIN_AS_INT64) *number = 0x80000000;
	else *number = *(INT64 *)&cell->aux;
	}
else if(cell->type == CELL_LONG)
	*number = cell->contents;
else if(cell->type == CELL_FLOAT)
	{
#ifdef WINCC
	if(isnan(*(double *)&cell->aux) || !_finite(*(double *)&cell->aux)) *number = 0;
#else
	if(isnan(*(double *)&cell->aux)) *number = 0; 
#endif
	else if(*(double *)&cell->aux >  4294967295.0) *number = 0xFFFFFFFF;
	else if(*(double *)&cell->aux < -2147483648.0) *number = 0x80000000;
	else *number = *(double *)&cell->aux;
	}
#else
if(cell->type == CELL_LONG)
    *number = cell->contents;
else if(cell->type == CELL_FLOAT)
    {
    if(isnan(*(double *)&cell->contents)) *number = 0;
    else if(*(double *)&cell->contents >  9223372036854775807.0) *number = 0x7FFFFFFFFFFFFFFFLL;
    else if(*(double *)&cell->contents < -9223372036854775808.0) *number = 0x8000000000000000LL;
    else *number = *(double *)&cell->contents;
    }
#endif
else
	{
	*number = 0;
	return(errorProcArgs(ERR_NUMBER_EXPECTED, params));
	}

return(params->next);
}

#ifndef NEWLISP64
CELL * getInteger64(CELL * params, INT64 * number)
{
CELL * cell;
	
cell = evaluateExpression(params);

if(cell->type == CELL_INT64)
	*number = *(INT64 *)&cell->aux;
else if(cell->type == CELL_LONG)
	*number = (int)cell->contents;
else if(cell->type == CELL_FLOAT)
	{
#ifdef WINCC
	if(isnan(*(double *)&cell->aux) || !_finite(*(double *)&cell->aux)) *number = 0;
#else
	if(isnan(*(double *)&cell->aux)) *number = 0; 
#endif
	else if(*(double *)&cell->aux >  9223372036854775807.0) *number = 0x7FFFFFFFFFFFFFFFLL;
	else if(*(double *)&cell->aux < -9223372036854775808.0) *number = 0x8000000000000000LL;
	else *number = *(double *)&cell->aux;
	}
else
	{
	*number = 0;
	return(errorProcArgs(ERR_NUMBER_EXPECTED, params));
	}

return(params->next);
}

#else
CELL * getInteger64(CELL * params, INT64 * number)
{
CELL * cell;

cell = evaluateExpression(params);

if(cell->type == CELL_LONG)
    *number = cell->contents;
else if(cell->type == CELL_FLOAT)
    {
    if(isnan(*(double *)&cell->contents)) *number = 0;
    else if(*(double *)&cell->contents >  9223372036854775807.0) *number = 0x7FFFFFFFFFFFFFFFLL;
    else if(*(double *)&cell->contents < -9223372036854775808.0) *number = 0x8000000000000000LL;
    else *number = *(double *)&cell->contents;
    }
else
    {
    *number = 0;
    return(errorProcArgs(ERR_NUMBER_EXPECTED, params));
    }

return(params->next);
}
#endif

CELL * getIntegerExt(CELL * params, UINT * number, int evalFlag)
{
CELL * cell;

if(evalFlag)
	cell = evaluateExpression(params);
else cell = params;

#ifndef NEWLISP64
if(cell->type == CELL_INT64)
	{
	if(*(INT64 *)&cell->aux >  0xFFFFFFFF) *number = 0xFFFFFFFF;
	else if(*(INT64 *)&cell->aux < INT32_MIN_AS_INT64) *number = 0x80000000;
	else *number = *(INT64 *)&cell->aux;
	}
else if(cell->type == CELL_LONG)
	*number = cell->contents;
else if(cell->type == CELL_FLOAT)
	{
#ifdef WINCC
	if(isnan(*(double *)&cell->aux) || !_finite(*(double *)&cell->aux)) *number = 0;
#else
	if(isnan(*(double *)&cell->aux)) *number = 0; 
#endif
	else if(*(double *)&cell->aux >  4294967295.0) *number = 0xFFFFFFFF;
	else if(*(double *)&cell->aux < -2147483648.0) *number = 0x80000000;
	else *number = *(double *)&cell->aux;
	}
#else
if(cell->type == CELL_LONG)
    *number = cell->contents;
else if(cell->type == CELL_FLOAT)
    {
    if(isnan(*(double *)&cell->contents)) *number = 0;
    else if(*(double *)&cell->contents >  9223372036854775807.0) *number = 0x7FFFFFFFFFFFFFFFLL;
    else if(*(double *)&cell->contents < -9223372036854775808.0) *number = 0x8000000000000000LL;
    else *number = *(double *)&cell->contents;
    }
#endif
else
	{
	*number = 0;
	return(errorProcArgs(ERR_NUMBER_EXPECTED, params));
	}

return(params->next);
}


CELL * getFloat(CELL * params, double * floatNumber)
{
CELL * cell;

cell = evaluateExpression(params);

#ifndef NEWLISP64
if(cell->type == CELL_FLOAT)
	*floatNumber = *(double *)&cell->aux;
else if(cell->type == CELL_INT64)
	*floatNumber = *(INT64 *)&cell->aux;
#else
if(cell->type == CELL_FLOAT)
    *floatNumber = *(double *)&cell->contents;
#endif
else if(cell->type == CELL_LONG)
	*floatNumber = (long)cell->contents;
else
	{
	*floatNumber = 0.0;
	return(errorProcArgs(ERR_NUMBER_EXPECTED, params));
	}

return(params->next);
}


CELL * getString(CELL * params, char * * stringPtr)
{
CELL * cell;

cell = evaluateExpression(params);

if(cell->type != CELL_STRING)
	{
	*stringPtr = "";
	return(errorProcArgs(ERR_STRING_EXPECTED, params));
	}
*stringPtr = (char *)cell->contents;
return(params->next);
}


CELL * getStringSize(CELL * params, char * * stringPtr, size_t * size, int evalFlag)
{
CELL * cell;

if(params == nilCell)
	return(errorProc(ERR_MISSING_ARGUMENT));

if(evalFlag)
	cell = evaluateExpression(params);
else cell = params;

if(cell->type != CELL_STRING)
	{
	*stringPtr = "";
	return(errorProcArgs(ERR_STRING_EXPECTED, params));
	}

*stringPtr = (char *)cell->contents;
if(size) *size = cell->aux - 1;
return(params->next);
}


CELL * getSymbol(CELL * params, SYMBOL * * symbol)
{
CELL * cell;

cell = evaluateExpression(params);

if(cell->type != CELL_SYMBOL)
	{
	if(cell->type == CELL_DYN_SYMBOL)
		{
		*symbol = getDynamicSymbol(cell);
		return(params->next);
		}
	*symbol = nilSymbol;
	return(errorProcArgs(ERR_SYMBOL_EXPECTED, params));
	}

*symbol = (SYMBOL *)cell->contents;
return(params->next);
}

/* only used for internal syms: $timer, $error-event and $signal-1-> $signal-32 */
CELL * getCreateSymbol(CELL * params, SYMBOL * * symbol, char * name)
{
CELL * cell;

cell = evaluateExpression(params);

if(cell->type != CELL_SYMBOL)
    {
    if(cell->type == CELL_DYN_SYMBOL)
        {
        *symbol = getDynamicSymbol(cell);
        return(params->next);
        }
	*symbol = translateCreateSymbol(name, CELL_NIL, mainContext, TRUE);
    (*symbol)->flags |= SYMBOL_PROTECTED | SYMBOL_GLOBAL;
	(*symbol)->contents = (UINT)copyCell(cell);
    }
else
	*symbol = (SYMBOL *)cell->contents;

return(params->next);
}


CELL * getContext(CELL * params, SYMBOL * * context)
{
CELL * cell;

cell = evaluateExpression(params);

if(cell->type == CELL_CONTEXT || cell->type == CELL_SYMBOL)
	*context = (SYMBOL *)cell->contents;
else	
        {
        *context = NULL;
	return(errorProcArgs(ERR_CONTEXT_EXPECTED, params));
	}

if(symbolType(*context) != CELL_CONTEXT)
	return(errorProcExt(ERR_CONTEXT_EXPECTED, params));

return(params->next);
}


CELL * getListHead(CELL * params, CELL * * list)
{
CELL * cell;

cell = evaluateExpression(params);

if(!isList(cell->type))
	{
	*list = copyCell(nilCell);
	return(errorProcArgs(ERR_LIST_EXPECTED, params));
	}
*list = (CELL *)cell->contents; 
return(params->next);
}


/* ------------------------------- core predicates ------------------------ */

CELL * p_setlocale(CELL * params)
{
struct lconv * lc;
char * locale;
UINT category;

if(params != nilCell)
	params = getString(params, &locale);
else locale = NULL;

if(params != nilCell)
	getInteger(params, &category);
else category = LC_ALL;

locale = setlocale(category, locale);

if(locale == NULL)
	return(nilCell);

stringOutputRaw = (strcmp(locale, "C") == 0);

lc = localeconv();	
lc_decimal_point = *lc->decimal_point;

return(stuffString(locale));
}


CELL * p_commandLine(CELL * params)
{
commandLineFlag = getFlag(params);
return((commandLineFlag == FALSE ? nilCell : trueCell));
}


CELL * p_quote(CELL * params)
{
return(copyCell(params));
}


CELL * p_eval(CELL * params)
{
if(params->type == CELL_SYMBOL)
	params = (CELL*)((SYMBOL *)params->contents)->contents;
else
	params = evaluateExpression(params);

if(params->type == CELL_SYMBOL)
	{
	if(symbolProtectionLevel && symbolProtectionLevel == (recursionCount - 1))
		{
		if(isProtected(((SYMBOL *)params->contents)->flags))
			symbolProtectionLevel = 0xFFFFFFFF;
		}
	/* eval returns original symbol contents for usage in macros */
	pushResultFlag = 0;
	return(evaluateExpression(params));
	}

return(copyCell(evaluateExpression(params)));
}


CELL * p_catch(CELL * params)
{
jmp_buf errorJumpSave;
int envStackIdxSave;
int lambdaStackIdxSave;
int recursionCountSave;
int value;
CELL * expr;
CELL * result;
SYMBOL * symbol = NULL;
SYMBOL * contextSave;

expr = params;
if(params->next != nilCell)
    {
    getSymbol(params->next, &symbol);
    if(isProtected(symbol->flags))
        return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(symbol)));
    }

memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));
envStackIdxSave = envStackIdx;
recursionCountSave = recursionCount;
lambdaStackIdxSave = lambdaStackIdx;
contextSave = currentContext;

if((value = setjmp(errorJump)) != 0)
    {
    memcpy(errorJump, errorJumpSave, (sizeof(jmp_buf)));
    recoverEnvironment(envStackIdxSave);
    recursionCount = recursionCountSave;
    lambdaStackIdx = lambdaStackIdxSave;
    currentContext = contextSave;
    evalCatchFlag--;

    if(value == EXCEPTION_THROW)
        {
		if(symbol == NULL) return(throwResult);
        deleteList((CELL*)symbol->contents);
        symbol->contents = (UINT)throwResult;
        return(trueCell);
        }
        
    if(errorStream.buffer != NULL)
        {
		if(symbol == NULL) 
			{
			if(errorEvent == nilSymbol && evalCatchFlag == 0)
				varPrintf(OUT_CONSOLE, "\n%.1024s\n", errorStream.buffer);
			longjmp(errorJump, value);
			}
        deleteList((CELL*)symbol->contents);
        symbol->contents = (UINT)stuffString(errorStream.buffer);
        }

    return(nilCell);
    }

evalCatchFlag++;
result = copyCell(evaluateExpression(expr));
evalCatchFlag--;
memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));

if(symbol == NULL) return(result);

deleteList((CELL*)symbol->contents);
symbol->contents = (UINT)result;

return(trueCell);
}


CELL * p_throw(CELL * params)
{
if(evalCatchFlag == 0) 
    return(errorProc(ERR_THROW_WO_CATCH));

throwResult = copyCell(evaluateExpression(params));
longjmp(errorJump, EXCEPTION_THROW);

return(trueCell);
}

CELL * p_throwError(CELL * params)
{
evalFunc = NULL;
errorProcExt(ERR_USER_ERROR, evaluateExpression(params));
return(nilCell);
}

CELL * p_evalString(CELL * params)
{
SYMBOL * context = currentContext;
char * evalString;

params = getString(params, &evalString);
if(params->next != nilCell)
	{
	if((context = getCreateContext(params->next, TRUE)) == NULL)
		return(errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, params->next));
	}

return(copyCell(sysEvalString(evalString, params, context)));
}

CELL * sysEvalString(char * evalString, CELL * proc, SYMBOL * context)
{
CELL * program;
STREAM stream;
CELL * resultCell = nilCell;
jmp_buf errorJumpSave;
int recursionCountSave;
int envStackIdxSave;
int resultIdxSave;
SYMBOL * contextSave = NULL;

makeStreamFromString(&stream, evalString);
recursionCountSave = recursionCount;
envStackIdxSave = envStackIdx;
resultIdxSave = resultStackIdx;
contextSave = currentContext;
currentContext = context;

if(proc != nilCell)
	{
	evalCatchFlag++;
	memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));

	if(setjmp(errorJump) != 0)
		{
		memcpy(errorJump, errorJumpSave, (sizeof(jmp_buf)));
		recoverEnvironment(envStackIdxSave);
		evalCatchFlag--;
		recursionCount = recursionCountSave;
		currentContext = contextSave;
		return(evaluateExpression(proc));
		}
	}

while(TRUE)
	{
	pushResult(program = getCell(CELL_QUOTE));
	if(compileExpression(&stream, program) == 0) break;
	resultCell = evaluateExpression((CELL *)program->contents);
	if(resultStackIdx > (MAX_RESULT_STACK - 256))
		{
		program = popResult();
		cleanupResults(resultIdxSave);
		pushResult(program);
		}
	}

if(proc != nilCell)
	{
	memcpy(errorJump, errorJumpSave, (sizeof(jmp_buf)));
	evalCatchFlag--;
	}

currentContext = contextSave;
return(resultCell);
}


CELL * p_curry(CELL * params)
{
CELL * lambda;
CELL * cell;
SYMBOL * xPtr;

xPtr = translateCreateSymbol("_x", CELL_NIL, currentContext, TRUE);
lambda = getCell(CELL_LAMBDA);
cell = getCell(CELL_EXPRESSION);
lambda->contents =  (UINT)cell;
cell->contents = (UINT)stuffSymbol(xPtr);
cell->next = getCell(CELL_EXPRESSION);
cell = cell->next;
cell->contents = (UINT)copyCell(params);
cell = (CELL *)cell->contents;
cell->next = copyCell(params->next);
cell = cell->next;
cell->next = stuffSymbol(xPtr);

return(lambda);
}


CELL * p_apply(CELL * params)
{
CELL * expr;
CELL * args;
CELL * cell;
CELL * result;
CELL * func;
ssize_t count, cnt;
int resultIdxSave;

func = evaluateExpression(params);

cell = copyCell(func);
expr = getCell(CELL_EXPRESSION);
expr->contents = (UINT)cell;

params = params->next;
args = evaluateExpression(params);

if(params->next != nilCell)
	getInteger(params->next, (UINT *)&count);
else count = 0x7FFFFFFF;
if(count < 2) count = 2;

resultIdxSave = resultStackIdx + 2;

if(args->type == CELL_EXPRESSION)
	{
	args = (CELL *)args->contents;        
	cnt = count;
REDUCE:
	while(args != nilCell && cnt-- > 0)
		{
		if(isSelfEval(args->type))
			{
			cell->next = copyCell(args);
			cell = cell->next;
			}
		else
			{
			cell->next = getCell(CELL_QUOTE);
			cell = cell->next;
			cell->contents = (UINT)copyCell(args);
			}
		args = args->next;
		}
	pushResult(expr);
	result = copyCell(evaluateExpression(expr));
	if(args == nilCell) return(result);
	cell = copyCell(func);
	expr = getCell(CELL_EXPRESSION);
	expr->contents = (UINT)cell;
	cell->next = getCell(CELL_QUOTE);
	cell = cell->next;
	cell->contents = (UINT)result;
	cnt = count - 1;
	cleanupResults(resultIdxSave);
	goto REDUCE;		
	}

pushResult(expr);
return(copyCell(evaluateExpression(expr)));
}


CELL * p_args(CELL * params)
{
if(params != nilCell) 
	return(copyCell(implicitIndexList((CELL*)argsSymbol->contents, params)));
return(copyCell((CELL*)argsSymbol->contents));
}

/* in-place expansion, if symbol==NULL all uppercase, nil vars are expanded */
CELL * expand(CELL * expr, SYMBOL * symbol)
{
CELL * cell = nilCell;
SYMBOL * sPtr;
int enable = 1;
CELL * cont, * rep;
#ifdef SUPPORT_UTF8
int wchar;
#endif

if(expr->type == CELL_SYMBOL)
	return(expr);
/*
	return(copyCell(expr));
*/

if(isEnvelope(expr->type))
	cell = (CELL*)expr->contents;

while(cell != nilCell)
	{	
	if(cell->type == CELL_SYMBOL && (cell->contents == (UINT)symbol || symbol == NULL) )
		{
		sPtr = (SYMBOL *)cell->contents;
		if(symbol == NULL)
			{
#ifndef SUPPORT_UTF8
			enable = (toupper(*sPtr->name) == *sPtr->name);
#else
    		utf8_wchar(sPtr->name, &wchar);
			enable = (towupper(wchar) == wchar);
#endif
			cont = (CELL*)sPtr->contents;
			enable = (enable && cont->contents != (UINT)nilCell 
							&& cont->contents != (UINT)nilSymbol);
			}

		if(symbol || enable)
			{
			rep = copyCell((CELL*)sPtr->contents);
			cell->type = rep->type;
			cell->aux = rep->aux;
			cell->contents = rep->contents;
			rep->type = CELL_LONG;
			rep->aux = 0;
			rep->contents = 0;
			deleteList(rep);
			}
		}

	else if(isEnvelope(cell->type)) expand(cell, symbol);
	cell = cell->next;
	}

return(expr);
}

CELL * blockExpand(CELL * block, SYMBOL * symbol)
{
CELL * expanded = nilCell;
CELL * next = nilCell;

while(block != nilCell)
	{
	if(expanded == nilCell)
		{
		next = expand(copyCell(block), symbol);
		expanded = next;
		}
	else
		{
		next->next = expand(copyCell(block), symbol);
		next = next->next;
		}
	block = block->next;
	}

return(expanded);
}


CELL * p_expand(CELL * params)
{
SYMBOL * symbol;
CELL * expr;
CELL * next;
CELL * list;
CELL * cell;

expr = evaluateExpression(params);
if(!isList(expr->type) && expr->type != CELL_QUOTE)
	return(errorProcExt(ERR_LIST_EXPECTED, expr));

params = next = params->next;
if(params == nilCell)
	return(expand(copyCell(expr), NULL));

while((params = next) != nilCell)
	{
	next = params->next;
	params = evaluateExpression(params);
	if(params->type == CELL_SYMBOL)
		symbol = (SYMBOL*)params->contents;
	else if(params->type == CELL_DYN_SYMBOL)
		symbol = getDynamicSymbol(params);
	else if(params->type == CELL_EXPRESSION)
		{
		list = (CELL*)params->contents;
		while(list != nilCell)
			{
			if(list->type != CELL_EXPRESSION)
				return(errorProcExt(ERR_LIST_EXPECTED, list));
			cell = (CELL *)list->contents;
			if(cell->type != CELL_SYMBOL)
				return(errorProcExt(ERR_SYMBOL_EXPECTED, cell));
			symbol = (SYMBOL*)cell->contents;
			pushEnvironment(symbol->contents);
			pushEnvironment(symbol);
			symbol->contents = (UINT)cell->next;
			expr = expand(copyCell(expr), symbol);
			symbol = (SYMBOL*)popEnvironment();
			symbol->contents = popEnvironment();
			pushResult(expr);
			list = list->next;
			continue;
			}
		break;
		}
	else 
		return(errorProcExt(ERR_LIST_OR_SYMBOL_EXPECTED, params));
	expr = expand(copyCell(expr), symbol);
	pushResult(expr);
	}

return(copyCell(expr));
}


CELL * defineOrMacro(CELL * params, UINT cellType)
{
SYMBOL * symbol;
CELL * argsPtr;
CELL * args;
CELL * lambda;

if(params->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_LIST_OR_SYMBOL_EXPECTED, params));

/* symbol to be defined */
argsPtr = (CELL *)params->contents;
if(argsPtr->type != CELL_SYMBOL)
	{
	if(argsPtr->type == CELL_DYN_SYMBOL)
		symbol = getDynamicSymbol(argsPtr);
	else
		return(errorProcExt(ERR_SYMBOL_EXPECTED, params));
	}
else symbol = (SYMBOL *)argsPtr->contents;

if(isProtected(symbol->flags))
	return(errorProcExt(ERR_SYMBOL_PROTECTED, params));

/* local symbols */
argsPtr = copyList(argsPtr->next);
lambda = getCell(cellType);
lambda->aux = (UINT)nilCell;
args = getCell(CELL_EXPRESSION);
args->contents = (UINT)argsPtr;
/* body expressions */
args->next = copyList(params->next);
lambda->contents = (UINT)args;

deleteList((CELL *)symbol->contents);

symbol->contents = (UINT)lambda;

pushResultFlag = FALSE;
return(lambda);
}

#define TYPE_SET 1
#define TYPE_CONSTANT 2
#define TYPE_DEFINE 3

CELL * p_define(CELL * params)
{
if(params->type != CELL_SYMBOL)
	{
	if(params->type != CELL_DYN_SYMBOL)
		return(defineOrMacro(params, CELL_LAMBDA));
	return(setDefine(getDynamicSymbol(params), params->next, TYPE_SET));
	}

return(setDefine((SYMBOL *)params->contents, params->next, TYPE_SET));
}


CELL * p_defineMacro(CELL * params)
{
return(defineOrMacro(params, CELL_MACRO));
}


CELL * p_setq(CELL * params)
{
SYMBOL * symbol;
CELL * next;

for(;;)
	{
	if(params->type != CELL_SYMBOL)
		{
		if(params->type == CELL_DYN_SYMBOL)
			symbol = getDynamicSymbol(params);
		else
			return(errorProcExt(ERR_SYMBOL_EXPECTED, params));
		}
	else
		symbol = (SYMBOL *)params->contents;
	params = params->next;
	next = params->next;
	if(params == nilCell)
		return(copyCell((CELL*)symbol->contents));
	if(next == nilCell) return(setDefine(symbol, params, TYPE_SET));
	setDefine(symbol, params, TYPE_SET);
	params = next;
	}
}


CELL * p_set(CELL *params)
{
SYMBOL * symbol;
CELL * next;

for(;;)
	{
	params = getSymbol(params, &symbol);
	next = params->next;
	if(params == nilCell)
		return(copyCell((CELL*)symbol->contents));
	if(next == nilCell) return(setDefine(symbol, params, TYPE_SET));
	setDefine(symbol, params, TYPE_SET);
	params = next;
	}
}


CELL * p_constant(CELL *params)
{
SYMBOL * symbol;
CELL * next;

for(;;)
	{
	params = getSymbol(params, &symbol);
	/* protect contexts from being set, but not vars holding contexts */
	if(symbolType(symbol) == CELL_CONTEXT && (SYMBOL *)((CELL *)symbol->contents)->contents == symbol)
		return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(symbol)));
	next = params->next;
	if(symbol->context != currentContext)
		return(errorProcExt2(ERR_NOT_CURRENT_CONTEXT, stuffSymbol(symbol)));
	symbol->flags |= SYMBOL_PROTECTED;
	if(params == nilCell)
		return(copyCell((CELL*)symbol->contents));
	if(next == nilCell) return(setDefine(symbol, params, TYPE_CONSTANT));
	setDefine(symbol, params, TYPE_CONSTANT);
	params = next;
	}
}


CELL * setDefine(SYMBOL * symbol, CELL * params, int type)
{
CELL * cell;

if(isProtected(symbol->flags))
	{
	if(type == TYPE_CONSTANT)
		{
		if(symbol == nilSymbol || symbol == trueSymbol)
			return(errorProcExt2(ERR_SYMBOL_EXPECTED, stuffSymbol(symbol)));
		}
	else
		return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(symbol)));
	}

cell = copyCell(evaluateExpression(params));

deleteList((CELL *)symbol->contents);
symbol->contents = (UINT)(cell);

pushResultFlag = FALSE; 
return(cell);
}


CELL * p_global(CELL * params)
{
SYMBOL * sPtr;

do
	{
	params = getSymbol(params, &sPtr);
	if(sPtr->context != mainContext || currentContext != mainContext)
		return(errorProcExt2(ERR_NOT_IN_MAIN, stuffSymbol(sPtr)));
	else 
		sPtr->flags |= SYMBOL_GLOBAL;
	} while (params != nilCell);

return(stuffSymbol(sPtr));
}

#define LET_STD 0
#define LET_NEST 1
#define LET_EXPAND 2
#define LET_LOCAL 3

CELL * let(CELL * params, int type);

CELL * p_let(CELL * params) { return(let(params, LET_STD)); }
CELL * p_letn(CELL * params) { return(let(params, LET_NEST)); }
CELL * p_letExpand(CELL * params) { return(let(params, LET_EXPAND)); }
CELL * p_local(CELL * params) { return(let(params, LET_LOCAL)); }

CELL * let(CELL * params, int type)
{
CELL * inits;
CELL * cell;
CELL * result = nilCell;
CELL * args = NULL, * list = NULL;
CELL * body;
SYMBOL * symbol;
int localCount = 0;

if(params->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_INVALID_LET, params));

/* evaluate symbol assignments in parameter list 
   handle double syntax classic: (let ((v1 e1) (v2 e2) ...) ...) 
                            and: (let (v1 e1 v2 e2 ...) ...)
*/
inits = (CELL*)params->contents;
body = params->next;

if(type == LET_LOCAL)
	{
	while(inits != nilCell)
		{
		if(inits->type != CELL_SYMBOL)
			return(errorProcExt(ERR_SYMBOL_EXPECTED, inits));
		symbol = (SYMBOL *)inits->contents;
		if(isProtected(symbol->flags))
        		return(errorProcExt(ERR_SYMBOL_PROTECTED, inits));
		pushEnvironment(symbol->contents);
		pushEnvironment(symbol);
		symbol->contents = (UINT)nilCell;
		localCount++;
		inits = inits->next;
		}
	goto EVAL_LET_BODY;	
	}

while(inits != nilCell)
	{
	if(inits->type != CELL_EXPRESSION)
		{
		if(inits->type != CELL_SYMBOL)
			return(errorProcExt(ERR_INVALID_LET, inits));
		cell = inits;
		inits = ((CELL*)cell->next)->next;
		}
	else 
		{
		cell = (CELL *)inits->contents;
		if(cell->type != CELL_SYMBOL)
			return(errorProcExt(ERR_SYMBOL_EXPECTED, inits));
		inits = inits->next;
		}

	if(type == LET_STD || type == LET_EXPAND)
		{
		if(localCount == 0) 
			list = args = copyCell(evaluateExpression(cell->next));
		else 
			{
			args->next = copyCell(evaluateExpression(cell->next));
			args = args->next;
			}
		}
	else /* LET_NEST */
		{
		symbol = (SYMBOL *)cell->contents;
		if(isProtected(symbol->flags))
        		return(errorProcExt(ERR_SYMBOL_PROTECTED, cell));
		args = copyCell(evaluateExpression(cell->next));
		pushEnvironment((CELL *)symbol->contents);
		pushEnvironment((UINT)symbol);
		symbol->contents = (UINT)args;
		}

	localCount++;
	}

/* save symbols and get new bindings */
if(type == LET_STD || type == LET_EXPAND) 
	{
	inits = (CELL*)params->contents;
	while(inits != nilCell)
		{
		if(inits->type == CELL_EXPRESSION)
			{
			cell = (CELL *)inits->contents;
			inits = inits->next;
			}
		else
			{
			cell = inits;
			inits = ((CELL*)cell->next)->next;
			}	

		symbol = (SYMBOL *)cell->contents;

		if(isProtected(symbol->flags))
			return(errorProcExt(ERR_SYMBOL_PROTECTED, cell));

		pushEnvironment((CELL *)symbol->contents);
		pushEnvironment((UINT)symbol);
		symbol->contents = (UINT)list;

		args = list;
		list = list->next;
		args->next = nilCell; /* decouple */

		/* hook in LET_EXPAND mode here */
		if(type == LET_EXPAND)
			{
			body = blockExpand(body, symbol);
			pushResult(body);
			}

		}
	}

EVAL_LET_BODY:
/* evaluate body expressions */
while(body != nilCell)
    {
    if(result != nilCell) deleteList(result);
    result = copyCell(evaluateExpression(body));
	body = body->next;
    }

/* restore environment */
while(localCount--)
	{
	symbol = (SYMBOL *)popEnvironment();
	deleteList((CELL *)symbol->contents);
	symbol->contents = popEnvironment();
	}

return(result);
}

CELL * p_first(CELL * params)
{
char str[2];
CELL * cell;

cell = evaluateExpression(params);

if(cell->type == CELL_STRING)
    {
    if((str[0] = *(char *)cell->contents) == 0)
	return(stuffString(""));
#ifndef SUPPORT_UTF8
    str[1] = 0;
    return(stuffString(str));
#else
    return(stuffStringN((char*)cell->contents, utf8_1st_len((char*)cell->contents)));
#endif
    }

else if(isList(cell->type))
	return(copyCell((CELL *)cell->contents));
else if(cell->type == CELL_ARRAY)
	return(copyCell(*(CELL * *)cell->contents)); 

return(errorProcExt(ERR_ARRAY_LIST_OR_STRING_EXPECTED, params));
}


CELL * p_rest(CELL * params)
{
CELL * cell;
CELL * tail;

cell = evaluateExpression(params);
if(cell->type == CELL_STRING)
    {
    if(*(char *)cell->contents == 0)
	return(stuffString(""));
#ifndef SUPPORT_UTF8
    return(stuffString((char *)(cell->contents + 1)));
#else
    return(stuffString((char *)(cell->contents + utf8_1st_len((char *)cell->contents))));
#endif
    }

else if(isList(cell->type))
	{
	tail = getCell(CELL_EXPRESSION);
	tail->contents = (UINT)copyList(((CELL*)cell->contents)->next);
	return(tail);
	}
else if(cell->type == CELL_ARRAY)
	return(subarray(cell, 1, -1));

return(errorProcExt(ERR_ARRAY_LIST_OR_STRING_EXPECTED, params));
}


CELL * implicitNrestSlice(CELL * num, CELL * params)
{
CELL * list;
CELL * rest;
ssize_t  n, len;
#ifdef SUPPORT_UTF8
char * str;
int utf8len;
#endif

getIntegerExt(num, (UINT *)&n, FALSE);
list = evaluateExpression(params);

if(list->type == CELL_CONTEXT)
	list = (CELL *)(translateCreateSymbol(
		((SYMBOL*)list->contents)->name,
		CELL_NIL,
		(SYMBOL*)list->contents,
		TRUE))->contents;

/* slice  */
if(isNumber(list->type))
    {
    getIntegerExt(list, (UINT*)&len, FALSE);
    list = evaluateExpression(params->next);

	if(list->type == CELL_CONTEXT)
		list = (CELL *)(translateCreateSymbol(
			((SYMBOL*)list->contents)->name,
			CELL_NIL,
			(SYMBOL*)list->contents,
			TRUE))->contents;

    if(isList(list->type))    
        return(sublist((CELL *)list->contents, n, len));
    else if(list->type == CELL_STRING)
        return(substring((char *)list->contents, list->aux-1, n, len));
	else if(list->type == CELL_ARRAY)
		return(subarray(list, n, len));
    }
    
/* nrest lists */
else if(isList(list->type))
    {
    list = (CELL *)list->contents;

    if(n < 0) n = convertNegativeOffset(n, list);
    
    while(n-- && list != nilCell)
      list = list->next;
  
    rest = getCell(CELL_EXPRESSION);
    rest->contents = (UINT)copyList(list);
    return(rest);
    }

/* nrest strings */
else if(list->type == CELL_STRING)
    {
#ifndef SUPPORT_UTF8
    if(n < 0) n = adjustNegativeIndex(n, list->aux - 1);
    else if(n > list->aux - 1) n = list->aux - 1;
    return(stuffStringN((char *)list->contents + n, list->aux - 1 - n));
#else
    len = 0;
    str = (char *)list->contents;    
    utf8len = utf8_wlen(str);
    if(n < 0) n = adjustNegativeIndex(n, utf8len);
    else if(n > utf8len) n = utf8len;
    while(n--) len += utf8_1st_len(str + len);
    return(stuffStringN((char *)list->contents + len, list->aux - 1 - len));
#endif
    }

else if(list->type == CELL_ARRAY)
	return(subarray(list, n, -1));

return(errorProcExt(ERR_ILLEGAL_TYPE, params));
}


CELL * p_cons(CELL * params)
{
CELL * cons;
CELL * head;
CELL * tail;

if(params == nilCell)
	return(getCell(CELL_EXPRESSION));

head = copyCell(evaluateExpression(params));

cons = getCell(CELL_EXPRESSION);
cons->contents = (UINT)head;
params = params->next;

if(params != nilCell)
    {
    tail = evaluateExpression(params);
    
    if(isList(tail->type))
        {
		if(params->next != nilCell)	
			{
			if(((CELL*)params->next)->contents == -1)
				{
				cons->contents = (UINT)copyList((CELL *)tail->contents);
				tail = (CELL*)cons->contents;
				while(tail->next != nilCell)
					tail = tail->next;
				tail->next = head;
				return(cons);
				}
			}	
        head->next = copyList((CELL *)tail->contents);
        cons->type = tail->type;	
        }
    else
		head->next = copyCell(tail);
    }

return(cons);
}



CELL * p_list(CELL * params)
{
CELL * list;
CELL * lastCopy;
CELL * copy;
CELL * cell;
int resultIdxSave;

list = getCell(CELL_EXPRESSION);
lastCopy = NULL;

resultIdxSave = resultStackIdx;
while(params != nilCell)
	{
	cell = evaluateExpression(params);
	if(cell->type == CELL_ARRAY)
		copy = arrayList(cell);
	else
		copy = copyCell(cell);
	if(copy != nilCell)
		{
		if(lastCopy == NULL)
			list->contents = (UINT)copy;
		else lastCopy->next = copy;
		}
	params = params->next;
	lastCopy = copy;
	cleanupResults(resultIdxSave);
	}
return(list);
}



CELL * p_last(CELL * params)
{
CELL * list;
char * str;
#ifdef SUPPORT_UTF8
char * ptr;
int len;
#endif

list = evaluateExpression(params);
if(list->type == CELL_STRING)
	{
	str = (char *)list->contents;
#ifndef SUPPORT_UTF8
	return(stuffString(str + list->aux - 2));
#else
	ptr = str;
	while((len = utf8_1st_len(str)) != 0)
		{
		ptr = str;
		str += len;
		}
	return(stuffStringN(ptr, utf8_1st_len(ptr)));
#endif
	}

else if(isList(list->type))
	{
	list = (CELL *)list->contents;
	while(list->next != nilCell) list = list->next;
	return(copyCell(list));
	}

else if(list->type == CELL_ARRAY)
	return(copyCell(*((CELL * *)list->contents + (list->aux - 1) / sizeof(UINT) - 1)));

return(errorProcExt(ERR_ARRAY_LIST_OR_STRING_EXPECTED, params));
}


/* -------------------------- program flow  and logical ------------------ */

CELL * evaluateBlock(CELL * cell)
{
CELL * result;

result = nilCell;

while(cell != nilCell)
	{
	result = evaluateExpression(cell);
	cell = cell->next;
	}
return(result);
}


CELL * p_if(CELL * params)
{
CELL * cell;

cell = evaluateExpression(params);
while(isNil(cell) || isEmpty(cell))
	{
	params = params->next;
	if(params->next == nilCell) 
		return(copyCell(cell));
	params = params->next;
	cell = evaluateExpression(params);
	}

if(params->next == nilCell) return(copyCell(cell));

return((copyCell(evaluateExpression(params->next)))); 
}


CELL * p_unless(CELL * params)
{
CELL * cell;

cell = evaluateExpression(params);
if(!isNil(cell) && !isEmpty(cell))
	params = params->next;

return((copyCell(evaluateExpression(params->next)))); 
}


CELL * p_condition(CELL * params)
{
CELL * condition;
CELL * eval = nilCell;

while(params != nilCell)
	{
	if(params->type == CELL_EXPRESSION)
		{
		condition = (CELL *)params->contents;
		eval = evaluateExpression(condition);
		if(!isNil(eval) && !isEmpty(eval))
			{
			if(condition->next != nilCell)
				return(copyCell(evaluateBlock(condition->next)));
			return(copyCell(eval));
			}
		params = params->next;
		}
	else return(errorProc(ERR_LIST_EXPECTED));
	}

return(copyCell(eval));
}


CELL * p_case(CELL * params)
{
CELL * cases;
CELL * cond;

cases = params->next;
params = evaluateExpression(params);
while(cases != nilCell)
  {
  if(cases->type == CELL_EXPRESSION)
    {
    cond = (CELL *)cases->contents;
    if(compareCells(params, cond) == 0
	  || (cond->type == CELL_SYMBOL && symbolType((SYMBOL *)cond->contents) == CELL_TRUE)
          || cond->type == CELL_TRUE)
	return(copyCell(evaluateBlock(cond->next)));
    }
	cases = cases->next;
  }
return(nilCell);
}

#define REPEAT_WHILE 0
#define REPEAT_DOWHILE 1
#define REPEAT_UNTIL 2
#define REPEAT_DOUNTIL 3

CELL * p_while(CELL * params) { return(repeat(params, REPEAT_WHILE)); }
CELL * p_doWhile(CELL * params) { return(repeat(params, REPEAT_DOWHILE)); }
CELL * p_until(CELL * params) { return(repeat(params, REPEAT_UNTIL)); }
CELL * p_doUntil(CELL * params) { return(repeat(params, REPEAT_DOUNTIL)); }

/* in 9.0.11 back to 8.8.3 behaviour, speed impact too big on some
   algorithms
*/

#define OLD
#ifdef OLD
CELL * repeat(CELL * params, int type)
{
CELL * result;
CELL * cell;
int resultIdxSave;

resultIdxSave = resultStackIdx;
result = nilCell;
while(TRUE)
    {
    switch(type)
        {
        case REPEAT_WHILE:
            cell = evaluateExpression(params);
            if(isNil(cell) || isEmpty(cell)) goto END_REPEAT;
            cleanupResults(resultIdxSave);
            result = evaluateBlock(params->next);
            continue;
        case REPEAT_DOWHILE:
            result = evaluateBlock(params->next);
            cell = evaluateExpression(params);
            if(isNil(cell) || isEmpty(cell)) goto END_REPEAT;
            cleanupResults(resultIdxSave);
            continue;
        case REPEAT_UNTIL:
            cell = evaluateExpression(params);
            if(!isNil(cell) && !isEmpty(cell)) goto END_REPEAT;
            cleanupResults(resultIdxSave);
            result = evaluateBlock(params->next);
            continue;
        case REPEAT_DOUNTIL:
            result = evaluateBlock(params->next);
            cell = evaluateExpression(params);
            if(!isNil(cell) && !isEmpty(cell)) goto END_REPEAT;
            cleanupResults(resultIdxSave);
            continue;
        default:
            break;
        }
    }
END_REPEAT:
return(copyCell(result));
}
#endif

#ifdef POST_8_8_3
CELL * repeat(CELL * params, int type)
{
CELL * result;
CELL * cell;
int resultIdxSave;

resultIdxSave = resultStackIdx;
result = nilCell;
while(TRUE)
	{
	switch(type)
		{
		case REPEAT_WHILE:
			cell = evaluateExpression(params);
            if(isNil(cell) || isEmpty(cell)) goto END_REPEAT;
			cleanupResults(resultIdxSave);
			deleteList(result);
			result = copyCell(evaluateBlock(params->next));
			continue;
		case REPEAT_DOWHILE:
			deleteList(result);
			result = copyCell(evaluateBlock(params->next));
			cell = evaluateExpression(params);
            if(isNil(cell) || isEmpty(cell)) goto END_REPEAT;
			cleanupResults(resultIdxSave);
			continue;
		case REPEAT_UNTIL:
			cell = evaluateExpression(params);
            if(!isNil(cell) && !isEmpty(cell)) goto END_REPEAT;
			cleanupResults(resultIdxSave);
			deleteList(result);
			result = copyCell(evaluateBlock(params->next));
			continue;
		case REPEAT_DOUNTIL:
			deleteList(result);
			result = copyCell(evaluateBlock(params->next));
			cell = evaluateExpression(params);
            if(!isNil(cell) && !isEmpty(cell)) goto END_REPEAT;
			cleanupResults(resultIdxSave);
			continue;
		default:
			break;
		}

	}

END_REPEAT:

return(result);
}
#endif


CELL * getPushSymbolParam(CELL * params, SYMBOL * * sym)
{
SYMBOL * symbol;
CELL * cell;

if(params->type != CELL_EXPRESSION)
	return(errorProcExt(ERR_LIST_EXPECTED, params));

cell = (CELL *)params->contents;
if(cell->type != CELL_SYMBOL)
	return(errorProcExt(ERR_SYMBOL_EXPECTED, cell));

*sym = symbol = (SYMBOL *)cell->contents;
if(isProtected(symbol->flags))
	return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(symbol)));

pushEnvironment((CELL *)symbol->contents);
pushEnvironment((UINT)symbol);
symbol->contents = (UINT)nilCell;

return(cell->next);
}

CELL * loop(CELL * params, int forFlag)
{
CELL * cell;
CELL * cond = nilCell;
CELL * block;
SYMBOL * symbol;
double fromFlt, toFlt, interval, step, cntFlt;
INT64 stepCnt, i;
INT64 fromInt64, toInt64;
int intFlag;
int resultIdxSave;

cell = getPushSymbolParam(params, &symbol);

/* integer loops for dotimes and (for (i from to) ...) */
if((intFlag = ((CELL *)cell->next)->next == nilCell))
	{
	if(forFlag)
		{
		cell = getInteger64(cell, &fromInt64);
		getInteger64(cell, &toInt64);
		stepCnt = (toInt64 > fromInt64) ? toInt64 - fromInt64 : fromInt64 - toInt64;
		}
	else /* dotimes */
		{
		fromInt64 = toInt64 = 0;
		cond = getInteger64(cell, &stepCnt);
		}
	}
else /* float (for (i from to step) ...) */
	{
	cell = getFloat(cell, &fromFlt);
	cell = getFloat(cell, &toFlt);
	cond = getFloat(cell, &step);
	if(isnan(fromFlt) || isnan(toFlt) || isnan(step))
		return(errorProc(ERR_INVALID_PARAMETER_NAN));
	if(step < 0) step = -step;
	if(fromFlt > toFlt) step = -step;
	cntFlt = (fromFlt < toFlt) ? (toFlt - fromFlt)/step : (fromFlt - toFlt)/step;
	stepCnt = (cntFlt > 0.0) ? floor(cntFlt + 0.0000000001) : floor(-cntFlt + 0.0000000001);
	}
	
block = params->next;
resultIdxSave = resultStackIdx;
cell = nilCell;
for(i = 0; i <= stepCnt; i++)
	{
	if(!forFlag && i == stepCnt) break;
	deleteList((CELL *)symbol->contents);
	if(intFlag)	
		{
		symbol->contents = 
			(UINT)stuffInteger64((fromInt64 > toInt64) ? fromInt64 - i: 
                                                         fromInt64 + i);
		}
	else
		{
		interval = fromFlt + i * step;
		symbol->contents = (UINT)stuffFloat(&interval);
		}
	cleanupResults(resultIdxSave);
	if(cond != nilCell)  
			{
			cell = evaluateExpression(cond);
			if(!isNil(cell)) break;
			}
	cell = evaluateBlock(block);
	}

cell = copyCell(cell);
deleteList((CELL *)symbol->contents);
symbol = (SYMBOL*)popEnvironment();
symbol->flags &= ~SYMBOL_PROTECTED;
symbol->contents = (UINT)popEnvironment();

return(cell);
}


CELL * p_dotimes(CELL * params)
{
return(loop(params, 0));
}

CELL * p_for(CELL * params)
{
return(loop(params, 1));
}

CELL * p_dolist(CELL * params)
{
return(dolist(params, 0));
}

CELL * p_doargs(CELL * params)
{
return(dolist(params, 1));
}

CELL * dolist(CELL * params, int argsFlag)
{
CELL * cell;
CELL * list;
CELL * cond;
SYMBOL * symbol;
CELL * cellIdx;
CELL * result;
int resultIdxSave;

cell = getPushSymbolParam(params, &symbol);

pushEnvironment(dolistIdxSymbol->contents);
pushEnvironment(dolistIdxSymbol);
cellIdx = stuffInteger(0);
dolistIdxSymbol->contents = (UINT)cellIdx;

/* back to pre 8.3.2 behaviour, copying the list,
but now list destruction on result stack to be
safe for throw */
if(argsFlag)
	{
	list = copyCell((CELL *)argsSymbol->contents);
	cond = cell;
	}
else
	{
	list = copyCell(evaluateExpression(cell));
	if(!isList(list->type))
		return(errorProcExt(ERR_LIST_EXPECTED, cell));
	cond = cell->next;
	}


/* make sure worklist gets destroyed */
pushResult(list); 
list = (CELL *)list->contents;

resultIdxSave = resultStackIdx;
cell = nilCell;
while(list!= nilCell)
	{
	cleanupResults(resultIdxSave);
	deleteList((CELL *)symbol->contents);
	symbol->contents = (UINT)copyCell(list);
	if(cond != nilCell)
		{
		cell = evaluateExpression(cond);
		if(!isNil(cell)) break;
		}
	cell = evaluateBlock(params->next);
	cellIdx->contents += 1;
	list = list->next;
	}

result = copyCell(cell);

deleteList(cellIdx);
dolistIdxSymbol = (SYMBOL*)popEnvironment();
dolistIdxSymbol->contents = (UINT)popEnvironment();
deleteList((CELL *)symbol->contents);
symbol = (SYMBOL*)popEnvironment();
symbol->contents = (UINT)popEnvironment();

return(result);
}


CELL * p_evalBlock(CELL * params)
{
return(copyCell(evaluateBlock(params)));
}


CELL * p_silent(CELL * params)
{
evalSilent  = TRUE;

return(copyCell(evaluateBlock(params)));
}


CELL * p_and(CELL * params)
{
CELL * result = nilCell;

while(params != nilCell)
	{
	result = evaluateExpression(params);
	if(isNil(result) || isEmpty(result)) return(copyCell(result));
	params = params->next;
	}

return(copyCell(result));     
}


CELL * p_or(CELL * params)
{
CELL * result = nilCell;

while(params != nilCell)
	{
	result = evaluateExpression(params);
	if(!isNil(result) && !isEmpty(result)) 
		return(copyCell(result));
	params = params->next;
	}

return(copyCell(result));
}


CELL * p_not(CELL * params)
{
CELL * eval;

eval = evaluateExpression(params);
if(isNil(eval) || isEmpty(eval)) 
    return(trueCell);
return(nilCell);
}





/* ------------------------------ I / O --------------------------------- */

CELL * p_print(CELL * params)
{
return println(params, FALSE);
}


CELL * p_println(CELL * params)
{
return println(params, TRUE);
}


CELL * println(CELL * params, int lineFeed)
{
CELL * result;

result = nilCell;
while(params != nilCell)
	{
	result = evaluateExpression(params);
	if(printCell(result, 0, OUT_DEVICE)  == 0)
		return(nilCell);
	params = params->next;
	}

if(lineFeed) varPrintf(OUT_DEVICE, LINE_FEED);

return(copyCell(result));
}


CELL * p_device(CELL * params)
{
if(params != nilCell)
	getInteger(params, &printDevice);
return(stuffInteger(printDevice));
}


CELL * p_load(CELL * params)
{
char * fileName;
CELL * result = nilCell;
CELL * next;
SYMBOL * context;
int count = 0;
char * str;

/* get last parameter */
if((next = params) == nilCell)
	errorProc(ERR_MISSING_ARGUMENT);
while(next->next != nilCell)
	{
	count++;
	next = next->next;
	}

next = evaluateExpression(next);
if(next->type == CELL_STRING)
	{
	count++;
	context = mainContext;
	}
else
	{
	if(count == 0)
		errorProcExt(ERR_STRING_EXPECTED, next);
	if((context = getCreateContext(next, FALSE)) == NULL)
		errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, next);
	next = NULL;
	}

while(count--)
	{
	/* if last arg was a string, avoid double evaluation */
	if(count == 0 && next != NULL)
		getStringSize(next, &fileName, NULL, FALSE);
	else 
		params = getString(params, &fileName);

	/* check for URL format */
	if(my_strnicmp(fileName, "http://", 7) == 0)
		{
		str = alloca(MAX_URL_LEN + 16);
		snprintf(str, MAX_URL_LEN + 16, "(get-url \"%s\" 60000)", fileName);
		result = sysEvalString(str, nilCell, context);
		if(memcmp((char *)result->contents, "ERR:", 4) == 0)
			return(errorProcExt2(ERR_ACCESSING_FILE, stuffString((char *)result->contents)));
		else
			result = copyCell(sysEvalString((char *)result->contents, nilCell, context));
		}
    else if(my_strnicmp(fileName, "file://", 7) == 0)
		result = loadFile(fileName + 7, 0, 0, context);
	else 
		result = loadFile(fileName, 0, 0, context);

	if(result == NULL)
	    return(errorProcExt2(ERR_ACCESSING_FILE, stuffString(fileName)));
	}

return(result);
}


void saveContext(SYMBOL * sPtr, UINT device)
{
SYMBOL * contextSave;

contextSave = currentContext;

currentContext = sPtr;

if(sPtr != mainContext)
	varPrintf(device, "%s(context '%s)%s%s", 
		LINE_FEED, sPtr->name, LINE_FEED, LINE_FEED);


saveSymbols((SYMBOL *)((CELL*)sPtr->contents)->aux, device);

if(sPtr != mainContext)
	varPrintf(device, "%s(context 'MAIN)%s%s", 
		LINE_FEED, LINE_FEED, LINE_FEED);

currentContext = contextSave;
}


void saveSymbols(SYMBOL * sPtr, UINT device)
{
int type;

if(sPtr != NIL_SYM && sPtr != NULL)
	{
	saveSymbols(sPtr->left, device);
	type = symbolType(sPtr);
	if(type == CELL_CONTEXT)
		{
		if(sPtr == (SYMBOL *)((CELL *)sPtr->contents)->contents)
			{
			if(sPtr != currentContext) saveContext(sPtr, device);
			}
		else printSymbol(sPtr, device);	
		}
	else if(type != CELL_PRIMITIVE && type != CELL_NIL
		&& sPtr != trueSymbol && type != CELL_IMPORT_CDECL
		&& sPtr != argsSymbol
#ifdef WINCC
		&& type != CELL_IMPORT_DLL
#endif
		)
		if(*sPtr->name != '$') printSymbol(sPtr, device);
	saveSymbols(sPtr->right, device);
	}
}


CELL * p_save(CELL * params)
{
char * fileName;
char * str;
STREAM strStream;
UINT printDeviceSave;
CELL * result;
SYMBOL * contextSave;
SYMBOL * data;

contextSave = currentContext;
currentContext = mainContext;
printDeviceSave = printDevice;

params = getString(params, &fileName);

/* check for URL format */
if(my_strnicmp(fileName, "http://", 7) == 0)
	{
	openStrStream(&strStream, MAX_STRING, 0);
	serializeSymbols(params, (UINT)&strStream);
	data = translateCreateSymbol("$data", CELL_SYMBOL, mainContext, TRUE);
	data->contents = (UINT)stuffString(strStream.buffer);
	str = alloca(MAX_URL_LEN + 20);
	snprintf(str, MAX_URL_LEN + 20, "(put-url \"%s\" $data 60000)", fileName);
	result = sysEvalString(str, nilCell, mainContext);
	closeStrStream(&strStream);
	deleteList((CELL *)data->contents);
	data->contents = (UINT)nilCell;
	return(copyCell(result));
	}
else
	{
	if(my_strnicmp(fileName, "file://", 7) == 0)
		fileName = fileName + 7; 
	if( (printDevice = (UINT)openFile(fileName, "write", NULL)) == (UINT)-1)
		return(errorProcExt2(ERR_SAVING_FILE, stuffString(fileName)));
	serializeSymbols(params, OUT_DEVICE);
	close((int)printDevice);
	}

currentContext = contextSave;
printDevice = printDeviceSave;
return(trueCell);
}

void serializeSymbols(CELL * params, UINT device)
{
SYMBOL * sPtr;

if(params->type == CELL_NIL)
	saveSymbols((SYMBOL *)((CELL*)currentContext->contents)->aux, device);
else
    while(params != nilCell)
	{
	params = getSymbol(params, &sPtr);
	if(symbolType(sPtr) == CELL_CONTEXT)
		saveContext((SYMBOL*)((CELL *)sPtr->contents)->contents, device);
	else        
		printSymbol(sPtr, device);
	}
}

/* ----------------------- copy a context with 'new' -------------- */
static SYMBOL * fromContext;
static SYMBOL * newContext;
static int overWriteFlag;

CELL * copyContextList(CELL * cell);
UINT * copyContextArray(CELL * array);


CELL * copyContextCell(CELL * cell)
{
CELL * newCell;
SYMBOL * sPtr;
SYMBOL * newSptr;

if(firstFreeCell == NULL) allocBlock();
newCell = firstFreeCell;
firstFreeCell = newCell->next;
++cellCount;

newCell->type = cell->type;
newCell->next = nilCell;
newCell->aux = cell->aux;
newCell->contents = cell->contents;

if(cell->type == CELL_DYN_SYMBOL)
	{
	sPtr = (SYMBOL*)cell->aux;
	if(sPtr->context == fromContext)
		newCell->aux =
			(UINT)translateCreateSymbol(sPtr->name, 0, newContext, TRUE);
	newCell->contents = (UINT)allocMemory(strlen((char *)cell->contents) + 1);
	memcpy((void *)newCell->contents,
		(void*)cell->contents, strlen((char *)cell->contents) + 1);
	}

if(cell->type == CELL_SYMBOL)
	{
	/* if the cell copied itself contains a symbol copy it recursevely,
	   if new, if not done here it might not been seen as new later and left
           without contents */
	sPtr = (SYMBOL *)cell->contents;
	if(sPtr->context == fromContext && !(sPtr->flags & SYMBOL_BUILTIN))
		{
		if((newSptr = lookupSymbol(sPtr->name, newContext)) == NULL)
			{
			newSptr = translateCreateSymbol(sPtr->name, symbolType(sPtr), newContext, TRUE);
			newSptr->contents = (UINT)copyContextCell((CELL*)sPtr->contents);
			}
		newCell->contents = (UINT)newSptr;
		}
	}

if(isEnvelope(cell->type))
        {
        if(cell->type == CELL_ARRAY)
                newCell->contents = (UINT)copyContextArray(cell);
        else
	        newCell->contents = (UINT)copyContextList((CELL *)cell->contents);
        }

else if(cell->type == CELL_STRING)
	{
	newCell->contents = (UINT)allocMemory((UINT)cell->aux);
	memcpy((void *)newCell->contents,
		(void*)cell->contents, (UINT)cell->aux);
	}

return(newCell);
}


CELL * copyContextList(CELL * cell)
{
CELL * firstCell;
CELL * newCell;

if(cell == nilCell || cell == trueCell) return(cell);

firstCell = newCell = copyContextCell(cell);

while((cell = cell->next) != nilCell)
	{
	newCell->next = copyContextCell(cell);
	newCell = newCell->next;
	}
	
return(firstCell);
}


UINT * copyContextArray(CELL * array)
{
CELL * * newAddr;
CELL * * orgAddr;
CELL * * addr;
size_t size;

addr = newAddr = (CELL * *)callocMemory(array->aux);

size = (array->aux - 1) / sizeof(UINT);
orgAddr = (CELL * *)array->contents;

while(size--)
	*(newAddr++) = copyContextCell(*(orgAddr++));
	
return((UINT*)addr);
}


void iterateSymbols(SYMBOL * sPtr)
{
int type, newFlag = FALSE;
SYMBOL * newPtr;

if(sPtr != NIL_SYM && sPtr != NULL && !(sPtr->flags & SYMBOL_BUILTIN))
	{
	iterateSymbols(sPtr->left);
	type = symbolType(sPtr);

	/* check for default symbol */
	if(*sPtr->name == *fromContext->name && strcmp(sPtr->name, fromContext->name) == 0)
		{
		if((newPtr = lookupSymbol(newContext->name, newContext)) == NULL)
			{
			newPtr = translateCreateSymbol(newContext->name, type, newContext, TRUE);
			newFlag = TRUE;
			}
		}
	else
		{
		if((newPtr = lookupSymbol(sPtr->name, newContext)) == NULL)
			{
			newPtr = translateCreateSymbol(sPtr->name, type, newContext, TRUE);
			newFlag = TRUE;
			}
		}

	if(overWriteFlag == TRUE || newFlag == TRUE)
		{
		deleteList((CELL *)newPtr->contents);
		newPtr->contents = (UINT)copyContextCell((CELL*)sPtr->contents);
		}

	iterateSymbols(sPtr->right);
	}
}



CELL * p_new(CELL * params)
{
CELL * next;

overWriteFlag = FALSE;

params = getContext(params, &fromContext);
if(!fromContext) return(nilCell); /* for debug mode */

next = params->next;

if(params == nilCell)
	newContext = currentContext;
else 
	{
	params = evaluateExpression(params);
	if(params->type == CELL_CONTEXT || params->type == CELL_SYMBOL)
		newContext = (SYMBOL *)params->contents;
	else
		return(errorProcExt(ERR_CONTEXT_EXPECTED, params));

        overWriteFlag = (evaluateExpression(next)->type != CELL_NIL);

	/* allow symbols to be converted to contexts */
	if(symbolType(newContext) != CELL_CONTEXT)
		{
		if(isProtected(newContext->flags))
			return(errorProcExt(ERR_SYMBOL_PROTECTED, params));

		if(newContext->context != mainContext)
			return(errorProcExt2(ERR_NOT_IN_MAIN, stuffSymbol(newContext)));

		deleteList((CELL *)newContext->contents);
		makeContextFromSymbol(newContext, NULL);
		}
	}

if(newContext == mainContext)
	return(errorProc(ERR_TARGET_NO_MAIN));

iterateSymbols((SYMBOL *)((CELL*)fromContext->contents)->aux);

return(copyCell((CELL*)newContext->contents));
}


CELL * p_defineNew(CELL * params)
{
SYMBOL * sourcePtr;
SYMBOL * targetPtr;
char * name;

params = getSymbol(params, &sourcePtr);
if(params != nilCell)
	{
	params = getSymbol(params, &targetPtr);
	name = targetPtr->name;
	newContext = targetPtr->context;
	}
else
	{
	name = sourcePtr->name;
	newContext = currentContext;
	}

if(newContext == mainContext)
	return(errorProc(ERR_TARGET_NO_MAIN));

fromContext = sourcePtr->context;
targetPtr = translateCreateSymbol(name, symbolType(sourcePtr), newContext, TRUE);

deleteList((CELL *)targetPtr->contents);
targetPtr->contents = (UINT)copyContextCell((CELL*)sourcePtr->contents);

return(stuffSymbol(targetPtr));
}
	


/* ------------------------------ system ------------------------------ */

CELL * isType(CELL *, int);

CELL * p_isNil(CELL * params)
{
if(isNil(evaluateExpression(params)))
        return(trueCell);

return(nilCell);
}

CELL * p_isEmpty(CELL * params)
{
return(isEmptyFunc(evaluateExpression(params)));
}

CELL * isEmptyFunc(CELL * cell)
{
if(cell->type == CELL_STRING)
    {
    if(*(char*)cell->contents == 0)
        return(trueCell);
    else return(nilCell);
    }

if(!isList(cell->type))
		return(errorProcExt(ERR_LIST_OR_STRING_EXPECTED, cell));
if(cell->contents == (UINT)nilCell)
	return(trueCell);
return(nilCell);
}

CELL * isZero(CELL * cell)
{
#ifndef NEWLISP64
if(cell->type == CELL_INT64)
	{
	if(*(INT64 *)&cell->aux == 0)
		return(trueCell);
	else
		return(nilCell);
	}
#endif

if(cell->type == CELL_FLOAT)
	{
#ifndef NEWLISP64
	if(*(double *)&cell->aux == 0.0)
#else
    if(*(double *)&cell->contents == 0.0)
#endif
		return(trueCell);
	else
		return(nilCell);
	}	

if(cell->type == CELL_LONG)
	{
	if(cell->contents == 0)
		return(trueCell);
	}

return(nilCell);
}


CELL * p_isNull(CELL * params)
{
CELL * cell;

cell = evaluateExpression(params);
if(isNil(cell))
	return(trueCell);

if( (cell->type == CELL_STRING || isList(cell->type)))
	return(isEmptyFunc(cell));

#ifndef NEWLISP64
if(cell->type == CELL_FLOAT && (isnan(*(double *)&cell->aux)) )
#else
if(cell->type == CELL_FLOAT && (isnan(*(double *)&cell->contents)))
#endif
	return(trueCell);

return(isZero(cell));
}


CELL * p_isZero(CELL * params)
{
params = evaluateExpression(params);
return(isZero(params));
}


CELL * p_isTrue(CELL * params)
{
params = evaluateExpression(params);
if(!isNil(params) && !isEmpty(params))
        return(trueCell);

return(nilCell);
}

CELL * p_isInteger(CELL * params)
{
params = evaluateExpression(params);
if((params->type & COMPARE_TYPE_MASK) == CELL_INT)
	return(trueCell);
return(nilCell);
}


CELL * p_isFloat(CELL * params)
	{ return(isType(params, CELL_FLOAT)); }
	
CELL * p_isNumber(CELL * params)
{
params = evaluateExpression(params);
if(isNumber(params->type)) return(trueCell);
return(nilCell);
}

CELL * p_isString(CELL * params)
	{ return(isType(params, CELL_STRING)); }

CELL * p_isSymbol(CELL * params)
        { return(isType(params, CELL_SYMBOL)); }

CELL * p_isContext(CELL * params)
{
char * symStr;
SYMBOL * ctx;

/* check type */
if(params->next == nilCell) 
    return(isType(params, CELL_CONTEXT)); 

/* check for existense of symbol */
params = getContext(params, &ctx);
if(!ctx) return(nilCell); /* for debug mode */
getString(params, &symStr);

return (lookupSymbol(symStr, ctx) ? trueCell : nilCell);    
}

CELL * p_isPrimitive(CELL * params)
	{ return(isType(params, CELL_PRIMITIVE)); }

CELL * p_isAtom(CELL * params)
{
if(params == nilCell)
	return(errorProc(ERR_MISSING_ARGUMENT));
params = evaluateExpression(params);
if(params->type & ENVELOPE_TYPE_MASK) return(nilCell);
return(trueCell);
}

CELL * p_isQuote(CELL *params)
	{ return(isType(params, CELL_QUOTE)); }

CELL * p_isList(CELL * params)
	{ return(isType(params, CELL_EXPRESSION)); }

CELL * p_isLambda(CELL * params)
	{ return(isType(params, CELL_LAMBDA)); }

CELL * p_isMacro(CELL * params)
	{ return(isType(params, CELL_MACRO)); }

CELL * p_isArray(CELL * params)
	{ return(isType(params, CELL_ARRAY)); }

CELL * isType(CELL * params, int operand)
{
CELL * contextCell;

if(params == nilCell)
	return(errorProc(ERR_MISSING_ARGUMENT));
params = evaluateExpression(params);
if((UINT)operand == params->type) return(trueCell);
switch(operand)
	{
	case CELL_PRIMITIVE:
		if(params->type == CELL_IMPORT_CDECL
#ifdef WINCC
		|| params->type == CELL_IMPORT_DLL 
#endif
		)
			return(trueCell);
		break;
	case CELL_EXPRESSION:
		if(isList(params->type)) return(trueCell);
                break;
	case CELL_SYMBOL:
		if(params->type == CELL_DYN_SYMBOL) /* check if already created */
			{
			contextCell = (CELL *)((SYMBOL *)params->aux)->contents;
			if(contextCell->type != CELL_CONTEXT)
				fatalError(ERR_CONTEXT_EXPECTED, 
					stuffSymbol((SYMBOL*)params->aux), TRUE);
			if(lookupSymbol((char *)params->contents, (SYMBOL*)contextCell->contents))
				return(trueCell);
			}
			
		break;
	default:
		break;
	}

return(nilCell);
}


CELL * p_isLegal(CELL * params)
{
char * symStr;

getString(params, &symStr);

if(isLegalSymbol(symStr)) return(trueCell);

return(nilCell);
}


int isLegalSymbol(char * source)
{
STREAM stream;
char token[MAX_SYMBOL + 1];
int tklen;

if(*source == (char)'"' || *source == (char)'{' || *source == (char)'[' 
   || (unsigned char)*source <= (unsigned char)' ' || *source == (char)';' || *source == (char)'#')
        return(0);

makeStreamFromString(&stream, source);

return( (getToken(&stream, token, &tklen) == TKN_SYMBOL) && tklen == strlen(source));
}


CELL * p_exit(CELL * params)
{
UINT result;

if(demonMode) 
	{
	fclose(IOchannel);
#ifndef WINCC
	IOchannel = NULL;
#endif
	longjmp(errorJump, ERR_USER_RESET);
	}

if(params != nilCell) getInteger(params, (UINT*)&result);
else result = 0;
exit(result);
return(trueCell);
}



CELL * p_reset(CELL * params)
{
#ifndef LIBRARY
#ifndef WINCC
if (getFlag(params))
	execv(MainArgs[0], MainArgs);
#endif
#endif

longjmp(errorJump, ERR_USER_RESET);
return(nilCell);
}


CELL * p_errorEvent(CELL * params)
{
CELL * symCell;

if(params != nilCell) getCreateSymbol(params, &errorEvent, "$error-event");
symCell = getCell(CELL_SYMBOL);
symCell->contents = (UINT)errorEvent;
return(symCell);
}

#ifndef WINCC

CELL * p_timerEvent(CELL * params)
{
CELL * symCell;
double seconds;
UINT timerOption = 0;
struct itimerval timerVal;
struct itimerval outVal;
static double duration;

if(params != nilCell) 
  {
  params = getCreateSymbol(params, &timerEvent, "$timer");

  if(params != nilCell)
    {
    params = getFloat(params, &seconds);
    duration = seconds;
    if(params != nilCell)
        getInteger(params, (UINT*)&timerOption);
    memset(&timerVal, 0, sizeof(timerVal));
    timerVal.it_value.tv_sec = seconds;
    timerVal.it_value.tv_usec = (seconds - timerVal.it_value.tv_sec) * 1000000;
    if(setitimer((int)timerOption, &timerVal, &outVal) == -1)
      return(nilCell);
    return(stuffInteger(0));
    }
  else
    getitimer(timerOption, &outVal);

  seconds = duration - (outVal.it_value.tv_sec + outVal.it_value.tv_usec / 1000000.0);
  return(stuffFloat(&seconds));
  }
  
symCell = getCell(CELL_SYMBOL);
symCell->contents = (UINT)timerEvent;
return(symCell);
}
#endif

CELL * p_signal(CELL * params)
{
CELL * symCell;
SYMBOL * signalEvent;
UINT sig;
char sigStr[12];

params = getInteger(params, (UINT *)&sig);
if(sig > 32 || sig < 1) return(nilCell);
    
if(params != nilCell)
      {
      if(params->contents == (UINT)nilSymbol)
          signalEvent = nilSymbol;
      else
          {
	      snprintf(sigStr, 11, "$signal-%ld", sig);
          getCreateSymbol(params, &signalEvent, sigStr);
          }
      symHandler[sig - 1] = signalEvent;
      if(signal(sig, signal_handler) == SIG_ERR)
          return(nilCell);
      }
  
symCell = getCell(CELL_SYMBOL);
symCell->contents = (UINT)symHandler[sig - 1];
return(symCell);
}


CELL * p_errorNumber(CELL * params)
{
return(stuffInteger((UINT)errorReg));
}


CELL * p_errorText(CELL * params)
{
UINT errorNumber = errorReg;

if(params == nilCell)
	{
	if(errorStream.buffer != NULL)
		return(stuffString(errorStream.buffer));
	}
else
	getInteger(params, &errorNumber);


if(errorNumber > MAX_ERROR_NUMBER)
	errorNumber = ERR_NUMBER_OUT_OF_RANGE;

return(stuffString(errorMessage[errorNumber]));
}



CELL * p_dump(CELL * params)
{
CELL * blockPtr;
int i;
CELL * cell;

if(params != nilCell)
	{
	cell = evaluateExpression(params);
	return(stuffIntegerList
           (5, cell, cell->type, cell->next, cell->aux, cell->contents));
	}

blockPtr = cellMemory;
while(blockPtr != NULL)
	{
	for(i = 0; i <  MAX_BLOCK; i++)
		{
		if(*(UINT *)blockPtr != CELL_FREE)
			{
			varPrintf(OUT_DEVICE, "address=%lX type=%d contents=", blockPtr, blockPtr->type);
			printCell(blockPtr, TRUE, OUT_DEVICE);
			varPrintf(OUT_DEVICE,LINE_FEED);
			}
		++blockPtr;
		}
	blockPtr = blockPtr->next;
	}
return(trueCell);
}


CELL * p_mainArgs(CELL * params)
{
CELL * cell;
ssize_t idx;

cell = (CELL*)mainArgsSymbol->contents;
if(params != nilCell)
    {
    getInteger(params, (UINT *)&idx);
    cell = (CELL *)cell->contents;
    if(idx < 0) idx = convertNegativeOffset(idx, (CELL *)cell);
    while(idx--) cell = cell->next;
    }

pushResultFlag = FALSE;
return(cell);
}


CELL * p_context(CELL * params)
{
SYMBOL * sPtr;
SYMBOL * cPtr;
char * newSymStr;

if(params->type == CELL_NIL)
	return(copyCell((CELL *)currentContext->contents));

if((cPtr = getCreateContext(params, TRUE)) == NULL)
    return(errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, params));
    
if(params->next == nilCell)
    {
    currentContext = cPtr;
    return(copyCell( (CELL *)currentContext->contents));
    }
    
/* create symbol from string contents for context */
params = getString(params->next, &newSymStr);
sPtr = translateCreateSymbol(newSymStr, CELL_NIL, cPtr, TRUE);
if(params == nilCell)
	{
	pushResultFlag = FALSE;
    return(CELL *)sPtr->contents;
    }

if(strcmp(cPtr->name, sPtr->name) == 0)
	return(nilCell);

return(setDefine(sPtr, params, TYPE_SET));
}


SYMBOL * getCreateContext(CELL * cell, int evaluate)
{
SYMBOL * contextSymbol;

if(evaluate)
	cell = evaluateExpression(cell);

if(cell->type == CELL_SYMBOL || cell->type == CELL_CONTEXT)
    contextSymbol = (SYMBOL *)cell->contents;
else
    return(NULL);


if(symbolType(contextSymbol) != CELL_CONTEXT)
	{
	if(isProtected(contextSymbol->flags))
		return(NULL);

	if(contextSymbol->context != mainContext)
		{
		contextSymbol= translateCreateSymbol(
			contextSymbol->name, CELL_CONTEXT, mainContext, 1);
		}

	if(symbolType(contextSymbol) != CELL_CONTEXT)
		{
        if(isProtected(contextSymbol->flags))
        	errorProcExt(ERR_CONTEXT_EXPECTED, stuffSymbol(contextSymbol));

		deleteList((CELL *)contextSymbol->contents);
		makeContextFromSymbol(contextSymbol, NULL);
		}
	}

/* if this is a context var retrieve the real context symbol */
return((SYMBOL *)((CELL *)contextSymbol->contents)->contents);
}


CELL * p_default(CELL * params)
{
SYMBOL * contextSymbol;

getContext(params, &contextSymbol);

return(stuffSymbol(translateCreateSymbol(contextSymbol->name, CELL_NIL, contextSymbol, TRUE)));
}


CELL * p_systemSymbol(CELL * params)
{
UINT idx;

getInteger(params, &idx);

if(idx > 15 || idx < 0) return(nilCell);

return(copyCell((CELL*)sysSymbol[idx]->contents));
}


/* end of file */
