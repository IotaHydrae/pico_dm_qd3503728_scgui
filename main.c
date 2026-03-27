// Copyright (c) 2024 embeddedboys developers

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico/time.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/stdio_uart.h"
#include "pico/stdio_usb.h"

#include "hardware/pll.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"

#include "ili9488.h"
#include "ft6236.h"
#include "backlight.h"

#include "sc_gui.h"
#include "sc_obj.h"
#include "sc_demo_test.h"

static void scgui_run_demos(bool loop);

bool scgui_tick(struct repeating_timer *t)
{
	system_tick++;
	sc_task_loop(NULL);
	return true;
}

void scgui_lcd_refresh_cb(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			  color_t *color)
{
	// printf("%s, %d, %d, %d, %d, %d\n", __func__, x, y, w, h, w * h);
	ili9488_video_flush(x, y, x + w - 1, y + h - 1, color,
			    w * h * sizeof(color_t));
}

int main(void)
{
	/* NOTE: DO NOT MODIFY THIS BLOCK */
#define CPU_SPEED_MHZ (DEFAULT_SYS_CLK_KHZ / 1000)
	if (CPU_SPEED_MHZ > 266 && CPU_SPEED_MHZ <= 360)
		vreg_set_voltage(VREG_VOLTAGE_1_20);
	else if (CPU_SPEED_MHZ > 360 && CPU_SPEED_MHZ <= 396)
		vreg_set_voltage(VREG_VOLTAGE_1_25);
	else if (CPU_SPEED_MHZ > 396)
		vreg_set_voltage(VREG_VOLTAGE_MAX);
	else
		vreg_set_voltage(VREG_VOLTAGE_DEFAULT);

	set_sys_clock_khz(CPU_SPEED_MHZ * 1000, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
			CPU_SPEED_MHZ * MHZ, CPU_SPEED_MHZ * MHZ);
	stdio_uart_init_full(uart0, 115200, 16, 17);
	stdio_usb_init();

	printf("\n\n\nPICO DM QD3503728 Display Template\n");

	ili9488_driver_init();
	ft6236_driver_init();
	sleep_ms(10);
	backlight_driver_init();
	backlight_set_level(100);
	printf("backlight set to 100%%\n");

	struct repeating_timer scgui_tick_timer;
	add_repeating_timer_ms(1, scgui_tick, NULL, &scgui_tick_timer);

	extern lv_font_t lv_font_20;
	sc_gui_init(scgui_lcd_refresh_cb, 0xffff, C_RED, C_BLUE, &lv_font_20);
	sc_clear(0, 0, LCD_HOR_RES, LCD_VER_RES, C_WHITE);

	scgui_run_demos(true);

	printf("going to loop, %lld\n", time_us_64() / 1000);
	for (;;) {
		// sc_task_loop(NULL);

		// if (ft6236_is_pressed())
		// 	printf("pressed at (%d, %d)\n", ft6236_read_x(),
		// 	       ft6236_read_y());

		tight_loop_contents();
		sleep_ms(200);
	}

	return 0;
}

struct sc_demo {
	u8 idx;
	const char *name;
	void (*func)(unsigned long data);
	unsigned long data;

	bool is_task;
};

#define DEFINE_SC_DEMO(idx, name, func, data, is_task) \
	{ idx, name, func, data, is_task }

static void sc_demo_colors(void)
{
	uint16_t colors[] = { C_RED,	 C_GREEN, C_BLUE, C_YELLOW,
			      C_MAGENTA, C_CYAN,  C_WHITE };
	for (int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
		sc_clear(0, 0, LCD_HOR_RES, LCD_VER_RES, colors[i]);

		sleep_ms(500);
	}
}

static const struct sc_demo sc_demos[] = {
	DEFINE_SC_DEMO(__COUNTER__, "colors", (void *)sc_demo_colors, 0, false),
	DEFINE_SC_DEMO(__COUNTER__, "rect_pfs", (void *)sc_demo_rect_pfs, 50,
		       false),
	DEFINE_SC_DEMO(__COUNTER__, "Image_zip", (void *)sc_demo_Image_zip, 0,
		       false),
	DEFINE_SC_DEMO(__COUNTER__, "text", (void *)sc_demo_text, 0, false),
	DEFINE_SC_DEMO(__COUNTER__, "commpose", (void *)sc_demo_commpose, 0,
		       false),
	DEFINE_SC_DEMO(__COUNTER__, "gif", (void *)sc_demo_gif_task, 50, true),
	DEFINE_SC_DEMO(__COUNTER__, "trans", (void *)sc_demo_trans_task, 50,
		       true),
	// DEFINE_SC_DEMO(__COUNTER__, "edit", (void *)sc_demo_edit_task, 0, false),
	DEFINE_SC_DEMO(__COUNTER__, "chart", (void *)sc_demo_chart_task, 50,
		       true),
	DEFINE_SC_DEMO(__COUNTER__, "menu", (void *)sc_demo_menu_task, 50,
		       true),
	DEFINE_SC_DEMO(__COUNTER__, "DrawEye", (void *)sc_demo_DrawEye_task, 50,
		       true),
	DEFINE_SC_DEMO(__COUNTER__, "watch", (void *)sc_watch_demo_tast, 50,
		       true),
	{ /* sentinel */ },
};

static void scgui_run_demos(bool loop)
{
	const struct sc_demo *demo;

	for (demo = sc_demos; demo->func; demo++) {
		printf("Run [%d] demo '%s'\n", demo->idx, demo->name);
		if (demo->is_task)
			sc_create_task(0, (void *)demo->func, demo->data);
		else
			demo->func(demo->data);

		sleep_ms(1000);
	}

	if (loop) {
		sc_delete_task(0);
		scgui_run_demos(loop);
	}
}
