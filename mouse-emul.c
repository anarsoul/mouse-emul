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
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "mouse-emul.h"
#include "options.h"

#define MAX_ACCEL 24
#define ACCEL_DIVIDOR 3
#define POLL_TIMEOUT_MS 1000

static int want_to_exit;

void sighandler(int signum)
{
	switch (signum) {
	case SIGTERM:
	case SIGINT:
		want_to_exit = 1;
		break;
	default:
		/* TODO: print some usefull info on SIGUSR1/SIGUSR2 */
		warn("Got signal %d\n", signum);
		break;

	}
}

void die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void warn(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
}

int send_event(int ufile, __u16 type, __u16 code, __s32 value)
{
	struct input_event event;

	memset(&event, 0, sizeof(event));
	event.type = type;
	event.code = code;
	event.value = value;

	if (write(ufile, &event, sizeof(event)) != sizeof(event)) {
		warn("Error during event sending");
		return -1;
	}

	return 0;
}

void process_event(int ufile, struct input_event evt)
{
	static int enabled, tmp_enabled;
	static int dx, dy;
	static int moving, accel;

	if ((evt.code == toggle_code) && (evt.value == 1))
		enabled ^= evt.value;

	if (evt.code == mod_code)
		tmp_enabled = (evt.value == 1);

	/* No emulation enabled? Passthrough event */
	if (!enabled && !tmp_enabled) {
		send_event(ufile, EV_KEY, evt.code, evt.value);
		send_event(ufile, EV_SYN, SYN_REPORT, 0);
		return;
	}

	if (evt.code == up_code) {
		if (evt.value == 0)
			moving--;
		else if (evt.value == 1)
			moving++;
		dy = evt.value == 0 ? 0 : -1;
	} else if (evt.code == down_code) {
		if (evt.value == 0)
			moving--;
		else if (evt.value == 1)
			moving++;
		dy = evt.value == 0 ? 0 : 1;
	} else if (evt.code == right_code) {
		if (evt.value == 0)
			moving--;
		else if (evt.value == 1)
			moving++;
		dx = evt.value == 0 ? 0 : 1;
	} else if (evt.code == left_code) {
		if (evt.value == 0)
			moving--;
		else if (evt.value == 1)
			moving++;
		dx = evt.value == 0 ? 0 : -1;
	} else if (evt.code == lbutton_code) {
		send_event(ufile, EV_KEY, BTN_LEFT, evt.value);
	} else if (evt.code == rbutton_code) {
		send_event(ufile, EV_KEY, BTN_RIGHT, evt.value);
	} else if (evt.code == mbutton_code) {
		send_event(ufile, EV_KEY, BTN_MIDDLE, evt.value);
	} else {
		if (codes[evt.code])
			send_event(ufile, EV_KEY, codes[evt.code], evt.value);
		else
			send_event(ufile, EV_KEY, evt.code, evt.value);
	}

	/* Clamp value */
	moving = moving < 0 ? 0 : moving;
	moving = moving > 4 ? 4 : moving;

	if (moving) {
		if (accel < MAX_ACCEL)
			accel++;

		send_event(ufile, EV_REL, REL_X, dx * (1 + accel / ACCEL_DIVIDOR));
		send_event(ufile, EV_REL, REL_Y, dy * (1 + accel / ACCEL_DIVIDOR));
	} else
		accel = 0;

	send_event(ufile, EV_SYN, SYN_REPORT, 0);
}

int main(int argc, char *argv[])
{
	int evdev;
	int ufile, i, cnt, res;
	int value;
	struct input_event ev[64];
	struct pollfd pollfd;

	struct uinput_user_dev uinp;
	struct input_event event;

	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGUSR1, sighandler);
	signal(SIGUSR2, sighandler);

	options_init(argc, argv);

	ufile = open("/dev/input/uinput", O_WRONLY);
	if (ufile == -1)
		ufile = open("/dev/uinput", O_WRONLY);

	if (ufile == -1)
		die("Could not open uinput: %s\n", strerror(errno));

	evdev = open(dev_name, O_RDONLY);
	if (evdev == -1)
		die("Could not open %s: %s\n", dev_name, strerror(errno));

	res = ioctl(evdev, EVIOCGRAB, &value);
	if (res)
		die("Could not grab %s: %s\n", dev_name, strerror(errno));

	/* Everything is ready, it's time to go into background */
	if (background)
		daemon(0, 1);

	memset(&uinp, 0, sizeof(uinp));
	strncpy(uinp.name, EMU_NAME, sizeof(EMU_NAME));
	uinp.id.version = 4;
	uinp.id.bustype = BUS_USB;

	/* We can emit key and move relative events */
	ioctl(ufile, UI_SET_EVBIT, EV_KEY);
	ioctl(ufile, UI_SET_EVBIT, EV_REL);
	ioctl(ufile, UI_SET_RELBIT, REL_X);
	ioctl(ufile, UI_SET_RELBIT, REL_Y);

	/* Any key, any button */
	for (i = 0; i < KEY_MAX; i++)
		ioctl(ufile, UI_SET_KEYBIT, i);

	ioctl(ufile, UI_SET_KEYBIT, BTN_MOUSE);
	ioctl(ufile, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(ufile, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(ufile, UI_SET_KEYBIT, BTN_MIDDLE);

	res = write(ufile, &uinp, sizeof(uinp));

	if (ioctl(ufile, UI_DEV_CREATE) < 0)
		die("Error during input device creation: %s\n", strerror(errno));

	pollfd.fd = evdev;
	pollfd.events = POLLIN;
	while (!want_to_exit) {
		res = poll(&pollfd, 1, POLL_TIMEOUT_MS);

		if (!res || res == -1 || pollfd.revents != POLLIN)
			continue;

		cnt = read(evdev, ev, sizeof(ev));
		if (cnt == -1) {
			warn("Read returned error: %s\n", strerror(errno));
			break;
		}
		for (i = 0;
		     i < cnt / sizeof(struct input_event);
		     i++) {
			if (EV_KEY == ev[i].type)
				process_event(ufile, ev[i]);
		}
	}
	warn("%s: terminating...\n", argv[0]);
	ioctl(ufile, UI_DEV_DESTROY);
	close(ufile);
}
