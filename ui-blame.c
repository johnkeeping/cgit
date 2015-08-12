/* ui-blame.c: functions for showing blame output
 *
 * Copyright (C) 2015 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-blame.h"
#include "html.h"
#include "ui-shared.h"

static void start_output(void)
{
	cgit_print_layout_start();
	html("<table class='blob'>");
}

static void end_output(void)
{
	html("</table>");
	cgit_print_layout_end();
}

static int parse_init_line(const char *line, unsigned char sha1[20],
		unsigned long *orig_line, unsigned long *final_line,
		unsigned long *count)
{
	char *end;
	if (get_sha1_hex(line, sha1) < 0)
		return -1;

	*orig_line = strtoul(line + 41, &end, 10);
	/* We already have a count, so there isn't one on this line. */
	if (*count)
		return 0;

	end++;
	*final_line = strtoul(end, &end, 10);

	if (!*end)
		return -1;
	end++;
	*count = strtoul(end, NULL, 10);

	return 0;
}

static const char *author_abbrev(const char *author)
{
	static char buf[4];
	size_t idx = 0;
	const char *c = strrchr(author, ' ');
	buf[idx++] = *author;
	if (c && c != author)
		buf[idx++] = c[1];
	buf[idx] = '\0';
	return buf;
}

static int process_blame(FILE *out)
{
	/*
	 * The format is:
	 *	<sha1> <orig-line> <cur-line> [<count>]
	 *	<header> <value>
	 *	...
	 *	<TAB> <content>
	 *
	 * STATE_INIT means we expect a SHA1, otherwise it's a header or
	 * content depending on whether or not the line starts with <TAB>.
	 */
	enum {
		STATE_INIT,
		STATE_HEADER,
	} state = STATE_INIT;
	int output_started = 0;
	int first = 0;
	int previous = 0;
	unsigned long line = 1;
	unsigned long orig_line = 0;
	unsigned long final_line = 0;
	unsigned long count = 0;
	unsigned char sha1[20];
	struct strbuf buf = STRBUF_INIT;
	struct strbuf author = STRBUF_INIT;
	time_t author_time = 0;
	int author_tz = 0;

	while (strbuf_getline(&buf, out) != EOF) {
		const char *value;
		if (ferror(out))
			break;

		switch (state) {
		case STATE_INIT:
			first = !count;
			previous = 0;

			if (parse_init_line(buf.buf, sha1, &orig_line, &final_line, &count) < 0)
				goto err;

			if (!output_started) {
				start_output();
				output_started = 1;
			}
			html("<tr>");
			state++;
			break;

		case STATE_HEADER:
			if (buf.buf[0] == '\t') {
				if (first) {
					htmlf("<td rowspan='%lu' class='lines%s' title='",
							count, previous ? "" : " noprevious");
					html_attr(fmt("%s, %s", author.buf,
						show_date(author_time, author_tz, cgit_date_mode(DATE_ISO8601))));
					htmlf("'><pre>");
					cgit_commit_link(fmt("%s", find_unique_abbrev(sha1, DEFAULT_ABBREV)),
						NULL, NULL, NULL, sha1_to_hex(sha1), ctx.qry.path);
					if (count > 1) {
						html("<br>");
						html_txt(author_abbrev(author.buf));
					}
					html("</pre></td>");
					first = 0;
				}
				htmlf("<td class='linenumbers'><a name='l%lu' href='#l%lu'>%lu</a></td><td class='lines'><pre>",
						line, line, line);
				html_txt(buf.buf + 1);
				html("</pre></td></tr>");

				line++;
				count--;
				state = STATE_INIT;
			} else if (skip_prefix(buf.buf, "author ", &value)) {
				strbuf_reset(&author);
				strbuf_addstr(&author, value);
			} else if (skip_prefix(buf.buf, "author-time ", &value)) {
				author_time = (time_t) strtoul(value, NULL, 10);
			} else if (skip_prefix(buf.buf, "author-tz ", &value)) {
				strtol_i(value, 10, &author_tz);
			} else if (skip_prefix(buf.buf, "previous ", &value)) {
				previous++;
			}
			break;
		}
	}

	return output_started;
err:
	return -1;
}

void cgit_print_blame(void)
{
	FILE *out;
	int output_started = 0;
	struct child_process proc = CHILD_PROCESS_INIT;

	if (!ctx.qry.path) {
		cgit_print_error("Bad request");
		return;
	}

	argv_array_push(&proc.args, "blame");
	argv_array_push(&proc.args, "--line-porcelain");
	if (ctx.qry.ignorews)
		argv_array_push(&proc.args, "-w");
	argv_array_push(&proc.args, ctx.qry.sha1 ? ctx.qry.sha1 : ctx.qry.head);
	argv_array_push(&proc.args, "--");
	argv_array_push(&proc.args, ctx.qry.path);

	proc.out = -1;
	proc.no_stdin = 1;
	proc.no_stderr = 1;
	proc.git_cmd = 1;

	if (start_command(&proc) < 0)
		goto err;

	out = fdopen(proc.out, "r");
	if (!out)
		goto err;

	output_started = process_blame(out);

	if (finish_command(&proc) != 0) {
		if (!output_started) {
			cgit_print_error_page(404, "Not found", "Not found");
			return;
		}
		/* What to do? - we started getting data then something went wrong. */
	}

	if (output_started)
		end_output();

	return;
err:
	cgit_print_error_page(500, "Internal server error", "Failed to run blame");
}
