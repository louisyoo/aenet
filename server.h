
#ifndef __AE_SERVER_H__
#define __AE_SERVER_H__
#include <time.h>

#define MAXFD 1024
#define WORKER_PROCESS_COUNT 3

extern aEventBase aEvBase;
extern aWorkerBase aWorkerBase;

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
	bool running;
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
	bool running;
    int worker_process_counter;
}aEventBase;


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
void addSignal( int sig, void(*handler)(int), bool restart = true );
void runWorkerProcess( int pidx ,int pipefd );

#endif
