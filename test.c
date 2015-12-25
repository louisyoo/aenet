
//https://github.com/lchb369/Aenet.git

#include <stdio.h>
#include "server.h"

int main( int argc,char *argv[] )
{
	aeServer serv;
	startServer( "0.0.0.0" , 3002 );
	return 0;
}
