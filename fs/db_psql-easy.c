/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1996,1997,1998 DESY Hamburg DMG-Division
 * All rights reserved.
 *
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
#include <stdio.h>
#include <sys/types.h>
/* #include <sys/stat.h>  */
#include <string.h>
#include <stdlib.h>
#include "dbglue.h"
#include "libpq-fe.h"

/* keep Fermi happy */
static const char defaultConnectString[] = "user=enstore";

#define TEXT   0
#define BINARY 1
/*#define BYTEAOID 17     /* This should be taken from postgres include file */


int  mdxInit( char * (*getenv)( const char * name ) , int (*prt)(int , char *,...)){
    return 0 ;
}

mdlDatum mdxFirst( MDX_FILE conn )
{
    mdlDatum val ;
    PGresult *res;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;
    /* Start a transaction block */
    res = PQexec((PGconn *)conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
    PQclear(res);
    
    /* Fetch row from pnfs */
    res = PQexec((PGconn *)conn, "DECLARE cr BINARY CURSOR FOR select pnfsid from pnfs");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DECLARE CURSOR failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    PQclear(res);
    
    res = PQexec((PGconn *)conn, "FETCH FIRST in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH FIRST failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    
    value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
    len =  PQgetlength(res, 0, 0);    /* Get result length          */
    val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
    memcpy(val.dptr, value, len);     /* Copy the result in place   */
    val.dsize = len;                  /* Stopy the result length    */
    if (val.dptr) 
      val.flags = MDX_FREE ;          /*VP ??? */
    PQclear(res);
    return val ;
}

mdlDatum mdxNext( MDX_FILE conn, mdlDatum key )
{
    mdlDatum val ;

    PGresult *res;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;

    res = PQexec((PGconn *)conn, "FETCH NEXT in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH NEXT failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    if (PQntuples(res) > 0) {
        value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
        len =  PQgetlength(res, 0, 0);    /* Get result length          */
        val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
        memcpy(val.dptr, value, len);     /* Copy the result in place   */
        val.dsize = len;                  /* Stopy the result length    */
        if (val.dptr) 
          val.flags = MDX_FREE ;          /*VP ??? */
    } else {             /* If fetch returns no tuples we have reached the end of result set. Close the transaction. */
	PQclear(res);

	/* close the cursor ... we don't bother to check for errors ... */
	res = PQexec((PGconn *)conn, "CLOSE cr");
	PQclear(res);

	/* end the transaction */
	res = PQexec((PGconn *)conn, "END");
    }
    PQclear(res);
    return val ;
}

void mdxBreak( MDX_FILE conn )
{
    PGresult *res;

    /* close the cursor ... we don't bother to check for errors ... */
    res = PQexec((PGconn *)conn, "CLOSE cr");
    PQclear(res);

    /* end the transaction */
    res = PQexec((PGconn *)conn, "END");
    PQclear(res);
}

int mdxScan( MDX_FILE mdx )
{
    int i;
    mdlDatum key;

    key = mdxFirst ( mdx );
    while ( key.dptr ) {
        for (i = 0; i < 8; i++)
            fprintf(stderr,"%2.2x",key.dptr[i]);
        fprintf(stderr,"\n");
        if (key.dptr) free(key.dptr);
        key = mdxNext( mdx, key );
    };
  
    return 0 ;
}

MDX_FILE mdxOpen( char *name, int flags, int mode )
{
    PGconn     *conn;   /* Connection to the database */
    char query[128];
    char *dbName;
    char *connectString;

    /* Make a connection to the database */
    dbName = rindex(name, '/');
    if( dbName == NULL ) {
        dbName = name;
    }else{
        ++dbName;
    }

    connectString = getenv("dbConnectString");
    if( connectString == NULL ) {
        connectString = defaultConnectString;
    }
    sprintf(query, "dbname=%s %s", dbName, connectString);
    conn = PQconnectdb(query);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(conn) != CONNECTION_BAD) {
        return((MDX_FILE) conn);
    } 
    fprintf(stderr, "Connection to database '%s' failed.\n", name);
    fprintf(stderr, "%s", PQerrorMessage(conn));
    PQfinish(conn); /* exit(1);  /* Exit nicely */
    return((MDX_FILE) 0);
}


MDX_FILE mdxCreate( char *name, int flags, int mode )
{
    PGconn     *conn;   /* Connection to the database */
    PGresult   *res;    /* Result Set                 */
    char query[128];
    char *dbName;
    char *connectString;

    /* Make a connection to the database */
    connectString = getenv("dbConnectString");
    if( connectString == NULL ) {
        connectString = defaultConnectString;
    }
    sprintf(query, "dbname=template1 %s", connectString);

    conn = PQconnectdb(query);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(conn) != CONNECTION_BAD) {
        dbName = rindex(name, '/')+1;
/*        sprintf(query, "CREATE DATABASE \"%s\" WITH LOCATION 'PGDATA2'", dbName); */
        sprintf(query, "CREATE DATABASE \"%s\"", dbName);
        res = PQexec(conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "CREATE DATABASE command failed: %s\n", PQerrorMessage(conn));
            PQclear(res);
            PQfinish(conn);
            return((MDX_FILE) 0);
        }
        PQclear(res);
        PQfinish(conn);        /* We don't need the connection to 'template1' anymore */

        if ((conn = mdxOpen(name, flags, mode)) != NULL) {
            res = PQexec(conn, "CREATE TABLE pnfs (pnfsid bytea PRIMARY KEY, data bytea, date timestamp)");
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                fprintf(stderr, "CREATE TABLE command failed: %s\n", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return((MDX_FILE) 0);
            }
            return((MDX_FILE) conn);
        }
    } 
    fprintf(stderr, "Connection to database '%s' failed.\n", name);
    fprintf(stderr, "%s", PQerrorMessage(conn));
    PQfinish(conn); /* exit(1);  /* Exit nicely */
    return((MDX_FILE) 0);
}

int mdxCheck( char *name )
{
    return 0;
}

int mdxFetch( MDX_FILE conn, mdlDatum key, mdlDatum *val )
{
    PGresult   *res;
    char *value;
    int len;
    const char *paramValues[1] =  {key.dptr};
/*     Oid paramTypes[1] = {BYTEAOID};  */
    int paramLengths[1] = {key.dsize};
    int paramFormats[1] = {BINARY};
    val->dptr = NULL; val->dsize = 0; val->flags = 0;

    /* Fetch row from pnfs */
    res = PQexecParams((PGconn *)conn,
                       "SELECT data FROM pnfs WHERE pnfsid = $1",
                       1,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */
    if (PQresultStatus(res) != PGRES_TUPLES_OK)  {
#ifdef CDEB
        fprintf(stderr, "FETCH failed: %s\n", PQerrorMessage((PGconn *)conn));
#endif
        goto bad;
    }
    if (PQntuples(res) > 0) {
        value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
        len =  PQgetlength(res, 0, 0);    /* Get result length          */
        val->dptr  = (char *)malloc(len);  /* Allocate memory for result */
        memcpy(val->dptr, value, len);     /* Copy the result in place   */
        val->dsize = len;                  /* Story the result length    */
        if (val->dptr) 
            val->flags = MDX_FREE;
        PQclear(res);
        return 0;
    }
    PQclear(res);
    val->dptr = NULL; val->dsize = 0; val->flags = 0;
    return 0;

 bad:
    PQclear(res);
#ifdef CDEB
    fprintf(stderr, "FETCH failed: %s\n", PQerrorMessage((PGconn *)conn));
#endif
    val->dptr = NULL; val->dsize = 0; val->flags = 0;
    return -1;
}

int mdxStore( MDX_FILE conn, mdlDatum key , mdlDatum val ) 
{
    PGresult   *res;
    const char *paramValues[2] = { key.dptr, val.dptr };
/*     Oid paramTypes[2] = {BYTEAOID, BYTEAOID};  */
    int paramLengths[2] = { key.dsize, val.dsize };
    int paramFormats[2] = {BINARY, BINARY};
    int len;
# if 1
    /* Start a transaction block */
    res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage(conn));
	PQclear(res);
        goto bad;
    }
    PQclear(res);  /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */

    res = PQexecParams((PGconn *)conn,
                       "DELETE FROM pnfs WHERE pnfsid=$1",
                       1,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DELETE FROM pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
	PQclear(res);
        goto bad;
    }
    PQclear(res);
# endif
    res = PQexecParams((PGconn *)conn,
                       "INSERT INTO pnfs VALUES ($1, $2, now())",
                       2,               /* two params                          */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT INTO pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
	PQclear(res);
        goto bad;
    }
    PQclear(res);
# if 1
    /* end the transaction */
    res = PQexec(conn, "COMMIT");
    PQclear(res);
# endif
    return(0);

    bad:  
# if 1
    /* cancel the transaction */
    res = PQexec(conn, "ROLLBACK");
    PQclear(res);
# endif
    return -1;
}

int mdxIoctl( MDX_FILE mdxf , int argc , char * argv [] , int * replyLen , char *reply ){
    return -1 ;
}

int mdxDelete( MDX_FILE conn, mdlDatum key )
{
    PGresult   *res;
    const char *paramValues[1] = {key.dptr};
/*     Oid paramTypes[1] = {BYTEAOID};  */
    int paramLengths[1] =  {key.dsize};
    int paramFormats[1] = {1};

    res = PQexecParams((PGconn *)conn,
                       "DELETE FROM pnfs WHERE pnfsid=$1",
                       1,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DELETE FROM pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int mdxClose( MDX_FILE mdx )
{
    PQfinish((PGconn *)mdx);
    return 0 ;
}

int mdxFlush( MDX_FILE mdx , int force ) 
{
    return 0 ;
}

int mdxReadLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxWriteLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxCommitLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxAbortLock( MDX_FILE mdx )
{
    return 0 ;
} 
