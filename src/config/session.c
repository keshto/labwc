// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <wlr/util/log.h>
#include "common/buf.h"
#include "common/spawn.h"
#include "common/string-helpers.h"

static bool
isfile(const char *path)
{
	struct stat st;
	return (!stat(path, &st));
}

static bool
string_empty(const char *s)
{
	return !s || !*s;
}

static void
process_line(char *line)
{
	if (string_empty(line) || line[0] == '#') {
		return;
	}
	char *key = NULL;
	char *p = strchr(line, '=');
	if (!p) {
		return;
	}
	*p = '\0';
	key = string_strip(line);

	struct buf value;
	buf_init(&value);
	buf_add(&value, string_strip(++p));
	buf_expand_shell_variables(&value);
	if (string_empty(key) || !value.len) {
		goto error;
	}
	setenv(key, value.buf, 1);
error:
	free(value.buf);
}

void
read_environment_file(const char *filename)
{
	char *line = NULL;
	size_t len = 0;
	FILE *stream = fopen(filename, "r");
	if (!stream) {
		return;
	}
	wlr_log(WLR_INFO, "read environment file %s", filename);
	while (getline(&line, &len, stream) != -1) {
		char *p = strrchr(line, '\n');
		if (p) {
			*p = '\0';
		}
		process_line(line);
	}
	free(line);
	fclose(stream);
}

static const char *
build_path(const char *dir, const char *filename)
{
	if (string_empty(dir) || string_empty(filename)) {
		return NULL;
	}
	int len = strlen(dir) + strlen(filename) + 2;
	char *buffer = calloc(len, 1);
	strcat(buffer, dir);
	strcat(buffer, "/");
	strcat(buffer, filename);
	return buffer;
}

void
session_environment_init(const char *dir)
{
	const char *environment = build_path(dir, "environment");
	if (!environment) {
		return;
	}
	read_environment_file(environment);
	free((void *)environment);
}

void
session_autostart_init(const char *dir)
{
	const char *autostart = build_path(dir, "autostart");
	if (!autostart) {
		return;
	}
	if (!isfile(autostart)) {
		wlr_log(WLR_ERROR, "no autostart file");
		goto out;
	}
	wlr_log(WLR_INFO, "run autostart file %s", autostart);
	int len = strlen(autostart) + 4;
	char *cmd = calloc(len, 1);
	strcat(cmd, "sh ");
	strcat(cmd, autostart);
	spawn_async_no_shell(cmd);
	free(cmd);
out:
	free((void *)autostart);
}
