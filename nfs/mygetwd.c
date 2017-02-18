#include <stdio.h>
#include <sys/param.h>


main()
{
  char pathname[MAXPATHLEN];

  (void)getwd( pathname ) ;

  printf( " getwd : %s\n" , pathname ) ;
  exit(0);

}

