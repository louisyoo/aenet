
#ifndef __AE_SERVER_H__
#define __AE_SERVER_H__
#include <time.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h> 
#include "ae.h"
#define MAXFD 1024
#define WORKER_PROCESS_COUNT 3



struct aeEventLoop;
typedef struct UserClient
{
  int flags;
  int fd;
  char recv_buffer[10240];
  int  read_index;
  char* client_ip;
  int client_port;
}userClient;


typedef struct _workerInfo
{
    pid_t pid;
    int pipefd[2];
}workerInfo;

typedef struct _workerBase
{
	int   pidx; //主进程中分的编号0-x
	pid_t pid;
	int pipefd;
	int running;
	aeEventLoop *el;
	int maxClient;
	//clientList.. zlist
}aWorkerBase;

typedef struct _aEventBase
{
	int listenfd;
	int listenIPv6fd;
	int evfd; //epollfd
	pid_t pid;
	int usable_cpu_num;
	
	aeEventLoop *el; 
	int sig_pipefd[2];
	workerInfo worker_process[ WORKER_PROCESS_COUNT ];
	int running;
        int worker_process_counter;
}aEventBase;

typedef struct _userClient
{
  int flags;
  int fd;
  char recv_buffer[10240];
  int  read_index;
  char* client_ip;
  int client_port;
}userClient;


typedef struct _aeServer
{
   void (*runForever )( char* ip, int port );
   void (*onConnect)( aeServer *serv, int fd , int fromid );
   int  (*onReceive)( aeServer *serv, userClient* client );
   void (*onClose)( aeServer *serv , int fd );
   void (*send)( aeServer *serv, int fd, char* data , int len );
   void (*close)( aeServer *serv, int fd );
}aeServer;

void initOnLoopStart( aeEventLoop *el );
void onReadableEvent(aeEventLoop *el, int fd, void *privdata, int mask);
void installWorkerProcess();
void runMasterLoop();
void masterSignalHandler( int sig );
void installMasterSignal( aeEventLoop *l );
void initServerBase();
int startServer( char* ip , int port );

//=============child process============

userClient *newClient( aeEventLoop *el , int fd);
void readFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
void initWorkerOnLoopStart( aeEventLoop *l);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
//======
userClient* newClient( aeEventLoop *el , int fd);
void freeClient( userClient* c  );
//======
void onRecv( userClient *c , int len );
void onClose( userClient *c );
//int onTimer(struct aeEventLoop *l,long long id,void *data);
//======
void acceptCommonHandler( aeEventLoop *el,int fd,char* client_ip,int client_port, int flags);
void readFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
int timerCallback(struct aeEventLoop *l,long long id,void *data);
void finalCallback( struct aeEventLoop *l,void *data );
void addSignal( int sig, void(*handler)(int), int restart );
void runWorkerProcess( int pidx ,int pipefd );


aEventBase aEvBase;
aWorkerBase aWorker;
#endif
