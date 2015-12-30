

#include "httpServer.h"

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
	int buffer_pos; //当前解析的位置，在buffer中的偏移量
	
	headerFiled fileds[30]; //分析的结果数组，因为不宜搜索和读取。因此有了。headerOptions
	headerParams params;   //分析的结果结构体
	
	(char*)(findValueInFileds( char* key ));
}httpHeader;



#define CHUNK_SZ 128;
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
	int read_len;
	header->buffer_pos += getLeftEolLength( buffer );
	read_len = len - header->buffer_pos;
	
	offset = bufferLineSearchEOL( header , buffer+header->buffer_pos , read_len , eol_style );
	if( offset < 0 )
	{
		return AE_ERR;
	}
	
	//表示buffer的起始位置
	return offset;
}


//return char* point to first space position in s;
char* findSpace(  char* s , int len )
{
	char *s_end, *sp;
	s_end = s + len;
	while( s < s_end )
	{
		//每次循环查找128bytes
		size_t chunk = ( s + CHUNK_SZ < s_end ) ? CHUNK_SZ : ( s_end - s );
		sp = memchr( s , AE_SPACE , chunk );
		if( sp )
		{
			return sp;		//xxxxx\r\ncccccc\r\n    	cr:xxxxx
		}
		s += CHUNK_SZ;
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
	int offset;
	offset = bufferReadln( header , buffer , len , AE_EOL_CRLF );
	if( offset != AE_OK )
	{
		//error means header uncomplate, or a wrong header.
		return AE_ERR;
	}
	
	//first line end position except CRLF
	header->buffer_pos += offset;
	
	//find SPACE pos in ( buffer , buffer + header->buffer_pos );
	char* space;
	int find_count = 0;
	int section = 1;
	while( section <= 3 )
	{
		//因为是第一行，从开头到行末。
		space = findSpace( buffer + find_count , offset );
		//only third times find space=NULL
		if( space == NULL && section != 3 )
		{
			return AE_ERR;
		}
		
		find_count = space - ( buffer + find_count );
		if( section == 1 )
		{	
			memcpy( header->params.method , buffer , find_count );
		}
		else if( section == 2 )
		{
			memcpy( header->params.uri , buffer , find_count );
		}
		else if( section == 3 )
		{
			memcpy( header->params.version , buffer , find_count );
			break;
		}
		
		//加1是因为要去除掉一个空格的位置
		find_count++;
	}
	
	
	//..
	
}

static void  readingHeaderFirstLine( httpHeader* header , char* buffer , int len )
{
	int ret = parseFirstLine(  buffer , len );
}


static void  readingHeaders( httpHeader* header , char* buffer , int len )
{
	int ret = bufferReadln(  buffer ,  len , AE_EOL_CRLF );
}


static int httpHeaderParse( char* buffer , int len )
{
	httpHeader* header = zmalloc( httpHeader );
	header->buffer_pos = 0;
	
	readingHeaderFirstLine( header , buffer , len );
	readingHeaders( header );
	
	
	zfree( header );
}




