/* nl-web.c --- HTTP network protocol routines for newLISP
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
#include <errno.h>
#include "protos.h"

#ifdef WINCC
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define BUFFSIZE 10240

#ifndef WINCC
#define SOCKET_ERROR -1
#else
#define fgets win32_fgets
#define fgetc win32_fgetc
#define close closesocket
#endif

#define ERROR_BAD_URL "ERR: bad formed URL"


#define MAX_PROTOCOL 8
#define NO_FLAGS_SET 0

#ifdef WINCC
struct timezone {
       int     tz_minuteswest;
       int     tz_dsttime;
};

int gettimeofday( struct timeval *tp, struct timezone *tzp );
#endif

int parseUrl(char *url, char * protocol, char * host, int * port, char * path, int maxlen);
void parsePath(char * url, char * path, int maxlen);
size_t parseValue(char * str);
void trimTrailing(char * ptr);
CELL * base64(CELL * params, int type);

size_t Curl_base64_encode(const char *inp, size_t insize, char **outptr);
size_t Curl_base64_decode(const char *src, char *dest);


jmp_buf socketTimeoutJump;
long socketTimeout;
struct timeval socketStart;

/* socket send and receive routines with timeout */
int recvc_tm(int sock)
{
struct timeval tm;
char chr;
ssize_t bytes;

while(wait_ready(sock, 1000, 0) <= 0)
  {
  if(socketTimeout)
    {
    gettimeofday(&tm, NULL);
    if(timediff(tm, socketStart) > socketTimeout)
        longjmp(socketTimeoutJump, 1);
    }
  } 
   
bytes = recv(sock, &chr, 1, NO_FLAGS_SET);
if(bytes <= 0) return(-1);

return(chr);
}


char * recvs_tm(char * buffer, size_t size, int sock)
{
ssize_t bytesReceived = 0;
int chr;

while(bytesReceived < size)
    {
    if((chr = recvc_tm(sock)) < 0)
        {
        if(bytesReceived == 0)
            return(NULL);
        else break;
        }

    *(buffer + bytesReceived++) = chr;
    if(chr == '\n') break;
    }

*(buffer + bytesReceived) = 0;
return(buffer);
}

size_t recvsize_tm(char * buffer, size_t size, int sock)
{
ssize_t sizeRead = 0;
size_t resultSize = 0;
struct timeval tm;

wait_ready(sock, 1000, 0);
memset(buffer, 0, size);
while( (sizeRead = recv(sock, buffer + resultSize, size, NO_FLAGS_SET)) < size)
  {
  if(socketTimeout)
    {
    gettimeofday(&tm, NULL);
    if(timediff(tm, socketStart) > socketTimeout)
        longjmp(socketTimeoutJump, 1);
    }

  if(size == 0) break;
  if(sizeRead <= 0)
      {
      sizeRead = 0;
      break;
      }
  resultSize += sizeRead;
  size -= sizeRead;
  wait_ready(sock, 1000, 0);
  }

return(resultSize + sizeRead);
}

ssize_t sendf(int sock, char * format, ...)
{
char * buffer;
va_list argptr;
int result;
 
va_start(argptr,format);
/* new in 7201 , defined in nl-filesys.c if not in libc */
vasprintf(&buffer, format, argptr); 

result = send(sock, buffer, strlen(buffer), NO_FLAGS_SET);

freeMemory(buffer);
va_end(argptr);

return(result);
}



CELL * p_getUrl(CELL * params)
{
return(getPutPostDeleteUrl(NULL, params, HTTP_GET_URL));
}


CELL * p_putUrl(CELL * params)
{
return(getPutPostDeleteUrl(NULL, params, HTTP_PUT_URL));
}


CELL * p_postUrl(CELL * params)
{
return(getPutPostDeleteUrl(NULL, params, HTTP_POST_URL));
}

CELL * p_deleteUrl(CELL * params)
{
return(getPutPostDeleteUrl(NULL, params, HTTP_DELETE_URL));
}

#define BASE64_ENC 0
#define BASE64_DEC 1

CELL * p_base64Enc(CELL * params) { return(base64(params, BASE64_ENC)); }
CELL * p_base64Dec(CELL * params) { return(base64(params, BASE64_DEC)); }

CELL * base64(CELL * params, int type)
{
char * inPtr;
char * outPtr;
size_t sizein, sizeout;
CELL * strCell;

getStringSize(params, &inPtr, &sizein, TRUE);


if(type == BASE64_ENC)
	{
	if((sizeout = Curl_base64_encode(inPtr, sizein, &outPtr)) == 0)
		return(stuffString(""));
	}
else    /* BASE64_DEC */
	{
	outPtr = allocMemory((sizein * 3) / 4 + 9);
	sizeout = Curl_base64_decode(inPtr, outPtr);
	*(outPtr + sizeout) = 0;
	}
	
strCell = getCell(CELL_STRING);
strCell->contents = (UINT)outPtr;
strCell->aux = sizeout + 1;

return(strCell);
}


CELL * getPutPostDeleteUrl(char * url, CELL * params, int type)
{
char * proxyUrl, * putPostStr = NULL, *contentType;
char * protocol;
char * host;
char * pHost;
char * path;
char * customHeader = NULL;
int maxlen;
int port, pPort, sock = 0;
char * option, * method = NULL;
char buff[BUFFSIZE];
char errorTxt[96];
char * buffPtr;
char * resultPtr = NULL;
int haveContentLength = FALSE, headRequest = FALSE, listFlag = FALSE;
int chunked = FALSE;
ssize_t sizeRead = 0;
size_t resultSize = 0, fileSize = 0, size = 0;
CELL * result, * cell;
CELL * headerCell = NULL;
int ch;
int responseLoop;

/* get parameters */

if(url == NULL)
	params = getString(params, &url);

if(type == HTTP_PUT_URL || type == HTTP_PUT_APPEND_URL || type == HTTP_POST_URL)
	params = getStringSize(params, &putPostStr, &size, TRUE);

if(type == HTTP_POST_URL)
    {
    if(params->type != CELL_NIL)
		params = getString(params, &contentType);
    else 
		contentType = "application/x-www-form-urlencoded";
    }

result = evaluateExpression(params);
socketTimeout = 0;
if(isNumber(result->type))
    params = getIntegerExt(result, (UINT*)&socketTimeout, FALSE);
else if(isString(result->type))
    {
    option = (char *)result->contents;    
    if(my_strnicmp(option, "header", 6) == 0)
	headRequest = TRUE;
    if(my_strnicmp(option, "list", 5) == 0)
	listFlag = TRUE;
    if(params->next != nilCell)
        params = getInteger(params->next, (UINT*)&socketTimeout);
    }
else if(result != nilCell)
    return(errorProcExt(ERR_NUMBER_OR_STRING_EXPECTED, params));


/* if timeout is specified, custom-header can be specified too */
if(socketTimeout && params != nilCell)
        getString(params, &customHeader);

maxlen = strlen(url);
if(maxlen < MAX_URL_LEN) maxlen = MAX_URL_LEN;

protocol = alloca(8);
host = alloca(maxlen + 1);
pHost = alloca(maxlen + 1);
path = alloca(maxlen + 1);

/* parse URL for parameters */
if(parseUrl(url, protocol, host, &port, path, maxlen) == FALSE)
	return(stuffString(ERROR_BAD_URL));

proxyUrl = getenv("HTTP_PROXY");
if(proxyUrl == NULL)
	{
	strncpy(pHost, host, maxlen);
	pPort = port;
	}
else
	{
	if(parseUrl(proxyUrl, protocol, pHost, &pPort, NULL, maxlen) == FALSE)
		return(stuffString(ERROR_BAD_URL));
	}

/* start timer */
gettimeofday(&socketStart, NULL);
/* connect to host */
CONNECT_TO_HOST:
if(sock) 
    close(sock);

if((sock = netConnect(pHost, pPort, SOCK_STREAM, NULL, 3)) == SOCKET_ERROR)
	return(stuffString("ERR: could not connect"));

if(type == HTTP_GET_URL)
	method = (headRequest == TRUE) ? "HEAD" : "GET";
else if(type == HTTP_PUT_URL || type == HTTP_PUT_APPEND_URL)
	method = "PUT";
else if(type == HTTP_POST_URL)
	method = "POST";
else if(type == HTTP_DELETE_URL)
	method = "DELETE";

/* send header */
if(proxyUrl != NULL)
	sendf(sock, "%s %s://%s:%d/%s HTTP/1.1\r\n", method, protocol, host, port, path);
else	
	sendf(sock, "%s /%s HTTP/1.1\r\n", method, path);

/* obligatory host spec */
sendf(sock, "Host: %s\r\n", host);

/* send optional custom header entries */
if (customHeader != NULL)
    sendf(sock, customHeader);
else
    sendf(sock, "User-Agent: newLISP v%d\r\n", version);


sendf(sock, "Connection: close\r\n");

/* expanded header for PUT, POST and body */
if(type == HTTP_GET_URL || type == HTTP_DELETE_URL)
	{
	sendf(sock, "\r\n");
#ifdef DEBUG
	varPrintf(OUT_CONSOLE, "\r\n");
#endif
	}
else if(type == HTTP_PUT_URL || type == HTTP_PUT_APPEND_URL)
	{
	if(type == HTTP_PUT_APPEND_URL) sendf(sock, "Pragma: append\r\n");
	sendf(sock, "Content-type: text/html\r\nContent-length: %d\r\n\r\n", size);
	send(sock, putPostStr, size, NO_FLAGS_SET);
#ifdef DEBUG
	varPrintf(OUT_CONSOLE, "Content-type: text/html\r\nContent-length: %d\r\n\r\n", size);
	varPrintf(OUT_CONSOLE, putPostStr);
#endif
	}
else if(type == HTTP_POST_URL)
	{
	sendf(sock, "Content-type: %s\r\nContent-length: %d\r\n\r\n", contentType, size);
	send(sock, putPostStr, size, NO_FLAGS_SET);
#ifdef DEBUG
	varPrintf(OUT_CONSOLE, "Content-type: %s\r\nContent-length: %d\r\n\r\n", contentType, size);
	varPrintf(OUT_CONSOLE, putPostStr);
#endif
	}

if(setjmp(socketTimeoutJump) != 0)
    {
    if(sock) close(sock);
    if(resultPtr != NULL) free(resultPtr);
    return(stuffString("ERR: timeout"));
    }
  
/* Retrieve HTTP response and check for status code. */
responseLoop = 0;
READ_RESPONSE:
if(++responseLoop == 4)
        return(stuffString("ERR: invalid response from server"));
       

if (recvs_tm(buff, BUFFSIZE, sock) == NULL) 
   return(stuffString("ERR: no response from server"));

/* if(debug) printf(buff); */

/* go past first token */
for (buffPtr = buff; *buffPtr != '\0' && !isspace((int)*buffPtr); ++buffPtr) {;}

/* trim leading spaces */
while(isspace((int)*buffPtr)) ++buffPtr;

/* get status code */
switch (atoi(buffPtr)) 
	{
	case 0:
	case 100:
		/* drain and continue */
		while (ch = recvc_tm(sock), ch != EOF && ch != '\n') {;}
		goto READ_RESPONSE;
	case 200:
	case 201:
	case 205:
	case 206:
	case 300:
	case 301:
	case 302:
	case 303:
	case 307:
		break;
	default:
		if(strlen(buff) > 64) buff[64] = 0;
		snprintf(errorTxt, 94, "ERR: server code %d: %s", atoi(buffPtr), buff);
		return(stuffString(errorTxt));
	}

/* Retrieve HTTP headers. */
memset(buff, 0, BUFFSIZE);
if(listFlag || headRequest)
    headerCell = stuffString("");

/* Retrieve header */
while(strcmp(buff, "\r\n") != 0 && strcmp(buff, "\n") != 0)
	{
	if(recvs_tm(buff, BUFFSIZE, sock) == NULL)
		return(stuffString("ERR: problem in header"));

/*        if(debugFlag) printf(buff); */
	if(listFlag || headRequest) appendCellString(headerCell, buff, strlen(buff));

	if(my_strnicmp(buff, "content-length:", 15) == 0)
		{
		fileSize = parseValue(buff + 15);
		haveContentLength = TRUE;
		}
	if(my_strnicmp(buff, "location:", 9) == 0)
		{
		buffPtr = buff + 9;
		while(isspace((int)*buffPtr)) ++buffPtr;
		if(*buffPtr == '/')
			strncpy(path, buffPtr + 1, maxlen);
		else /* its a url or path */
			{
			if(parseUrl(buffPtr, protocol, host, &port, path, maxlen) == FALSE)
			    /* path only */
			    parsePath(buffPtr, path, buffPtr - buff);

			if(proxyUrl == NULL)
				{
				strncpy(pHost, host, maxlen);
				pPort = port;
				}
			}

		if(headerCell) deleteList(headerCell);	
		goto CONNECT_TO_HOST;
		}

	if(my_strnicmp(buff, "Transfer-Encoding:", 18) == 0 
					&& searchBuffer(buff, strlen(buff), "chunked", 7, 0) != 0xFFFFFFFF)
		chunked = TRUE;
	}
	
if(headRequest)
    return(headerCell);

/* Retrieve HTTP body. */
if(chunked == TRUE)
	{
	resultPtr = NULL;
	if(recvs_tm(buff, BUFFSIZE, sock) == NULL)
		return(stuffString("ERR: document empty"));
	while((size = strtoul(buff, NULL, 16)) > 0)
		{
		if(resultSize == 0)
			resultPtr = allocMemory(size + 1);
		else 
			resultPtr = reallocMemory(resultPtr, resultSize + size + 1);

                if(recvsize_tm(resultPtr + resultSize, size, sock) != size)
                    {
                    free(resultPtr);
                    return(stuffString("ERR: problem in chunked format"));
                    }
                resultSize += size;
		recvs_tm(buff, BUFFSIZE, sock); /* empty line */
		recvs_tm(buff, BUFFSIZE, sock); /*  chunck size  */
		}
	}

else if(haveContentLength == TRUE)
	{
	resultPtr = allocMemory(fileSize + 1);
	if(fileSize == 0 ||
              (resultSize = recvsize_tm(resultPtr, fileSize, sock)) == 0)
            {
            free(resultPtr);
            return(stuffString("ERR: document empty"));
            }
	}
	
	
else /* no content length given, relies on host closing the connection */
	{
	resultPtr = allocMemory(BUFFSIZE + 1);
	resultSize = 0;
	size = BUFFSIZE;
	memset(buff, 0, BUFFSIZE);
	while ((sizeRead = recvsize_tm(buff, BUFFSIZE - 1, sock)) > 0)
		{
		if((resultSize + sizeRead) > size)
			{
			size = resultSize + BUFFSIZE;
			resultPtr = reallocMemory(resultPtr, size + 1);
			}
		memcpy(resultPtr + resultSize, buff, sizeRead);
		resultSize += sizeRead;
		memset(buff, 0, BUFFSIZE);
		}
	}

if(resultPtr == NULL)
	result = stuffString("");
else
	{
	*(resultPtr + resultSize) = 0;
	result = getCell(CELL_STRING);
	result->contents = (UINT)resultPtr;
	result->aux = resultSize + 1;
	}

close(sock);

if(listFlag)
    {
    cell = getCell(CELL_EXPRESSION);
    cell->contents = (UINT)headerCell;
    headerCell->next = result;
    return(cell);
    }

return(result);
}


int parseUrl(char * url, char * protocol, char * host, int * port, char * path, int maxlen)
{
char * colonPtr;
char * slashPtr;
int len;

/* trim trailing whitespace like '/r/n' from url */
len = strlen(url);
while(*(url + len) <= ' ' && len > 0)
	{
	*(url + len) = 0;
	len--;
	}

*port = 80;

if(my_strnicmp(url, "http://", 7) == 0)
    {
    strncpy(protocol,"http", MAX_PROTOCOL );
    strncpy(host, url+7, maxlen);
    }
else if( my_strnicmp(url, "https://", 8) == 0)
    {
    strncpy(protocol, "https", MAX_PROTOCOL);
    strncpy(host, url+8, maxlen);
    }
else 
    return(FALSE);

colonPtr = strchr(host, ':');
slashPtr = strchr(host, '/');

if (colonPtr != NULL && (slashPtr == NULL || colonPtr < slashPtr)) 
	{
	*colonPtr++ = '\0';
	*port = atoi(colonPtr);
	}

if(path == NULL) return(TRUE);

if (slashPtr != NULL) 
	{
	*slashPtr++ = '\0';
	strncpy(path, slashPtr, maxlen);
	} 
else
	strncpy(path, "", maxlen);


return(TRUE);
}

void parsePath(char * url, char * path, int maxlen)
{
int len;

/* trim trailing whitespace like '/r/n' from url */
len = strlen(url);
while(*(url + len) <= ' ' && len > 0)
	{
	*(url + len) = 0;
	len--;
	}
	
/* trim leading whitespace */
while(*url <= ' ') url++;
strncpy(path, url, maxlen);
}

size_t parseValue(char * str)
{
char * number;
while(!isDigit((unsigned char)*str) && *str != 0) ++str;
number = str;
while(isDigit((unsigned char)*str)) ++str;
return atol(number);
}


/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2004, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: base64.c,v 1.32 2004/12/15 01:38:25 danf Exp $
 ***************************************************************************/

/* Base64 encoding/decoding

   this file from the cURL project is included in nl-web.c for the
   newLISP functions 'base64-enc' and 'base64-dec' 

   all #include statements have and the test harness rootines have
   been stripped.  2005-1-6 Lutz Mueller
*/


static void decodeQuantum(unsigned char *dest, const char *src)
{
  unsigned int x = 0;
  int i;
  for(i = 0; i < 4; i++) {
    if(src[i] >= 'A' && src[i] <= 'Z')
      x = (x << 6) + (unsigned int)(src[i] - 'A' + 0);
    else if(src[i] >= 'a' && src[i] <= 'z')
      x = (x << 6) + (unsigned int)(src[i] - 'a' + 26);
    else if(src[i] >= '0' && src[i] <= '9')
      x = (x << 6) + (unsigned int)(src[i] - '0' + 52);
    else if(src[i] == '+')
      x = (x << 6) + 62;
    else if(src[i] == '/')
      x = (x << 6) + 63;
    else if(src[i] == '=')
      x = (x << 6);
  }

  dest[2] = (unsigned char)(x & 255);
  x >>= 8;
  dest[1] = (unsigned char)(x & 255);
  x >>= 8;
  dest[0] = (unsigned char)(x & 255);
}

/*
 * Curl_base64_decode()
 *
 * Given a base64 string at src, decode it into the memory pointed to by
 * dest. Returns the length of the decoded data.
 */
size_t Curl_base64_decode(const char *src, char *dest)
{
  int length = 0;
  int equalsTerm = 0;
  int i;
  int numQuantums;
  unsigned char lastQuantum[3];
  size_t rawlen=0;

  while((src[length] != '=') && src[length])
    length++;
  while(src[length+equalsTerm] == '=')
    equalsTerm++;

  if(equalsTerm > 3) equalsTerm = 3; /* LM added 2006-09-08 */

  numQuantums = (length + equalsTerm) / 4;

  if(numQuantums == 0) return(0);

  rawlen = (numQuantums * 3) - equalsTerm;

  for(i = 0; i < numQuantums - 1; i++) {
    decodeQuantum((unsigned char *)dest, src);
    dest += 3; src += 4;
  }

  decodeQuantum(lastQuantum, src);
  for(i = 0; i < 3 - equalsTerm; i++)
    dest[i] = lastQuantum[i];

  return rawlen;
}

/* ---- Base64 Encoding --- */
static const char table64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Curl_base64_encode()
 *
 * Returns the length of the newly created base64 string. The third argument
 * is a pointer to an allocated area holding the base64 data. If something
 * went wrong, -1 is returned.
 *
 */
size_t Curl_base64_encode(const char *inp, size_t insize, char **outptr)
{
  unsigned char ibuf[3];
  unsigned char obuf[4];
  int i;
  int inputparts;
  char *output;
  char *base64data;

  char *indata = (char *)inp;

  *outptr = NULL; /* set to NULL in case of failure before we reach the end */

  if(0 == insize)
    insize = strlen(indata);

  base64data = output = (char*)malloc(insize*4/3+4);
  if(NULL == output)
    return 0;

  while(insize > 0) {
    for (i = inputparts = 0; i < 3; i++) {
      if(insize > 0) {
        inputparts++;
        ibuf[i] = *indata;
        indata++;
        insize--;
      }
      else
        ibuf[i] = 0;
    }

    obuf [0] = (ibuf [0] & 0xFC) >> 2;
    obuf [1] = ((ibuf [0] & 0x03) << 4) | ((ibuf [1] & 0xF0) >> 4);
    obuf [2] = ((ibuf [1] & 0x0F) << 2) | ((ibuf [2] & 0xC0) >> 6);
    obuf [3] = ibuf [2] & 0x3F;

    switch(inputparts) {
    case 1: /* only one byte read */
      snprintf(output, 5, "%c%c==",
               table64[obuf[0]],
               table64[obuf[1]]);
      break;
    case 2: /* two bytes read */
      snprintf(output, 5, "%c%c%c=",
               table64[obuf[0]],
               table64[obuf[1]],
               table64[obuf[2]]);
      break;
    default:
      snprintf(output, 5, "%c%c%c%c",
               table64[obuf[0]],
               table64[obuf[1]],
               table64[obuf[2]],
               table64[obuf[3]] );
      break;
    }
    output += 4;
  }
  *output=0;
  *outptr = base64data; /* make it return the actual data memory */

  return strlen(base64data); /* return the length of the new data */
}
/* ---- End of Base64 Encoding ---- */


/* --------------------------- HTTP server mode -----------------------------
 handles GET, POST, PUT and DELETE requests
 handles queries in GET requests and sets environment variable QUERY_STRING
 sets HTTP_HOST and HTTP_USER_AGENTT when present in client request header
 no special encodings are supported
 subset HTTP/1.0 compliant
*/

int sendHTTPmessage(char * fmt, char * str);
void handleHTTPcgi(char * command, char * query);
size_t readHeader(char * buff, int * pragmaFlag);
ssize_t readPayLoad(size_t size, char * content, int outFile, char * request);
int endsWith(char * str, char * ext);

void sendHTTPpage(char * content, size_t size, char * media, int closeFlag)
{
varPrintf(OUT_CONSOLE, "HTTP/1.0 200 OK\r\nServer: newLISP v.%d (%s)\r\n", version, ostype);
if(media != NULL) /* else Content-type: is provided by CGI page */
  varPrintf(OUT_CONSOLE, "Content-length: %d\r\nContent-type: %s\r\n\r\n", size, media);
#ifndef WINCC
/* size = fwrite(content, 1, size, IOchannel); */ /* does not work wirh xinetd on OSX */
size = write(fileno(IOchannel), content, size); 
fflush(IOchannel); 
if(closeFlag) 
	{
	fclose(IOchannel); 
	IOchannel = NULL;
	}
#else
if(IOchannel != NULL && isSocketStream(IOchannel))
	{
	send(getSocket(IOchannel), content, size, NO_FLAGS_SET);
    close(getSocket(IOchannel));
	}
else 
	varPrintf(OUT_CONSOLE, content);
return;
#endif
}

#define ERR_411 "ERR:411 length required for: %s\r\n"
#define MAX_BUFF 1024
#define DEFAULT_PAGE_1 "index.html"
#define DEFAULT_PAGE_2 "index.cgi"
#define CGI_EXTENSION ".cgi"
#define MEDIA_TXT "text/html"

#define ERROR_404 "ERR:404 File not found: %s\r\n"

int executeHTTPrequest(char * request, int type)
{
char * sptr;
char * query;
int len;
char * content = NULL;
ssize_t transferred, size;
char buff[MAX_BUFF];
int outFile;
int pragmaFlag;
char * fileMode = "w";
char * mediaType;
char * command;
CELL * result = NULL;

chdir(startupDir);
query = sptr = request;

setenv("DOCUMENT_ROOT", startupDir, 1);

/* stuff after request */
while(*sptr > ' ') ++sptr;
*sptr = 0;
while(*query != 0 && *query != '?') ++query;
if(*query == '?')
	{
	*query = 0;
	query++;
	}

setenv("QUERY_STRING", query, 1);

/* if httpd-conf is defined call it with the request 
   the httpd-conf procedure returns a string transformation
   of request, for security stuff, remappping etc. */

if(lookupSymbol("httpd-conf", mainContext) != NULL)
    {
    len = strlen(request) + strlen(query) + 32;
    command = alloca(len);
    snprintf(command, len - 1, "(httpd-conf \"%s\" \"%s\")", request, query);
    result = sysEvalString(command, nilCell, mainContext);
    if(result->type == CELL_STRING)
		request = (char *)result->contents;
	else if (isNil(result))
		{
		if(IOchannel != NULL) 
			{
			deleteList(result);
			fclose(IOchannel);
			IOchannel = NULL;
			}
		return(TRUE);
		}
    }

/* change to base dir of request file */
sptr = request + strlen(request);
while(*sptr != '/' && sptr != request) --sptr;
if(*sptr == '/') 
	{
	*sptr = 0;
	sptr++;
	chdir(request);
	request = sptr;
	}

if((len = strlen(request)) == 0)
	{
	if(isFile(DEFAULT_PAGE_2) == 0)	
		request = DEFAULT_PAGE_2;
	else
		request = DEFAULT_PAGE_1;
	len = strlen(request);
	}

size = readHeader(buff, &pragmaFlag);
switch(type)
	{
	case HTTP_GET_HEAD:
	case HTTP_GET_URL:
		if(endsWith(request, CGI_EXTENSION))
			handleHTTPcgi(request, query);
		else
			{
			if(endsWith(request, ".jpg")) mediaType = "image/jpeg";
			else if(endsWith(request, ".png")) mediaType = "image/png";
			else if(endsWith(request, ".gif")) mediaType = "image/gif";
			else if(endsWith(request, ".pdf")) mediaType = "application/pdf";
			else if(endsWith(request, ".mp3")) mediaType = "audio/mpeg";
			else if(endsWith(request, ".mov")) mediaType = "video/quicktime";
			else if(endsWith(request, ".mpg")) mediaType = "video/mpeg";
			else mediaType = MEDIA_TXT;

			if(type == HTTP_GET_HEAD)
				{
				snprintf(buff, MAX_BUFF - 1, 
#ifndef WINCC
					"Content-length: %lld\r\nContent-type: %s\r\n\r\n", 
					(long long int)fileSize(request), 
#else
					"Content-length: %ld\r\nContent-type: %s\r\n\r\n", 
					(long)fileSize(request), 
#endif
					mediaType);
				sendHTTPpage(buff, strlen(buff), NULL, TRUE);
				}
			else
				{
				if((size = readFile(request, &content)) == -1)
					sendHTTPmessage(ERROR_404, request);
				else
					sendHTTPpage(content, size, mediaType, TRUE);

				if(content) free(content);
				}
			}
		break;

	case HTTP_DELETE_URL:
		if(unlink(request) != 0)	
			sendHTTPmessage("ERR:500 Could not delete: %s\r\n", request);
		else
			sendHTTPmessage("File deleted: %s\r\n", request);
		break;

	case HTTP_POST_URL:
		if(!size)
			return(sendHTTPmessage(ERR_411, request));

		query = callocMemory(size + 1);
		if(readPayLoad(size, query, 0, request) != -1)
			handleHTTPcgi(request, query);
		free(query); 
		break;

	case HTTP_PUT_URL:
		if(pragmaFlag) fileMode = "a";

		if(!size)
			return(sendHTTPmessage(ERR_411, request));

		if( (outFile = openFile(request, fileMode, NULL)) == (int)-1)
			return(sendHTTPmessage("ERR:500 cannot create file: %s\r\n", request));

		transferred = readPayLoad(size, buff, outFile, request);
		close(outFile);

		if(transferred != -1)
			{
			snprintf(buff, 255, "%d bytes transferred for %s\r\n", (int)transferred, request);
			sendHTTPpage(buff, strlen(buff), MEDIA_TXT, TRUE);
			}
		break;

	default:
		break;
	}

if(result != NULL) deleteList(result);
return(TRUE);
}


int sendHTTPmessage(char * fmt, char * str)
{
char msg[256];

snprintf(msg, 255, fmt, str);
sendHTTPpage(msg, strlen(msg), MEDIA_TXT, TRUE);
return(0);
}


/* retrieve rest of header */
size_t readHeader(char * buff, int * pragmaFlag)
{
size_t size = 0;

*pragmaFlag = 0;

memset(buff, 0, MAX_LINE);

setenv("HTTP_HOST", "", 1);
setenv("HTTP_USER_AGENT", "", 1);

while(fgets(buff, MAX_LINE - 1, IOchannel) != NULL)
	{
	if(strcmp(buff, "\r\n") == 0 || strcmp(buff, "\n") == 0) break;
	if(my_strnicmp(buff, "content-length:", 15) == 0)
		size = parseValue(buff + 15);
	if(my_strnicmp(buff, "pragma: append", 14) == 0)
		*pragmaFlag = TRUE;
	if(my_strnicmp(buff, "Host:", 5) == 0)
		setenv("HTTP_HOST", buff + 5, 1);
	if(my_strnicmp(buff, "User-Agent:", 11) == 0)
		setenv("HTTP_USER_AGENT", buff + 11, 1);
	}


return(size);
}

ssize_t readPayLoad(size_t size, char * buff, int outFile, char * request)
{
ssize_t bytes;
size_t offset = 0, transferred = 0;

while(size > 0)
	{
#ifndef WINCC
	bytes = read(fileno(IOchannel), buff + offset, MAX_BUFF); 
#else
	if(IOchannel != NULL && isSocketStream(IOchannel))
		bytes = recv(getSocket(IOchannel), buff + offset, MAX_BUFF, NO_FLAGS_SET);
	else
		bytes = read(fileno(IOchannel), buff + offset, MAX_BUFF);
#endif
	if(bytes <= 0)
		{
		sendHTTPmessage("ERR:500 error reading data: %s\r\n", request);
		return(-1);
		}

	if(outFile)
		{
		if(write(outFile, buff + offset, bytes) != bytes)
			{
			sendHTTPmessage("ERR:500 cannot create file: %s\r\n", request);
			return(-1);
			}
		}
	else
		offset += bytes;

	transferred += bytes;
	size -= bytes;
	}	
#ifndef WINCC
fflush(NULL); 
#endif
return(transferred);
}


void handleHTTPcgi(char * request, char * query)
{
FILE * handle;
char * command;
char * content = NULL;
size_t size;
char tempfile[32];
#ifdef WINCC
char * ptr;
char * pos;
int bytes = 0;
#endif

srandom(milliSecTime());

if(isFile(request) != 0)
	{
	sendHTTPmessage(ERROR_404, request);
	return;
	}

if(isFile("/tmp") != 0)
    {
    sendHTTPmessage("ERR:500 need /tmp directory configured: %s\n", request);
    return;
    }

size = strlen(request) + 64;
command = alloca(size);
snprintf(tempfile, 30, "/tmp/nl%02x%08x%08x", (unsigned int)size, (unsigned int)random(), (unsigned int)random());
#ifndef WINCC
#ifndef OS2
snprintf(command, size - 1, "./%s > %s", request, tempfile);
#endif
#else
snprintf(command, size - 1, "newlisp %s > %s", request, tempfile);
#endif

if((handle = popen(command, "w")) == NULL)
	{
	sendHTTPmessage("ERR:500 failed creating pipe: %s\n", request);
	return;
	}

fwrite(query, 1, strlen(query), handle);
fflush(handle);
pclose(handle);

size = readFile(tempfile, &content);
if(size == -1)	
	sendHTTPmessage("ERR:500 cannot read output of: %s", request);
else
	{
#ifdef WINCC
	/* replace all \r\r\n with \r\n (artefact of Win32 out-piping) */
	ptr = content;
	bytes = 0;
	while((pos = strstr(ptr, "\r\r\n")))
		{
		memcpy(content + bytes, ptr, pos - ptr);
		bytes += pos - ptr;
		memcpy(content + bytes, "\r\n", 2);
		bytes += 2;
		ptr = pos + 3 ;
		--size;
		}
	memcpy(content + bytes, ptr, size - bytes);
	*(content + size) = 0;
#endif	
	sendHTTPpage(content, size, NULL, TRUE);
	}
	
unlink(tempfile);
if(content) free(content);
}	


int endsWith(char * str, char * ext)
{
size_t size, len;

size = strlen(str);
len =  strlen(ext);

return(strncmp(str + size - len, ext, len) == 0);
}

/* eof */

