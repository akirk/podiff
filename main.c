/* This file is part of PODIFF.
   Copyright (C) 2011 Sergey Poznyakoff

   PODIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   PODIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this PODIFF.  If not, see <http://www.gnu.org/licenses/>. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "podiff.h"

const char *progname;
int skip_fuzzy_option;
int test_option;
unsigned maxwidth = 76;
int keep_comments_option;
int git_diff_header;

char *diff_command = "diff";
char *tmpdir = "/tmp";
char *tmppattern = "podiff.XXXXXX";

void
error(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void *
emalloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		error("out of memory");
	return p;
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		error("out of memory");
	return p;
}	

#define ISWS(c) ((c) == ' ' || (c) == '\t')

void
format_string_n(FILE *fp, const char *prefix, int n, const char *str)
{
	size_t off;
	size_t width = 0;
	size_t init = 0;
	enum { s_seq, s_ws } state = ISWS(*str) ? s_ws : s_seq;
	
	init = fprintf(fp, "%s", prefix);
	if (n >= 0)
		init += fprintf(fp, "[%d]", n);
	fputc(' ', fp);
	init++;

	off = width = 0;
	while (1) {
		if (str[width + off] == 0 || str[width + off] == '\n') {
			if (state == s_seq && init + width + off < maxwidth) {
				width += off;
			}
			/* skip */;
		} else if (state == s_seq) {
			if (init + width + off >= maxwidth && width) {
				;
			} else {
				if (ISWS(str[width + off])) {
					state = s_ws;
					width += off;
					off = 0;
				} else
					off++;
				continue;
			}
		} else /* state == s_ws */ {
		        if (init + width + off < maxwidth) {
				if (ISWS(str[width + off]))
					width++;
				else
					state = s_seq;
				continue;
			} else {
				width += off;
			}
		}
		off = 0;
		
		if (!width)
			break;
		
		fputc('"', fp);
		fwrite(str, width, 1, fp);
		str += width;
		width = off = 0;
		if (*str == '\n') {
			fwrite("\\n", 2, 1, fp);
			str++;
		}
		fputc('"', fp);
		fputc('\n', fp);
		init = 0;
	}

	if (init)
		fwrite("\"\"\n", 3, 1, fp);
}
	
void
format_string(FILE *fp, const char *prefix, const char *str)
{
	format_string_n(fp, prefix, -1, str);
}

void
msgtrans_to_file(struct list *lp, FILE *fp)
{
	struct list_entry *ent;
	for (ent = lp->head; ent; ent = ent->next) {
		struct msgtrans *p = ent->data;

		if (p->comment) {
			struct list_entry *strent;

			for (strent = p->comment->head; strent;
			     strent = strent->next) {
				char *str = strent->data;
				fprintf(fp, "%s\n", str);
			}
		}
					
			
		format_string(fp, "msgid", p->msgid);
		if (p->msgid_plural)
			format_string(fp, "msgid_plural", p->msgid);
		if (list_size(p->msgstr) == 1)
			format_string(fp, "msgstr",
				      list_index(p->msgstr, 0));
		else {
			int i;
			struct list_entry *strent;

			for (strent = p->msgstr->head, i = 0; strent;
			     strent = strent->next, i++) {
				char *str = strent->data;
				format_string_n(fp, "msgstr", i, str);
			}
		}
		fputc('\n', fp);
	}
}

char *
po_file_normalize(const char *file)
{
	struct list *lp;
	mode_t m;
	char *tmpname;
	int fd;
	FILE *fp;
	
	lp = parse_po(file);
	list_qsort(lp);

	m = umask(077);
	tmpname = emalloc (strlen(tmpdir) + 1 + strlen(tmppattern) + 1);
	sprintf(tmpname, "%s/%s", tmpdir, tmppattern);
	fd = mkstemp(tmpname);
	umask(m);
	if (fd == -1)
		error("cannot create temporary: %s", strerror(errno));
	fp = fdopen(fd, "w");
	msgtrans_to_file(lp, fp);
	fclose(fp);
	return tmpname;
}

void
help(FILE *fp)
{
	fprintf(fp, "usage: %s [OPTIONS] FILE1 FILE2\n", progname);
	fprintf(fp, "Compare two PO files\n"); 
	fprintf(fp, "   or: %s [OPTIONS] PATH OLD-FILE OLD-HEX OLD-MODE NEW-FILE NEW-HEX NEW-MODE\n", progname);
	fprintf(fp, "Compare two revisions of a Git-controlled file\n"); 
	fprintf(fp, "   or: %s -t [OPTIONS] FILE\n", progname);
	fprintf(fp, "Test mode: create a normalized version of FILE on stdout\n"); 
	fprintf(fp, "   or: %s -v\n", progname);
	fprintf(fp, "Print version information\n");
	fprintf(fp, "   or: %s -h\n", progname);
	fprintf(fp, "Print this help summary\n");
	fprintf(fp, "\nOPTIONS:\n");
	fprintf(fp, " -d CMD  use CMD instead of the default \"diff\"\n");
	fprintf(fp, " -D OPT  pass OPT to diff\n");
	fprintf(fp, " -f      skip fuzzy translations\n");
	fprintf(fp, " -k      keep user comments in output\n");
	fprintf(fp, " -w NUM  set output text width to NUM colums\n");
	fprintf(fp, "\nReport bugs to <gray@gnu.org>.\n");
}

void
show_version()
{
	puts("podiff " VERSION "\n"
"Copyright (C) 2011 Sergey Poznyakoff\n"
"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n");
}

int
main(int argc, char **argv)
{
	int rc;
	struct txtacc *diffargs;
	char *tmp[2];
	char *args, *command;
	size_t size;
	int ignore_rc;

	progname = argv[0];
	diffargs = txtacc_create();
	while ((rc = getopt(argc, argv, "d:D:fhktvw:")) != EOF) {
		switch (rc) {
		case 'f':
			skip_fuzzy_option = 1;
			break;

		case 'd':
			diff_command = optarg;
			break;
			
		case 'D':
			txtacc_grow(diffargs, " ", 1);
			txtacc_grow(diffargs, optarg, strlen(optarg));
			break;

		case 'k':
			keep_comments_option = 1;
			break;
			
		case 't':
			test_option = 1;
			break;
			
		case 'h':
			help(stdout);
			exit(0);

		case 'v':
			show_version();
			exit(0);

		case 'w':
			maxwidth = atoi(optarg);
			if (!maxwidth) {
				error("invalid width: %s", optarg);
				exit(2);
			}
			break;
			
		default:
			exit(2);
		}
	}

	argc -= optind;
	argv += optind;

	if (test_option) {
		struct list *lp;

		if (argc != 1) {
			error("wrong number of arguments");
			exit(2);
		}

		lp = parse_po(argv[0]);
		list_qsort(lp);
		msgtrans_to_file(lp, stdout);
		exit(0);
	}

	if (argc == 0) {
		help(stderr);
		exit(2);
	} else if (argc == 2) {
		/* Assume command line mode */
		tmp[0] = po_file_normalize(argv[0]);
		tmp[1] = po_file_normalize(argv[1]);
	} else {
		/* Assume Git mode.  Arguments: */
		/*  0      1        2        3       4        5        6    */
		/* path old-file old-hex old-mode new-file new-hex new-mode */
		if (argc != 7) {
			error("wrong number of arguments");
			exit(2);
		}
		
		tmp[0] = po_file_normalize(argv[1]);
		tmp[1] = po_file_normalize(argv[4]);
		ignore_rc = git_diff_header = 1;
	}
	
	txtacc_grow(diffargs, " ", 1);
	txtacc_grow(diffargs, tmp[0], strlen(tmp[0]));

	txtacc_grow(diffargs, " ", 1);
	txtacc_grow(diffargs, tmp[1], strlen(tmp[1]));
	
	args = txtacc_finish(diffargs, 0);
	size = strlen(diff_command) + 1 + strlen(args) + 1;
	command = emalloc(size);
	sprintf(command, "%s %s", diff_command, args);
	if (git_diff_header) {
		if (strcmp(argv[3], argv[6])) {
			printf("old mode %s\n", argv[3]);
			printf("new mode %s\n", argv[6]);
			printf("index %.8s..%.8s\n", argv[2], argv[5]);
		} else
			printf("index %.8s..%.8s %s\n", argv[2], argv[5], argv[6]);
		fflush(stdout);
	}
	rc = system(command);
	unlink(tmp[0]);
	unlink(tmp[1]);
	return ignore_rc ? 0 : WEXITSTATUS(rc);
}
