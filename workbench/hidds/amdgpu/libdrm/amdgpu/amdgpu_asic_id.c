/*
 * Copyright © 2017 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// secure_getenv requires _GNU_SOURCE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"

static int parse_one_line(struct amdgpu_device *dev, const char *line)
{
	char *buf, *saveptr;
	char *s_did;
	uint32_t did;
	char *s_rid;
	uint32_t rid;
	char *s_name;
	char *endptr;
	int r = -EINVAL;

	/* ignore empty line and commented line */
	if (strlen(line) == 0 || line[0] == '#')
		return -EAGAIN;

	buf = strdup(line);
	if (!buf)
		return -ENOMEM;

	/* device id */
	s_did = strtok_r(buf, ",", &saveptr);
	if (!s_did)
		goto out;

	did = strtol(s_did, &endptr, 16);
	if (*endptr)
		goto out;

	if (did != dev->info.asic_id) {
		r = -EAGAIN;
		goto out;
	}

	/* revision id */
	s_rid = strtok_r(NULL, ",", &saveptr);
	if (!s_rid)
		goto out;

	rid = strtol(s_rid, &endptr, 16);
	if (*endptr)
		goto out;

	if (rid != dev->info.pci_rev_id) {
		r = -EAGAIN;
		goto out;
	}

	/* marketing name */
	s_name = strtok_r(NULL, ",", &saveptr);
	if (!s_name)
		goto out;

	/* trim leading whitespaces or tabs */
	while (isblank(*s_name))
		s_name++;
	if (strlen(s_name) == 0)
		goto out;

	dev->marketing_name = strdup(s_name);
	if (dev->marketing_name)
		r = 0;
	else
		r = -ENOMEM;

out:
	free(buf);

	return r;
}

static void amdgpu_parse_proc_cpuinfo(struct amdgpu_device *dev)
{
	const char *search_key = "model name";
	const char *radeon_key = "Radeon";
	char *line = NULL;
	size_t len = 0;
	FILE *fp;

	fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return;
	}

	while (getline(&line, &len, fp) != -1) {
		char *saveptr;
		char *value;

		if (strncmp(line, search_key, strlen(search_key)))
			continue;

		/* check for parts that have both CPU and GPU information */
		value = strstr(line, radeon_key);

		/* get content after the first colon */
		if (value == NULL) {
			value = strstr(line, ":");
			if (value == NULL)
				continue;
			value++;
		}

		/* strip whitespace */
		while (*value == ' ' || *value == '\t')
			value++;
		saveptr = strchr(value, '\n');
		if (saveptr)
			*saveptr = '\0';

		/* Add AMD to the new string if it's missing from slicing/dicing */
		if (strncmp(value, "AMD", 3) != 0) {
			char *tmp = malloc(strlen(value) + 5);

			if (!tmp)
				break;
			sprintf(tmp, "AMD %s", value);
			dev->marketing_name = tmp;
		} else
			dev->marketing_name = strdup(value);
		break;
	}

	free(line);
	fclose(fp);
}

#if HAVE_SECURE_GETENV
static char *join_path(const char *dir, const char *file) {
	size_t dir_len = strlen(dir);
	size_t file_len = strlen(file);
	char *full_path = NULL;

	int need_slash = ((dir_len > 0) && (dir[dir_len - 1] != '/'));
	size_t total_len = dir_len + (need_slash ? 1 : 0) + file_len + 1; // +1 for null terminator

	if (dir_len == 0) {
		return strdup(file);
	}

	full_path = malloc(total_len);
	if (!full_path) {
		return NULL; // Memory allocation failed
	}

	strcpy(full_path, dir);
	if (need_slash) {
		full_path[dir_len] = '/';
		dir_len++;
	}
	strcpy(full_path + dir_len, file);

	return full_path;
}

static char **split_env_var(const char *env_var_content)
{
	char **ret = NULL;
	char *dup_env_val;
	int elements = 1;
	int index = 1;

	if (!env_var_content || env_var_content[0] == '\0')
		return NULL;

	for(char *p = (char *)env_var_content; *p; p++) {
		if (*p == ':')
			elements++;
	}

	dup_env_val = strdup(env_var_content);
	if (!dup_env_val) {
		return NULL;
	}
	ret = malloc(sizeof(char *) * (elements + 1));
	ret[0] = dup_env_val;
	for(char *p = (char *)dup_env_val; *p; p++) {
		if (*p == ':') {
			*p = 0;
			ret[index++] = p + 1;
		}
	}
	ret[index] = NULL; // ensure that the last element in the array is NULL
	return ret;
}

static void split_env_var_free(char **split_var)
{
	if (split_var) {
		// remember that the first element also points to the whole duplicated string,
		// which was modified in place by replacing ':' with '\0' characters
		free(split_var[0]);
		free(split_var);
	}
}

static char *find_asic_id_table(void)
{
	// first check the paths in AMDGPU_ASIC_ID_TABLE_PATHS environment variable
	const char *amdgpu_asic_id_table_paths = secure_getenv("AMDGPU_ASIC_ID_TABLE_PATHS");
	const char *file_name = NULL;
	char *found_path = NULL;
	char **paths = NULL;

	if (!amdgpu_asic_id_table_paths)
		return NULL;

	// extract the file name from AMDGPU_ASIC_ID_TABLE
	file_name = strrchr(AMDGPU_ASIC_ID_TABLE, '/');
	if (!file_name)
		return NULL;
	file_name++; // skip the '/'

	paths = split_env_var(amdgpu_asic_id_table_paths);
	if (!paths)
		return NULL;

	// for each path, join with file_name and check if it exists
	for (int i = 0; paths[i] != NULL; i++) {
		char *full_path = join_path(paths[i], file_name);
		if (!full_path) {
			continue;
		}
		if (access(full_path, R_OK) == 0) {
			found_path = full_path;
			break;
		}
	}
	split_env_var_free(paths);
	return found_path;
}
#endif

void amdgpu_parse_asic_ids(struct amdgpu_device *dev)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t n;
	int line_num = 1;
	int r = 0;

	char *amdgpu_asic_id_table_path = NULL;
#if HAVE_SECURE_GETENV
	// if this system lacks secure_getenv(), don't allow extra paths
	// for security reasons.
	amdgpu_asic_id_table_path = find_asic_id_table();
#endif
	// if not found, use the default AMDGPU_ASIC_ID_TABLE path
	if (!amdgpu_asic_id_table_path)
		amdgpu_asic_id_table_path = strdup(AMDGPU_ASIC_ID_TABLE);

	fp = fopen(amdgpu_asic_id_table_path, "r");
	if (!fp) {
		fprintf(stderr, "%s: %s\n", amdgpu_asic_id_table_path,
			strerror(errno));
		goto get_cpu;
	}

	/* 1st valid line is file version */
	while ((n = getline(&line, &len, fp)) != -1) {
		/* trim trailing newline */
		if (line[n - 1] == '\n')
			line[n - 1] = '\0';

		/* ignore empty line and commented line */
		if (strlen(line) == 0 || line[0] == '#') {
			line_num++;
			continue;
		}

		drmMsg("%s version: %s\n", amdgpu_asic_id_table_path, line);
		break;
	}

	while ((n = getline(&line, &len, fp)) != -1) {
		/* trim trailing newline */
		if (line[n - 1] == '\n')
			line[n - 1] = '\0';

		r = parse_one_line(dev, line);
		if (r != -EAGAIN)
			break;

		line_num++;
	}

	if (r == -EINVAL) {
		fprintf(stderr, "Invalid format: %s: line %d: %s\n",
			amdgpu_asic_id_table_path, line_num, line);
	} else if (r && r != -EAGAIN) {
		fprintf(stderr, "%s: Cannot parse ASIC IDs: %s\n",
			__func__, strerror(-r));
	}

	free(line);
	fclose(fp);

get_cpu:
	free(amdgpu_asic_id_table_path);
	if (dev->info.ids_flags & AMDGPU_IDS_FLAGS_FUSION &&
	    dev->marketing_name == NULL) {
		amdgpu_parse_proc_cpuinfo(dev);
	}
}
