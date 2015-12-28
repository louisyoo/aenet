
#ifndef __AE_HTTP_SERVER_H__
#define __AE_HTTP_SERVER_H__


#include "server.h";

#define AE_SPACE       ' '
#define AE_CRLF        "\r\n"
#define AE_OK        0
#define AE_ERR       -1
#define AE_TRUE        1
#define AE_FALSE       0
#define AE_HTTP_HEADER_MAX_SIZE          8192
#define AE_STRL(s)             s, sizeof(s)
#define AE_RETURN_FOR_CONTINUE_RECEIVE            return
#define AE_BREAK_FOR_CONTINUE_RECEIVE             break

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


typedef struct {
    ngx_str_t                         name;
    ngx_uint_t                        offset;
    ngx_http_header_handler_pt        handler;
} ngx_http_header_t;


typedef void (*headerLineCallback)( requestHeaderField* this );
typedef struct
{
	char* name;
	char* value;
	headerLineCallback handle;
}requestHeaderField;

typedef struct
{
	int length;
	requestHeaderField fileds[30]; //ngx_http_request.c:ngx_http_headers_in
	
}requestHeader;




static inline int strnpos(char *haystack, int haystack_length, char *needle, int needle_length)
{
    assert(needle_length > 0);
    int i;
    for (i = 0; i < (int) (haystack_length - needle_length + 1); i++)
    {
        if ((haystack[0] == needle[0]) && (0 == memcmp(haystack, needle, needle_length)))
        {
            return i;
        }
        haystack++;
    }
    return -1;
}





httpServer aeHttpServ;
httpRequest* aeRequest; 

#endif
