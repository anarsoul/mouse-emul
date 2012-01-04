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

#ifndef __OPTIONS_H
#define __OPTIONS_H

extern char dev_name[1024];

extern int left_code, right_code, down_code, up_code;
extern int toggle_code, mod_code;
extern int lbutton_code, mbutton_code, rbutton_code;
extern int codes[KEY_CNT];
extern int background;

void options_init(int argc, char *argv[]);

#endif
