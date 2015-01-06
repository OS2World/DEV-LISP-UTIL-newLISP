/*
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

#ifdef MINGW
#define WINCC
#endif

#ifdef WINCE
#define WINCC
#endif

#include "newlisp.h"
#include <string.h>

#ifdef WINCC
#include <winsock2.h>
#ifndef WINCE
#include <ws2tcpip.h>
#endif
#define fdopen win32_fdopen
#define SHUT_RDWR 2
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#ifdef SOLARIS
#include <stropts.h>
#include <sys/conf.h>
#include <netinet/in_systm.h>
#ifndef TRU64
#define FIONREAD I_NREAD
#endif
#endif

#ifdef OS2 
#define socklen_t int 
#define SHUT_RDWR 2 
#endif 

/*
#ifdef MAC_OSX
#define socklen_t int
#endif
*/

#include "protos.h"

#define MAX_PENDING_CONNECTS 128
#define NO_FLAGS_SET 0

#ifdef WINCC
#define socklen_t int
#define close closesocket
#else
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif


#define ERR_INET_OPEN_SOCKET 1
#define ERR_INET_HOST_UNKNOWN 2
#define ERR_INET_INVALID_SERVICE 3
#define ERR_INET_CONNECT_FAILED 4
#define ERR_INET_ACCEPT 5
#define ERR_INET_CONNECTION_DROPPED 6
#define ERR_INET_CONNECTION_BROKEN 7
#define ERR_INET_READ 8
#define ERR_INET_WRITE 9
#define ERR_INET_CANNOT_BIND 10
#define ERR_INET_TOO_MUCH_SOCKETS 11
#define ERR_INET_LISTEN_FAILED 12
#define ERR_INET_BAD_FORMED_IP 13
#define ERR_INET_SELECT_FAILED 14
#define ERR_INET_PEEK_FAILED 15
#define ERR_INET_NOT_VALID_SOCKET 16

#define isnum(A) ((A)>= '0' && (A) <= '9')

typedef struct
    {
    int socket;
    void * next;
    } INET_SESSION;

INET_SESSION * netSessions = NULL;

int deleteInetSession(int sock);

#define READY_READ 0
#define READY_WRITE 1

int errorIdx = 0;

extern int logTraffic;
extern int noPromptMode;

#ifdef WINCC
struct timezone {
       int     tz_minuteswest;
       int     tz_dsttime;
};
int gettimeofday(struct timeval * tv, struct timezone * tz);
#endif

/********************** auxiliary functions *******************/


unsigned int asciiIPtoLong(char * ptr)
{
char token[5];
int count, len;
char ip[4];

count = len = 0;
while(*ptr != 0 && count < 4)
    {
    len = 0;
    while(isnum(*ptr) && len < 3)
        token[len++] = *ptr++;
    token[len] = 0;
    if(*ptr == '.' || *ptr == 0)
        {
        ip[count++] = (char)atoi(token);
        if(*ptr == '.') ptr++;
        }
    else return(0);
    }

if(count == 4) return(*(unsigned int *)ip);
return(0);
}

int createInetSession(int sock)
{
INET_SESSION * iSession;

iSession = (INET_SESSION *)malloc(sizeof(INET_SESSION));

iSession->socket = sock;

if(netSessions == NULL)
    {
    netSessions = iSession;
    iSession->next = NULL;
    }
else
    {
    iSession->next = netSessions;
    netSessions = iSession;
    }
return(TRUE);
}


int deleteInetSession(int sock)
{
INET_SESSION * session;
INET_SESSION * previous;

if(netSessions == NULL)
    return(0);
else
    session = previous = netSessions;

while(session)
    {
    if(session->socket == sock)
        {
        if(session == netSessions)
            netSessions = session->next;
        else
            previous->next = session->next;
        free((char *)session);
        return(TRUE);
        }
    previous = session;
    session = session->next;
    }

return(FALSE);
}

/************ get service port from service name string ********/


CELL * getServicePort(CELL * params, int * portNo, char * protocol) 
{ 
struct servent * pSe; 
CELL * cell;
UINT port= 0;
 
cell = evaluateExpression(params); 
 
if(isNumber(cell->type)) 
	{
    getIntegerExt(cell, (UINT*)&port, FALSE); 
	*portNo = port; 
	}
 
else if(cell->type == CELL_STRING) 
    { 
#ifndef WINCE
    if((pSe = getservbyname((char *)cell->contents, protocol)) == NULL) 
        return(netError(ERR_INET_INVALID_SERVICE)); 
#else
    return(netError(ERR_INET_INVALID_SERVICE));
#endif
    *portNo = pSe->s_port; 
    } 
else  
    return(errorProcExt(ERR_NUMBER_OR_STRING_EXPECTED, params)); 
 
return(trueCell); 
} 

/********************* user functions **************************/


CELL * p_netClose(CELL * params) 
{
UINT sock; 
 
getInteger(params, &sock); 
deleteInetSession((int)sock);

if(!getFlag(params->next))
	shutdown(sock, SHUT_RDWR);

if(close((int)sock) == SOCKET_ERROR)
    return(netError(ERR_INET_NOT_VALID_SOCKET));

errorIdx = 0;
return(trueCell);
}

CELL * p_netSessions(CELL * params)
{
INET_SESSION * session;
INET_SESSION * sPtr;
CELL * sList;
CELL * last;

session = netSessions;
sList = getCell(CELL_EXPRESSION);
last = NULL;

while(session)
    {
    sPtr = session;
    session = session->next;
    if(last == NULL)
        {
        last = stuffInteger(sPtr->socket);
        sList->contents = (UINT)last;
        }
    else
        {
        last->next = stuffInteger(sPtr->socket);
        last = last->next;
        }
    }

return(sList);
}



/*********************************************************************/
CELL * p_netService(CELL * params) 
{
struct servent * pSe; 
char * service; 
char * protocol; 
int port; 
 
params = getString(params, &service); 
getString(params, &protocol); 
 
#ifndef WINCE
if((pSe = getservbyname(service, protocol)) == NULL) 
    return(netError(ERR_INET_INVALID_SERVICE)); 
#else
return(netError(ERR_INET_INVALID_SERVICE)); 
#endif
 
port = (int)ntohs(pSe->s_port); 
 
errorIdx = 0; 
return(stuffInteger((UINT)port)); 
} 


CELL * p_netConnect(CELL * params)
{
int sock;
UINT ttl = 3;
char * remoteHostName; 
int portNo, type; 
char * protocol = NULL;

params = getString(params, &remoteHostName); 
if(getServicePort(params, &portNo, "tcp") == nilCell) 
    return(netError(ERR_INET_INVALID_SERVICE)); 

type = SOCK_STREAM;
if(params->next != nilCell)
    {
    params = getString(params->next, &protocol);
    *protocol = toupper(*protocol);
    type = SOCK_DGRAM;
    if(*protocol == 'M')
        {
        if(params != nilCell)
            getInteger(params, &ttl);
        }
    }

if((sock = netConnect(remoteHostName, portNo, type, protocol, (int)ttl)) == SOCKET_ERROR)
    return(netError(errorIdx));

createInetSession(sock);

errorIdx = 0;
return(stuffInteger((UINT)sock)); 
}



int netConnect(char * remoteHostName, int portNo, int type, char * prot, int ttl)
{
struct sockaddr_in dest_sin;
struct in_addr iaddr;
struct hostent * pHe;
int sock, idx;
/* char opt; */
int opt;

/* create socket */
if((sock = socket(AF_INET, type, 0)) == INVALID_SOCKET)
    {
    errorIdx = ERR_INET_OPEN_SOCKET;
    return(SOCKET_ERROR);
    }

if(prot != NULL) if(*prot == 'M' || *prot == 'B')
    {
    memset(&iaddr, 0, sizeof(iaddr));
    iaddr.s_addr = INADDR_ANY;

    if(*prot == 'M')
        {
        setsockopt(sock, 0, IP_MULTICAST_IF, (const char *)&iaddr, sizeof(iaddr));
        opt = ttl;
        setsockopt(sock, 0, IP_MULTICAST_TTL, (const char *)&opt, sizeof(opt));
        }

        if(*prot == 'B')
            {
            opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&opt, sizeof(opt));
            }
    }

if((pHe = gethostbyname(remoteHostName)) == NULL)
    {
    errorIdx = ERR_INET_HOST_UNKNOWN;
    return(SOCKET_ERROR);
    }

for(idx = 0; ; idx++)
    {
    memcpy((char *)&(dest_sin.sin_addr),
        pHe->h_addr_list[idx], pHe->h_length);

    dest_sin.sin_port = htons((u_short)portNo);
    dest_sin.sin_family = AF_INET;

    if(connect(sock,(struct sockaddr *)&dest_sin, sizeof(dest_sin)) == 0)
        break;

    if(pHe->h_addr_list[idx+1] != NULL)
        continue;

    close(sock);
    errorIdx = ERR_INET_CONNECT_FAILED;
    return(SOCKET_ERROR);
    }

errorIdx = 0;
return(sock);
}

/********* should be called after listen/accept notification **********/

CELL * p_netAccept(CELL * params) 
{
int sock;
UINT listenSock;

getInteger(params, &listenSock); 

if((sock = netAccept((int)listenSock)) == INVALID_SOCKET)
    return(netError(ERR_INET_ACCEPT));

return(stuffInteger(sock)); 
}


int netAccept(int listenSock)
{
int sock;
struct sockaddr_in dest_sin;
socklen_t dest_sin_len;

/* create a new connection */
dest_sin_len = sizeof(struct sockaddr_in);
sock = accept((int)listenSock,
    (struct sockaddr *) &dest_sin, (void *)&dest_sin_len);

if(sock != INVALID_SOCKET) 
    {
    createInetSession(sock);
    errorIdx = 0;
    }

return(sock);
}


/******************* returns remote IP and port number *************/

#define LOCAL_INFO 0
#define PEER_INFO 1


int getPeerName(int sock, int peerLocalFlag, char * IPaddress)
{
socklen_t address_sin_len; 
struct sockaddr_in address_sin; 
unsigned char ipNo[4]; 

*IPaddress = 0;
address_sin_len = sizeof(address_sin); 
if(peerLocalFlag == LOCAL_INFO) 
    { 
    if(getsockname(sock,  
        (struct sockaddr *)&address_sin, (void *)&address_sin_len) 
            == SOCKET_ERROR) 
        return(0); 
    } 
else 
    { 
    if(getpeername(sock,  
        (struct sockaddr *)&address_sin, (void *)&address_sin_len) 
            == SOCKET_ERROR) 
        return(0); 
    } 
 
/* return address IP number  */
memset(IPaddress, 0, sizeof(IPaddress)); 
memcpy(ipNo, &address_sin.sin_addr.s_addr, sizeof(ipNo)); 
snprintf(IPaddress, 15, "%d.%d.%d.%d", ipNo[0], ipNo[1], ipNo[2], ipNo[3]); 

return(ntohs(address_sin.sin_port));
}


CELL * p_netLocal(CELL * params) 
{
return(netPeerLocal(params, LOCAL_INFO));
}

CELL * p_netPeer(CELL * params)
{
return(netPeerLocal(params, PEER_INFO));
}


CELL * netPeerLocal(CELL * params, int peerLocalFlag) 
{ 
CELL * result; 
CELL * cell; 
char name[16];
UINT addressPort, sock;

getInteger(params, &sock); 

if((addressPort = getPeerName((int)sock, peerLocalFlag, name)) == 0)
    return(nilCell);
 
result = getCell(CELL_EXPRESSION); 
result->contents = (UINT)stuffString(name); 
cell = (CELL *)result->contents; 
cell->next = stuffInteger((UINT)addressPort); 
 
errorIdx = 0; 
return(result); 
} 
 

CELL * p_netLookup(CELL * params)
{
union ipSpec 
    {
    unsigned int no;
    unsigned char chr[4];
    } ip;

struct sockaddr_in address;
struct hostent * pHe;
char * hostString;
char IPaddress[16];
int forceByName = 0;

params = getString(params, &hostString);
forceByName = getFlag(params);

/* get hostname from ip-number */
if(isDigit((unsigned char)*hostString) && !forceByName)
    {
    if((ip.no = asciiIPtoLong(hostString)) == 0)
        return(netError(ERR_INET_BAD_FORMED_IP));

    if((pHe = gethostbyaddr((char *) &ip.no,4,PF_INET)) == NULL)
        return(nilCell);

    return(stuffString((char *)pHe->h_name));
    }

/* get ip-number from hostname */
if((pHe = gethostbyname(hostString)) == NULL)
    return(nilCell);

memcpy((char *)&(address.sin_addr), pHe->h_addr_list[0], pHe->h_length);

memset(IPaddress, 0, sizeof(IPaddress)); 
memcpy(ip.chr, &address.sin_addr.s_addr, sizeof(ip.chr)); 
snprintf(IPaddress, 15, "%d.%d.%d.%d", ip.chr[0], ip.chr[1], ip.chr[2], ip.chr[3]); 

errorIdx = 0;
return(stuffString(IPaddress));
}

CELL * netReceive(int sock, SYMBOL * readSymbol, size_t readSize, CELL * params);

CELL * p_netReceive(CELL * params) 
{ 
UINT sock;
SYMBOL * readSymbol;
size_t readSize;

params = getInteger(params, &sock);
params = getSymbol(params, &readSymbol);
params = getInteger(params, (UINT *)&readSize);

return(netReceive((int)sock, readSymbol, readSize, params));
}

CELL * netReceive(int sock, SYMBOL * readSymbol, size_t readSize, CELL * params)
{
char * waitFor;
ssize_t bytesReceived;
size_t length;
int found;
STREAM netStream;
char chr;
CELL * cell;

if(isProtected(readSymbol->flags))
    return(errorProcExt2(ERR_SYMBOL_PROTECTED, stuffSymbol(readSymbol)));

memset(&netStream, 0, sizeof(netStream));

if(params == nilCell)
    {
    openStrStream(&netStream, readSize, 0);
    found = 1;
    bytesReceived  = recv(sock, netStream.buffer, readSize, NO_FLAGS_SET);
    }
else
    {
    getString(params, &waitFor);
    openStrStream(&netStream, MAX_LINE, 0);
    found = bytesReceived = 0;
    length = strlen(waitFor);
    while(bytesReceived < (int)readSize)
        {
        if(recv(sock, &chr, 1, NO_FLAGS_SET) <= 0)
            {
            bytesReceived = 0;
            break;
            }
        writeStreamChar(&netStream, chr); 
         if(++bytesReceived < length) continue;
        if(strcmp(waitFor,  netStream.ptr - length) == 0)
             {
            found = 1;
            break;
            }        
        }
            
    }

if(bytesReceived == 0 || found == 0) 
    { 
    closeStrStream(&netStream); 
    deleteInetSession(sock); 
    close(sock); 
    return(netError(ERR_INET_CONNECTION_DROPPED)); 
    } 

if(bytesReceived == SOCKET_ERROR) 
    { 
    closeStrStream(&netStream);         
    deleteInetSession(sock); 
    close(sock); 
    return(netError(ERR_INET_READ)); 
    } 
  
cell = stuffStringN(netStream.buffer, bytesReceived);
closeStrStream(&netStream);

deleteList((CELL *)readSymbol->contents); 
readSymbol->contents = (UINT)cell; 
 
errorIdx = 0; 
return(stuffInteger(bytesReceived)); 
} 



CELL * netReceiveFrom(int sock, size_t readSize, int closeFlag)
{
int portNo;
char * buffer;
ssize_t bytesReceived;
struct sockaddr_in remote_sin;
CELL * cell;
CELL * result;
#ifdef WIN_32
int remote_sin_len;
#else
#ifdef TRU64
unsigned long remote_sin_len;
#else
socklen_t remote_sin_len;
#endif
#endif
char IPaddress[16];
unsigned char ipNo[4];

buffer = (char *)allocMemory(readSize + 1);
remote_sin_len = sizeof(remote_sin);
memset(&remote_sin, 0, sizeof(remote_sin));

bytesReceived = recvfrom(sock, buffer, readSize, 0, 
    (struct sockaddr *)&remote_sin, &remote_sin_len);

if(bytesReceived == SOCKET_ERROR) 
    {
    freeMemory(buffer);
    close(sock); 
    return(netError(ERR_INET_READ)); 
    }

memset(IPaddress, 0, sizeof(IPaddress));
memcpy(ipNo, &remote_sin.sin_addr.s_addr, sizeof(ipNo));
snprintf(IPaddress, 15, "%d.%d.%d.%d", ipNo[0], ipNo[1], ipNo[2], ipNo[3]);
portNo = ntohs(remote_sin.sin_port);


cell = result = getCell(CELL_EXPRESSION);
cell->contents = (UINT)stuffStringN(buffer, bytesReceived);
cell = (CELL *)cell->contents;
cell->next = stuffString(IPaddress);
((CELL*)cell->next)->next = stuffInteger(portNo);

freeMemory(buffer);

if(closeFlag) close(sock);

return(result);
}


CELL * p_netReceiveUDP(CELL * params) 
{ 
UINT portNo;
int sock;
size_t readSize;
INT64 wait = 0;
char * ifaddr = NULL;

params = getInteger(params, &portNo);
params = getInteger(params, (UINT *)&readSize);
if(params != nilCell)
    {
    params = getInteger64(params, &wait);
    if(params != nilCell)
        getString(params, &ifaddr);
    }

if((sock = netListenOrDatagram((int)portNo, SOCK_DGRAM, ifaddr, NULL)) == SOCKET_ERROR)
    return(nilCell);

/* if timeout parameter given wait for socket to be readable */
if(wait > 0)
        {
        if(wait_ready(sock, wait, READY_READ) <= 0)
                {
                close(sock);
                return(nilCell);
                }
        }

return(netReceiveFrom(sock, readSize, TRUE));
}


CELL * p_netReceiveFrom(CELL * params)
{
UINT sock;
size_t readSize;

params = getInteger(params, &sock);
getInteger(params, (UINT*)&readSize);

return(netReceiveFrom((int)sock, readSize, FALSE));
}


/**********************************************************************/

CELL * p_netSend(CELL * params) 
{
UINT sock; 
size_t size; 
CELL * cell; 
SYMBOL * writeSptr; 
char * buffer; 
ssize_t bytesSent; 

params = getInteger(params, (UINT*)&sock); 
cell = evaluateExpression(params);
if(cell->type == CELL_SYMBOL)
    {
    writeSptr = (SYMBOL *)cell->contents;
    cell = (CELL *)writeSptr->contents;
    }
else if(cell->type == CELL_DYN_SYMBOL)
    {
    writeSptr = getDynamicSymbol(cell);
    cell = (CELL *)writeSptr->contents;
    }

if(cell->type != CELL_STRING) 
    return(errorProcExt(ERR_STRING_EXPECTED, params)); 

params = params->next; 
if(params->type == CELL_NIL) 
    size = cell->aux - 1; 
else        
    getInteger(params, (UINT *)&size); 
 
buffer = (char *)cell->contents; 
if(size > (cell->aux - 1)) size = cell->aux - 1;  

if((bytesSent = send((int)sock, buffer, size, NO_FLAGS_SET))  == SOCKET_ERROR) 
    { 
    deleteInetSession((int)sock); 
    close((int)sock); 
    return(netError(ERR_INET_WRITE)); 
    }

errorIdx = 0; 
return(stuffInteger(bytesSent)); 
}

#define SEND_TO_UDP 0
#define SEND_TO_SOCK 1

CELL * netSendTo(CELL * params, int type)
{
char * remoteHost;
UINT remotePort;
struct sockaddr_in dest_sin;
struct hostent * pHe;
size_t size;
char * buffer;
ssize_t bytesSent;
long sock;
/* char one = 1; */
int one = 1;

params = getString(params, &remoteHost);
params = getInteger(params, &remotePort);
params = getStringSize(params, &buffer, &size, TRUE);

if((pHe = gethostbyname(remoteHost)) == NULL)
        return(netError(ERR_INET_HOST_UNKNOWN));

if(type == SEND_TO_UDP) /* for 'net-send-udp' */
	{
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
        return(netError(ERR_INET_OPEN_SOCKET));

    if(getFlag(params))
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&one, sizeof(one));
	}
else /* SEND_TO_SOCK , socket may or may not be UDP, for 'net-send-to' */
    {
    params = getInteger(params, (UINT *)&sock);
    }

memcpy((char *)&(dest_sin.sin_addr), pHe->h_addr_list[0], pHe->h_length);
dest_sin.sin_port = htons((u_short)remotePort);
dest_sin.sin_family = AF_INET;

bytesSent = sendto((int)sock, buffer, size, NO_FLAGS_SET,
        (struct sockaddr *)&dest_sin, sizeof(dest_sin));

if(type == SEND_TO_UDP) close((int)sock);

if(bytesSent == SOCKET_ERROR)
        return(netError(ERR_INET_WRITE));

errorIdx = 0;
return(stuffInteger(bytesSent));
}


CELL * p_netSendUDP(CELL * params)
{
return(netSendTo(params, SEND_TO_UDP));
}


CELL * p_netSendTo(CELL * params)
{
return(netSendTo(params, SEND_TO_SOCK));
}


/************************* listen **************************************/

CELL * p_netListen(CELL * params) 
{ 
UINT portNo;
char * ifAddr = NULL;
char * option = NULL;
char * mcAddr = NULL;
int sock, type; 
 
type = SOCK_STREAM;
params = getInteger(params, (UINT*)&portNo); 

if(params != nilCell)
    {
    params = getString(params, &ifAddr);
    if(*ifAddr == 0) ifAddr = NULL;
    if(params != nilCell)
        {
        params = getString(params, &option);
        if(*option == 'u' || *option == 'U')
            type = SOCK_DGRAM;
        else if(*option == 'm' || *option == 'M')
            {
            type = SOCK_DGRAM;
            mcAddr = ifAddr;
            ifAddr = NULL;
            }
        }
    }
    

if((sock = netListenOrDatagram(portNo, type, ifAddr, mcAddr)) == SOCKET_ERROR)
    return(nilCell);


return(stuffInteger(sock));
} 


int netListenOrDatagram(int portNo, int type, char * ifAddr, char * mcAddr)
{
int sock; 
int one = 1;
/* char one = 1; */
struct sockaddr_in local_sin; 
struct hostent * pHe;
struct ip_mreq mcast;

if((sock = socket(AF_INET, type, 0)) == INVALID_SOCKET)
    {
    errorIdx = ERR_INET_OPEN_SOCKET; 
    return SOCKET_ERROR;
    }

memset(&local_sin, 0, sizeof(local_sin));

if(ifAddr != NULL && *ifAddr != 0)
    {
    if((pHe = gethostbyname(ifAddr)) == NULL)
    {
    errorIdx = ERR_INET_HOST_UNKNOWN;
    return(SOCKET_ERROR);
    }
    memcpy((char *)&(local_sin.sin_addr), pHe->h_addr_list[0], pHe->h_length);
    }
else 
    local_sin.sin_addr.s_addr = INADDR_ANY; 


local_sin.sin_port = htons((u_short)portNo); 
local_sin.sin_family = AF_INET; 

setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&one, sizeof(one));
 
if(bind(sock, (struct sockaddr *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR) 
    { 
    close(sock);
    errorIdx = ERR_INET_CANNOT_BIND; 
    return(SOCKET_ERROR);
    } 

if(mcAddr != NULL)
    {
    memset(&mcast, 0, sizeof(mcast));
    mcast.imr_multiaddr.s_addr = inet_addr(mcAddr);
    mcast.imr_interface.s_addr = INADDR_ANY;    
    setsockopt(sock, 0, IP_ADD_MEMBERSHIP, (const char *)&mcast, sizeof(mcast));
    }

if(type == SOCK_STREAM)
    {
    if(listen(sock, MAX_PENDING_CONNECTS) == SOCKET_ERROR)  
    { 
    close(sock); 
    errorIdx = ERR_INET_LISTEN_FAILED;
    return(SOCKET_ERROR);
    } 
    }

createInetSession(sock); 

errorIdx = 0;
return(sock);
}


/* returns number of bytes ready to read */
CELL * p_netPeek(CELL * params)
{
long sock;
#ifdef WINCC
unsigned long result;
#else
int result;
#endif

getInteger(params, (UINT*)&sock);

if(ioctl((int)sock, FIONREAD, &result) == SOCKET_ERROR)
    return(netError(ERR_INET_PEEK_FAILED));

errorIdx = 0;
return(stuffInteger((UINT)result));
} 


typedef struct
    {
    int sock;
    void * next;
    } SOCKLIST;




/* checks a socket for readability/writeability  */
/* does not work right on client side? */
CELL * p_netSelect(CELL * params)
{
long value;
INT64 wait;
char * mode;
struct timeval timeOut;
fd_set socketSet;
SOCKLIST * sockPtr;
SOCKLIST * sockList = NULL;
CELL * cell;
CELL * list = NULL;
struct timeval* tmvPtr;

errorIdx = 0;

FD_ZERO(&socketSet);

cell = evaluateExpression(params);
if(isNumber(cell->type))
    getIntegerExt(cell, (UINT*)&value, FALSE);
else if(isList(cell->type))
    {
    cell = (CELL*)cell->contents;
    if(cell == nilCell) return(getCell(CELL_EXPRESSION));
    sockList = sockPtr = allocMemory(sizeof(SOCKLIST));
    sockPtr->sock = cell->contents;
    sockPtr->next = NULL;
    FD_SET(sockPtr->sock, &socketSet);
    value = 1;
    while((cell = cell->next) != nilCell)
        {
        sockPtr->next = allocMemory(sizeof(SOCKLIST));
        sockPtr = sockPtr->next;
        sockPtr->sock = cell->contents;        
        sockPtr->next = NULL;
        if(value == FD_SETSIZE)
            return(netError(ERR_INET_TOO_MUCH_SOCKETS));
        else value++;
        FD_SET(sockPtr->sock, &socketSet);
        }
    }
else return(errorProcExt(ERR_LIST_OR_NUMBER_EXPECTED, params));

params = getString(params->next, &mode);
getInteger64(params, &wait);

tmvPtr = (wait == -1) ? NULL : &timeOut;
timeOut.tv_sec = wait/1000000;
timeOut.tv_usec = wait - timeOut.tv_sec * 1000000;

if(sockList == NULL)
    {
    FD_SET((int)value, &socketSet);
    value = 1;
    }

/* printf("%d %d %d\n", timeOut.tv_sec, timeOut.tv_usec, sizeof(timeOut.tv_sec));  */

if(*mode == 'r')
    value = select(FD_SETSIZE, &socketSet, NULL, NULL, tmvPtr);
else if(*mode == 'w')
    value = select(FD_SETSIZE, NULL, &socketSet, NULL, tmvPtr);
else if(*mode == 'e')
    value = select(FD_SETSIZE, NULL, NULL, &socketSet, tmvPtr);

/* printf("%d value\n", value);  */

if(value >= 0)
    {
    if((sockPtr = sockList) == NULL)
        {
        if(value == 0) return(nilCell);
        else return(trueCell);
        }    

    cell = getCell(CELL_EXPRESSION);
    while(sockPtr != NULL)
        {
        if(FD_ISSET(sockPtr->sock, &socketSet))
            {
            if(list == NULL)
                {
                list = cell;
                cell->contents = (UINT)stuffInteger(sockPtr->sock);
                cell = (CELL *)cell->contents;
                }
            else
                {
                cell->next = stuffInteger(sockPtr->sock);
                cell = cell->next;
                }
            }
        sockPtr = sockPtr->next;
        free(sockList);
        sockList = sockPtr;
        }

    if(list == NULL) return(cell);
    else return(list);
    }

netError(ERR_INET_SELECT_FAILED);

if(sockList == NULL) return(nilCell);
return(getCell(CELL_EXPRESSION));
}

extern char logFile[];

void writeLog(char * text, int newLine)
{
int handle;

#ifdef WINCC
handle = open(logFile, O_RDWR | O_APPEND | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
#else
handle = open(logFile, O_RDWR | O_APPEND | O_BINARY | O_CREAT,
          S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH); /* rw-rw-rw */
#endif

write(handle, text, strlen(text));
if(newLine) write(handle, &LINE_FEED, strlen(LINE_FEED));
close(handle);
}


FILE * serverFD(int port, int reconnect)
{
static int sock, connection;
static struct sockaddr_in dest_sin;
socklen_t dest_sin_len;
char name[16];
char text[80];
time_t t;

if(! reconnect)
	{
	if((sock = netListenOrDatagram(port, SOCK_STREAM, NULL, NULL)) == SOCKET_ERROR)
	return NULL;
	snprintf(text, 78, "newLISP v.%d listening on port %d", version, port);
	writeLog(text, TRUE);
	}

if(reconnect)
	{
	deleteInetSession(connection);
	close(connection); 
	}

dest_sin_len = sizeof(struct sockaddr_in);
memset(&dest_sin, 0, dest_sin_len);

/*
dest_sin.sin_family = AF_INET;
dest_sin.sin_port = htons(port);
memset(&dest_sin.sin_addr, 0, sizeof(dest_sin.sin_addr));
*/

connection = accept(sock, (struct sockaddr *) &dest_sin, (void *)&dest_sin_len);

createInetSession(connection);

if(connection == SOCKET_ERROR)
	return NULL;

/* print log */
getPeerName(connection, PEER_INFO, name);
t = time(NULL);
snprintf(text, 78, "Connected to %s on %s", name, ctime(&t));
/* printf(text); */
writeLog(text, 0);

return(fdopen(connection, "r+")); 
}

/******************************* distributed computing ***********************/

#define MAX_BUFF 1024
CELL * netEvalError(int errNo);

typedef struct
	{
	char * host;
	int port;
	int sock;
	int timeOut;
	STREAM * netStream;
	CELL * result;
	void * next;
	} NETEVAL;

void freeSessions(NETEVAL * base);

CELL * p_netEval(CELL * params)
{
CELL * cell = NULL;
CELL * list = NULL;
NETEVAL * session = NULL;
NETEVAL * base = NULL;
char * host;
char * prog;
UINT port;
int ready;
int sock;
size_t size, count = 0;
ssize_t bytes;
long timeOut = MAX_LONG;
int start, elapsed = 0;
CELL * result;
STREAM * netStream;
CELL * netEvalIdle = NULL;
char buffer[MAX_BUFF];
STREAM evalStream;
int rawMode = FALSE;
int singleSession = FALSE;
jmp_buf errorJumpSave;
int errNo;
int resultStackIdxSave;

list  = evaluateExpression(params);
if(list->type == CELL_STRING)
	{
	host = (char *)list->contents;
	params = getIntegerExt(params->next, &port, TRUE);
	params = getStringSize(params, &prog, &size, TRUE);
	list = nilCell;
	singleSession = TRUE;
	goto SINGLE_SESSION;
	}
else if(list->type == CELL_EXPRESSION)
	{
	list = (CELL*)list->contents;
	params = params->next;
	}
else return(errorProcExt(ERR_LIST_OR_STRING_EXPECTED, params));

CREATE_SESSION:
if(!isList(list->type))
    return(errorProcExt(ERR_LIST_EXPECTED, list));
cell = (CELL *)list->contents;

/* get node parameters, since 8.9.8 evaluated */
memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));
if((errNo = setjmp(errorJump)) != 0)
	{
	memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
	freeSessions(base);
	longjmp(errorJump, errNo);
	}
cell = getStringSize(cell, &host, &size, TRUE);
cell = getIntegerExt(cell, (UINT *)&port, TRUE);
cell = getStringSize(cell, &prog, &size, TRUE);
rawMode = getFlag(cell);

memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));

SINGLE_SESSION:
if(base == NULL)
    {
    base = session = allocMemory(sizeof(NETEVAL));
    memset(base, 0, sizeof(NETEVAL));
    }
else
    {
    session->next = allocMemory(sizeof(NETEVAL));
    session = session->next;
    memset(session, 0, sizeof(NETEVAL));
    }

if((sock = netConnect(host, (int)port, SOCK_STREAM, NULL, 3)) == SOCKET_ERROR)
    {
    session->result = netEvalError(errorIdx);
    goto CONTINUE_CREATE_SESSION;
    }

session->host = host;
session->port = port;
session->sock = sock;

if( send(sock, "[cmd]\n", 6, NO_FLAGS_SET)  == SOCKET_ERROR ||
    send(sock, prog, size, NO_FLAGS_SET) == SOCKET_ERROR ||
    send(sock, "(exit)\n[/cmd]\n", 14, NO_FLAGS_SET) == SOCKET_ERROR )
    { 
    close(sock); 
    session->result = netEvalError(ERR_INET_WRITE); 
    goto CONTINUE_CREATE_SESSION;
    }

session->netStream = (void *)allocMemory(sizeof(STREAM));
memset(session->netStream, 0, sizeof(STREAM));
openStrStream(session->netStream, MAX_BUFF, 0);
/* prepend quote for evaluation */
writeStreamChar(session->netStream, '\'');
createInetSession(sock);
count++;
CONTINUE_CREATE_SESSION:
list = list->next;
if(list != nilCell) goto CREATE_SESSION;

/* get timeout and optional handler symbol */
session = base;
if(params != nilCell)
	params = getInteger(params, (UINT *)&timeOut);
if(params != nilCell)
		netEvalIdle = params;
   
/* printf("timeout %d idle-loop %X\n", timeOut, netEvalIdle); */
 
/* collect data from host in each active session */
while(count)
    {
    resultStackIdxSave = resultStackIdx;
    if( (netStream = session->netStream) == NULL) 
        {
        session = session->next;
        if(session == NULL) session = base;
        continue;
        }
        
    start = milliSecTime();

    if(netEvalIdle) 
        {
        cell = getCell(CELL_EXPRESSION);
        cell->contents = (UINT)copyCell(netEvalIdle);
        pushResult(cell);
        if(!evaluateExpressionSafe(cell, &errNo))
			{
			freeSessions(base);
			longjmp(errorJump, errNo);
			}
        }

    bytes = -1;
    ready = wait_ready(session->sock, 100, READY_READ);
    if(ready > 0)
        {
        memset(buffer, 0, MAX_BUFF);
        bytes = recv(session->sock, buffer, MAX_BUFF, NO_FLAGS_SET);
        if(bytes) writeStreamStr(netStream, buffer, bytes);
        }
    if(ready < 0 || bytes == 0 || elapsed >= timeOut)
        {
/*        printf("count=%d ready=%d bytes=%d elapsed=%d\n", count, ready, bytes, elapsed); */
        if(elapsed >= timeOut) result = copyCell(nilCell); 
        else if(rawMode) /* get raw buffer without the quote */
            result = stuffStringN(netStream->buffer + 1, netStream->position - 1);
        else 
            {
            makeStreamFromString(&evalStream, netStream->buffer);
            memcpy(errorJumpSave, errorJump, sizeof(jmp_buf));
            if((errNo = setjmp(errorJump)) != 0)
                {
                memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
                freeSessions(base);
                longjmp(errorJump, errNo);
                }
            result = evaluateStream(&evalStream, 0, TRUE);
            memcpy(errorJump, errorJumpSave, sizeof(jmp_buf));
            }

        if(netEvalIdle)
            {
            session->result = cell = getCell(CELL_EXPRESSION);
            cell->contents = (UINT)stuffString(session->host);
            cell = (CELL *)cell->contents;
            cell->next = stuffInteger(session->port);
            cell = cell->next;
            cell->next = result;
            }
        else
            session->result = result;
            
        closeStrStream(netStream);
        deleteInetSession(session->sock);
        close(session->sock);
        free(netStream);
        session->netStream = NULL;
            
        if(netEvalIdle)
            {
            list = getCell(CELL_EXPRESSION);
            list->contents = (UINT)copyCell(netEvalIdle);
            cell = getCell(CELL_QUOTE);
            cell->contents = (UINT)session->result;
            ((CELL*)list->contents)->next = cell;
            pushResult(list);
        	if(!evaluateExpressionSafe(list, &errNo))
				{
				freeSessions(base);
				longjmp(errorJump, errNo);
				}
            }

        count--;
        }
        
    /* check for rollover at midnight */
    if(milliSecTime() >= start) 
        elapsed += milliSecTime() - start;
    else 
        elapsed += milliSecTime();

    session = session->next;
    if(session == NULL) session = base;

    cleanupResults(resultStackIdxSave);
  }

/* free all sessions and configure result */
result = NULL;
while(base != NULL)
    {
    if(netEvalIdle == NULL)
        {
        if(result == NULL)
            {
			if(singleSession)
				result = base->result;
			else
				{
            	result = getCell(CELL_EXPRESSION);
            	result->contents = (UINT)base->result;
            	cell = base->result;
            	}
			}
        else
            {
            cell->next = base->result;
            cell = cell->next;
            }
        }
    session = base;
    base = base->next;
    free(session);
    }

if(netEvalIdle == NULL) return(result);
if(elapsed > timeOut) return(nilCell);
return(trueCell);
}    


void freeSessions(NETEVAL * base)
{
NETEVAL * session;

while(base != NULL)
    {
    if(base->netStream != NULL)
        {
        if(base->result != NULL)
          deleteList(base->result);
        closeStrStream(base->netStream);
        deleteInetSession(base->sock);
        close(base->sock);
        free(base->netStream);
        base->netStream = NULL;
        }
    session = base;
    base = base->next;
    free(session);
    }
}

/*********************** error handling ***************************************/

char * netErrorMsg[] =
    {
    "No error",
    "Cannot open socket",
    "Host name not known",
    "Not a valid service",
    "Connection failed",
    "Accept failed",
    "Connection closed",
    "Connection broken",
    "Socket recv failed",
    "Socket send failed",
    "Cannot bind socket",
    "Too much sockets in net-select",
    "Listen failed",
    "Badly formed IP",
    "Select failed",
    "Peek failed",
    "Not a valid socket"
    };


CELL * netError(int errorNo) 
{ 
errorIdx = errorNo; 
return(nilCell); 
} 

CELL * netEvalError(int errorNo)
{ 
errorIdx = errorNo; 
return(p_netLastError(NULL));
}

CELL * p_netLastError(CELL * params)
{
CELL * cell;
char str[64];

if(errorIdx == 0 || errorIdx > 16) return(nilCell);

cell = getCell(CELL_EXPRESSION);
cell->contents = (UINT)stuffInteger(errorIdx);
snprintf(str, 63, "ERR: %s", netErrorMsg[errorIdx]);
((CELL*)cell->contents)->next = stuffString(str);

return(cell);
}


#ifdef NET_PING
/* net-ping */

CELL * p_netPing(CELL * params)
{
CELL * address;
long timeout = 1000, listmode = 0;
long option = 0;
long count = -1;

address = evaluateExpression(params);
if(address->type == CELL_EXPRESSION)
  {
  address = (CELL *)address->contents;
  listmode = 1;
  }
else if(address->type != CELL_STRING)
  return(errorProcExt(ERR_LIST_OR_STRING_EXPECTED, address));

params = params->next;
if(params != nilCell)  
  {
  params = getInteger(params, (UINT *)&timeout);
  if(params != nilCell)
    params = getInteger(params, (UINT*)&count);
  if(params != nilCell)
    getInteger(params, (UINT*)&option);
  }

return(ping(address, (int)timeout, (int)listmode, (int)count, (int)option));
}


CELL * ping(CELL * address, int maxwait, int listmode, int count, int option)
{
char * host;
char * hostaddr = NULL;
struct hostent *hp;
struct sockaddr_in whereto;
struct sockaddr_in from;
struct protoent *proto;
int s, sockopt;
#ifdef WIN_32
int fromlen;
#else
#ifdef TRU64
unsigned long fromlen;
#else
#ifdef OS2
int fromlen;
#else
unsigned int fromlen;
#endif
#endif
#endif
unsigned char packet[64];
struct ip *ip;
struct icmp *icp = (struct icmp *) packet;
int  cc, broadcast = 0, batchmode = 0, wildcard = 0;
size_t len;
struct timeval tv, tp;
CELL * result = NULL;
CELL * cell = nilCell;

batchmode = listmode;

proto = getprotobyname("icmp");
if ((s = socket(AF_INET, SOCK_RAW, (proto ? proto->p_proto : 1))) < 0) 
    return(netError(ERR_INET_OPEN_SOCKET));
    
sockopt = 1;
setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *) &sockopt, sizeof(sockopt));

sockopt = 48 * 1024;
setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &sockopt, sizeof(sockopt));

if(option)
  {
  sockopt = 1;
  setsockopt(s, SOL_SOCKET, option, (char *)&sockopt, sizeof(sockopt));
  }

/* for each IP */
while(address != nilCell)
  {
  if(address->type != CELL_STRING)
    {
    shutdown(s, SHUT_RDWR);
    return(errorProcExt(ERR_STRING_EXPECTED, address));
    }

    host = (char *)address->contents;
    len = address->aux - 1;
    memset((char *)&whereto, 0, sizeof(struct sockaddr));
    whereto.sin_family = AF_INET;
    if(strncmp(host + len - 2, ".*", 2) == 0)
        wildcard = 254;
WILDCARD:    
    if(wildcard)
        {
        batchmode = TRUE;
        if(hostaddr == NULL) hostaddr = alloca(len + 1);
        memcpy(hostaddr, host, len);
        snprintf(hostaddr + len - 1, 4, "%d", wildcard--);
        }
    else
        hostaddr = host;
        
    
    if((whereto.sin_addr.s_addr = inet_addr(hostaddr)) == (unsigned)-1)
      {
      if(!(hp = gethostbyname(hostaddr)))
        {
        shutdown(s, SHUT_RDWR);
        return(netError(ERR_INET_HOST_UNKNOWN));
        }
      whereto.sin_family = hp->h_addrtype;
      memcpy((void *)&whereto.sin_addr, hp->h_addr, hp->h_length);
      }
      
    broadcast = ((whereto.sin_addr.s_addr & 0x000000ff) == 255);

    /* ping */
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = 0;
    icp->icmp_id = getpid() & 0xFFFF;

    cc = 64;            /* 8 + 56 data */

    icp->icmp_cksum = in_cksum((unsigned short *) icp, cc );

    gettimeofday(&tv, NULL );

    wait_ready(s, maxwait * 500, READY_WRITE);
    if(cc !=  sendto( s, packet, cc, 0,(struct sockaddr *)&whereto, sizeof(struct sockaddr) ))
        {
        shutdown(s, SHUT_RDWR);    
        return(netError(ERR_INET_WRITE));
        }
    if(!listmode && !wildcard) break;
    if(wildcard) goto WILDCARD;
    address = address->next;
    }
    /* else printf("->%s\n", hostaddr); */


/* wait for response(s) */
for (;;) {
    len = sizeof (packet);
    memset(packet, 0, len);

    fromlen = sizeof (from);
    
    if(wait_ready(s, 1000, READY_READ) <= 0)
        {
        gettimeofday(&tp, NULL);
        if(timediff(tp, tv) > maxwait) break;
        continue;
        }    

    if ( (cc=recvfrom(s, packet, len, 0, (struct sockaddr *)&from, &fromlen)) < 0)
        continue;
    
/*    from.sin_addr.s_addr = ntohl( from.sin_addr.s_addr ); */
    ip = (struct ip *) packet;
    len = ip->ip_hl << 2;
    icp = (struct icmp *)(packet + len);

    if(icp->icmp_id != (getpid() & 0xFFFF)) continue;    

        if(result == NULL)
          {
          result = getCell(CELL_EXPRESSION);
          cell = stuffString(inet_ntoa(from.sin_addr));
          result->contents = (UINT)cell;
          }
        else
          {
          cell->next = stuffString(inet_ntoa(from.sin_addr));
          cell = cell->next;
          }
        if(--count == 0) break;
    if( !(broadcast || batchmode) ) break;
    }
    
shutdown(s, SHUT_RDWR);    
errorIdx = 0;

return(result == NULL ? getCell(CELL_EXPRESSION) : result);
}

int in_cksum(unsigned short * addr, int len)
{
int nleft = len;
unsigned short *w = addr;
unsigned short  answer;
int sum = 0;

while( nleft > 1 )  {
    sum += *w++;
    nleft -= 2;
    }

if( nleft == 1 ) {
    u_short    u = 0;
    *(unsigned char *)(&u) = *(unsigned char *)w ;
    sum += u;
    }

sum = (sum >> 16) + (sum & 0xffff);
sum += (sum >> 16);
answer = ~sum;
return (answer);
}

#endif /* NET_PING */


/* check socket for readability or error */
int wait_ready(int sock, INT64 wait, int mode)
{
struct timeval timeOut;
fd_set socketSet;

FD_ZERO(&socketSet);
FD_SET(sock, &socketSet);

timeOut.tv_sec = wait/1000000;
timeOut.tv_usec = wait - timeOut.tv_sec * 1000000;

if(mode == READY_READ)
    return(select(FD_SETSIZE, &socketSet, NULL, &socketSet, &timeOut));
else
    return(select(FD_SETSIZE, NULL, &socketSet, &socketSet, &timeOut));
}


/* ----------------------------- socket->filestream stuff for win32 ------------------------*/

#ifdef WINCC

/*
These functions use the FILE structure to store the raw file handle in '->_file' and
set ->_flag to 0xFFFF, to identify this as a faked FILE structure.
*/

FILE * win32_fdopen(int handle, const char * mode)
{
FILE * fPtr;

if((fPtr = (FILE *)malloc(sizeof(FILE))) == NULL)
    return(NULL);

memset(fPtr, 0, sizeof(FILE));

#ifdef WINCE
fPtr->_file = handle;
fPtr->_flags = 0xFF;
#endif

#ifdef MINGW
fPtr->_file = handle;
fPtr->_flag = 0xFFFF;
#endif

return(fPtr);
}

int win32_fclose(FILE * fPtr)
{
if(isSocketStream(fPtr))
   return(close(getSocket(fPtr)));


return(fclose(fPtr));
}


/* for a full fprintf with format string and parameters
   see version previous to 9.0.2
*/
int win32_fprintf(FILE * fPtr, char * buffer)
{
int pSize;

if(!isSocketStream(fPtr))
    return(fprintf(fPtr, buffer));

pSize = strlen(buffer);

if((pSize = send(getSocket(fPtr), buffer, pSize, NO_FLAGS_SET)) == SOCKET_ERROR)
     {
     close(getSocket(fPtr));
     return(-1);
     }

return(pSize);
}

int win32_fgetc(FILE * fPtr)
{
char chr;

if(!isSocketStream(fPtr))
    return(fgetc(fPtr));

if(recv(getSocket(fPtr), &chr, 1, NO_FLAGS_SET) <= 0)
    {
    close(getSocket(fPtr));
    return(-1);
    }

return(chr);
}


char * win32_fgets(char * buffer, int  size, FILE * fPtr)
{
int bytesReceived = 0;
char chr;

if(!isSocketStream(fPtr))
    return(fgets(buffer, size - 1, fPtr));

while(bytesReceived < size)
    {
    if(recv(getSocket(fPtr), &chr, 1, NO_FLAGS_SET) <= 0)
        {
        close(getSocket(fPtr));
        return(NULL);
        }

    *(buffer + bytesReceived++) = chr;

    if(chr == '\n') break;
    }

*(buffer + bytesReceived) = 0;

return(buffer);
}

#endif

/* eof */
