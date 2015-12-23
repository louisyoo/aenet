
#ifndef __AE_HTTP_SERVER_H__
#define __AE_HTTP_SERVER_H__


#include "server.h";

#define AE_START_LINE  "-------------------------START----------------------------"
#define AE_END_LINE    "-------------------------END------------------------------"
#define AE_SPACE       ' '
#define AE_CRLF        "\r\n"
#define AE_OK        0
#define AE_ERR       -1
#define AE_TRUE        1
#define AE_FALSE       0
#define AE_HTTP_HEADER_MAX_SIZE          8192
#define AE_STRL(s)             s, sizeof(s)

enum httpMethod
{
	HTTP_METHOD_NONE = 0,
        HTTP_METHOD_DELETE, 
	HTTP_METHOD_GET, 
	HTTP_METHOD_HEAD, 
	HTTP_METHOD_POST, 
	HTTP_METHOD_PUT, 
	HTTP_METHOD_PATCH,
	HTTP_METHOD_OPTIONS,
};

enum httpVersion
{
    HTTP_VERSION_10 = 1,
    HTTP_VERSION_11,
};


typedef struct _httpServer
{
    char* listen_ip;
    int   listen_port;
    int open_http_protocol;
    int open_websocket_protocol;
    aeServer* sock;
	//server envar..	
}httpServer;



typedef struct _httpRequest
{
    int method;
    int offset;
    int version;
    int free_memory;
    int header_length;
    int content_length;
    userClient* sock;
	
}httpRequest;


httpServer aeHttpServ;
httpRequest* aeRequest; 

#endif
