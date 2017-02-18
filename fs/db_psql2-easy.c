/*
 * $Id: db_psql2-easy.c,v 1.6 2006-02-06 11:17:52 tigran Exp $
 */
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
#include <libpq-fe.h>

/* keep Fermi happy */
static const char defaultConnectString[] = "user=postgres dbname=pnfsdb";

#define TEXT   0
#define BINARY 1
/*#define BYTEAOID 17     /* This should be taken from postgres include file */


typedef struct {
	PGconn *conn;
	char *dbName;
	int inTransaction;
	
	/* predefined SQL statements */
	char *insertSQL;
	char *updateSQL;
	char *selectSQL;
	char *deleteSQL;
	char *opencursorSQL;	
	
} dbConnect;

int  mdxInit( char * (*getenv)( const char * name ) , int (*prt)(int , char *,...)){
    return 0 ;
}


mdlDatum mdxFirst( MDX_FILE conn )
{
    mdlDatum val ;
    PGresult *res;
    dbConnect *dconn = (dbConnect *)conn;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;
    /* Start a transaction block */
    res = PQexec(dconn->conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage(dconn->conn));
        PQclear(res);
        return val ;
    }
    /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
    PQclear(res);
    
    /* Fetch row from pnfs */

    res = PQexec(dconn->conn, dconn->opencursorSQL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DECLARE CURSOR failed: %s\n", PQerrorMessage(dconn->conn));
        PQclear(res);
        return val ;
    }
    PQclear(res);
    
    res = PQexec(dconn->conn, "FETCH FIRST in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH FIRST failed: %s\n", PQerrorMessage(dconn->conn) );
        PQclear(res);
        return val ;
    }
    
    value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
    len =  PQgetlength(res, 0, 0);    /* Get result length          */
    val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
    memcpy(val.dptr, value, len);     /* Copy the result in place   */
    val.dsize = len;                  /* Stopy the result length    */
    if ( value ) val.flags = MDX_FREE ; /* ??? */
    PQclear(res);
    return val ;
}

mdlDatum mdxNext( MDX_FILE conn, mdlDatum key )
{
    mdlDatum val ;

    PGresult *res;
    dbConnect *dconn = (dbConnect *)conn;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;

    res = PQexec(dconn->conn, "FETCH NEXT in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH NEXT failed: %s\n", PQerrorMessage(dconn->conn));
        PQclear(res);
        return val ;
    }
    if (PQntuples(res) > 0) {
        value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
        len =  PQgetlength(res, 0, 0);    /* Get result length          */
        val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
        memcpy(val.dptr, value, len);     /* Copy the result in place   */
        val.dsize = len;                  /* Stopy the result length    */
        if ( value ) val.flags = MDX_FREE ; /* ??? */
    } else {             /* If fetch returns no tuples we have reached the end of result set. Close the transaction. */
	PQclear(res);

	/* close the cursor ... we don't bother to check for errors ... */
	res = PQexec(dconn->conn, "CLOSE cr");
	PQclear(res);

	/* end the transaction */
	res = PQexec(dconn->conn, "END");
    }
    PQclear(res);
    return val ;
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
    char query[128];
    char *dbName;
    char *connectString;
    dbConnect *dconn;

    /* Make a connection to the database */
    dbName = rindex(name, '/');
    if( dbName == NULL ) {
	dbName = name;
    }else{
	++dbName;
    }
    
    dconn = malloc( sizeof(dbConnect) );
    if( dconn == NULL ) {
    	fprintf(stderr, "Malloc failed for database '%s'.\n", name);
    	return NULL;
    }
    memset(dconn, 0, sizeof(dbConnect) );
    dconn->conn = NULL;
    dconn->dbName = NULL;
    dconn->inTransaction = 0;
    
    sprintf(query, "%sConnect", dbName);    /* VP */
    connectString = getenv(query);          /* VP */
    if( connectString == NULL ) {
        connectString = getenv("dbConnectString");
    }      
    if( connectString == NULL ) {
        connectString = (char *)defaultConnectString;
    }
    sprintf(query, "%s", connectString);
    dconn->conn = PQconnectdb(query);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(dconn->conn) != CONNECTION_BAD) {

   	    dconn->dbName = strdup(dbName);
   	    
   	    sprintf(query, "INSERT INTO t_%s VALUES ($1, $2, now())", dbName);
   	    dconn->insertSQL = strdup(query);
   	    
   	    sprintf(query, "SELECT data FROM t_%s WHERE pnfsid=$1", dbName);
   	    dconn->selectSQL = strdup(query);

   	    sprintf(query, "UPDATE t_%s SET data=$2 WHERE pnfsid=$1", dbName);
   	    dconn->updateSQL = strdup(query);

   	    sprintf(query, "DECLARE cr BINARY CURSOR FOR select pnfsid from t_%s", dbName);
   	    dconn->opencursorSQL = strdup(query);

   	    sprintf(query, "DELETE FROM t_%s WHERE pnfsid=$1", dbName);
   	    dconn->deleteSQL = strdup(query);
   	       	    
        return((MDX_FILE) dconn);         
    } 
    fprintf(stderr, "Connection to database '%s' failed.\n", name);
    fprintf(stderr, "%s", PQerrorMessage(dconn->conn));    
    

	mdxClose(dconn->conn);
    
    return((MDX_FILE) 0);
}



MDX_FILE mdxCreate( char *name, int flags, int mode )
{
    dbConnect     *dconn;   /* Connection to the database */
    PGresult   *res;    /* Result Set                 */
    char query[256];

    dconn = mdxOpen(name, flags, mode);

    /* Check to see that the backend connection was successfully made */
    if (dconn != NULL) {

		sprintf(query, "CREATE TABLE t_%s (pnfsid bytea PRIMARY KEY, data bytea, date timestamp)", dconn->dbName);

        res = PQexec(dconn->conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "CREATE TABLE command failed: %s\n", PQerrorMessage(dconn->conn));
            PQclear(res);
            
            PQfinish(dconn->conn);
            free(dconn->dbName);
		    free(dconn);
		    
            return ((MDX_FILE) 0);
        }
	    PQclear(res);
	    return (MDX_FILE)dconn;
    }
    
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
    dbConnect *dconn = (dbConnect *)conn;    
    const char *paramValues[1] =  {key.dptr};
/*     Oid paramTypes[1] = {BYTEAOID};  */
    int paramLengths[1] = {key.dsize};
    int paramFormats[1] = {BINARY};
    val->dptr = NULL; val->dsize = 0; val->flags = 0;

    /* Fetch row from pnfs */
        res = PQexecParams(dconn->conn,
                           dconn->selectSQL,
                           1,               /* one param                           */
                           NULL,            /* let the backend deduce param type   */
                           paramValues, paramLengths, paramFormats,
                           1);              /* ask for binary results              */
    if (PQresultStatus(res) != PGRES_TUPLES_OK)  {
#ifdef CDEB
        fprintf(stderr, "FETCH failed: %s\n", dconn->conn);
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
    fprintf(stderr, "FETCH failed: %s\n", PQerrorMessage(dconn-conn);
#endif
    val->dptr = NULL; val->dsize = 0; val->flags = 0;
    return -1;
}

int mdxStore( MDX_FILE conn, mdlDatum key , mdlDatum val ) 
{
    PGresult   *res;
    dbConnect *dconn = (dbConnect *)conn;    
    const char *paramValues[2] = { key.dptr, val.dptr };
/*     Oid paramTypes[2] = {BYTEAOID, BYTEAOID};  */
    int paramLengths[2] = { key.dsize, val.dsize };
    int paramFormats[2] = {BINARY, BINARY};
    int len;
    int isUpdated = 0;

    /* Start a transaction block */
    res = PQexec(dconn->conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage(dconn->conn));
	PQclear(res);
        goto bad;
    }
    PQclear(res);  /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */


    res = PQexecParams(dconn->conn,
                       dconn->updateSQL,
                       2,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE pnfs TABLE failed: %s\n", PQerrorMessage(dconn->conn));
	    PQclear(res);
        goto bad;
    }

	/* get number of updated rows */    
	isUpdated = atoi(PQcmdTuples(res)); 
    PQclear(res);


	/* if there is updated rows, then insert one */
	if( ! isUpdated ) {
	    res = PQexecParams(dconn->conn,
	                       dconn->insertSQL,
	                       2,               /* two params                          */
	                       NULL,            /* let the backend deduce param type   */
	                       paramValues, paramLengths, paramFormats,
	                       1);              /* ask for binary results              */
	
	    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	        fprintf(stderr, "INSERT INTO pnfs TABLE failed: %s\n", PQerrorMessage(dconn->conn));
		PQclear(res);
	        goto bad;
	    }
	    PQclear(res);
	}

    /* end the transaction */
    res = PQexec(dconn->conn, "COMMIT");
    PQclear(res);

    return(0);

    bad:  

    /* cancel the transaction */
    res = PQexec(dconn->conn, "ROLLBACK");
    PQclear(res);

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
    dbConnect *dconn = (dbConnect *)conn;
    char query[256];

    res = PQexecParams(dconn->conn,
                       dconn->deleteSQL,
                       1,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DELETE FROM pnfs TABLE failed: %s\n", PQerrorMessage(dconn->conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int mdxClose( MDX_FILE conn )
{
    dbConnect *dconn = (dbConnect *)conn;
	
    PQfinish(dconn->conn);
    
    if(dconn->dbName != NULL ) {
	    free(dconn->dbName);
    }
        
    if(dconn->insertSQL != NULL ) {
    	free(dconn->insertSQL);
    }
    
    if(dconn->updateSQL != NULL ) {
    	free(dconn->updateSQL);    	
    }
    
    if(dconn->opencursorSQL) {
    	free(dconn->opencursorSQL);
    }
    
    free(dconn);
    return 0;
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
