
//https://github.com/lchb369/Aenet.git

#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include "ae.h"
#include <errno.h>
#include <sys/socket.h>
#include "server.h"

extern aEvBase;

//======================
void initWorkerOnLoopStart( aeEventLoop *l) 
{
    puts("Event Loop Init!!! \n");
}

//子进程接收请求。。
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int client_port, client_fd, max = 10;
    char client_ip[46];
    char neterr[1024];

    while(max--)
	{
        client_fd = anetTcpAccept( neterr, fd, client_ip, sizeof(client_ip), &client_port );
        if (client_fd == -1 ) {
            if (errno != EWOULDBLOCK)
                    printf("Accepting client connection: %s \n", neterr);
            return;
        }
        printf("Accepted %s:%d client_fd=%d \n", client_ip, client_port,client_fd );
        acceptCommonHandler( el ,client_fd,client_ip,client_port,0 );
    }
}

//子进程中
void acceptCommonHandler( aeEventLoop *el,int fd,char* client_ip,int client_port, int flags)
{
    userClient* c;
    if ((c = newClient( el ,fd)) == NULL)
    {
       	printf( "Error registering fd event for the new client\n" );
        close(fd); /* May be already closed, just ignore errors */
        return;
    }
	c->client_ip = client_ip;
	c->client_port = client_port;
    c->flags |= flags;
}

//子进程中
userClient* newClient( aeEventLoop *el , int fd)
{
    userClient *c = zmalloc(sizeof(userClient));
    if (fd != -1) {
        anetNonBlock(NULL,fd);
        anetEnableTcpNoDelay(NULL,fd);
        /*
        if (server.tcpkeepalive)
            anetKeepAlive(NULL,fd,server.tcpkeepalive);
	*/
        if (aeCreateFileEvent( el,fd,AE_READABLE,readFromClient, c) == -1 )
        {
            printf( "createFileEvent error fd =%d  \n" ,fd );
            close(fd);
            zfree(c);
            return NULL;
        }
    }

    c->flags = 0;
    c->fd = fd;
    c->read_index = 0;
    return c;
}

//子进程中
void freeClient( userClient* c  )
{
   if (c->fd != -1)
   {
		aeDeleteFileEvent( aWorker.el,c->fd,AE_READABLE);
		aeDeleteFileEvent( aWorker.el,c->fd,AE_WRITABLE);
		close(c->fd);
   } 
   zfree(c); 
}

//子进程
void onRecv( userClient *c , int len )
{
     printf( "recv len = %d \n" , len  );
     printf( "on_read recv data = %s \n" , c->recv_buffer );

     char buff[64] = "i recv your message:\n";
     anetWrite( c->fd , buff , sizeof( buff ));
     int sendlen = anetWrite( c->fd , c->recv_buffer , strlen( c->recv_buffer ));
}

void onClose( userClient *c )
{
     printf( "Client closed  = %d  \n", c->fd );
     freeClient( c );
}

void readFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    userClient *c = (userClient*) privdata;
    int nread, readlen,bufflen;
    readlen = 1024;
    memset( c->recv_buffer , 0 , sizeof(c->recv_buffer) );

    //if there use "read" need loop
    nread = recv(fd, c->recv_buffer , readlen , MSG_WAITALL );
    if (nread == -1) {
        if (errno == EAGAIN) {
            nread = 0;
        } else {
			onClose( c );
            return;
        }
    } else if (nread == 0) {
        onClose( c );
        return;
    }
    onRead( c ,nread );
}


int timerCallback(struct aeEventLoop *l,long long id,void *data)
{
	printf("now is %ld\n",time(NULL));
	printf("I'm time_cb,here [EventLoop: %p],[id : %lld],[data: %p] \n",l,id,data);
	return 5*1000;
}

void finalCallback(struct aeEventLoop *l,void *data)
{
    puts("call the unknow final function \n");
}


void addSignal( int sig, void(*handler)(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}



void childTermHandler( int sig )
{
    aWorker.running = false;
}

void childChildHandler( int sig )
{
    pid_t pid;
    int stat;
    while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
    {
        continue;
    }
}


/**
 * process event types:
 * 1,parent process send readable event
 * 2,process recv/send event 
*/
void runWorkerProcess( int pidx ,int pipefd )
{
	//每个进程私有的。
	aWorker.pid = getpid();
	aWorker.maxClient=1024;
	aWorker.pidx = pidx;
	aWorker.pipefd = pipefd;
	aWorker.running = true;
	
	//这里要安装信号接收器..
	addSignal( SIGTERM, childTermHandler, false );
    addSignal( SIGCHLD, childChildHandler );
	
	aWorker.el = aeCreateEventLoop( aWorker.maxClient );
	
	aeSetBeforeSleepProc( aWorker.el,initWorkerOnLoopStart );
	int res;

	//等待父进程管道通知有新连接到来,所以关注管道
	res = aeCreateFileEvent( aWorker.el,
		aWorker.pipefd,
		AE_READABLE,
		acceptTcpHandler,NULL
	);
	printf("create file event is ok? [%d]\n",res==0 );
	
	//定时器
	//res = aeCreateTimeEvent(el,5*1000,timerCallback,NULL,finalCallback);
	//printf("create time event is ok? [%d]\n",!res);

	aeMain(aWorker.el);
	aeDeleteEventLoop(aWorker.el);
}
