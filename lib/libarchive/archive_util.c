/*-
 * Copyright (c) 2003-2004 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archive_platform.h"
__FBSDID("$FreeBSD$");

#include <sys/types.h>

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif
#include <string.h>

#include "archive.h"
#include "archive_private.h"

int
archive_errno(struct archive *a)
{
	return (a->archive_error_number);
}

const char *
archive_error_string(struct archive *a)
{

	if (a->error != NULL  &&  *a->error != '\0')
		return (a->error);
	else
		return (NULL);
}


int
archive_format(struct archive *a)
{
	return (a->archive_format);
}

const char *
archive_format_name(struct archive *a)
{
	return (a->archive_format_name);
}


int
archive_compression(struct archive *a)
{
	return (a->compression_code);
}

const char *
archive_compression_name(struct archive *a)
{
	return (a->compression_name);
}

void
archive_set_error(struct archive *a, int error_number, const char *fmt, ...)
{
	va_list ap;
#ifdef HAVE_STRERROR_R
	char errbuff[512];
#endif
	char *errp;

	a->archive_error_number = error_number;
	if (fmt == NULL) {
		a->error = NULL;
		return;
	}

	va_start(ap, fmt);
	archive_string_vsprintf(&(a->error_string), fmt, ap);
	if(error_number > 0) {
		archive_strcat(&(a->error_string), ": ");
#ifdef HAVE_STRERROR_R
#ifdef STRERROR_R_CHAR_P
		errp = strerror_r(error_number, errbuff, sizeof(errbuff));
#else
		strerror_r(error_number, errbuff, sizeof(errbuff));
		errp = errbuff;
#endif
#else
		/* Note: this is not threadsafe! */
		errp = strerror(error_number);
#endif
		archive_strcat(&(a->error_string), errp);
	}
	a->error = a->error_string.s;
	va_end(ap);
}
