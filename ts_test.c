/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_test.c,v 1.6 2004/10/19 22:01:27 dlowder Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "tslib.h"
#include "fbutils.h"
double opVal, value;
int opSort;
/*is caculate*/
int isCaculate = 0;

static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

struct ts_button {
	int x, y, w, h;
	char *text;
	int flags;
#define BUTTON_ACTIVE 0x00000001
};

/* [inactive] border fill text [active] border fill text */
static int button_palette [6] =
{
	1, 4, 2,
	1, 5, 0
};

#define NR_BUTTONS 18
static struct ts_button buttons [NR_BUTTONS];

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void button_draw (struct ts_button *button)
{
	int s = (button->flags & BUTTON_ACTIVE) ? 3 : 0;
	rect (button->x, button->y, button->x + button->w - 1,
	      button->y + button->h - 1, button_palette [s]);
	fillrect (button->x + 1, button->y + 1,
		  button->x + button->w - 2,
		  button->y + button->h - 2, button_palette [s + 1]);
	put_string_center (button->x + button->w / 2,
			   button->y + button->h / 2,
			   button->text, button_palette [s + 2]);
}

static void refresh_screen ()
{
	int i;

	/*draw a rectangle*/
	fillrect (0, 0, xres - 1, yres - 1, 0);
	//put_string_center (xres/2, yres/4,   "TSLIB test program", 1);
	//put_string_center (xres/2, yres/4+20,"Touch screen to move crosshair", 2);

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]);
}

double caculate(int op){
	switch(op){
	case 1:
		opVal += value;
		break;
	case 2:
		opVal -= value;
		break;
	case 3: 
		opVal *= value;
		break;
	case 4:
		opVal /= value;
		break;
	}
	value = opVal;
	opSort = 0;
}

static int button_handle (struct ts_button *button, struct ts_sample *samp)
{
	int inside = (samp->x >= button->x) && (samp->y >= button->y) &&
		(samp->x < button->x + button->w) &&
		(samp->y < button->y + button->h);

	if (samp->pressure > 0) {
		if (inside) {
			if (!(button->flags & BUTTON_ACTIVE)) {
				button->flags |= BUTTON_ACTIVE;
				button_draw (button);

				/*Calculator Process*/
				if(button->text[0] == 'C'){ // Clear
					opVal = 0;
					value = 0;
					isCaculate = 0;
				}
				else if(button->text[0] == "D"){ // DELete
					opVal = 0;
					value /= 10;
					isCaculate = 0;
				}
				else if(button->text[0] == '+'){
					if(opSort)
						caculate(opSort);
					opVal = value;
					value = 0;
					opSort = 1;
					isCaculate = 0;
				}
				else if(button->text[0] == '-'){
					if(opSort)
						caculate(opSort);
					opVal = value;
					value = 0;
					opSort = 2;
					isCaculate = 0;
				}
				else if(button->text[0] == '*'){
					if(opSort)
						caculate(opSort);
					opVal = value;
					value = 0;
					opSort = 3;
					isCaculate = 0;
				}
				else if(button->text[0] == '/'){
					if(opSort)
						caculate(opSort);
					opVal = value;
					value = 0;
					opSort = 4;
					isCaculate = 0;
				}
				else if(button->text[0] == '='){
					switch(opSort){
					case 1:
						opVal += value;
						break;
					case 2:
						opVal -= value;
						break;
					case 3: 
						opVal *= value;
						break;
					case 4:
						opVal /= value;
						break;
					}
					value = opVal;
					isCaculate = 1;
					opSort = 0;
				}
				else{ //number
					if(isCaculate){
						isCaculate = 0;
						value = 0;
					}
					value = 10*value + button->text[0] - '0';//char->int 
				}

				refresh_screen();

				char str_value[50];//int -> char for display
				sprintf(str_value,"%lf",value);
				put_string_center(xres/2,7*yres/50,str_value,1);

			}
		} else if (button->flags & BUTTON_ACTIVE) {
			button->flags &= ~BUTTON_ACTIVE;
			button_draw (button);
		}
	} else if (button->flags & BUTTON_ACTIVE) {
		button->flags &= ~BUTTON_ACTIVE;
		button_draw (button);
                return 1;
	}

        return 0;
}


int main()
{
	struct tsdev *ts;
	int x, y;
	unsigned int i;
	unsigned int mode = 0;

	char *tsdevice=NULL;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	if ((tsdevice = getenv("TSLIB_TSDEVICE")) == NULL) {
#ifdef USE_INPUT_API
		tsdevice = strdup ("/dev/input/event0");
#else
		tsdevice = strdup ("/dev/touchscreen/ucb1x00");
#endif /* USE_INPUT_API */
        }

	ts = ts_open (tsdevice, 0);

	if (!ts) {
		perror (tsdevice);
		exit(1);
	}

	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	x = xres/2;
	y = yres/2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	memset (&buttons, 0, sizeof (buttons));
	/*buttons [0].w = buttons [1].w = xres / 4;
	buttons [0].h = buttons [1].h = 20;
	buttons [0].x = xres / 4 - buttons [0].w / 2;
	buttons [1].x = (3 * xres) / 4 - buttons [0].w / 2;
	buttons [0].y = buttons [1].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	buttons [0].*/

	/*weight height*/
	for(i=1; i<17; i++){
		buttons [i].w = xres / 7;
		buttons [i].h = yres / 5;
	}	

	buttons [0].w = (2*xres/7) + (xres/49);
	buttons [0].h = yres / 5;

	/*result a half of xres and position is center*/
	buttons [17].w = xres / 2;
	buttons [17].h = yres / 5;
	buttons [17].text = "";

	/*value*/
	buttons [0].text = "0";
	buttons [1].text = "1";
	buttons [2].text = "2";
	buttons [3].text = "3";
	buttons [4].text = "4";
	buttons [5].text = "5";
	buttons [6].text = "6";
	buttons [7].text = "7";
	buttons [8].text = "8";
	buttons [9].text = "9";
	buttons [10].text = "+";
	buttons [11].text = "-";
	buttons [12].text = "*";
	buttons [13].text = "/";
	buttons [14].text = "=";
	buttons [15].text = "C";
	buttons [16].text = "D";

	/*
	 *800 * 480 
	 *
	 *such design: the y-blank is yres/25 = 96px, x-blank is xres/49 = 114px; 
	 * */
	
	/*
	 *
	 *	   	17
	 *15 16 10 11 12 13 
	 *1  2  3  4  5  14
	 *6  7  8  9	0  
	 *
	 * */

	/*the x position of buttons*/
	buttons [15].x = buttons [1].x = buttons [6].x = xres / 49;
	buttons [16].x = buttons [2].x = buttons [7].x = (xres/7) + (2*xres/49);
	buttons [10].x = buttons [3].x = buttons [8].x = (2*xres/7) + (3*xres/49);
	buttons [11].x = buttons [4].x = buttons [9].x = (3*xres/7) + (4*xres/49);
	buttons [12].x = buttons [5].x = buttons [0].x = (4*xres/7) + (5*xres/49);
	buttons [13].x = buttons [14].x = (5*xres/7) + (6*xres/49);

	/*the y position of buttons*/
	buttons [15].y =buttons [16].y =buttons [10].y =buttons [11].y =buttons [12].y = buttons [13].y = (yres/5) + (2*yres/25);
	buttons [1].y =buttons [2].y =buttons [3].y =buttons [4].y =buttons [5].y = buttons [14].y = (2*yres/5) + (3*yres/25);
	buttons [6].y =buttons [7].y =buttons [8].y =buttons [9].y =buttons [0].y = (3*yres/5) + (4*yres/25);

	/*result center*/
	buttons [17].x = xres / 4;
	buttons [17].y = yres / 25;

	/*refresh*/
	refresh_screen ();

	while (1) {
		struct ts_sample samp;
		int ret;

		// Show the cross 
		if ((mode & 15) != 1)
			put_cross(x, y, 2 | XORMODE);

		ret = ts_read(ts, &samp, 1);

		// Hide it 
		if ((mode & 15) != 1)
			put_cross(x, y, 2 | XORMODE);

		if (ret < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}

		if (ret != 1)
			continue;

		for (i = 0; i < NR_BUTTONS; i++)
			if (button_handle (&buttons [i], &samp))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen ();
					break;
				case 1:
					mode = 1;
					refresh_screen ();
					break;
				}

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

		if (samp.pressure > 0) {
			if (mode == 0x80000001)
				line (x, y, samp.x, samp.y, 2);
			x = samp.x;
			y = samp.y;
			mode |= 0x80000000;
		} else
			mode &= ~0x80000000;
	}
	close_framebuffer();
}
