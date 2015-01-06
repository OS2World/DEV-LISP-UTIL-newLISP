/*
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
#ifdef SUPPORT_UTF8
#include <wctype.h>
#endif
#include "pcre.h"
#include "protos.h"

extern SYMBOL * sysSymbol[];
#define OVECCOUNT (MAX_REGEX_EXP * 3)    /*  max sub expressions in PCRE */

void regexError(char * msg_1, int num, const char * msg_2);
pcre * pcreCachedCompile(char * pattern, int options);

CELL * cellTokenString(char * *source, size_t * srclen, char *separator, pcre *re);

/* ---------------------------- string processing ----------------------- */

int my_strnicmp(char * s1, char * s2, ssize_t size)
{
while(toupper(*s1) == toupper(*s2))
	{
	if(--size <= 0) return(0);
	s1++, s2++;
	}
return(toupper(*s1) - toupper(*s2));
}


CELL * substring(char * string, ssize_t slen, ssize_t offset, ssize_t len)
{
if(offset < 0)
	{
	offset = slen + offset;
	if(offset < 0) offset = 0;
	}
else
	offset = (offset > slen) ? slen : offset;
	

if(len < 0) len = slen;
len = ((offset + len) > slen) ? (slen - offset) : len;

return(stuffStringN(string + offset, len));
}

#define SEARCH_SIZE 0X4000

CELL * p_search(CELL * params)
{
UINT fileHandle;
ssize_t bytesRead;
ssize_t position;

#ifdef LFS
off_t filePosition;
off_t foundPosition;
double result;
#else
ssize_t filePosition;
ssize_t foundPosition;
#endif

char * searchString;
char * buffer;
long options;
size_t len;

params = getInteger(params, (UINT *)&fileHandle);
params = getStringSize(params, &searchString, &len, TRUE);
if(len == 0) return(nilCell);

options = -1;
if(params != nilCell)
	{
	getInteger(params, (UINT *)&options);        
	}

buffer = (char *)allocMemory(SEARCH_SIZE + 1);
if((filePosition = lseek((int)fileHandle, 0, SEEK_CUR)) == -1) 
	return(nilCell);

foundPosition = 0;

do
    {
    memset(buffer, 0, SEARCH_SIZE + 1);
    bytesRead = read((int)fileHandle, buffer, SEARCH_SIZE);

    if(options == -1)
        position = searchBuffer(buffer, bytesRead, searchString, len, 1);
    else
        position = searchBufferRegex(buffer, 0, searchString, (int)bytesRead , options, NULL);

    if(position != -1)
        {
        foundPosition = filePosition + position;
        break;
        }

    filePosition = filePosition + bytesRead - len;
    lseek((int)fileHandle, filePosition, SEEK_SET);
    } while (bytesRead == SEARCH_SIZE);

freeMemory(buffer);

if(position == -1) return(nilCell);

lseek((int)fileHandle, foundPosition, SEEK_SET);
#ifdef LFS
result = foundPosition;
return(stuffFloat(&result));
#else
return(stuffInteger(foundPosition));
#endif
}



CELL * implicitIndexString(CELL * cell, CELL * params)
{
ssize_t index;
char * ptr;
#ifndef SUPPORT_UTF8
char str[2];
#else
ssize_t i, p;
#endif

ptr = (char*)cell->contents;
getInteger(params, (UINT *)&index);

#ifndef SUPPORT_UTF8
index = adjustNegativeIndex(index, cell->aux - 1);
str[0] = *(ptr + index);
str[1] = 0;
return(stuffString(str));
#else
index = adjustNegativeIndex(index, utf8_wlen(ptr));
for(i = 0; i < index; i++)
	{
	p = utf8_1st_len(ptr);
	ptr += p;
	}
return(stuffStringN(ptr, utf8_1st_len(ptr)));
#endif
}


CELL * p_char(CELL * params)
{
char * string;
ssize_t offset;
size_t len;
int  num;
CELL * datCell;
char buff[2];


datCell = evaluateExpression(params);
switch(datCell->type)
	{
	case CELL_STRING:
		string = (char *)datCell->contents;

#ifndef SUPPORT_UTF8
		len = (size_t)datCell->aux - 1;
#else
		len = utf8_wlen(string);
#endif

		if(params->next != nilCell)
			getInteger(params->next, (UINT*)&offset);
		else offset = 0;

		offset = adjustNegativeIndex(offset, len);

#ifndef SUPPORT_UTF8
		return(stuffInteger((UINT)*((unsigned char *)string + (UINT)offset)));

	case CELL_LONG:
		buff[0] = (int)datCell->contents;
		break;
#ifndef NEWLISP64
	case CELL_INT64:
		num = *(INT64 *)&datCell->aux;
		buff[0] = num;
		break;
	case CELL_FLOAT:
		num = *(double*)&datCell->aux;
		buff[0] = num;
#else /* NEWLISP64 */
	case CELL_FLOAT:
		num = *(double*)&datCell->contents;
		buff[0] = num;
#endif /* NEWLISP64 */
		break;


#else /* SUPPORT_UTF8 */
		while(offset--) string += utf8_1st_len(string);
		utf8_wchar(string, &num);	
		return(stuffInteger(num));

#ifndef NEWLISP64
	case CELL_FLOAT:
	case CELL_LONG:
	case CELL_INT64:
		string = allocMemory(UTF8_MAX_BYTES + 1);	
		if(datCell->type == CELL_FLOAT)
			num = *(double*)&datCell->aux;
		else if(datCell->type == CELL_INT64)
			num = *(INT64 *)&datCell->aux;
		else
			num = datCell->contents;
#else /* NEWLISP64 */
	case CELL_FLOAT:
	case CELL_LONG:
		string = allocMemory(UTF8_MAX_BYTES + 1);	
		if(datCell->type == CELL_FLOAT)
			num = *(double*)&datCell->contents;
		else
			num = (int)datCell->contents;
#endif /* NEWLISP 64 */
		len = wchar_utf8(num, string);		
		datCell = stuffStringN(string, len);
		free(string);
		return(datCell);
#endif /* SUPPORT_UTF8 */
	default:
		buff[0] = 0;
	}

return(stuffStringN(buff, 1));
}


CELL * p_explode(CELL * params)
{
char * string;
CELL * cell;
CELL * result;
ssize_t size;
ssize_t len = 1;
int flag = 0;
#ifdef SUPPORT_UTF8
int clen, i;
#endif

cell = evaluateExpression(params);
if(isList(cell->type))
	return(explodeList((CELL*)cell->contents, params->next));

getStringSize(cell, &string, (size_t *)&size, TRUE);
if(params->next != nilCell)
	{
	params = getInteger(params->next, (UINT *)&len);
	flag = getFlag(params);
	}

result = cell = getCell(CELL_EXPRESSION);
if(size == 0 || len <= 0) return(result);

#ifndef SUPPORT_UTF8
if(flag && size < len) return(result);

cell->contents = (UINT)stuffStringN(string, len);
cell = (CELL *)cell->contents;
string += len;

while((size -= len) > 0)
	{
	if(flag && size < len) break;
	cell->next = stuffStringN(string, (size >= len) ? len : size);
        cell = cell->next;
	string += len;
	}
#else
size = utf8_wlen(string);
for(i = 0, clen = 0; i < len; i++)
	clen += utf8_1st_len(string + clen);

if(flag && size < len) return(result);

cell->contents = (UINT)stuffStringN(string, clen);
cell = (CELL *)cell->contents;
string += clen;
while((size -= len) > 0)
	{
	if(flag && size < len) break;
	for(i = 0, clen = 0; i < len; i++)
		clen += utf8_1st_len(string + clen);
	cell->next = stuffStringN(string, clen);
	cell = cell->next;
	string += clen;
	}
#endif

return(result);
}


#define STR_LOWER 0
#define STR_UPPER 1
#define STR_TITLE 2


CELL * strUpperLower(CELL * params, int type)
{
#ifndef SUPPORT_UTF8
char * string;
char * ptr;
#else
char * utf8str;
int * unicode;
int * ptr;
size_t size;
#endif
CELL * cell;
int option = FALSE;

#ifndef SUPPORT_UTF8
params = getString(params, &string);
if(params != nilCell)
  {
  params = evaluateExpression(params);
  if(params->contents != (UINT)nilCell)
    option = TRUE;
  }

cell = stuffString(string);
ptr = (void *)cell->contents;

if(type == STR_UPPER)
    while(*ptr) { *ptr = toupper(*ptr); ptr++; }
else
    {
    if(type == STR_TITLE)
      if(*ptr) { *ptr = toupper(*ptr); ptr++; }
    if(((type == STR_TITLE) && option) || (type == STR_LOWER))
      while(*ptr) { *ptr = tolower(*ptr); ptr++; }
    }
#else

params = getString(params, &utf8str);
option = getFlag(params);

size =  utf8_wlen(utf8str);

unicode = allocMemory((size + 1) * sizeof(int));
size = utf8_wstr(unicode, utf8str, size);

ptr = unicode;

/* Note that on many platforms towupper/towlower
do not work correctly for non-ascii unicodes */

if(type == STR_UPPER)
    while(*ptr) { *ptr = towupper(*ptr); ptr++; }
else
    {
    if(type == STR_TITLE)
      if(*ptr) { *ptr = towupper(*ptr); ptr++; }
    if(((type == STR_TITLE) && option) || (type == STR_LOWER))
      while(*ptr) { *ptr = towlower(*ptr); ptr++; }
    }

utf8str = allocMemory(size * UTF8_MAX_BYTES + 1);
size =  wstr_utf8(utf8str, unicode, size * UTF8_MAX_BYTES + 1);
utf8str = reallocMemory(utf8str, size + 1);

free(unicode);
cell = getCell(CELL_STRING);
cell->contents = (UINT)utf8str;
cell->aux = size + 1;
#endif

return(cell);
}

CELL * p_upper(CELL * params) {return(strUpperLower(params, STR_UPPER));}
CELL * p_lower(CELL * params) {return(strUpperLower(params, STR_LOWER));}
CELL * p_title(CELL * params) {return(strUpperLower(params, STR_TITLE));}

char * getFormatType(char * fmt, int * type)
{
PARSE_FORMAT:

/* get % */
while(*fmt != '%' && *fmt != 0) fmt++;
if(*fmt == 0) 
	{
    *type = 0;
	return(fmt);
    }
fmt++;

/* is it double %% for '%' */
if(*fmt == '%')
    {
    fmt++;
    goto PARSE_FORMAT;
    }

/* get width spec */

/* force + before numbers */
if(*fmt == '+') 
  	fmt++;
else
    {
    /* left align numbers or strings */
    if(*fmt == '-')
        {
        fmt++;
        /* force + before numbers */
        if(*fmt == '+') fmt++;
        }
    }

while(isdigit((int)*fmt) && *fmt != 0) fmt++;
if(*fmt == 0) return(NULL);

/* its a float or string with dot precision*/
if(*fmt == '.')
    {
    fmt++;
    while(isdigit((int)*fmt) && *fmt !=0) fmt++;
    if(*fmt == 0) return(NULL);

	if(*fmt == 'f' || *fmt == 'g' || *fmt =='G' || *fmt == 'e' || *fmt == 'E')
        {
        *type = CELL_FLOAT;
        return(++fmt);
        }
    if(*fmt == 's')
        {
        *type = CELL_STRING;
        return(++fmt);
        }
    else return(NULL);
    }

/* its a float without dot */
if(*fmt == 'f' || *fmt == 'g' || *fmt =='G' || *fmt == 'e' || *fmt == 'E')
    {
    *type = CELL_FLOAT;
    return(++fmt);
    }

/* its an integer or character */
#ifdef TRU64
if(*fmt == 'd' || *fmt == 'i' || *fmt == 'u' || *fmt == 'x' || *fmt == 'X' || *fmt == 'c' || *fmt == 'o')
#else
if(*fmt == 'd' || *fmt == 'u' || *fmt == 'x' || *fmt == 'X' || *fmt == 'c' || *fmt == 'o')
#endif
    {
#ifdef TRU64
#ifndef NEWLISP64
    *type = CELL_INT64;
#else
	*type = CELL_LONG;
#endif
#else
	*type = CELL_LONG;
#endif
    return(++fmt);
    }

/* its a string */
if(*fmt == 's')
    {
    *type = CELL_STRING;
    return(++fmt);
    }

#ifndef WINCC
#ifdef TRU64 /* supporting ld, li, lu, lx, lX formats */
if(*fmt == 'l' &&  
		(*(fmt + 1) == 'd' || *(fmt + 1) == 'i' || *(fmt + 1) == 'u' || *(fmt + 1) =='x' || *(fmt + 1) == 'X'))
    {
#ifndef NEWLISP64
    *type = CELL_INT64;
#else
    *type = CELL_LONG;
#endif
    return(fmt+2);
    }
#else /* all other UNIX suporting lld, llu, llx, llX formats */
if(*fmt == 'l' && *(fmt + 1) == 'l' && 
		(*(fmt + 2) == 'd' || *(fmt + 2) == 'u' || *(fmt + 2) =='x' || *(fmt + 2) == 'X'))
    {
#ifndef NEWLISP64
    *type = CELL_INT64;
#else
	*type = CELL_LONG;
#endif
    return(fmt+3);
    }
#endif
#else /* MinGW uses MS conventions */
if(memcmp(fmt, "I64", 3) == 0 &&
		(*(fmt + 3) == 'd' || *(fmt + 3) == 'u' || *(fmt + 3) =='x' || *(fmt + 3) == 'X'))
    {
    *type = CELL_INT64;
    return(fmt+4);
    }
#endif

/* L and q seem not to be suported on most GCC although in the docs
if(*fmt == 'L' && (*(fmt + 1) == 'd' || *(fmt + 1) =='x' || *(fmt + 1) == 'X'))
    {
    *type = CELL_INT64;
    return(fmt+3);
    }
*/

/* its a wchar_t unicode string */
if(*fmt == 'l' && *(fmt + 1) == 's')
    {
    *type = CELL_STRING;
    return(fmt+2);
    }

return(NULL);
}

CELL * p_format(CELL * params)
{
char * format;
char * fmt;
char * nextfmt;
CELL * cell;
STREAM fmtStream;
int fType;
double floatNum;
UINT intNum;
#ifndef NEWLISP64
INT64 bigNum;
#endif
int evalFlag = TRUE;
char saveChar;

params = getString(params, &format);
fmt = format;

openStrStream(&fmtStream, MAX_STRING, 0);

while(params->type != CELL_NIL)
    {
    nextfmt = getFormatType(fmt, &fType);

    if(nextfmt == NULL)
		{
		closeStrStream(&fmtStream);
        return(errorProcExt2(ERR_FORMAT_STRING, stuffString(format)));
		}

	saveChar = *nextfmt;
	*nextfmt = 0;

	if(evalFlag) 
		cell = evaluateExpression(params);
	else
		cell = params;

	if(cell->type == CELL_EXPRESSION && fmt == format)
		{
		params = (CELL *)cell->contents;
		evalFlag = FALSE;
		cell = params;
		}


    if(fType == 0)
		{
		*nextfmt = saveChar;
		closeStrStream(&fmtStream);
        return(errorProcExt(ERR_FORMAT_NUM_ARGS, params));
		}

    if(fType == CELL_LONG)
        {
        if(isNumber(cell->type))
			cell = getIntegerExt(cell, &intNum, FALSE);
		else goto FORMAT_DATA_ERROR;

		varPrintf((UINT)&fmtStream, fmt, intNum);
		goto NEXT_FORMAT;
		}
#ifndef NEWLISP64
    if(fType == CELL_INT64)
        {
        if(isNumber(cell->type))
			cell = getInteger64(cell, &bigNum);
		else goto FORMAT_DATA_ERROR;

		varPrintf((UINT)&fmtStream, fmt, bigNum);
		goto NEXT_FORMAT;
		}
#endif
    if(fType == CELL_FLOAT)
		{
		if(cell->type == CELL_FLOAT)
#ifndef NEWLISP64
			floatNum = *(double *)&cell->aux;
		else if(cell->type == CELL_INT64)
			floatNum = *(INT64 *)&cell->aux;
#else
			floatNum = *(double *)&cell->contents;
#endif
		else if(cell->type == CELL_LONG)
			floatNum = (long)cell->contents;
		else goto FORMAT_DATA_ERROR;

		varPrintf((UINT)&fmtStream, fmt, floatNum);
		goto NEXT_FORMAT;
		}

    if(fType != cell->type)
		goto FORMAT_DATA_ERROR;

	varPrintf((UINT)&fmtStream, fmt, cell->contents);

	NEXT_FORMAT:
	*nextfmt = saveChar;
	fmt = nextfmt;
    params = params->next;
	continue;


	FORMAT_DATA_ERROR:
	*nextfmt = saveChar;
	closeStrStream(&fmtStream);
    return(errorProcExt(ERR_FORMAT_DATA_TYPE, params));
    }

getFormatType(fmt, &fType);
if(fType != 0)
	{
	closeStrStream(&fmtStream);
    errorProcExt2(ERR_FORMAT_NUM_ARGS, stuffString(format));
	}

writeStreamStr(&fmtStream, fmt, 0);

cell = getCell(CELL_STRING);
cell->aux = fmtStream.position + 1;
cell->contents = (UINT)fmtStream.buffer;

return(cell);
}

void openStrStream(STREAM * stream, int buffSize, int reopenFlag)
	{
	if(stream->buffer != NULL && reopenFlag)
		freeMemory(stream->buffer);
	stream->buffer = stream->ptr = callocMemory(buffSize + 1);
	stream->size = buffSize;	
	stream->position = stream->handle = 0;
	}

void closeStrStream(STREAM * stream)
	{
	if(stream->buffer != NULL)
		freeMemory(stream->buffer);
	stream->buffer = stream->ptr = NULL;
	stream->size = stream->position = 0;
	if(stream->handle != 0)
            {
            close((int)stream->handle);
            stream->handle = 0;
            }
	}
	
#define NEW_WRITE_STREAM

void writeStreamChar(STREAM * stream, char chr)
{
if(stream->position == stream->size)
	{
	stream->size += stream->size / 2;
#ifdef NEW_WRITE_STREAM
	stream->buffer = reallocMemory(stream->buffer, stream->size + 1);
	memset(stream->buffer + stream->position, 0, stream->size - stream->position + 1);
	stream->ptr = stream->buffer + stream->position;
#else 
	stream->ptr = callocMemory(stream->size + 1);
	memcpy(stream->ptr, stream->buffer, stream->position);
	free(stream->buffer);
	stream->buffer = stream->ptr;
	stream->ptr += stream->position;
#endif
	}
*(stream->ptr++) = chr;
stream->position++;
}


void writeStreamStr(STREAM * stream, char * buff, size_t length)
{
size_t newPosition;

if(length == 0) length = strlen(buff);
newPosition = stream->position + length;

if(newPosition >=  stream->size)
	{
	while(newPosition >= stream->size)
		stream->size += stream->size / 2;
#ifdef NEW_WRITE_STREAM
	stream->buffer = reallocMemory(stream->buffer, stream->size + 1);
	memset(stream->buffer + stream->position, 0, stream->size - stream->position + 1);
	stream->ptr = stream->buffer + stream->position;
#else 
	stream->ptr = callocMemory(stream->size + 1);
	memcpy(stream->ptr, stream->buffer, stream->position);
	free(stream->buffer);
	stream->buffer = stream->ptr;
	stream->ptr += stream->position;
#endif
	}
	
memcpy(stream->ptr, buff, length);
stream->ptr += length;
stream->position = newPosition;
}


/* creates a memory buffer and reads byte into it until  
   the limiter is found 
*/

char * readStreamText(STREAM * stream, char * limit)
{
STREAM outStream;
ssize_t findPos = -1;
size_t searchLen;
size_t llen = strlen(limit);
char * result;

memset(&outStream, 0, sizeof(STREAM));
openStrStream(&outStream, MAX_STRING, 0);
while(findPos == -1)
    {
    if((searchLen = strlen(stream->ptr)) < llen)
		break;
    findPos = searchBuffer(stream->ptr, searchLen, limit, llen, TRUE);
    if(findPos != -1)
        {
        writeStreamStr(&outStream, stream->ptr, findPos);
        stream->ptr += findPos + llen;
        result = allocMemory(outStream.position + 1);
		memcpy(result, outStream.buffer, outStream.position);
		*(result + outStream.position) = 0; 
        closeStrStream(&outStream);
        return(result);
        }

    writeStreamStr(&outStream, stream->ptr, searchLen - llen);

    /* adjustment for the first time, after it will be 0 always */
    stream->position += (stream->ptr - stream->buffer); 

    stream->position += searchLen - llen;

    if(stream->handle == 0) /* its not a file */
        {
        stream->buffer = stream->ptr;
        break;
        }
    else
        {
        lseek(stream->handle, stream->position, SEEK_SET);
        memset(stream->buffer, 0, stream->size + 1);
        if(read(stream->handle, stream->buffer, stream->size) > 0)
            stream->ptr = stream->buffer;
        else 
            {
            *stream->ptr = 0; 
            break;
            }
        }
    }

closeStrStream(&outStream);

return(NULL);
}


/* this is only used for reading with getToken()
   stream->size does not reflect the real size of
   the buffer as in makeStreamFromFile() 
*/
void makeStreamFromString(STREAM * stream, char * str)
{
memset(stream, 0, sizeof(STREAM));
stream->buffer = stream->ptr = str;
/* make getToken work to the end of str */
stream->size = strlen(str) + 4 * MAX_STRING;
}

int makeStreamFromFile(STREAM * stream, char * fileName, size_t size, size_t offset)
{
if((stream->handle = open(fileName, O_RDONLY | O_BINARY)) == -1)
	return(0);

stream->ptr = stream->buffer = (char *)callocMemory(size + 1);
if(offset != 0)
	lseek((int)stream->handle, offset, SEEK_SET);

stream->position = offset;
stream->size = size;

/* load first buffer */
if(read(stream->handle, stream->buffer, size) <= 0)
	{
	closeStrStream(stream);
	return(0);
	}

return(TRUE);
}


ssize_t searchBuffer(char * buffer,  size_t length, char * string, size_t size, int caseFlag)
{
size_t position = 0;

if(caseFlag == FALSE)
	{
	while(position < length)
		{
		if(toupper(*buffer) == toupper(*string))
			if(my_strnicmp(buffer, string, size) == 0) break;
		position++;
		buffer++;
		}
	}
else /* case sensitive */
	{
	while(position < length)
		{
		if(*buffer == *string)
			if(memcmp(buffer, string, size) == 0) break;
		position++;
		buffer++;
		}
	}

if(position == length) return(-1);
return(position);
}

/* eliminated in v.8.4.1
CELL * patternMatchS(char * pattern, char * string)
{
CELL * cell;
CELL * match;
CELL * star, * plus;
char * start;
int starLen, len;

start = string; starLen = len = 0;
MATCH:
switch(*pattern)
	{
	case '\\':
		++pattern;
		if(*pattern != *string) return(nilCell);
		break;
	case 0:
		if(*string != 0) return(nilCell);
		return(stuffStringN(start, len));
	case '?':
		if(*string == 0) return(nilCell);
		break;
	case '#':
		if(*string == 0) return(nilCell);
		if(!isDigit((unsigned char)*string)) return(nilCell);
		break;

	case '+':	
		if(*(pattern + 1) == 0)
			{
			if(*string == 0)
				plus = stuffString("");
			else if(*(string + 1) == 0)
				plus = stuffStringN(string, 1);
			else return(nilCell);

			if(len == 0) return(plus);
			cell = stuffStringN(start, len);
			cell->next = plus;
			return(cell);
			}
			
		if((match = patternMatchS(pattern+1, string)) != nilCell)
			plus = stuffString("");
		else if((match = patternMatchS(pattern+1, string+1)) != nilCell)
			plus = stuffStringN(string, 1);
		else return(nilCell);
			
		if(len == 0) cell = plus;
		else
			{
			cell = stuffStringN(start, len);
			cell->next = plus;
			}
		plus->next = match;
		return(cell);

	case '*':
		if(*(pattern + 1) == 0)
			{
			if(len == 0) return(stuffString(string));
			cell = stuffStringN(start, len);
			cell->next = stuffString(string);
			return(cell);
			}

		if((match = patternMatchS(pattern+1, string)) != nilCell)
			{
			star = stuffStringN(start+len, starLen);
			if(len == 0) cell = star;
			else
				{
				cell = stuffStringN(start, len);
				cell->next = star;
				}
			star->next = match;
			return(cell);
			}

		if(*string != 0) ++string, ++starLen;
		else return(nilCell);
		goto MATCH;

	default:
		if(*pattern != *string) return(nilCell);
		break;
	}
++pattern;
++string;
++len;
goto MATCH;
}
*/


CELL * p_integer(CELL * params)
{
char * intString;
INT64 num;
long base;
CELL * deflt;
INT64 result;

deflt = params->next;
params = evaluateExpression(params);

if(params->type == CELL_STRING)
	intString = (char *)params->contents;
else if(isNumber(params->type))
	{
	getInteger64(params, &num);
	return(stuffInteger64(num));
	}
else 
	return(copyCell(evaluateExpression(deflt)));

while(isspace((int)*intString)) intString++;
if(!isDigit((unsigned char)*intString))
	{
	if(*intString != '-' && *intString != '+')
		return(copyCell(evaluateExpression(deflt)));
	if(!isDigit((unsigned char)*(intString+1)))
		return(copyCell(evaluateExpression(deflt)));
	}

if(deflt->next != nilCell)
	getInteger(deflt->next, (UINT *)&base);
else base = 0;
#ifdef TRU64 
result = strtoul(intString, NULL, base); 
#else 
result = strtoull(intString,(char **)0, base); 
#endif 
return(stuffInteger64(result)); 
}

CELL * p_float(CELL * params)
{
char * fltString;
double value;
CELL * deflt;

deflt = params->next;
params = evaluateExpression(params);

if(params->type == CELL_STRING)
	fltString = (char *)params->contents;
else if(isNumber(params->type))
	{
	getFloat(params, &value);
	return(stuffFloat(&value));
	}
else
	return(copyCell(evaluateExpression(deflt)));

while(isspace((int)*fltString)) fltString++;
if(!isDigit((unsigned char)*fltString))
	{
	if(*fltString != '-' && *fltString != '+' && *fltString != lc_decimal_point)
		return(copyCell(evaluateExpression(deflt)));
	if(!isDigit((unsigned char)*(fltString+1)))
		return(copyCell(evaluateExpression(deflt)));
	}

value = atof(fltString);
return( stuffFloat(&value) );
}


CELL * p_symbol(CELL * params)
{
char * token;
char number[32];
SYMBOL * context;
SYMBOL * sPtr;
CELL * cell;
#ifdef WINCC
char * fmt = "%I64d";
#endif


cell = evaluateExpression(params);
switch(cell->type)
	{
	case CELL_LONG:
		snprintf(number, 30, "%ld", cell->contents);
		token = number;
		break;
#ifndef NEWLISP64
	case CELL_INT64:
#ifdef TRU64
        snprintf(number, 30, "%ld", *(INT64 *)&cell->aux); 
#else

#ifdef WINCC
        snprintf(number, 30, fmt, *(INT64 *)&cell->aux);
#else
        snprintf(number, 30, "%lld", *(INT64 *)&cell->aux);
#endif /* WINCC */

#endif /* TRUE64 */
		token = number;
		break;
#endif /* NEWLISP64 */
	case CELL_FLOAT:
#ifndef NEWLISP64
		snprintf(number, 30, "%1.10g",*(double *)&cell->aux);
#else
		snprintf(number, 30, "%1.10g",*(double *)&cell->contents);
#endif
		token = number;
		break;
	case CELL_STRING:
		token = (char*)cell->contents;
		break;
	case CELL_SYMBOL:
		sPtr = (SYMBOL*)cell->contents;
		token = sPtr->name;
		break;
	default:
		return(errorProcExt(ERR_NUMBER_OR_STRING_EXPECTED, params));
	}
params = params->next;
	
if(params == nilCell)
	context = currentContext;
else if((context = getCreateContext(params, TRUE)) == NULL)
	return(errorProcExt(ERR_SYMBOL_OR_CONTEXT_EXPECTED, params));

if(params->next != nilCell)
	{
	cell = evaluateExpression(params->next);
        if(cell->type == CELL_NIL)
		{
		sPtr = lookupSymbol(token, context);
		if(sPtr == NULL) return(nilCell);
		return(stuffSymbol(sPtr));
		}
	}

sPtr = translateCreateSymbol(token, CELL_NIL, context, TRUE);
return(stuffSymbol(sPtr));
}

	

CELL * p_symbolSource(CELL * params)
{
STREAM strStream;
CELL * cell;

openStrStream(&strStream, MAX_STRING, 0);
serializeSymbols(params, (UINT)&strStream);
cell = stuffString(strStream.buffer);
closeStrStream(&strStream);
return(cell);
}


CELL * p_string(CELL * params)
{
CELL * cell;
STREAM strStream;

openStrStream(&strStream, MAX_STRING, 0);
prettyPrintFlags |= PRETTYPRINT_STRING;
while (params != nilCell)
	{
	cell = evaluateExpression(params);
	if(cell->type == CELL_STRING) /* speed optimization for strings */
	    writeStreamStr(&strStream, (char *)cell->contents, 0);
	else
	    printCell(cell , FALSE, (UINT)&strStream);                                                    
    params = params->next;
	}
prettyPrintFlags &= ~PRETTYPRINT_STRING;
cell = stuffString(strStream.buffer);

closeStrStream(&strStream);
return(cell);
}


UINT getAddress(CELL * params)
{
UINT num;

params = evaluateExpression(params);

#ifndef NEWLISP64
if(params->type == CELL_INT64)
	{
	num = *(INT64 *)&params->aux;
	return(num);
	}
else if(params->type == CELL_FLOAT)
	{
	num = *(INT64 *)&params->aux;
	return(num);
	}
#else
if(params->type == CELL_FLOAT)
	{
	num = *(double *)&params->contents;
	return(num);
	}
#endif

return(params->contents);

}

CELL * p_getChar(CELL * params)
{
return(stuffInteger((UINT)*(char*)getAddress(params)));
}

CELL * p_getString(CELL * params)
{
return(stuffString((char *)getAddress(params)));
}

CELL * p_getInteger(CELL * params)
{
return(stuffInteger(*(unsigned int *)getAddress(params)));
}

CELL * p_getLong(CELL * params)
{
#ifndef NEWLISP64
return(stuffInteger64(*(INT64 *)getAddress(params)));
#else
return(stuffInteger(*(UINT *)getAddress(params)));
#endif
}

CELL * p_getFloat(CELL * params)
{
return(stuffFloat((double*)getAddress(params)));
}


CELL * p_address(CELL * params)
{
params = evaluateExpression(params);
switch(params->type)
    {
    case CELL_LONG:
        return(stuffInteger((UINT)&params->contents));
#ifndef NEWLISP64
    case CELL_INT64:
    case CELL_FLOAT:
        return(stuffInteger((UINT)&params->aux));
#else
	case CELL_FLOAT:
		return(stuffInteger((UINT)&params->contents));
#endif
    default:
        break;
    }
return(stuffInteger(params->contents));
}


CELL * p_copyMemory(CELL * params)
{
UINT toAddress, fromAddress, nBytes;
CELL * cell;

cell = evaluateExpression(params);
params = params->next;
fromAddress = getAddress(cell);

cell = evaluateExpression(params);
toAddress = getAddress(cell);

getInteger(params->next, &nBytes);

memcpy((char*)toAddress, (char*)fromAddress, nBytes);

return(stuffInteger(nBytes));
}


CELL * cellTokenString(char * * source, size_t * srclen, char * separator, pcre * re)
{
char * start;
char * src;
ssize_t len, slen;
int ovector[OVECCOUNT];
int rc;

if(*source == NULL) return(stuffString(""));
start = src = *source;
len = 0;
slen = strlen(separator);

if(re == NULL)
    {
    while(*src != 0)
	{
        if(*src == *separator)
            {
            if(strncmp(src, separator, slen) == 0)
                {
                *source = src+slen;
                if(**source == 0) /* last token is separator */
                     *source = NULL;
                return(stuffStringN(start, len));
                }
            }
	src++;
	len++;
        }
    *source = src;
    if(len == 0) return(NULL);
    return(stuffStringN(start, len));
    }
else  if(*src != 0)
    {
    len = *srclen;
    rc = pcre_exec(re, NULL, src, len, 0, 0, ovector, OVECCOUNT);

    /* matching failed */
    if (rc == -1)  
        {
        *source = src + len;
        return(stuffStringN(src, len));
        }

    /* error in pcre_exec() */
    if (rc < 0) 
        regexError("error in pcre_exec()", rc, "");

    *source = src + ovector[1];
    *srclen -= ovector[1];
    if(**source == 0) /* last token is separator */
        *source = NULL;

    if(ovector[1] - ovector[0] == 0)
        return(NULL);

    return(stuffStringN(src, ovector[0]));
    }

return(NULL);
}


CELL * p_parse(CELL * params)
{
CELL * cell;
CELL * newCell;
CELL * result;
char * string;
char * separator;
char token[MAX_STRING];
STREAM stream;
int tklen;
size_t srclen;
/* PCRE stuff */
long options = 0;
pcre *re = NULL;

params = getStringSize(params, &string, &srclen, TRUE);
if(params == nilCell) separator = NULL;
else 
    {
    params = getString(params, &separator);
    if(params != nilCell) 
        {
        getInteger(params, (UINT *)&options);
        /* Compile the regular expression in separator */
        re = pcreCachedCompile(separator, options); 
        }
    else re = NULL;
    }

if(separator == NULL) 
    makeStreamFromString(&stream, string);

result = cell = getCell(CELL_EXPRESSION);
while(string != NULL)
	{
	if(separator != NULL)
		{
		if((newCell = cellTokenString(&string, &srclen, separator, re)) == NULL)
			break;
		}
	else
		{
		if(getToken(&stream, token, &tklen) == TKN_EMPTY) break;
		newCell = stuffString(token);
		}

	if(cell == result)
		result->contents = (UINT)newCell;
	else 
		cell->next = newCell;

	cell = newCell;
        if(string == NULL) 
            newCell->next = stuffString("");
	}

return(result);
}

#define PACK_NONE 0
#define PACK_CHAR 1
#define PACK_BYTE 2
#define PACK_INT 3
#define PACK_UNSIGNED_INT 4
#define PACK_LONG 5
#define PACK_UNSIGNED_LONG 6
#define PACK_LONG_LONG 7
#define PACK_UNSIGNED_LONG_LONG 8
#define PACK_FLOAT 9
#define PACK_DOUBLE 10
#define PACK_STRING 11
#define PACK_NULL 12
#define PACK_BIG_ENDIAN 13
#define PACK_LITTLE_ENDIAN 14


void swapEndian(char * data, int n)
{
char tmp[8];
int i;

i = n;
while(i) { tmp[n - i] = data[i - 1]; i--; }
   
memcpy(data, &tmp[0], n);
}


CELL * p_pack(CELL * params)
{
char * format;
char * source;
char * packed;
char * pPtr;
CELL * cell;
CELL * next;
ssize_t length, ln;
int len, type;
int bigEndian = 1, endianSwitch = 0;
char chrV;
unsigned char byteV;
short int shortV;
unsigned short int uint16V;  /* 16 bit */
unsigned int uint32V;         /* 32 bit */
unsigned long long  uint64V; /* 64 bit */
float floatV;
double doubleV;

/* find out endianess */
bigEndian = (*((char *)&bigEndian) == 0) ;

params = getString(params, &format);
source = format;
length = 0;
while((source = parsePackFormat(source, &len, &type)) != NULL)
	length += len;

if(length == 0) return(stuffString(""));
pPtr = packed = allocMemory(length);
source = format;
length = 0;

while((source = parsePackFormat(source, &len, &type)) != NULL)
	{
	if(type == PACK_NULL)
		{
		memset(pPtr, 0, len);
		pPtr += len;
		length += len;
		continue;
		}
		
	else if(type == PACK_LITTLE_ENDIAN || type == PACK_BIG_ENDIAN)
		{
		endianSwitch = ((type == PACK_BIG_ENDIAN) != bigEndian);
		continue;
   		}

	if(params->type == CELL_NIL) break;
	next = params->next;
	cell = evaluateExpression(params);

#ifndef NEWLISP64
	if(cell->type == CELL_FLOAT || cell->type == CELL_INT64)
		uint64V = *(INT64 *)&cell->aux;
	else /* CELL_LONG and CELL_STRING */
		uint64V = cell->contents;
#else
	uint64V = cell->contents;
#endif
	
	switch(type)
		{
		case PACK_NONE:
			break;

		case PACK_BYTE:
			byteV = (char)uint64V;
			memcpy(pPtr, &byteV, 1);
			break;

		case PACK_CHAR:
			chrV = (char)uint64V;
			memcpy(pPtr, &chrV, 1);
			break;

		case PACK_INT:
			shortV = (short int)uint64V;
			memcpy(pPtr, &shortV, 2);
			if(endianSwitch) swapEndian(pPtr, 2);
			break;

		case PACK_UNSIGNED_INT:
			uint16V = (unsigned short int)uint64V;
			memcpy(pPtr, &uint16V, 2);
			if(endianSwitch) swapEndian(pPtr, 2);
			break;

		case PACK_LONG:
		case PACK_UNSIGNED_LONG:
			uint32V = (unsigned int)uint64V;
			memcpy(pPtr, &uint32V, 4);
			if(endianSwitch) swapEndian(pPtr, 4);
			break;

		case PACK_LONG_LONG:
		case PACK_UNSIGNED_LONG_LONG:
			memcpy(pPtr, &uint64V, 8);
			if(endianSwitch) swapEndian(pPtr, 8);
			break;

		case PACK_FLOAT:
		case PACK_DOUBLE:
			if(cell->type == CELL_FLOAT)
				doubleV = *(double *)&uint64V;
			else doubleV = (double)uint64V;
			if(type == PACK_FLOAT)
				{
				floatV = doubleV;
				memcpy(pPtr, &floatV, 4);
				if(endianSwitch) swapEndian(pPtr, 4);
				}
			else
				{
				memcpy(pPtr, &doubleV, 8);
				if(endianSwitch) swapEndian(pPtr, 8);
				}
			break;
			
		case PACK_STRING:
			if(cell->type == CELL_STRING)
				{
				ln = cell->aux - 1;
				if(len <= ln)
					memcpy(pPtr, (void *)cell->contents, len);
				else
					{
					memcpy(pPtr, (void*)cell->contents, ln);
					memset(pPtr + ln, 0, len - ln);
					}
				}
			else memset(pPtr,  0, len);
			break;

		default:
			break;
		}

	pPtr += len;
	params = next;
	length += len;
	}

cell = stuffStringN(packed, length);
free(packed);
return(cell);
}


char * parsePackFormat(char * format, int * length, int * type)
{
*length = 0;
while(*format == ' ') format++;

if(*format == 0) return(NULL);

switch(*format)
	{
	case '<':
		*type = PACK_LITTLE_ENDIAN;
		format++;
		break;
	        
	case '>':
		*type = PACK_BIG_ENDIAN;
		format++;
		break;
                
	case 'b':
		*length = 1;	
		*type = PACK_BYTE;
		format++;
		break;

	case 'c':
		*length = 1;	
		*type = PACK_CHAR;
		format++;
		break;
	case 's':
	case 'n':
		*type = (*format == 's') ? PACK_STRING : PACK_NULL;
		format++;
		if(isdigit((int)*format) )
			{
			*length = atol(format);
			while(isdigit((int)*format)) format++;
			}
		else *length = 1;
		
		break;
	case 'd':
	case 'u':
		*type = (*format == 'd') ? PACK_INT : PACK_UNSIGNED_INT;
		*length = 2;
		format++;
		break;
	case 'l':
		if(*(format + 1) == 'd' || *(format + 1) == 'u')
			{
			*length = 4;
			*type = (*(format + 1) == 'd') ? PACK_LONG : PACK_UNSIGNED_LONG;
			format += 2;
			}
		else if(*(format + 1) == 'f')
			{
			*length = 8;
			*type = PACK_DOUBLE;
			format += 2;
			}
		else 
			{
			*type = PACK_NONE;
			format++;
			}
		break;
	case 'L':
		if(*(format + 1) == 'd' || *(format + 1) == 'u')
			{
			*length = 8;
			*type = (*(format + 1) == 'd') ? PACK_LONG_LONG : PACK_UNSIGNED_LONG_LONG;
			format += 2;
			}
		else 
			{
			*type = PACK_NONE;
			format++;
			}
		break;
	case 'f':
		*length = 4;
		*type = PACK_FLOAT;
		format++;
		break;
	default:
		*type = PACK_NONE;
		format++;
		break;
	}

return(format);
}


CELL * p_unpack(CELL * params)
{
char * format;
char * source;
char * pPtr;
CELL * cell;
CELL * result;
CELL * next;
size_t length, maxlen;
int len;
int type;
int bigEndian = 1, endianSwitch = 0;
char chrV;
unsigned char byteV;
short shortV;
unsigned short uint16V;
int int32V;
unsigned int uint32V;
unsigned long long int uint64V;
float floatV;
double doubleV;

/* find out endianess */
bigEndian = (*((char *)&bigEndian) == 0) ;

params = getString(params, &format);
params = evaluateExpression(params);

if(params->type == CELL_STRING)
    {
    pPtr = (char *)params->contents; 
    maxlen = params->aux - 1;
    }
else
    {
    getIntegerExt(params, (void*)&pPtr, FALSE);
    maxlen = MAX_LONG;
    }
    
length = 0;
source = format;
result = getCell(CELL_EXPRESSION);
next = NULL;

while( (source = parsePackFormat(source, &len, &type))!= NULL)
	{
	if(length + len > maxlen) break;

	if(type == PACK_LITTLE_ENDIAN || type == PACK_BIG_ENDIAN)
		{
		endianSwitch = ((type == PACK_BIG_ENDIAN) != bigEndian);
		continue;
		}
                
	switch(type)
		{		
		case PACK_NULL:
			pPtr += len;
			length += len;
			continue;
	
		case PACK_BYTE:
			memcpy(&byteV, pPtr, 1);
			cell = getCell(CELL_LONG);
			cell->contents = byteV;
			break;

		case PACK_CHAR:
			memcpy(&chrV, pPtr, 1);
			cell = getCell(CELL_LONG);
			cell->contents = chrV;
			break;

		case PACK_INT:
			memcpy(&shortV, pPtr, 2);
		        if(endianSwitch) swapEndian((char*)&shortV, 2);
			cell = getCell(CELL_LONG);
			cell->contents = (int)shortV;
			break;

		case PACK_UNSIGNED_INT:
			memcpy(&uint16V, pPtr, 2);
		        if(endianSwitch) swapEndian((char*)&uint16V, 2);
			cell = getCell(CELL_LONG);
			cell->contents = uint16V;
			break;

		case PACK_LONG:
			memcpy(&int32V, pPtr, 4);
			if(endianSwitch) swapEndian((char *)&int32V, 4);
#ifndef NEWLISP64
			cell = getCell(CELL_INT64);
			*(INT64 *)&cell->aux = int32V;
#else
			cell = getCell(CELL_LONG);
			*(long *)&cell->contents = int32V;
#endif
			break;

		case PACK_UNSIGNED_LONG:
			memcpy(&uint32V, pPtr, 4);
			if(endianSwitch) swapEndian((char*)&uint32V, 4);
#ifndef NEWLISP64
			cell = getCell(CELL_INT64);
			*(INT64 *)&cell->aux = uint32V;
#else
			cell = getCell(CELL_LONG);
			cell->contents = uint32V;
#endif
			break;

		case PACK_LONG_LONG:
		case PACK_UNSIGNED_LONG_LONG:
			memcpy(&uint64V, pPtr, 8);
		        if(endianSwitch) swapEndian((char*)&uint64V, 8);
#ifndef NEWLISP64
			cell = getCell(CELL_INT64);
			memcpy(&cell->aux, &uint64V, 8);
#else
			cell = getCell(CELL_LONG);
			cell->contents = uint64V;
#endif
			break;

		case PACK_FLOAT:
			memcpy(&floatV, pPtr, 4);
		        if(endianSwitch) swapEndian((char*)&floatV, 4);
			doubleV = floatV;
			cell = stuffFloat(&doubleV);
			break;

		case PACK_DOUBLE:
			memcpy(&doubleV, pPtr, 8);
			if(endianSwitch) swapEndian((char*)&doubleV, 8);
			cell = stuffFloat(&doubleV);
			break;
			
		case PACK_STRING:
			cell = stuffStringN(pPtr, len);
			break;

		default:
			cell = getCell(CELL_NIL);
			break;
		}

	pPtr += len; length += len;
	if(next == NULL)
		result->contents = (UINT)cell;
	else	
		next->next = cell;
	
	next = cell;
	}

return(result);
}


CELL * p_trim(CELL * params)
{
char * str;
size_t left, right, len;
char * trimChr;
#ifndef SUPPORT_UTF8
char * ptr;
char lchr, rchr;
#else
int * wstr;
int * wptr;
int lchr, rchr;
CELL * result;
#endif

params = getString(params, &str);

#ifndef SUPPORT_UTF8
len = strlen(str);
ptr = str;
#else
len = utf8_wlen(str);
wptr = wstr = allocMemory((len + 1) * sizeof(int));
len = utf8_wstr(wstr, str, len);
#endif

if(len == 0)
	return(stuffString(str));

if(params == nilCell)	
	lchr = rchr = 32;
else 
	{
	params = getString(params, &trimChr);
#ifndef SUPPORT_UTF8
	lchr = *trimChr;
#else
	utf8_wchar(trimChr, &lchr);
#endif
	if(params != nilCell)
		{
		getString(params, &trimChr);
#ifndef SUPPORT_UTF8
		rchr = *trimChr;
#else
		utf8_wchar(trimChr, &rchr);
#endif
		}		
	else rchr = lchr;
	}

left = right = 0;
#ifndef SUPPORT_UTF8
while(*str == lchr) str++, left++;
#else
while(*wstr == lchr) wstr++, left++;
#endif

if(left == len)
	return(stuffString(""));

#ifndef SUPPORT_UTF8
str = ptr + len - 1;
while(*str == rchr && right < len) str--, ++right;

return(stuffStringN(ptr + left, len - left - right));

#else

wstr = wptr + len - 1;
while(*wstr == rchr && right < len) wstr--, ++right;

*(wptr + len - right) = 0;

str = allocMemory((len - left - right + 1) * UTF8_MAX_BYTES);
len = wstr_utf8(str, wptr + left, (len - left - right) * UTF8_MAX_BYTES);
free(wptr);
result = stuffStringN(str, len);
free(str);
return(result);
#endif

}

/* ------- find-all ---- finds all strings matching a pattern in a list ----- */

CELL * p_findAll(CELL * params)
{
char * pattern;
char * str;
long options = 0;
ssize_t findPos = -1;
size_t size;
int len;
int offset = 0;
CELL * result = nilCell;
CELL * cell = NULL;
CELL * exprCell;
CELL * exprRes;
int errNo;

params = getString(params, &pattern);
params = getStringSize(params, &str, &size, TRUE);
exprCell = params;
params = params->next;
if(params != nilCell)
	getInteger(params, (UINT *)&options);
	
while( (findPos = searchBufferRegex(str, offset, pattern, (int)size, options, &len)) != -1)
	{
	if(exprCell != nilCell)
		{
		if((exprRes = evaluateExpressionSafe(exprCell, &errNo)) == NULL)
        	{
			pushResult(result); /* push for later deletion */
       		longjmp(errorJump, errNo);
       		}
		exprRes = copyCell(exprRes);
		}
	else
		exprRes = stuffStringN(str + findPos, len);

	if(findPos == offset && len == 0)
		return(nilCell);
		
	if(result == nilCell)
		{
		result = getCell(CELL_EXPRESSION);
		cell = exprRes;
		result->contents = (UINT)cell;
		}
	else
		{
		cell->next = exprRes;
		cell = cell->next;
		}

	offset = (findPos + len);
	}

return(result);
}



/* ------- PRCE Perl Compatible Regular Expressions for all platforms ------- */
/*                    see also http://www.pcre.org/                           */


void regexError(char * msg_1, int num, const char * msg_2);



CELL * p_regex(CELL * params)
{
pcre *re;
int ovector[OVECCOUNT];
int rc, idx;
char * pattern;
char * string;
long options = 0;
int len;
CELL * cell, * result, * strCell;

cell = getString(params, &pattern);
strCell = evaluateExpression(cell);
if(strCell->type != CELL_STRING)
	return(errorProcExt(ERR_STRING_EXPECTED, cell));

string = (char *)strCell->contents;

params = cell->next;
if(params != nilCell)
    params = getInteger(params, (UINT *)&options);

/* Compile the regular expression in the first argument */
re = pcreCachedCompile(pattern, (int)options);

/* Compilation succeeded: match the subject in the second argument */
rc = pcre_exec(
    re,                    /* the compiled pattern */
    NULL,                  /* no extra data - we didn't study the pattern */
    string,                /* the subject string */
    (int)strCell->aux - 1, /* the length of the subject */
    0,                     /* start at offset 0 in the subject */
    0,                     /* default options */
    ovector,               /* output vector for substring information */
    OVECCOUNT);            /* number of elements in the output vector */

/* Matching failed */
if (rc == -1)
    return(nilCell);

/* error in pcre_exec() */
if (rc < 0) 
    regexError("error in pcre_exec()", rc, "");

/* Match succeded */

/* Show substrings stored in the output vector */
result = cell = getCell(CELL_EXPRESSION);
for(idx = 0; idx < rc; idx++)
    {
    len = ovector[2*idx+1] - ovector[2*idx];
    strCell = stuffStringN(string + ovector[2*idx], len);

    deleteList((CELL*)sysSymbol[idx]->contents);
    sysSymbol[idx]->contents = (UINT)copyCell(strCell);

    if(idx == 0)
        {
        cell->contents = (UINT)strCell;
        cell = (CELL *)cell->contents;
        }
    else	
        {
        cell->next = strCell;
        cell = cell->next;
        }

    cell->next = stuffInteger(ovector[2*idx]);
    cell = cell->next;
    cell->next = stuffInteger(len);
    cell = cell->next;
    }

return(result);
}


ssize_t searchBufferRegex(char * string,  int offset, 
                          char * pattern, int length, int options, int * len)
{
pcre *re;
int ovector[OVECCOUNT];
int rc, idx;
CELL * cell;

options &= ~REPLACE_ONCE; /* turn custom bit off for PCRE */

/* Compile the regular expression in the first argument */
re = pcreCachedCompile(pattern, options);

/* Compilation succeeded: match the subject in the second argument */
rc = pcre_exec(re, NULL, string, length, offset, 0, ovector, OVECCOUNT);

/* matching failed */
if (rc == -1)  
    return(-1);

/* error in pcre_exec() */
if (rc < 0) 
    regexError("error in pcre_exec()", rc, "");


for(idx = 0; idx < rc; idx++)
    {
    cell = stuffStringN(string  + ovector[2*idx], ovector[2*idx+1] - ovector[2*idx]);
    deleteList((CELL*)sysSymbol[idx]->contents);
    sysSymbol[idx]->contents = (UINT)cell;
    }

if(len != NULL)
    *len = ovector[1] - ovector[0];

return((UINT)ovector[0]);
}


pcre * pcreCachedCompile(char * pattern, int options)
{
const char * error;
int errOffset;
static char * cPattern = NULL;
static pcre * re = NULL;
static int cacheOptions = -1;
UINT len;

if((cPattern == NULL) || (strcmp(cPattern, pattern) != 0) || (options != cacheOptions))
    {
    cacheOptions = options;
    if(cPattern != NULL) freeMemory(cPattern);
    len = strlen(pattern);
    cPattern = (char *)allocMemory(len + 1);
    memcpy(cPattern, pattern, len + 1);
#ifdef MINGW
    if(re != NULL) free(re);
#else
    if(re != NULL) (pcre_free)(re);
#endif
    re = pcre_compile(pattern, options, &error, &errOffset, NULL); 

    /* Compilation failed: print the error message and exit */
    if (re == NULL) 
        {
        freeMemory(cPattern);
        cPattern = NULL;
        regexError("offset", errOffset, error);
        }
    } 

return(re);
}
    

void regexError(char * msg_1, int number, const char * msg_2)
{
CELL * cell;
char * errorBuff = malloc(256);
snprintf(errorBuff, 256,  "%s %d %s:", msg_1, number, msg_2);
cell = stuffString(errorBuff);
free(errorBuff);
fatalError(ERR_REGEX, cell, 0);
}



/* replace string with or without (options = -1) regular expressions */

typedef struct {
    int offset;
    int length;
    char * repStr;
    int repLen;
    void * next;
    } REGEX;

void freeRegex(REGEX * regex);

char * replaceString
   (char * keyStr, int keyLen, char * buff, size_t buffLen, CELL * exprCell, 
    UINT * cnt, int options, size_t * newLen)
{
ssize_t oldLen, findPos;
size_t count, offset;
size_t repLen;
char * newBuff;
CELL * cell;
REGEX * start_rx = NULL, * end_rx = NULL;
int resultStackIdxSave;
int errNo;
int bias = 0;

*newLen = oldLen = 0;
*cnt = 0;
offset = count = 0;

/* save all found string and fill sys variables $0, $1 ... etc */
while(offset <= buffLen)
    {
    if(options == -1)
        findPos = searchBuffer(buff + offset, buffLen - offset, keyStr, keyLen, TRUE);
    else
        findPos = searchBufferRegex(buff, (int)offset, keyStr, (int)buffLen, options, &keyLen);

    if(findPos == -1) break;
    
    if(options != -1) findPos -= offset;

    if(count == 0)
        start_rx = end_rx = (REGEX *)callocMemory(sizeof(REGEX));
    else
        {
        end_rx->next = callocMemory(sizeof(REGEX));
        end_rx = end_rx->next;
        }

    end_rx->next = NULL;
    end_rx->offset = findPos + offset; /* pos where pattern found */

    resultStackIdxSave = resultStackIdx; 

	if((cell = evaluateExpressionSafe(exprCell, &errNo)) == NULL)
		{
		freeRegex(start_rx);
		longjmp(errorJump, errNo);
		}	

    end_rx->length = keyLen;           /* length of pattern found */
    if(cell->type == CELL_STRING)
        {
        repLen = end_rx->repLen = cell->aux-1;
        end_rx->repStr = (char *)allocMemory(repLen + 1); /* replacement string */
        memcpy(end_rx->repStr, (char *)cell->contents, cell->aux); 
        }
    else /* if replacement expression is not string leave old content */
        {
        repLen = end_rx->repLen = keyLen;
        end_rx->repStr = (char *)callocMemory(repLen + 1);
        memcpy(end_rx->repStr, buff + offset + findPos, keyLen);
        }

    cleanupResults(resultStackIdxSave); 

    count++;
    offset += findPos + keyLen; /* next start for search */
    oldLen += keyLen; /* space occupied by old content */
    *newLen += repLen; /* space needed by new replacement content */

    bias = (keyLen == 0);
    offset += bias;

    if(options != -1)
	if(options & REPLACE_ONCE) break; 
    }

if(count == 0) return(NULL);

*newLen = buffLen - oldLen + *newLen;
newBuff = callocMemory(*newLen + 1);

end_rx = start_rx;

/* count is now offset into the new buffer */
*cnt = count;
count = start_rx->offset;
memcpy(newBuff, buff, count);  

while(start_rx != NULL) /* replace */
    {
    repLen = start_rx->repLen;
    memcpy(newBuff + count, start_rx->repStr, repLen);
    count += repLen;
    freeMemory(start_rx->repStr);

    end_rx = start_rx->next;
    if(end_rx != NULL) /* copy from buffer */
      {
      repLen =  end_rx->offset - start_rx->offset - start_rx->length;
      memcpy(newBuff + count, buff + start_rx->offset + start_rx->length, repLen);
      count += repLen;
      }                   
    
    freeMemory(start_rx);
    start_rx = end_rx;
    }
/*
printf("count %d buffLen %d offset %d bias %d str %s lencpy %d\n",  
        count, buffLen, offset, bias, buff + offset - bias, buffLen - offset + bias);
*/
memcpy(newBuff + count, buff + offset - bias, buffLen - offset + bias);
return(newBuff);
}


void freeRegex(REGEX * regex)
{
REGEX * oldRegex;

while(regex != NULL)
	{
    if(regex->repStr != NULL)
		freeMemory(regex->repStr);
    oldRegex = regex;
	regex = regex->next;
	free(oldRegex);
	}
}

/* eof */


