

#include <stdio.h>
#include "zmalloc.h"


typedef struct user_t
{
  int no;
  char* name;

}user;
int main()
{
  user* u=(user*)zmalloc( sizeof(user));

  u->no = 1000;
  u->name = "xxxxxx";

  printf( "user no = %d,name=%s \n" ,u->no , u->name );
  zfree( u );

  return 0;
}
