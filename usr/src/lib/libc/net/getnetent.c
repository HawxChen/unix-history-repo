/*	getnetent.c	4.3	82/10/06	*/

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

#define	MAXALIASES	35

static char *NETDB = "/usr/lib/networks";
static FILE *netf = NULL;
static char line[BUFSIZ+1];
static struct netent net;
static char *net_aliases[MAXALIASES];
static int stayopen = 0;
static char *any();

setnetent(f)
	int f;
{
	if (netf == NULL)
		netf = fopen(NETDB, "r" );
	else
		rewind(netf);
	stayopen |= f;
}

endnetent()
{
	if (netf && !stayopen) {
		fclose(netf);
		netf = NULL;
	}
}

struct netent *
getnetent()
{
	char *p;
	register char *cp, **q;

	if (netf == NULL && (netf = fopen(NETDB, "r" )) == NULL)
		return (NULL);
again:
	p = fgets(line, BUFSIZ, netf);
	if (p == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	net.n_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	net.n_net = inet_addr(cp);
	net.n_addrtype = AF_INET;
	net.n_aliases = net_aliases;
	q = net_aliases, cp = p;
	while (*cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &net_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (*cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&net);
}

static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}
