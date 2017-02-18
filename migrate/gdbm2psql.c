
# define PNFSIDMIN 0x00000000
# define PNFSIDMAX 0xFFFFFFFF

/* include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "gdbmerrno.h"
#include "extern.h"

#include "getopt.h"
#include "dbglue.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

extern const char * gdbm_version;

gdbm_file_info *gdbm_file;

mdlDatum mdxFirst1( MDX_FILE conn );
mdlDatum mdxNext1( MDX_FILE conn, mdlDatum key );


void
usage (s)
     char *s;
{
  printf(
      "Usage: %s [-r or -ns] [-b block-size] [-c cache-size] [-g gdbm-file]\n",
      s);
  exit (2);
}


/* The test program allows one to call all the routines plus the hash function.
   The commands are single letter commands.  The user is prompted for all other
   information.  See the help command (?) for a list of all commands. */

int
main (argc, argv)
     int argc;
     char *argv[];

{

  char  cmd_ch;

  datum key_data = {0, 0};
  datum data_data = {0, 0};
  datum return_data = {0, 0};

  char key_line[500];
  char data_line[1000];

  char done = FALSE;
  int  opt;
  char reader = FALSE;
  char newdb = FALSE;
  int  fast  = 0;
  int offline = 0;

  int  cache_size = DEFAULT_CACHESIZE;
  int  block_size = 0;

  char *file_name = NULL;

  int rcnt = 0;
  time_t         ctim;
  time_t         dtim;
  time_t         otim = time(NULL);
  int i;
  int key, msw = 0;

  /* Argument checking. */
  opterr = 0;
  while ((opt = getopt (argc, argv, "osrnc:b:i:")) != -1)
    switch (opt) {
    case 'o':  offline = 1;
               break;
    case 's':  fast = GDBM_SYNC;
               if (reader) usage (argv[0]);
               break;
    case 'r':  reader = TRUE;
               if (newdb) usage (argv[0]);
               break;
    case 'n':  newdb = TRUE;
               if (reader) usage (argv[0]);
               break;
    case 'c':  cache_size = atoi(optarg);
    	       break;
    case 'b':  block_size = atoi(optarg);
               break;
    case 'i':  file_name = optarg;
               break;
    default:  usage(argv[0]);
    }

  if(file_name == NULL) 
    file_name = "junk.gdbm";

  /* Initialize variables. */
  key_data.dptr = NULL;
  data_data.dptr = data_line;

  if (reader)
    {
      gdbm_file = gdbm_open (file_name, block_size, GDBM_READER, 00664, NULL);
    }
  else if (newdb)
    {
      gdbm_file =
    	gdbm_open (file_name, block_size, GDBM_NEWDB | fast, 00664, NULL);
    }
  else
    {
      gdbm_file =
    	gdbm_open (file_name, block_size, GDBM_WRCREAT | fast, 00664, NULL);
    }
  if (gdbm_file == NULL)
    {
      printf("gdbm_open failed, %s\n", gdbm_strerror(gdbm_errno));
      exit (2);
    }

  if (gdbm_setopt(gdbm_file, GDBM_CACHESIZE, &cache_size, sizeof(int)) == -1)
    {
      printf("gdbm_setopt failed, %s\n", gdbm_strerror(gdbm_errno));
      exit(2);
    }

  if (!offline) {
    /* Welcome message. */
    printf ("\nWelcome to the gdbm conversion program.  Type ? for help.\n\n");
  }

  while (!done)
    {
      if (!offline) {
      printf ("com -> "); 
      cmd_ch = getchar ();
      if (cmd_ch != '\n')
	{
	  char temp;
	  do
	      temp = getchar ();
	  while (temp != '\n' && temp != EOF);
	}
      if (cmd_ch == EOF) cmd_ch = 'q';
      } else {
	cmd_ch = 'g';
      }
      switch (cmd_ch)
	{
	case 'f':
	  {
	    unsigned int tmp;
	  if (key_data.dptr != NULL) free (key_data.dptr);
	  printf ("key -> ");
	  gets (key_line);
 	  key_data.dptr = key_line; 
	  for (i = 0; i < 12; i++) {
	    sscanf(&key_line[i*2], "%2x", &tmp);
/* 	    fprintf(stderr, "%s: %2.2x, ", &key_line[i*2], tmp); */
	    key_data.dptr[i] = tmp;
	  }
	  key_data.dsize = 12;
	  fprintf(stderr, "\n");
	  for (i = 0; i < 12; i++) {
	    fprintf(stderr, "%2.2X", (unsigned char)key_data.dptr[i]);
	  }
	  return_data = gdbm_fetch (gdbm_file, key_data);
	  if (return_data.dptr != NULL)
	    {
/* 	      printf ("data is ->%s\n\n", return_data.dptr); */
	      printf ("data size is ->%d\n\n", return_data.dsize);
	      free (return_data.dptr);
	    }
	  else
	    printf ("No such item found.\n\n");
	  key_data.dptr = NULL;
	  break;
	  }
	case 'n':
	  {
	    unsigned int tmp;
	  if (key_data.dptr != NULL) free (key_data.dptr);
	  printf ("key -> ");
	  gets (key_line);
	  key_data.dptr = key_line;
	  key_data.dsize = strlen (key_line)+1;
	  for (i = 0; i < 12; i++) {
	    sscanf(&key_line[i*2], "%2x", &tmp);
/* 	    fprintf(stderr, "%s: %2.2x, ", &key_line[i*2], tmp); */
	    key_data.dptr[i] = tmp;
	  }
	  key_data.dsize = 12;
	  fprintf(stderr, "\n");
	  for (i = 0; i < 12; i++) {
	    fprintf(stderr, "%2.2X", (unsigned char)key_data.dptr[i]);
	  }
	  return_data = gdbm_nextkey (gdbm_file, key_data);
	  if (return_data.dptr != NULL)
	    {
	      key_data = return_data;
/* 	      printf ("key is  ->%s\n", key_data.dptr); */
	      fprintf (stderr, "\nkey is  ->");
	      for (i = 0; i < 12; i++) {
		fprintf(stderr, "%2.2X", (unsigned char)key_data.dptr[i]);
	      }
	      return_data = gdbm_fetch (gdbm_file, key_data);
/* 	      printf ("data is ->%s\n\n", return_data.dptr); */
	      printf ("data size is ->%d\n\n", return_data.dsize);
	      free (return_data.dptr);
	    }
	  else
	    {
	      printf ("No such item found.\n\n");
	      key_data.dptr = NULL;
	    }
	  break;
	  }

	case '1':
	  if (key_data.dptr != NULL) free (key_data.dptr);
	  key_data = gdbm_firstkey (gdbm_file);
	  if (key_data.dptr != NULL)
	    {
 	      printf ("key is  ->%s\n", key_data.dptr); 
	      for (i = 0; i < 12; i++) {
		fprintf(stderr, "%2.2X", (unsigned char)key_data.dptr[i]);
	      }
	      return_data = gdbm_fetch (gdbm_file, key_data);
	      printf ("data is ->%s\n\n", return_data.dptr);
	      free (return_data.dptr);
	    }
	  else
	    printf ("No such item found.\n\n");
	  break;

	case '2':
	  return_data = gdbm_nextkey (gdbm_file, key_data);
	  if (return_data.dptr != NULL)
	    {
	      free (key_data.dptr);
	      key_data = return_data;
/* 	      printf ("key is  ->%s\n", key_data.dptr); */
	      printf ("key is  ->");
	      for (i = 0; i < 12; i++) {
		fprintf(stderr, "%2.2X", (unsigned char)key_data.dptr[i]);
	      }
	      return_data = gdbm_fetch (gdbm_file, key_data);
/* 	      printf ("data is ->%s\n\n", return_data.dptr); */
	      printf ("data size is ->%d\n\n", return_data.dsize);
	      free (return_data.dptr);
	    }
	  else
	    printf ("No such item found.\n\n");
	  break;

	
	/* Standard cases found in all test{dbm,ndbm,gdbm} programs. */
	case '\n':
	  printf ("\n");
	  break;

	case 'c':
	  {
	    int temp;
	    temp = 0;
	    if (key_data.dptr != NULL) free (key_data.dptr);
	    return_data = gdbm_firstkey (gdbm_file);
	    while (return_data.dptr != NULL)
	      {
                   /*DEBUG*/
                    if (!memcmp(return_data.dptr,"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00",6)) {
		      /* \000\000\000\000\000\000\000\000\000\001\000\000 */
                      fprintf(stderr, "key is found at %d\n", temp);
		      for (i = 0; i < 12; i++) {
			fprintf(stderr, "%2.2X", (unsigned char)return_data.dptr[i]);
		      }
		      fprintf(stderr,"\n");
		    }
                    /*DEBUG*/
		temp++;
		if ((temp % 1000)==0)
		  fprintf(stderr,"%d ", temp);
		key_data = return_data;
		return_data = gdbm_nextkey (gdbm_file, key_data);
		free (key_data.dptr);
	      }
	    printf ("There are %d records in the database.\n\n", temp);
	    key_data.dptr = NULL;
	  }
	  break;

	case 'q':
	  done = TRUE;
	  break;

	case 'g':
            {
                mdlDatum key, val;
                int i, count = 0;
                MDX_FILE db = mdxCreate(file_name, 0, 0 );
                otim = time(NULL);
                if (db == NULL) {
                    fprintf (stderr, "Can't create database '%s'.\n", file_name);
                    break;
                }
                if (key_data.dptr != NULL) free (key_data.dptr);
                if (return_data.dptr != NULL) free (return_data.dptr);
                
                key_data = gdbm_firstkey (gdbm_file);
                if (key_data.dptr == NULL) {
                    fprintf (stderr, "No such item found.\n");
                    break;
                }
                while (key_data.dptr != NULL) {
                    return_data = gdbm_fetch (gdbm_file, key_data);   
/*                     count++; */
                    if (count++ % 10000 == 0) {
		        mdxCommit(db); 
		        mdxBegin(db);
                        ctim = time(NULL);
                        dtim = ctim - otim;
                        otim = ctim;
                        fprintf(stderr, "Put record #%d into the database...", count);
                        fprintf(stderr,"time=%d\n", dtim);
                    }
                    /* Store (key_data, return_data) into Postgres DB */
                    key.dptr = key_data.dptr;     key.dsize = key_data.dsize;
                    val.dptr = return_data.dptr;  val.dsize = return_data.dsize;
                    /*DEBUG*/
                    if (!memcmp(key.dptr,"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00",6)) {
		      /* \000\000\000\000\000\000\000\000\000\001\000\000 */
/*                       fprintf(stderr, "key 000000000000000000010000 is found at %d\n", count-1); */
                      fprintf(stderr, "key 000000000000xxxxxxxxxxxx is found at %d\n", count-1);
                    }
		    /*
		    for (i = 0; i < 12; i++) {
		      fprintf(stderr, "%2.2X", (unsigned char)key.dptr[i]);
		    }
		    fprintf(stderr,"\n");
		    */
                    /*DEBUG*/
                    if (mdxStore(db, key, val) < 0) {
                        fprintf (stderr, "Can't store record %d into databse '%s'.\n", count, file_name);
                        break;
                    }
                    free (return_data.dptr);                          /* Free memory */
                    return_data = gdbm_nextkey (gdbm_file, key_data); /* Get next key */
                    free (key_data.dptr);                             /* Free key memory */
                    key_data = return_data;                           /* Store next key for the next request */
                }
                mdxCommit(db); 
                mdxClose(db);
                fprintf (stderr, "There are %d records in the database.\n", count);
            }
 	    done = TRUE;
            break;

	case 's':  /* Sequential scan */
            {
                mdlDatum key, val;
                int i, count = 0;
		unsigned long j;

		struct {
		  unsigned short dbid; unsigned short dummy; 
		  unsigned long pnfs2; unsigned long pnfs1;
		} pnfsid;

                MDX_FILE db = mdxCreate(file_name, 0, 0 );
                otim = time(NULL);
                if (db == NULL) {
                    fprintf (stderr, "Can't create database '%s'.\n", file_name);
                    break;
                }
                if (key_data.dptr    != NULL) free (key_data.dptr);
                if (return_data.dptr != NULL) free (return_data.dptr);
                
		pnfsid.dbid = 0x0007; pnfsid.dummy = 0x0000; 
		pnfsid.pnfs2 = 0x00000000; pnfsid.pnfs1 = 0x00000000;

		key_data.dptr = (char *)&pnfsid;
		key_data.dsize = 12;

		fprintf(stderr,"sizeof pnfsid=%d\n", sizeof(pnfsid));
                for (j = PNFSIDMIN; j < PNFSIDMAX; j += 0x8) {
		    pnfsid.pnfs1 = j;
/* 		    fprintf(stderr,"%04X%04X%08lX%08lX\n", pnfsid.dbid, pnfsid.dummy, pnfsid.pnfs2, pnfsid.pnfs1); */
                    return_data = gdbm_fetch (gdbm_file, key_data);
		    if (return_data.dptr == NULL) {
		        continue;
		    }
                    if (count++ % 10000 == 0) {
		        mdxCommit(db); 
		        mdxBegin(db);
                        ctim = time(NULL);
                        dtim = ctim - otim;
                        otim = ctim;
                        fprintf(stderr, "Put record #%d into the database...", count);
                        fprintf(stderr,"time=%d\n", dtim);
                    }
                    /* Store (key_data, return_data) into Postgres DB */
                    key.dptr = key_data.dptr;     key.dsize = key_data.dsize;
                    val.dptr = return_data.dptr;  val.dsize = return_data.dsize;
                    /*DEBUG*/
                    if (!memcmp(key.dptr,"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00",6)) {
		      /* \000\000\000\000\000\000\000\000\000\001\000\000 */
/*                       fprintf(stderr, "key 000000000000000000010000 is found at %d\n", count-1); */
                      fprintf(stderr, "key 000000000000xxxxxxxxxxxx is found at %d\n", count-1);
                    }
		    /*
		    for (i = 0; i < 12; i++) {
		      fprintf(stderr, "%2.2X", (unsigned char)key.dptr[i]);
		    }
		    fprintf(stderr,"\n");
		    */
                    /*DEBUG*/
                    if (mdxStore(db, key, val) < 0) {
                        fprintf (stderr, "Can't store record %d into databse '%s'.\n", count, file_name);
                        break;
                    }
                    free (return_data.dptr);                          /* Free memory */
                }
		/* Special key */
		pnfsid.dbid = 0x0000; pnfsid.dummy = 0x0000; 
		pnfsid.pnfs2 = 0x00000000; pnfsid.pnfs1 = 0x00000100;

		key_data.dptr = (char *)&pnfsid;
		key_data.dsize = 12;

		fprintf(stderr,"%04X%04X%08lX%08lX\n", pnfsid.dbid, pnfsid.dummy, pnfsid.pnfs2, pnfsid.pnfs1);
		return_data = gdbm_fetch (gdbm_file, key_data);
		if (return_data.dptr == NULL) {
		  fprintf(stderr, "key 000000000000000000000100 is NOT found!\n");
		}
		/* Store (key_data, return_data) into Postgres DB */
		key.dptr = key_data.dptr;     key.dsize = key_data.dsize;
		val.dptr = return_data.dptr;  val.dsize = return_data.dsize;
		/*DEBUG*/
		if (!memcmp(key.dptr,"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00",6)) {
		  /* \000\000\000\000\000\000\000\000\000\001\000\000 */
/*                       fprintf(stderr, "key 000000000000000000010000 is found at %d\n", count-1); */
		  fprintf(stderr, "key 000000000000xxxxxxxxxxxx is found at %d\n", count-1);
		}
		/*DEBUG*/
		if (mdxStore(db, key, val) < 0) {
		  fprintf (stderr, "Can't store record %d into databse '%s'.\n", count, file_name);
		  break;
		}
		free (return_data.dptr);                          /* Free memory */

                mdxCommit(db); 
                mdxClose(db);
                fprintf (stderr, "There are %d records in the database.\n", count);
                fprintf (stderr, "Counter = %lx\n", j);
            }
 	    done = TRUE;
            break;

	case 't':
            {
                mdlDatum *key, keys[10001], val;
                int i, j;
                MDX_FILE db = mdxOpen(file_name, 0, 0 );
                otim = time(NULL);
                if (db == NULL) {
                    fprintf (stderr, "Can't open database '%s'.\n", file_name);
                    break;
                }
                
		key = keys;
                key[0] = mdxFirst(db);
                if (key->dptr == NULL) {
                    fprintf (stderr, "No such item found.\n");
                    break;
                }
                for (i = 0; i < 10000; i++) {
                    val = mdxNext(db, key[i]);                        /* Get next key */
                    key[i+1] = val;                                     /* Store next key for the next request */
                }
		mdxBreak(db);
# if 0                
		for (i = 0; i < 10000; i++) {
  		    for (j = 0; j < 12; j++) {
		        fprintf(stderr, "%2.2X", (unsigned char)key[i].dptr[j]);
		    }
                    fprintf (stderr, "\n");
		}
# endif
# if 1
		for (i = 0; i < 10000; i++) {
/* 		  fprintf(stderr, "trying to fetch key %d...\n", i); */
                    val = mdxFetch(db, key[i]);   
                    /* Store (key_data, return_data) into Postgres DB */
                    if (mdxStore(db, key[i], val) < 0) {
                        fprintf (stderr, "Can't store record %d into databse '%s'.\n", i, file_name);
                        break;
                    }
                    free(val.dptr);                                   /* Free memory */
                }
# endif
		ctim = time(NULL);
		dtim = ctim - otim;
		fprintf(stderr, "Update %d records in the database...", 10000);
		fprintf(stderr,"time=%d\n", dtim);

                mdxClose(db);
            }
 	    done = TRUE;
            break;
	case 'V':
	  printf ("%s\n\n", gdbm_version);
	  break;

	case '?':
 	  printf ("c - count (number of entries)\n"); 
 	  printf ("f - fetch\n"); 
 	  printf ("n - nextkey\n"); 
 	  printf ("q - quit\n"); 
 	  printf ("s - scan and convert (experimental!)\n"); 
	  printf ("g - convert db\n");
	  printf ("t - test db (10000 records)\n");
	  printf ("1 - firstkey\n");
	  printf ("2 - nextkey on last key (from n, 1 or 2)\n\n");
	  printf ("V - print version of gdbm\n");
	  break;

	default:
	  printf ("What? \n\n");
	  break;

	}
    }

  /* Quit normally. */
  exit (0);

}

