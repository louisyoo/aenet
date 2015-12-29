

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
	//..
	
}headerOptions;

typedef struct
{
	int header_length;//header部分总长
	int content_length;
	int filed_nums; //headerFiled numbers
	int buffer_pos; //当前解析的位置，在buffer中的偏移量
	
	headerFiled fileds[30]; //分析的结果数组，因为不宜搜索和读取。因此有了。headerOptions
	headerOptions option;   //分析的结果结构体
	
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



int bufferLineSearchEOL( httpHeader* header , char* buffer , int len , char* eol_style )
{
	//先清空左边空格
	header->buffer_pos += getLeftEolLength( buffer );
	char* cp = findEolChar( buffer , len );
	int offset = cp - buffer;
	if( cp && offset > 0 )
	{
		header->buffer_pos += offset;
		return offset;
	}
	
	return AE_ERR;
}


//以\r\n结尾读取一行
char* bufferReadln( char* buffer , int lineSize , char* eol_style )
{
	char* result;
	lineSize = bufferLineSearchEOL( buffer , lineSize , eol_style );
	//...是否找到
}



static int parseFirstLine( httpHeader* header , char* buffer , int len )
{
	int lineLength;
	char* line;
	line = bufferReadln( buffer , len , AE_EOL_CRLF );
	
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




