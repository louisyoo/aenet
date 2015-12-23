
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

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = 10;
    char cip[46];
    char neterr[1024];
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

//监听信号事件,中断退出时要相应处理保存等操作



int main(int argc,char *argv[])
{
	aeEventLoop *l; 
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

	l = aeCreateEventLoop( 1024 );
	aeSetBeforeSleepProc(l,loop_init);
	int res;

	res = aeCreateFileEvent(l,sockfd[0],AE_READABLE,acceptTcpHandler,user_data);
	printf("create file event is ok? [%d]\n",res==0 );

	res = aeCreateTimeEvent(l,5*1000,time_cb,NULL,fin_cb);
	printf("create time event is ok? [%d]\n",!res);

	aeMain(l);

	aeDeleteEventLoop(l);
	puts("Everything is ok !!!\n");
	return 0;
}
