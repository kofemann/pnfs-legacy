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
#include <unistd.h>
#include <stdio.h>

main()
{
       char *cwd;
       if ((cwd = getcwd(NULL, 64)) == NULL) {
              perror("pwd");
              exit(2);
       }

       (void)printf("%s\n", cwd);
       return(0);
}
-
