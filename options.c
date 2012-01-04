/*  
 *  mouse-emul - Tiny mouse emulator
 *  Copyright (C) 2011 Vasily Khoruzhick (anarsoul@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

#include "mouse-emul.h"

char dev_name[1024];

int left_code = KEY_LEFT, right_code = KEY_RIGHT, down_code = KEY_DOWN, up_code = KEY_UP;
int toggle_code = KEY_OPTION, mod_code = KEY_LEFTALT;
int lbutton_code = KEY_ENTER, mbutton_code = KEY_PLAYCD, rbutton_code = KEY_STOPCD;
int background;
int codes[KEY_CNT];

static const char short_options[] = "d:c:bh";

static const struct option long_options[] = {
	{"device", required_argument, NULL, 'd'},
	{"config", required_argument, NULL, 'c'},
	{"daemon", no_argument, NULL, 'b'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, 0, 0}
};

static void usage(int argc, char *argv[])
{
	printf("Usage: %s [options]\n\n"
	       "-d | --device name	Input device to use as source [/dev/input/event1]\n"
	       "-c | --config name	Config file [/etc/mouse-emu]\n"
	       "-b | --daemon		Run daemon in the background\n"
	       "-h | --help		Print this message\n", argv[0]);
}

#define EXTRACT_RVALUE \
	errno = 0; \
	code2 = strtol(ptr + 1, NULL, 10); \
	if (errno) {\
		warn("Syntax error at %d\n", lineno); \
		continue; \
	} \
	if (code2 < 0 || code2 > KEY_MAX) { \
		warn("Code is out of range at line %d\n", lineno); \
		continue; \
	}

static void parse_config(const char *filename)
{
	FILE *in;
	char line[1024], *ptr;
	long int code, code2;
	int lineno = 0;

	in = fopen(filename, "r");
	if (!in) {
		warn("Could not open config file %s: %s\n", filename, strerror(errno));
		return;
	}

	while (fgets(line, sizeof(line), in)) {
		lineno++;

		ptr = strchr(line, '=');
		if (ptr) {
			*ptr = '\0';
			if (strcmp(line, "left") == 0) {
				EXTRACT_RVALUE;
				left_code = code2;
			} else if (strcmp(line, "right") == 0) {
				EXTRACT_RVALUE;
				right_code = code2;
			} else if (strcmp(line, "up") == 0) {
				EXTRACT_RVALUE;
				up_code = code2;
			} else if (strcmp(line, "down") == 0) {
				EXTRACT_RVALUE;
				down_code = code2;
			} else if (strcmp(line, "lbutton") == 0) {
				EXTRACT_RVALUE;
				lbutton_code = code2;
			} else if (strcmp(line, "rbutton") == 0) {
				EXTRACT_RVALUE;
				rbutton_code = code2;
			} else if (strcmp(line, "mbutton") == 0) {
				EXTRACT_RVALUE;
				mbutton_code = code2;
			} else if (strcmp(line, "toggle") == 0) {
				EXTRACT_RVALUE;
				toggle_code = code2;
			} else if (strcmp(line, "mod") == 0) {
				EXTRACT_RVALUE;
				mod_code = code2;
			} else {
				errno = 0;
				code = strtol(line, NULL, 10);
				if (errno) {
					warn("Syntax error at line %d\n", lineno);
					continue;
				}
				if (code < 0 || code > KEY_MAX) {
					warn("Code is out of range at line %d\n", lineno);
					continue;
				}

				EXTRACT_RVALUE;
				codes[code] = code2;
			}
		} else {
			warn("Syntax error at line %d\n", lineno);
			continue;
		}
	}

	fclose(in);
}

void options_init(int argc, char *argv[])
{
	char config_name[1024];

	memset(codes, 0, sizeof(codes));
	strcpy(dev_name, "/dev/input/event1");
	strcpy(config_name, "/etc/mouse-emulrc");

	for (;;) {
		int index;
		int c;

		c = getopt_long(argc, argv, short_options, long_options,
				&index);
		if (c < 0)
			break;

		switch (c) {
		case 0:	/* getopt_long() flag */
			break;
		case 'd':
			strncpy(dev_name, optarg, sizeof(dev_name));
			break;
		case 'c':
			strncpy(config_name, optarg, sizeof(config_name));
			break;
		case 'b':
			background = 1;
			break;
		case 'h':
			usage(argc, argv);
			exit(EXIT_SUCCESS);
		default:
			usage(argc, argv);
			exit(EXIT_FAILURE);
		}
	}

	parse_config(config_name);
}
