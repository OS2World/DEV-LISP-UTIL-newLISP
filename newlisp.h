/* newlisp.h - header file for newLISP
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

#ifdef TRU64
#define strtoll strtol
#define strtoull strtoul
#endif

/* 
This is for 64bit large file support (LFS),
*/
#define LFS

#ifdef LFS
#ifdef SOLARIS
#define _LARGEFILE64_SOURCE 1
#endif
#define _FILE_OFFSET_BITS 64
#endif

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <setjmp.h>
#include <stdarg.h>
#include <dirent.h>
#include <limits.h>

/* some Linux do UTF-8 but do not have wcsftime() 
   buggy in some GCC, i.e. MinGW and Solaris
*/

#ifdef SUPPORT_UTF8
#ifdef LINUX
#include <wchar.h>
#define WCSFTIME
#endif
#endif

#ifdef MINGW
#include <windef.h>
#include <winbase.h>
#else
#include <termios.h>
#include <sys/wait.h>
#endif

#include <unistd.h>

#ifndef WINCE /* add again in nl-filesys.c */
#include <sys/time.h>
#endif

#include <math.h>
#include <float.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>

#ifndef SOLARIS
#ifndef _BSD
#ifndef MAC_OSX
#include <malloc.h>
#endif
#endif
#else
#include <alloca.h>
#endif


#ifdef CYGWIN
#define WIN_32
#endif

#ifdef MINGW
#define WINCC
#define isSocketStream(A) ((A)->_flag == 0xFFFF)
#define getSocket(A) ((A)->_file)
#endif 

#ifdef WINCE
#define WINCC
#define isSocketStream(A) ((A)->_flags == 0xFF)
#define getSocket(A) ((A)->_file)
#endif

#ifdef OS2
#define vasprintf my_vasprintf 
#define MY_VASPRINTF 
#endif

#ifdef MAC_OSX
#define vasprintf my_vasprintf
#define MY_VASPRINTF
#endif

#ifdef SOLARIS
#define vasprintf my_vasprintf
#define setenv my_setenv
#define MY_VASPRINTF
#define MY_SETENV
#endif

#ifdef WINCC
#define WIN_32
#define vasprintf my_vasprintf
#define setenv my_setenv
#define random rand
#define srandom srand
#define MY_VASPRINTF
#define MY_SETENV
#define ioctl ioctlsocket
#define LINE_FEED "\r\n"
#define LITTLE_ENDIAN

#define off_t off64_t
#define lseek lseek64
#define ftell ftello64

#else /* not WINCC */
#define LINE_FEED "\n"
#define NET_PING
#endif

#ifndef WIN_32
#define NANOSLEEP
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define UTF8_MAX_BYTES 6

#define UINT unsigned long 

#define INT16 short int
#ifndef NEWLISP64
#define MAX_LONG 0x7FFFFFFF
#else
#define MAX_LONG 0x7FFFFFFFFFFFFFFFLL
#endif

#ifndef NEWLISP64
#ifdef TRU64
#define INT64 long
#define UINT64 unsigned long
#else
#define INT64 long long int
#define UINT64 unsigned long long int
#endif
#else
#define INT64 long
#define UINT64 unsigned long
#endif

#define pushEnvironment(A) (*(envStack + envStackIdx++) = (UINT)(A))
#define popEnvironment() (*(envStack + --envStackIdx))

#define pushResult(A) (*(resultStack + resultStackIdx++) = (UINT)(A))
#define popResult() ((CELL *)*(resultStack + --resultStackIdx))

#define freeMemory free

#define INT32_MIN_AS_INT64 (((long long int)0xFFFFFFFF << 32) | 0x80000000)
#define MY_INT64_MAX (((long long int)0x7FFFFFFF << 32) | 0xFFFFFFFF)
#define TRUE 1
#define FALSE 0
  
#define MAX_STRING 2048
#define MAX_LINE 255
#define MAX_SYMBOL 255

#define MAX_READ_LEN 0x4000
#define MAX_PRINT_LEN 0x4000
#define MAX_LOAD_BUFFER 0x4000
#define MAX_FILE_BUFFER 0x4000
#define MAX_BLOCK 999
#define MAX_URL_LEN 256

#define MAX_REGEX_EXP 16

/* token types */
#define TKN_ERROR -1
#define TKN_EMPTY 0
#define TKN_CHARACTER 1
#define TKN_HEX 2
#define TKN_OCTAL 3
#define TKN_DECIMAL 4
#define TKN_FLOAT 5
#define TKN_STRING 6
#define TKN_SYMBOL 7
#define TKN_CONTEXT 8
#define TKN_LEFT_PAR '('
#define TKN_RIGHT_PAR ')'
#define TKN_QUOTE '\''

/* symbol flags types and masks */
#define PRINT_TYPE_MASK 0x0F
#define SYMBOL_PROTECTED 0x10
#define SYMBOL_GLOBAL 0x20
#define SYMBOL_BUILTIN 0x40
#define SYMBOL_PRIMITIVE 0x80

/* cell masks */
#define RAW_TYPE_MASK 0x0FFF
#define COMPARE_TYPE_MASK 0x000F
#define ENVELOPE_TYPE_MASK 0x0010
#define LIST_TYPE_MASK 0x0020
#define SYMBOL_TYPE_MASK 0x0040
#define NUMBER_TYPE_MASK 0x0080
#define EVAL_SELF_TYPE_MASK 0x0100
#define INT64_MASK 0x0200

/* cell types */
#define CELL_NIL (0 | EVAL_SELF_TYPE_MASK)
#define CELL_TRUE (1 | EVAL_SELF_TYPE_MASK)
#define CELL_INT 2 /* any INT */
#define CELL_LONG (2 | EVAL_SELF_TYPE_MASK | NUMBER_TYPE_MASK)
#define CELL_INT64 (2 | EVAL_SELF_TYPE_MASK | NUMBER_TYPE_MASK | INT64_MASK)
#define CELL_FLOAT (3 | EVAL_SELF_TYPE_MASK | NUMBER_TYPE_MASK)
#define CELL_STRING (4 | EVAL_SELF_TYPE_MASK)
#define CELL_SYMBOL (5 | SYMBOL_TYPE_MASK)
#define CELL_CONTEXT 6
#define CELL_PRIMITIVE (7 | EVAL_SELF_TYPE_MASK)
#define CELL_IMPORT_CDECL (8 | EVAL_SELF_TYPE_MASK)
#define CELL_IMPORT_DLL (9 | EVAL_SELF_TYPE_MASK)
#define CELL_QUOTE (10 | ENVELOPE_TYPE_MASK)
#define CELL_EXPRESSION (11 | ENVELOPE_TYPE_MASK | LIST_TYPE_MASK)
#define CELL_LAMBDA (12 | ENVELOPE_TYPE_MASK | LIST_TYPE_MASK | EVAL_SELF_TYPE_MASK)
#define CELL_MACRO (13 | ENVELOPE_TYPE_MASK | LIST_TYPE_MASK | EVAL_SELF_TYPE_MASK)
#define CELL_ARRAY (14 | ENVELOPE_TYPE_MASK | EVAL_SELF_TYPE_MASK)
#define CELL_DYN_SYMBOL (15 | SYMBOL_TYPE_MASK)
#define CELL_FREE 0xFF


#define isEnvelope(A) ((A) & ENVELOPE_TYPE_MASK)
#define isList(A) ((A) & LIST_TYPE_MASK)
#define isArray(A) ((A) == CELL_ARRAY)
#define isString(A) ((A) == CELL_STRING)
#define isNumber(A) ((A) & NUMBER_TYPE_MASK)
#define isSymbol(A) ((A) & SYMBOL_TYPE_MASK)
#define isSelfEval(A) ((A) & EVAL_SELF_TYPE_MASK)
#define isProtected(A) ((A) & SYMBOL_PROTECTED)
#define isGlobal(A) ((A) & SYMBOL_GLOBAL)
#define isDigit(A) isdigit((int)(A))

#define isNil(A) ((A)->type == CELL_NIL || ((A)->type == CELL_SYMBOL && (A)->contents == (UINT)nilSymbol))
#define isEmpty(A) ((A)->type == CELL_EXPRESSION && (A)->contents == (UINT)nilCell)

#define symbolType(A) ((CELL*)(A)->contents)->type

/* redefine some functions */
#ifdef NEWLISP64
#define stuffInteger64 stuffInteger
#endif


/* RED BLACK binary balanced tree: nl-symbol.c */
#define BLACK 0
#define RED 1
#define NIL_SYM &sentinel
#define LOOKUP_ONLY 0		/* symbol lookup only, if not found return NULL */
#define FORCE_CREATION 1	/* if symbol does not exist, create it */


/* traceFlag */
#define TRACE_TRUE 0x0001
#define TRACE_IN_ENTRY 0x0002
#define TRACE_IN_EXIT 0x0004
#define TRACE_IN_DEBUG 0x0008
#define TRACE_DEBUG_PENDING 0x0010
#define TRACE_DEBUG_EVAL 0x0020
#define TRACE_DEBUG_STEP 0x0040
#define TRACE_DEBUG_NEXT 0x0080
#define TRACE_SIGINT 0x1000
#define TRACE_TIMER  0x2000
#define TRACE_SIGNAL 0x4000

/* error handling */

#define ERR_NOT_ENOUGH_MEMORY 1
#define ERR_OUT_OF_ENV_STACK 2
#define ERR_OUT_OF_CALL_STACK 3
#define ERR_ACCESSING_FILE 4
#define ERR_EXPRESSION 5
#define ERR_MISSING_PAR 6
#define ERR_STRING_TOO_LONG 7
#define ERR_MISSING_ARGUMENT 8
#define ERR_NUMBER_OR_STRING_EXPECTED 9
#define ERR_NUMBER_EXPECTED 10
#define ERR_STRING_EXPECTED 11
#define ERR_SYMBOL_EXPECTED 12
#define ERR_CONTEXT_EXPECTED 13
#define ERR_SYMBOL_OR_CONTEXT_EXPECTED 14
#define ERR_LIST_EXPECTED 15
#define ERR_LIST_OR_SYMBOL_EXPECTED 16
#define ERR_LIST_OR_STRING_EXPECTED 17
#define ERR_LIST_OR_NUMBER_EXPECTED 18
#define ERR_ARRAY_EXPECTED 19
#define ERR_ARRAY_LIST_OR_STRING_EXPECTED 20
#define ERR_LAMBDA_EXPECTED 21
#define ERR_MACRO_EXPECTED 22
#define ERR_INVALID_FUNCTION 23
#define ERR_INVALID_LAMBDA 24
#define ERR_INVALID_MACRO 25
#define ERR_INVALID_LET 26
#define ERR_SAVING_FILE 27
#define ERR_MATH 28
#define ERR_NOT_MATRIX 29
#define ERR_WRONG_DIMENSIONS 30
#define ERR_SINGULAR 31
#define ERR_REGULAR_EXPRESSION 32
#define ERR_THROW_WO_CATCH 33
#define ERR_IMPORT_LIB_NOT_FOUND 34
#define ERR_IMPORT_FUNC_NOT_FOUND 35
#define ERR_SYMBOL_PROTECTED 36
#define ERR_NUMBER_OUT_OF_RANGE 37
#define ERR_REGEX 38
#define ERR_MISSING_TEXT_END 39
#define ERR_FORMAT_NUM_ARGS 40
#define ERR_FORMAT_STRING 41
#define ERR_FORMAT_DATA_TYPE 42
#define ERR_INVALID_PARAMETER_0 43
#define ERR_INVALID_PARAMETER_NAN 44
#define ERR_ILLEGAL_TYPE 45
#define ERR_NOT_IN_MAIN 46
#define ERR_NOT_CURRENT_CONTEXT 47
#define ERR_TARGET_NO_MAIN 48
#define ERR_ARRAY_INDEX_OUT_BOUNDS 49
#define ERR_NESTING_TOO_DEEP 50
#define ERR_USER_ERROR 51
#define ERR_USER_RESET 52
#define ERR_SIGINT 53
#define MAX_ERROR_NUMBER 53

/* I/O routines */

#define OUT_NULL 0
#define OUT_DEVICE 1
#define OUT_CONSOLE 2
#define OUT_LOG 3

/* HTTP in nl-web.c */
#define HTTP_GET_URL 0
#define HTTP_GET_HEAD 1
#define HTTP_PUT_URL 2
#define HTTP_PUT_APPEND_URL 3
#define HTTP_POST_URL 4
#define HTTP_DELETE_URL 5

/* additional regex option */
#define REPLACE_ONCE 0x8000

extern int vasprintf (char **, const char *, va_list);

/* ---------------------------- standard types ------------------------- */

typedef struct
	{
	int handle;
	char *ptr;
	char *buffer;
	size_t position;
	size_t size;
	} STREAM;

typedef struct tagSYMBOL
	{
	short int flags;
	short int color;
	char * name;
	UINT contents; 
	struct tagSYMBOL * context;
	struct tagSYMBOL * parent;
	struct tagSYMBOL * left;
	struct tagSYMBOL * right;   
	} SYMBOL;

typedef struct 
	{
	UINT type;
	void * next;
	UINT aux;
	UINT contents;
	} CELL;

typedef struct
	{
	char * name;
	CELL * (*function)(CELL *);
	short int prettyPrint;
	} PRIMITIVE;

/* --------------------------- globals -------------------------------- */

extern char startupDir[];
extern FILE * IOchannel;
extern int MAX_CPU_STACK;
extern long MAX_CELL_COUNT;
extern int version;
extern int opsys;
extern char ostype[];
extern size_t cellCount;
extern size_t symbolCount;
extern int recursionCount;
extern UINT printDevice;
extern UINT * resultStack;
extern UINT * envStack;
extern int resultStackIdx;
extern int envStackIdx;
extern CELL * trueCell;
extern CELL * nilCell;
extern STREAM strStream;
extern SYMBOL * nilSymbol;
extern SYMBOL * trueSymbol;
extern SYMBOL * startSymbol;
extern SYMBOL * questionSymbol;
extern SYMBOL * atSymbol;
extern SYMBOL * mainContext;
extern SYMBOL * currentContext;
extern SYMBOL * errorEvent;
extern SYMBOL sentinel;
extern int commandLineFlag;
extern int traceFlag;
extern int errorReg;
extern char * errorMessage[];
extern jmp_buf errorJump;
extern int pushResultFlag;
extern int prettyPrintFlags;
#define PRETTYPRINT_DOUBLE 1
#define PRETTYPRINT_STRING 2
extern char lc_decimal_point;
/* end of file */

