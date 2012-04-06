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

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <stdint.h>

extern char dev_name[1024];

extern uint32_t left_code, right_code, down_code, up_code;
extern uint32_t toggle_code, mod_code;
extern uint32_t lbutton_code, mbutton_code, rbutton_code;

struct uint_str_tuple {
	const char *str;
	uint16_t uint;
};

/* We can do remap only for KEY, SW and BTN events */
enum event_types {
	EVENT_KEY = 0,
	EVENT_SW,
	EVENT_TYPES,
};

#define TYPE_SHIFT 16
#define TYPE_MASK 0xffff0000
#define CODE_MASK 0x0000ffff

/* KEY_CNT is a bit optimistic, but keeping 0xffff entries is an overkill
 * type is stored in most significant 16 bits, code in less significant
 */
extern uint32_t codes[EVENT_TYPES][KEY_CNT];
extern int background;
extern uint16_t type_linux_to_local[EV_CNT];
extern uint16_t type_local_to_linux[EVENT_TYPES];

void options_init(int argc, char *argv[]);

#endif
