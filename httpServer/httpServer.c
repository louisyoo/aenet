
//https://github.com/lchb369/Aenet.git

#include <stdio.h>
#include "httpServer.h"
#include "server.h"

//这是个全局变量
requestHeaderField  requestHeaderFeilds[] = {
    { "Host", "", 				aeHttpProcessHost },
    { "Connection", "", 		aeHttpProcessConnection },
    { "If-Modified-Since", "",	aeHttpProcessUniqueHeaderLine },
    { "If-Unmodified-Since","",	aeHttpProcessUniqueHeaderLine },
	{ "If-Match","",			aeHttpProcessUniqueHeaderLine }, //单一行
	{ "If-None-Match","",		aeHttpProcessUniqueHeaderLine }, 
	{ "User-Agent","",			aeHttpProcessUserAgent },
	{ "Referer","",				aeHttpProcessHeaderLine }, 
	{ "Content-Length","",		aeHttpProcessUniqueHeaderLine }, 
	{ "Content-Type","",		aeHttpProcessHeaderLine },
	{ "Range","",				aeHttpProcessHeaderLine },
	{ "If-Range","",			aeHttpProcessUniqueHeaderLine }, 
	{ "Transfer-Encoding","",	aeHttpProcessHeaderLine },
	{ "Expect","",				aeHttpProcessUniqueHeaderLine }, 
	{ "Upgrade","",				aeHttpProcessHeaderLine },
#if (AE_HTTP_GZIP)
	{ "Accept-Encoding","",		aeHttpProcessHeaderLine },
	{ "Via","",					aeHttpProcessHeaderLine },
#endif
    { "Authorization","",		aeHttpProcessUniqueHeaderLine},
	{ "Keep-Alive","",			aeHttpProcessHeaderLine },
#if (AE_HTTP_X_FORWARDED_FOR)
    { "X-Forwarded-For","",		aeHttpProcessMultiHeaderLines },
#endif

#if (AE_HTTP_REALIP)
    { "X-Real-IP","", 			aeHttpProcessHeaderLine },
#endif

#if (AE_HTTP_HEADERS)
	{ "Accept","", 				aeHttpProcessHeaderLine },
	{ "X-Real-IP","", 			aeHttpProcessHeaderLine },
	{ "Accept-Language","", 	aeHttpProcessHeaderLine },
#endif

#if (AE_HTTP_DAV)
	{ "Depth","", 				aeHttpProcessHeaderLine },
	{ "Destination","", 		aeHttpProcessHeaderLine },
	{ "Overwrite","", 			aeHttpProcessHeaderLine },
	{ "Date","", 				aeHttpProcessHeaderLine },
#endif
	{ "Cookie","",				aeHttpProcessMultiHeaderLines },
    { "", "", NULL }
};


int httpResponse(  userClient *c ,char* data,int len )
{
	 int sendlen;
        // sendlen = serv->send( c->fd , data , len );
	 return sendlen;
}

/*
1,数组存储一个key-val结构体,表示header头的，键值对，参考nginx
2,先检测header是否收全，找\r\n\r\n 换行符。从头往后找。
*/
int parseRequestProtocol()
{
    char *buf = aeRequest->sock->recv_buffer;
    char *pe = buf + aeRequest->sock->recv_length;
	
    //http method
    if (memcmp(buf, "GET", 3) == 0)
    {
        aeRequest->method = HTTP_METHOD_GET;
        aeRequest->offset = 4;
        buf += 4;
    }
    else if (memcmp(buf, "POST", 4) == 0)
    {
        aeRequest->method = HTTP_METHOD_POST;
        aeRequest->offset = 5;
        buf += 5;
    }
    else if (memcmp(buf, "PUT", 3) == 0)
    {
        aeRequest->method = HTTP_METHOD_PUT;
        aeRequest->offset = 4;
        buf += 4;
    }
    else if (memcmp(buf, "PATCH", 5) == 0)
    {
        aeRequest->method = HTTP_METHOD_PATCH;
        aeRequest->offset = 6;
        buf += 6;
    }
    else if (memcmp(buf, "DELETE", 6) == 0)
    {
        aeRequest->method = HTTP_METHOD_DELETE;
        aeRequest->offset = 7;
        buf += 7;
    }
    else if (memcmp(buf, "HEAD", 4) == 0)
    {
        aeRequest->method = HTTP_METHOD_HEAD;
        aeRequest->offset = 5;
        buf += 5;
    }
    else if (memcmp(buf, "OPTIONS", 7) == 0)
    {
        aeRequest->method = HTTP_METHOD_OPTIONS;
        aeRequest->offset = 8;
        buf += 8;
    }
    else
    {
        return AE_ERR;
    }

    //http version
    char *p;
    char cmp = 0;
    for (p = buf; p < pe; p++)
    {
        if (cmp == 0 && *p == AE_SPACE)
        {
            cmp = 1;
        }
        else if (cmp == 1)
        {
            if (p + 8 > pe)
            {
                return AE_ERR;
            }
            if (memcmp(p, "HTTP/1.1", 8) == 0)
            {
                aeRequest->version = HTTP_VERSION_11;
                break;
            }
            else if (memcmp(p, "HTTP/1.0", 8) == 0)
            {
                aeRequest->version = HTTP_VERSION_10;
                break;
            }
            else
            {
                return AE_ERR;
            }
        }
    }
    p += 8;
    aeRequest->offset += 8;
    return AE_OK;
}


int parseRequestContentLength( httpRequest *request)
{
    char *buffer = request->sock->recv_buffer;
    char *buf = buffer + request->offset;
	
    int len = request->sock->recv_length - request->offset;
    char *pe = buf + len;
    char *p;

    for (p = buf; p < pe; p++)
    {
        if (*p == '\r' && *(p + 1) == '\n')
        {
			//int strncasecmp(const char *s1, const char *s2, size_t n)
            if (strncasecmp(p + 2, AE_STRL("Content-Length") - 1) == 0)
            {
                return AE_TRUE;
            }
            else
            { 
                p++;
            }
        }
    }
    return AE_FALSE;
}


void httpRequestParseHeader(  httpRequest* request )
{
	if( request->method == HTTP_METHOD_NONE && parseRequestProtocol() == AE_ERR )
	{
		  printf( "parseRequestProtocol not complate .. \n" );
		if( request->sock->recv_length < AE_HTTP_HEADER_MAX_SIZE )
		{
			return;//continue recv
		}

		//TODO::02考虑此处发异常响应包给客户端

		//断开连接，释放内存
		printf( "parseRequestProtocol error .. \n" );
		closeHttpConnect( aeRequest->sock );
		return;
	}
	
	printf("Request->method=%d and Request->version=%d \n", request->method , request->version );
	//DELETE
	if (request->method == HTTP_METHOD_DELETE)
	{
		if (request->content_length == 0 && parseRequestContentLength(request) == AE_FALSE )
		{
			//goto http_no_entity;
		}
		else
		{
			//goto http_entity;
		}
	}
	//GET HEAD OPTIONS
	/*
	else if (request->method == HTTP_METHOD_GET || request->method == HTTP_METHOD_HEAD || request->method == HTTP_METHOD_OPTIONS)
	{
		http_no_entity:
		//意思是如果 包头接收完了，倒数4个字符是包头部结束符。
		if (memcmp(buffer->str + buffer->length - 4, "\r\n\r\n", 4) == 0)
		{
			swReactorThread_dispatch_string_buffer(conn, buffer->str, buffer->length);
			swHttpRequest_free(conn);
		}
		else if (buffer->size == buffer->length)
		{
			swWarn("http header is too long.");
			goto close_fd;
		}
		//wait more data
		else
		{
			goto recv_data;
		}
	}
	//POST PUT HTTP_PATCH
	else if (request->method == HTTP_POST || request->method == HTTP_PUT || request->method == HTTP_PATCH)
	{
		http_entity:
		if (request->content_length == 0)
		{
			if (swHttpRequest_get_content_length(request) < 0)
			{
				if (buffer->size == buffer->length)
				{
					swWarn("http header is too long.");
					goto close_fd;
				}
				else
				{
					goto recv_data;
				}
			}
			else if (request->content_length > protocol->package_max_length)
			{
				swWarn("content-length more than the package_max_length[%d].", protocol->package_max_length);
				goto close_fd;
			}
		}

		uint32_t request_size = 0;

		//http header is not the end
		if (request->header_length == 0)
		{
			if (buffer->size == buffer->length)
			{
				swWarn("http header is too long.");
				goto close_fd;
			}
			if (swHttpRequest_get_header_length(request) < 0)
			{
				goto recv_data;
			}
			request_size = request->content_length + request->header_length;
		}
		else
		{
			request_size = request->content_length + request->header_length;
		}

		if (request_size > buffer->size && swString_extend(buffer, request_size) < 0)
		{
			goto close_fd;
		}

		//discard the redundant data
		if (buffer->length > request_size)
		{
			buffer->length = request_size;
		}

		if (buffer->length == request_size)
		{
			swReactorThread_dispatch_string_buffer(conn, buffer->str, buffer->length);
			swHttpRequest_free(conn);
		}
		else
		{
#ifdef SW_HTTP_100_CONTINUE
			//Expect: 100-continue
			if (swHttpRequest_has_expect_header(request))
			{
				swSendData _send;
				_send.data = "HTTP/1.1 100 Continue\r\n\r\n";
				_send.length = strlen(_send.data);

				int send_times = 0;
				direct_send:
				n = swConnection_send(conn, _send.data, _send.length, 0);
				if (n < _send.length)
				{
					_send.data += n;
					_send.length -= n;
					send_times++;
					if (send_times < 10)
					{
						goto direct_send;
					}
					else
					{
						swWarn("send http header failed");
					}
				}
			}
			else
			{
				swTrace("PostWait: request->content_length=%d, buffer->length=%zd, request->header_length=%d\n",
						request->content_length, buffer->length, request->header_length);
			}
#endif
			goto recv_data;
		}
	}
	else
	{
		swWarn("method no support");
		goto close_fd;
	}
	
	*/
	

return AE_OK;
}

int requestHeaderHasComplete()
{
	int n = strnpos( aeRequest->sock->recv_buffer, aeRequest->sock->recv_length , "\r\n\r\n" , 4 );
	if( n < 0 )
	{
		return AE_FALSE;
	}
	
	if( n > AE_HTTP_HEADER_MAX_SIZE )
	{
		closeHttpConnect( aeRequest->sock );
		return AE_FALSE;
	}
	
	aeRequest->header_length = n + 4;
	return AE_TRUE;
}

//子进程
void onHttpRequest( aeServer* serv , userClient *c , int len )
{
     printf( "httpRequest Length:%d Data:\n%s" , len , c->recv_buffer  );
	 if( aeRequest->sock == NULL )aeRequest->sock=c;
	  
	 if( requestHeaderHasComplete() == AE_FALSE )
	 {
		 AE_RETURN_FOR_CONTINUE_RECEIVE;
	 }
	 httpRequestParseHeader( aeRequest );
	 
	 closeHttpConnect( c );	
//	 int sendlen;
//	 sendlen = httpResponse( c ,  c->recv_buffer , strlen( c->recv_buffer ) );
}


void closeHttpConnect( userClient *c )
{
	 printf( "Worker Client closed  = %d  \n", c->fd );
	 
	 //关闭socket userClient
     aeHttpServ.sock->close( c );
	 //TODO::01...这里可以不用施放,memset清空就可以了，
	 //但是相应的分配内存时，在main之前就分配,否则会内存泄露。
	 if( aeRequest )
	 zfree( aeRequest );
}

void onHttpClose( aeServer* serv ,userClient *c )
{
    closeHttpConnect( c );
}

void onHttpConnect( aeServer* serv , userClient *c )
{
	 aeRequest = zmalloc( sizeof( httpRequest ));
	 aeRequest->sock = c;
	 aeRequest->method = HTTP_METHOD_NONE;
	 aeRequest->content_length = 0;
	 
     printf( "New Client Connected fd =%d \n" , c->fd );
}


void initHttpServer( aeServer* serv  )
{
	 bzero( &aeHttpServ , sizeof( aeHttpServ ));
	 
	 aeRequest = NULL;
     aeHttpServ.open_http_protocol = 1;
	 aeHttpServ.open_websocket_protocol = 0;
	 aeHttpServ.sock = serv;
	 
	 serv->onConnect = &onHttpConnect;
	 serv->onRecv = &onHttpRequest;
	 serv->onClose = &onHttpClose;
}

void runHttpServer( char* ip , int port )
{
	 aeHttpServ.listen_ip = ip;
	 aeHttpServ.listen_port = port;
	 
	 aeServer* serv = aeServerCreate();  
     initHttpServer( serv );
	 serv->runForever( ip , port );
}


int main( int argc,char *argv[] )
{
     runHttpServer( "0.0.0.0" , 3002 );
     return 0;
}
