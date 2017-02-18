%{

#include <stdio.h>
#include <string.h>
#include "md2scan.h"

static char *input_string ;
static int  is_last = { 0 } , input_pos = { 0 } ;
static char last_back ;
 
#ifndef USE_FLEX

#undef output
#undef input
#undef unput


#define input()    (is_last?(is_last=0,last_back):input_string[input_pos++])
#define output(x)  (x)
#define unput(x)   {is_last=1;last_back=(x);}

#define YY_BUFFER_STATE     int
#define yy_scan_string(x)   (0);input_string=(x);is_last=0;input_pos=0;   
#define yy_delete_buffer(x) 

#endif

#define  stateA  (0)
#define  stateB  (1)
#define  stateC  (2)
#define  stateD  (3)
#define  stateE  (4)
#define  stateF  (5)

%}

valids      .
eol         [\n]

%%

\.\(         { return MDS_START ; }
\)           { return MDS_END ;   }
\)\(         { return MDS_NAME ;  } 
{eol}        { return MDS_INVALID ; }
[^()]+       { return MDS_TOKEN ; }


%%

int md2scanObjectString( char *string , md_object *obj )
{
  int in , state , rest , len , argc ;
  char *cursor ;
  YY_BUFFER_STATE hdl ;
  
    hdl = yy_scan_string( string ) ;

    state = stateA ;
    cursor= obj -> args ;
    obj -> argc  = 0 ;
    rest  = sizeof( obj->args ) ;
 
    while( in = yylex() ){

      if( in == MDS_INVALID )goto bad ;

      switch( state ){
        case stateA : if( in == MDS_START )state = stateB ;
                      else goto bad ; break ;
        case stateB :
                 if( in == MDS_TOKEN ){
                     state = stateC ; 
                     strncpy( obj -> type , yytext , MAX_OBJECT_TYPE-1 ) ;
                     obj -> type[MAX_OBJECT_TYPE-1] = '\0' ;
                 }else{
                     goto bad ;
                 }
                 break ; 
        case stateC :
                 if( in == MDS_NAME ){
                     state = stateD ; 
                 }else{
                     goto bad ;
                 }
                 break ; 
        case stateD :
                 if( in == MDS_TOKEN ){
                     len = strlen( yytext ) ;
                     if( len >= (rest-1) )return -1 ;
                     obj -> argv[obj->argc++] = cursor ;
                     strcpy( cursor , yytext ) ;
                     cursor += ( len + 1 ) ;
                     rest   -= ( len + 1 ) ;
                     state = stateE ; 
                 }else if( in == MDS_NAME ){
                     obj -> argv[obj->argc++] = cursor ;
                     *cursor = '\0' ;
                     cursor++ ;
                     rest-- ;
                 }else if( in == MDS_END ){
                     state = stateF ; 
                     obj -> argv[obj->argc++] = cursor ;
                     *cursor = '\0' ;
                     goto theEnd ;
                 }else{
                     goto bad ;
                 }
                 break ; 
        case stateE :
                 if( in == MDS_NAME ){
                     state = stateD ; 
                 }else if( in == MDS_END ){
                     state = stateF ; 
                     goto theEnd ;
                 }else{
                     goto bad ;
                 }
                 break ; 
        case stateF :
                 if( in == MDS_END ){
                     goto theEnd ;
                 }
        default : goto bad ;
      }
    }
bad :

    yy_delete_buffer( hdl ) ;
    return -1 ;

theEnd :
    
    obj -> name = obj -> args ;
    yy_delete_buffer( hdl ) ;
    return 0 ;
}
int yywrap(){ return 1 ;}

#ifdef COMPILE_WITH_MAIN
#ifdef FROM_INPUT
main( argc ,argv )
int argc ;
char *argv[] ;
{
  int rc , i ,j ;
  md_object obj ; 
  if( argc < 2 )exit(1);

for ( j = 1 ; j < argc ; j++ ){

  rc = md2scanObjectString( argv[j] , &obj );
  if( rc ){
      printf( " Syntax Error\n" ) ;
      exit(1) ;
  }

  printf( " ---------------------------------------------\n"  ) ;
  printf( " Object Type : %s\n" , obj.type ) ;
  printf( " Object Name : %s\n" , obj.name ) ;
  for( i = 0 ; i < obj.argc ; i++ )
      printf( "   %d : %s\n" , i , obj.argv[i] ) ;

}

}
#else
main( argc ,argv )
int argc ;
char *argv[] ;
{
  int rc , i ,j , m ;
  md_object obj ; 
  if( argc < 2 )exit(1);

  sscanf( argv[1] , "%d" , &m ) ;
for ( j = 1 ; j < m ; j++ ){

  rc = md2scanObjectString( ".(halllo)(fsdfsdf)(sdfsdf)(sdfsdfsdfs)" , &obj );
  if( rc ){
      printf( " Syntax Error %d\n",j ) ;
      exit(1) ;
  }


}

}
#endif
#endif
