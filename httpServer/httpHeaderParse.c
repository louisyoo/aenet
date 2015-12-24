

//#include "httpServer.h"

#include <stdio.h>
#include <string.h>

#define AE_SPACE       ' '
#define AE_EOL_CRLF        "\r\n"
#define AE_OK        0
#define AE_ERR       -1
#define AE_TRUE        1
#define AE_FALSE       0
#define AE_HTTP_HEADER_MAX_SIZE          8192

//http://www.w3.org/Protocols/rfc2616/rfc2616.html
//响应完后，再将包头与包体从recv_buffer中移除
//查找eol在字符串中的位置，找不到就继续接收。
//如果header太长，返回错误，断开

//从s开头，在len个长度查找，每次循环最多找128bytes
typedef struct
{
	char* key;
	char* value;
	int   buffer_pos;//这个域的开始位置在buffer中的偏移量
}headerFiled;

//会把一些长见的option例举在这里，如果没有的，可以去fileds里去找,提高访问效率。
enum
{
	HEADER_METHOD = 1,
	HEADER_URI,
	HEADER_VERSION
};

typedef struct
{
	char* method;
	char* version;
	char* uri;
	//..
	
}headerParams;

typedef struct
{
	int header_length;//header部分总长
	int content_length;
	int filed_nums; //headerFiled numbers
	int buffer_pos; //解析的位置,作为起始位置
	
	headerFiled fileds[30]; //分析的结果数组，因为不宜搜索和读取。因此有了。headerOptions
	headerParams params;   //分析的结果结构体
	
}httpHeader;



#define CHUNK_SZ 128
static char* findEolChar( char* s , int len )
{
	char *s_end, *cr , *lf;
	s_end = s + len;
	while( s < s_end )
	{
		//每次循环查找128bytes
		size_t chunk = ( s + CHUNK_SZ < s_end ) ? CHUNK_SZ : ( s_end - s );
		cr = memchr( s , '\r' , chunk );
		lf = memchr( s , '\n' , chunk );
		if( cr )
		{
			if( lf && lf < cr )
			{
				return lf; 	//xxxxx\n\rcccccc     		lf:xxxxx
			}
			return cr;		//xxxxx\r\ncccccc\r\n    	cr:xxxxx
		}
		else if( lf )
		{
			return lf;		//xxxxx\ncccccccccc\r\n   	lf:xxxxx
		}
		s += CHUNK_SZ;
	}
	return NULL;
} 


//获取字符串左边"\r","\n"," "个数。
static int getLeftEolLength( char* s )
{
   int i,pos=0;
   for( i = 0; i<strlen( s );i++ )
   {
        if( memcmp( "\r" , s+i , 1 ) == 0 ||  memcmp( "\n" , s+i , 1 )==0 ||   memcmp( " " , s+i , 1 )==0 )
        {
		pos++;
        }
	else
	{
		return pos;
	}
  }
  return pos;
}


//返回的是在buffer中的偏移量
int bufferLineSearchEOL( httpHeader* header , char* buffer , int len , char* eol_style )
{
	//先清空左边空格
	//header->buffer_pos += getLeftEolLength( buffer );
	printf( "searchEOL buffer=%s \n" , buffer );
	char* cp = findEolChar( buffer , len );
	int offset = cp - buffer;
	if( cp && offset > 0 )
	{
		//header->buffer_pos += offset;
		return offset;
	}
	return AE_ERR;
}


//从上次取到的位置，在剩余的buffer长度中查找，以\r\n结尾读取一行
int bufferReadln( httpHeader* header , char* buffer , int len , char* eol_style )
{
	int read_len,offset;
	header->buffer_pos += getLeftEolLength( buffer+header->buffer_pos  );
	read_len = len - header->buffer_pos;

	offset = bufferLineSearchEOL( header , buffer+header->buffer_pos , read_len , eol_style );
	printf( "@@@@@@@@@@@@@@@@=%d \n", offset );
	if( offset < 0 )
	{
		return AE_ERR;
	}
	
	//表示buffer的起始位置
	return offset;
}

char* findChar(  char sp_char , char* dest , int len );
//return char* point to first space position in s;
//在s中查找，最多找len个长度s
char* findSpace(  char* s , int len )
{
	return findChar( AE_SPACE , s , len );
}


char* findChar(  char sp_char , char* dest , int len )
{
	char *s_end, *sp;
	s_end = dest + len;
	while( dest < s_end )
	{
		//每次循环查找128bytes
		size_t chunk = ( dest + CHUNK_SZ < s_end ) ? CHUNK_SZ : ( s_end - dest );
		sp = memchr( dest , sp_char , chunk );
		if( sp )
		{
			return sp;		//xxxxx\r\ncccccc\r\n    	cr:xxxxx
		}
		dest += CHUNK_SZ;
	}
	return NULL;
}

/*
by RFC2616
http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1
The Request-Line begins with a method token, followed by the Request-URI and the protocol version, and ending with CRLF. 
The elements are separated by SP characters. No CR or LF is allowed except in the final CRLF sequence.
Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
*/
static int parseFirstLine( httpHeader* header , char* buffer , int len )
{
	printf( "parseFirstLine len=%d\n" , len );
	int offset;
	offset = bufferReadln( header , buffer , len , AE_EOL_CRLF );

	printf( "offset=%d \n" , offset );

	if( offset < AE_OK )
	{
		//error means header uncomplate, or a wrong header.
		return AE_ERR;
	}
	
	//first line length except CRLF
	header->buffer_pos += offset;
	
	//find SPACE pos in ( buffer , buffer + header->buffer_pos );
	char* space;
	int find_count = 0;
	int pre_length = 0;
	int section = HEADER_METHOD;
	
	//因为三段只有两个空格啊，所以找两次就可以了。
	while( section < HEADER_VERSION )
	{
		//因为是第一行，从开头到行末。
		space = findSpace( buffer + find_count , offset );
		
		if( space == NULL )	
		{
			return AE_ERR;
		}
		
		pre_length += find_count;
		//本次找到了几个字符，这次找到在buffer中的位置-上次找到的位置
		find_count = space-(buffer + find_count);
		
		if( section == HEADER_METHOD )
		{	
			char method[16];
			memcpy( method , buffer , find_count );
			printf( "method:%s\n" , method );
		}
		else if( section == HEADER_URI )
		{
			char uri[16];
            memcpy( uri, buffer+pre_length , find_count );
            printf( "uri:%s\n" , uri );
			break;
		}
		section++;
		find_count++;//加1是因为要去除掉一个空格的位置
	}

	char ver[16];
    memcpy( ver, space+1 , header->buffer_pos -( space-buffer) );
    printf( "ver:%s\n" , ver );	

	return AE_OK;
}

static int  readingHeaderFirstLine( httpHeader* header , char* buffer , int len )
{
	return  parseFirstLine(  header , buffer , len );
}


static int readingHeaders( httpHeader* header , char* buffer , int len )
{
	int ret,end,offset;
	headerFiled filed;
	end = 0;
	while( end == 0 )
	{
		offset = bufferReadln( header , buffer , len , AE_EOL_CRLF );
		printf( "while offset=%d len=%d buffer=%s \n" , offset,len,buffer );
		if( offset < AE_OK )
		{
			//error means header uncomplate, or a wrong header.
			return AE_ERR;
		}
		
		if( offset == AE_OK )
		{
			break;
		}
		
		ret = readingSingleLine( header , buffer + header->buffer_pos , offset );
		if( ret < AE_OK )
		{
			printf( "error...\n");
			return AE_ERR;
		}
		
	
		//如果解析好了，指针往后移
		header->buffer_pos += offset;
	};
	
	return AE_OK;
}

int readingSingleLine(  httpHeader* header , char* org , int len )
{
	char* ret;
	int value_len;
	ret = findChar( ':' , org , len );

	printf( ">>>>>>>>>>>>>>>>>buffer=%s,len=%d,ret=%s<<<<<<<<<<<<<<<<<<<< \n" , org,len,ret );

	if( ret == NULL )
	{
		if(  header->filed_nums <= 0 )
		{
			
			return AE_ERR;
		}
		//放在上一个域的值里
		memcpy( header->fileds[header->filed_nums-1].value , org , len  );
		return AE_OK;
	}
	//org~ret :key   ret+1~org+len: value 
	memcpy( header->fileds[header->filed_nums].key , org , ret-org );

	printf( ">>>>>>>>>>>>>>>>>>>>>>>>>>>");

	value_len = len - ( ret - org ) - 1;
	memcpy( header->fileds[header->filed_nums].value , ret+1 , value_len  );
	header->filed_nums += 1;

	printf( "head key=%s \n" , header->fileds[header->filed_nums].key );
	return AE_OK;
}


static char* getHeaderParams(  httpHeader* header , char* key )
{
	int i;
	for( i = 0 ; i < header->filed_nums ; i++ )
	{
		if( memcmp( header->fileds[i].key , key ,strlen( key ) ) == 0 )
		{
			return  header->fileds[i].value;
		}
	}
	return "";
}


static int httpHeaderParse( httpHeader* header , char* buffer , int len )
{
	
	header->buffer_pos = 0;
	header->filed_nums = 0;
	
	int ret = 0;
	printf( "1111111111111111111111111111111\n");
	ret = readingHeaderFirstLine( header , buffer , len );
	if( ret < AE_OK )
	{
		return AE_ERR;
	}

	printf( "22222222222222222222222222222\n");
	ret = readingHeaders( header , buffer , len );
	if( ret < AE_OK )
	{
		return AE_ERR;
	}
	return AE_OK;	
}



int main()
{
	
	httpHeader* header = malloc( sizeof(httpHeader) );
	
	char* buffer = "GET /?xx=yy HTTP/1.1\r\nHost: 192.168.171.129:3002\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/\*;q=0.8\r\nAccept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive\r\n\r\n";


	httpHeaderParse( header , buffer , strlen( buffer ) );	
//	char* value = getHeaderParams( header , "Content-length" );

	printf( "over\n");	
	
	free( header );
	return 1;
}
