
//https://github.com/lchb369/Aenet.git

#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include "ae.h"
#include <errno.h>
#include <sys/socket.h>

#define MAXFD 1024


typedef struct UserClient
{
  int flags;
  int fd;
  char recv_buffer[10240];
  int  read_index;
}userClient;

void acceptCommonHandler( aeEventLoop *el , int fd, int flags);
userClient *newClient( aeEventLoop *el , int fd);
void readFromClient(aeEventLoop *el, int fd, void *privdata, int mask);

void loop_init(struct aeEventLoop *l) 
{
    puts("I'm loop_init!!! \n");
}

//当监听的listenfd有事件发生时，
void onReadableEvent(aeEventLoop *el, int fd, void *privdata, int mask)
{
	if( fd == el->listenfd )
	{
		//此处发送给子进程
		char neterr[1024];
		static int worker_process_counter = 0; 
		int new_conn = 1;
		send( worker_process[worker_process_counter++].pipefd[0], ( char* )&new_conn, sizeof( new_conn ), 0 );
		printf( "send request to child %d\n", worker_process_counter-1 );
		worker_process_counter %= WORKER_PROCESS_COUNT;
	}
	//如果收到信号事件
	else if( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
	{	
		int sig;
		char signals[1024];
		ret = recv( sig_pipefd[0], signals, sizeof( signals ), 0 );
		if( ret == -1 )
		{
			continue;
		}
		else if( ret == 0 )
		{
			continue;
		}
		else
		{
			for( int i = 0; i < ret; ++i )
			{
				switch( signals[i] )
				{
					case SIGCHLD:
					{
						pid_t pid;
						int stat;
						while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
						{
							for( int i = 0; i < WORKER_PROCESS_COUNT; ++i )
							{
								if( worker_process[i].pid == pid )
								{
									close( worker_process[i].pipefd[0] );
									worker_process[i].pid = -1;
								}
							}
						}
						stop_server = true;
						for( int i = 0; i < WORKER_PROCESS_COUNT; ++i )
						{
							if( worker_process[i].pid != -1 )
							{
								stop_server = false;
							}
						}
						break;
					}
					case SIGTERM:
					case SIGINT:
					{
						printf( "kill all the clild now\n" );
						for( int i = 0; i < WORKER_PROCESS_COUNT; ++i )
						{
							int pid = worker_process[i].pid;
							kill( pid, SIGTERM );
						}
						break;
					}
					default:
					{
						break;
					}
				}
			}
		}
		 
	}
	return;
}



//子进程接收请求。。
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = 10;
    char cip[46];
    char neterr[1024];
	static int worker_process_counter = 0; 
	
    while(max--) {
		
        cfd = anetTcpAccept( neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == -1 ) {
            if (errno != EWOULDBLOCK)
                    printf("Accepting client connection: %s \n", neterr);
            return;
        }
        printf("Accepted %s:%d cfd=%d \n", cip, cport,cfd );
        acceptCommonHandler( el ,cfd,0);
    }
}


void acceptCommonHandler( aeEventLoop *el , int fd, int flags)
{
    userClient* c;
    if ((c = newClient( el ,fd)) == NULL)
    {
       	printf( "Error registering fd event for the new client\n" );
        close(fd); /* May be already closed, just ignore errors */
        return;
    }
    c->flags |= flags;
}


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

void free_client( aeEventLoop *el , userClient* c  )
{
   if (c->fd != -1)
   {
		aeDeleteFileEvent( el,c->fd,AE_READABLE);
		aeDeleteFileEvent( el,c->fd,AE_WRITABLE);
		close(c->fd);
   } 
   zfree(c); 
}

void on_read( userClient *c , int len )
{
     printf( "recv len = %d \n" , len  );
     printf( "on_read recv data = %s \n" , c->recv_buffer );

     char buff[64] = "i recv your message:\n";
     anetWrite( c->fd , buff , sizeof( buff ));
     int sendlen = anetWrite( c->fd , c->recv_buffer , strlen( c->recv_buffer ));
}

void on_close( aeEventLoop *el , userClient *c )
{
     printf( "Client closed  = %d  \n", c->fd );
     free_client( el,c );
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
			on_close( el, c );
            return;
        }
    } else if (nread == 0) {
        on_close(el, c );
        return;
    }

    on_read( c ,nread );
}


int time_cb(struct aeEventLoop *l,long long id,void *data)
{
        printf("now is %ld\n",time(NULL));
        printf("I'm time_cb,here [EventLoop: %p],[id : %lld],[data: %p] \n",l,id,data);
        return 5*1000;

}

void fin_cb(struct aeEventLoop *l,void *data)
{
        puts("call the unknow final function \n");
}


//struct process_in_pool
#define WORKER_PROCESS_COUNT 3
struct worker_info
{
    pid_t pid;
    int pipefd[2];
};
worker_info worker_process[ WORKER_PROCESS_COUNT ];
int sig_pipefd[2];

//初始化子进程
int installWorkerProcess()
{
	char* neterr;
    for( int i = 0; i < WORKER_PROCESS_COUNT; ++i )
    {
        ret = socketpair( PF_UNIX, SOCK_STREAM, 0, worker_process[i].pipefd );
        assert( ret != -1 );
        worker_process[i].pid = fork();
        if( worker_process[i].pid < 0 )
        {
			//出错
            continue;
        }
        else if( worker_process[i].pid > 0 )
        {
			//父进程
            close( worker_process[i].pipefd[1] );
            anetNonBlock( neterr , worker_process[i].pipefd[0] );
            continue;
        }
        else
        {
			//子进程
            close( worker_process[i].pipefd[0] );
            anetNonBlock( neterr, worker_process[i].pipefd[1] );
            _runWorkerProcess( i );
            exit( 0 );
        }
    }
}

/*
 * 运行父进程
 *
*/
void runMasterLoop()
{
	aeEventLoop *l; 
	l = aeCreateEventLoop( 1024 );
	aeSetBeforeSleepProc(l,loop_init);
	int res;
	
	
	//监听定时事件
	installMasterSignal( l );

        res = aeCreateFileEvent(l,sockfd[0],AE_READABLE,onReadableEvent,user_data);
	printf("create file event is ok? [%d]\n",res==0 );
	//定时器事件
	res = aeCreateTimeEvent(l,5*1000,time_cb,NULL,fin_cb);
	printf("create time event is ok? [%d]\n",!res);

	aeMain(l);
	aeDeleteEventLoop(l);
}



void addsig( int sig, void(*handler)(int), bool restart = true )
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


//此处send是发给了主进程的event_loop，而不是发给子进程的。
void masterSignalHandler( int sig )
{
    int save_errno = errno;
    int msg = sig;
    send( sig_pipefd[1], ( char* )&msg, 1, 0 );
    errno = save_errno;
}


//这里的信号是server管理员执行的中断等操作引起的事件。
//所以此处的addEvent是加入到主进程event_loop中的。
void installMasterSignal( aeEventLoop *l )
{
	int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );
    assert( ret != -1 );
    anetNonBlock( sig_pipefd[1] );
	
	//把信号管道一端加到master event_loop中，使其被epoll关注
	//aeApiAddEvent( l, sig_pipefd[0], AE_READABLE );
	
	//TODO::
	res = aeCreateFileEvent(l,sig_pipefd[0],AE_READABLE,onReadableEvent,NULL);
	
	//装载信号，指定回调函数,如果用户引发信号事件，则回调。
    addsig( SIGCHLD, masterSignalHandler );
    addsig( SIGTERM, masterSignalHandler );
    addsig( SIGINT, masterSignalHandler );
    addsig( SIGPIPE, SIG_IGN );
	
    bool stop_server = false;
    int worker_process_counter = 0;
}

/**
 * 子进程:
 * 1,处理父进程传来的连接fd,并对其监听io事件
 * 2,处理客户端io事件
 * 3,处理父进程信号事件
*/
void _runWorkerProcess()
{
	aeEventLoop *l; 
	l = aeCreateEventLoop( 1024 );
	aeSetBeforeSleepProc(l,loop_init);
	int res;

	res = aeCreateFileEvent(l,sockfd[0],AE_READABLE,acceptTcpHandler,user_data);
	printf("create file event is ok? [%d]\n",res==0 );

	res = aeCreateTimeEvent(l,5*1000,time_cb,NULL,fin_cb);
	printf("create time event is ok? [%d]\n",!res);

	aeMain(l);
	aeDeleteEventLoop(l);
}



int main(int argc,char *argv[])
{
	
	char *msg = "Here std say:";
	char *user_data = malloc(50*sizeof(char));
	if(! user_data)
			assert( ("user_data malloc error",user_data) );
	memset(user_data,'\0',50);
	memcpy(user_data,msg,sizeof(msg));
	
	int sockfd[2];
	int sock_count = 0;          
	listenToPort( "0.0.0.0", 3002 , sockfd , &sock_count );
	printf( "listen count %d,listen fd %d \n",sock_count,sockfd[0] );

	//子进程操作都在这里。
	installWorkerProcess();
	
	//以下和子进程无关了
	runMasterLoop();

	puts("Everything is ok !!!\n");
	return 0;
}
