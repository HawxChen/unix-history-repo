/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)kvm_mkdb.c	5.14 (Berkeley) %G%";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <db.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <paths.h>

char *tmp;
#define basename(cp)	((tmp=rindex((cp), '/')) ? tmp+1 : (cp))

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	DB *db;
	int ch;
	char *nlistpath, *nlistname, dbtemp[MAXPATHLEN], dbname[MAXPATHLEN];

	while ((ch = getopt(argc, argv, "")) != EOF)
		switch((char)ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	nlistpath = argc > 1 ? argv[0] : _PATH_UNIX;
	nlistname = basename(nlistpath);

	(void)sprintf(dbtemp, "%skvm_%s.tmp", _PATH_VARRUN, nlistname);
	(void)sprintf(dbname, "%skvm_%s.db", _PATH_VARRUN, nlistname);
	(void)umask(0);
	db = dbopen(dbtemp, O_CREAT|O_WRONLY|O_EXCL,
	    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, DB_HASH, NULL);
	if (!db) {
		(void)fprintf(stderr,
		    "kvm_mkdb: %s: %s\n", dbtemp, strerror(errno));
		exit(1);
	}
	create_knlist(nlistpath, db);
	(void)(db->close)(db);
	if (rename(dbtemp, dbname)) {
		(void)fprintf(stderr, "kvm_mkdb: %s to %s: %s.\n",
		    dbtemp, dbname, strerror(errno));
		exit(1);
	}
	exit(0);
}

error(n)
	char *n;
{
	int sverr;

	sverr = errno;
	(void)fprintf(stderr, "kvm_mkdb: ");
	if (n)
		(void)fprintf(stderr, "%s: ", n);
	(void)fprintf(stderr, "%s\n", strerror(sverr));
	exit(1);
}

usage()
{
	(void)fprintf(stderr, "usage: kvm_mkdb [file]\n");
	exit(1);
}
