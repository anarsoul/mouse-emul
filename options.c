/*  
 *  mouse-emul - Tiny mouse emulator
 *  Copyright (C) 2011-2012 Vasily Khoruzhick (anarsoul@gmail.com)
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

#include "options.h"
#include "input_map.h"
#include "mouse-emul.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(*(a)))
#endif

char dev_name[4096];

uint32_t left_code = KEY_LEFT, right_code = KEY_RIGHT, down_code = KEY_DOWN, up_code = KEY_UP;
uint32_t toggle_code = KEY_OPTION, mod_code = KEY_LEFTALT;
uint32_t lbutton_code = KEY_ENTER, mbutton_code = KEY_PLAYCD, rbutton_code = KEY_STOPCD;
int background;

uint32_t codes[EVENT_TYPES][KEY_CNT];

uint16_t type_linux_to_local[EV_CNT] = {
	[EV_KEY] = EVENT_KEY,
	[EV_SW] = EVENT_SW,
};

uint16_t type_local_to_linux[EVENT_TYPES] = {
	[EVENT_KEY] = EV_KEY,
	[EVENT_SW] = EV_SW,
};

struct uint_str_tuple types_str[] = {
	{ .str = "KEY_", .uint = EVENT_KEY },
	{ .str = "SW_", .uint = EVENT_SW },
	{ .str = "BTN_", .uint = EVENT_KEY },
};

static const char short_options[] = "d:c:blh";

static const struct option long_options[] = {
	{"device", required_argument, NULL, 'd'},
	{"config", required_argument, NULL, 'c'},
	{"daemon", no_argument, NULL, 'b'},
	{"list", no_argument, NULL, 'l'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, 0, 0}
};

static void usage(int argc, char *argv[])
{
	printf("Usage: %s [options]\n\n"
	       "-d | --device name	Input device to use as source [/dev/input/event1]\n"
	       "                	  Use comma to separate multiple devices, i.e.\n"
	       "                	  /dev/input/event0,/dev/input/event1\n"
	       "-c | --config name	Config file [/etc/mouse-emu]\n"
	       "-b | --daemon		Run daemon in the background\n"
	       "-l | --list		List supported key codes\n"
	       "-h | --help		Print this message\n", argv[0]);
}

static void list_keycodes(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(linux_input_map); i++) {
		printf("%s\n", linux_input_map[i].str);
	}
}

/* XXX: This is definitely not an optimal solution
 * but it works only once at startup
 * so who cares?
 */
uint32_t get_code_for_str(const char *str)
{
	int i;
	uint32_t val;

	for (i = 0; i < ARRAY_SIZE(linux_input_map); i++) {
		if (strcmp(linux_input_map[i].str, str) == 0)
			break;
	}
	if (i == ARRAY_SIZE(linux_input_map))
		return 0;

	val = linux_input_map[i].uint;

	for (i = 0; i < ARRAY_SIZE(types_str); i++) {
		if (strcmp(str, types_str[i].str) == 0) {
			val |= (types_str[i].uint << TYPE_SHIFT);
		}
	}

	return val;
}

#define EXTRACT_RVALUE \
	code2 = get_code_for_str(ptr + 1); \
	if (code2 == 0) {\
		warn("Unknown code %s at %d\n", ptr + 1, lineno); \
		continue; \
	}

static void parse_config(const char *filename)
{
	FILE *in;
	char line[1024], *ptr;
	uint32_t code, code2;
	int lineno = 0;

	in = fopen(filename, "r");
	if (!in) {
		warn("Could not open config file %s: %s\n", filename, strerror(errno));
		return;
	}

	while (fgets(line, sizeof(line), in)) {
		lineno++;

		while ((ptr = strchr(line, '\n')) != NULL) {
			*ptr = '\0';
		}
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
			} else if (strcmp(line, "toggle") == 0) {
				EXTRACT_RVALUE;
				toggle_code = code2;
			} else if (strcmp(line, "mod") == 0) {
				EXTRACT_RVALUE;
				mod_code = code2;
			} else if (strcmp(line, "lbutton") == 0) {
				EXTRACT_RVALUE;
				lbutton_code = code2;
			} else if (strcmp(line, "rbutton") == 0) {
				EXTRACT_RVALUE;
				rbutton_code = code2;
			} else if (strcmp(line, "mbutton") == 0) {
				EXTRACT_RVALUE;
				mbutton_code = code2;
			} else {
				code = get_code_for_str(line);
				if (code != 0) {
					EXTRACT_RVALUE;
					printf("mapping code %x to code %x\n", code, code2);
					codes[(code & TYPE_MASK) >> TYPE_SHIFT][code & CODE_MASK] = code2;
				} else {
					warn("Uknown code %s at line %d\n", line, lineno);
				}
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
	int i;
	char config_name[1024];

	for (i = 0; i < EVENT_TYPES; i++)
		memset(codes[i], 0, sizeof(codes[i]));
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
		case 'l':
			list_keycodes();
			exit(EXIT_SUCCESS);
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
