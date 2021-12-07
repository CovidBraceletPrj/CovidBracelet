#if CONFIG_DISPLAY
#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#include "display.h"

K_THREAD_STACK_DEFINE(display_stack_area, 5000);

#if CONFIG_DISPLAY
const struct device* display_dev;
lv_obj_t* display_center_pane;
lv_obj_t* display_top_bar;
lv_obj_t* display_bot_bar;
lv_obj_t* display_contacts_label;
lv_obj_t* display_risk_contacts_label;
lv_obj_t* risk_contacts_button;
lv_obj_t* display_clock_label;
lv_obj_t* display_battery_label;
lv_obj_t* display_memory_label;
lv_obj_t* display_msg_label;
lv_style_t green_button_style;
lv_style_t yellow_button_style;
lv_style_t red_button_style;
#endif


int get_battery_percentage() {
	// TODO: Implement
	return 0;
}

int get_memory_percentage() {
	// TODO: Implement
	return 0;
}

int get_time() {
	// TODO: Implement
	return 0;
}

int get_contacts() {
	// TODO: Implement
	return 0;
}

int get_risk_contacts() {
	// TODO: Implement
	return 0;
}

#if CONFIG_DISPLAY

void display_thread(void* arg1, void* arg2, void* arg3) {
	static uint32_t sleep_time;
	while (1) {
		sleep_time = lv_task_handler();		
		update_display();
		k_msleep(sleep_time);
	}
}

int init_styles() {
	lv_style_init(&green_button_style);
	lv_style_init(&yellow_button_style);
	lv_style_init(&red_button_style);

	// Properties for all styles
	lv_style_set_bg_opa(&green_button_style, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_pad_top(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_bottom(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_left(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_right(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_border_width(&green_button_style, LV_STATE_DEFAULT, 1);
	lv_style_set_radius(&green_button_style, LV_STATE_DEFAULT, 10);

	lv_style_copy(&yellow_button_style, &green_button_style);
	lv_style_copy(&red_button_style, &green_button_style);
	
	// Set different colors
	lv_style_set_bg_color(&green_button_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	lv_style_set_text_color(&green_button_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_bg_color(&yellow_button_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
	lv_style_set_text_color(&yellow_button_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_color(&red_button_style, LV_STATE_DEFAULT, LV_COLOR_RED);
	lv_style_set_text_color(&red_button_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);

	return 0;
}

int update_display() {
	display_set_contacts(get_contacts());
	display_set_risk_contacts(get_risk_contacts());
	display_set_time(get_time());
	display_set_bat(get_battery_percentage());
	display_set_mem(get_memory_percentage());

    lv_task_handler();

	return 0;
}

int init_display() {
	init_styles();

	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		printk("device not found.  Aborting test.");
		return -1;
	}

	display_top_bar = lv_cont_create(lv_scr_act(), NULL);
	lv_obj_set_width(display_top_bar, lv_obj_get_width(lv_scr_act()));
	lv_cont_set_layout(display_top_bar, LV_LAYOUT_PRETTY_TOP);

	display_bot_bar = lv_cont_create(lv_scr_act(), NULL);
	lv_obj_set_width(display_bot_bar, lv_obj_get_width(lv_scr_act()));
	lv_cont_set_layout(display_bot_bar, LV_LAYOUT_PRETTY_BOTTOM);

	display_center_pane = lv_cont_create(lv_scr_act(), NULL);
	lv_obj_align(display_center_pane, display_top_bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
	lv_obj_set_width(display_center_pane, lv_obj_get_width(lv_scr_act()));
	lv_obj_set_height(display_center_pane, lv_obj_get_height(lv_scr_act()) - lv_obj_get_height(display_top_bar) - lv_obj_get_height(display_bot_bar));
	lv_cont_set_layout(display_center_pane, LV_LAYOUT_COLUMN_MID);

	lv_obj_align(display_bot_bar, display_center_pane, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

	display_contacts_label = lv_label_create(display_center_pane, NULL);
	lv_obj_set_height_margin(display_contacts_label, 20);
	lv_label_set_text(display_contacts_label, "");

	if (IS_ENABLED(CONFIG_LVGL_POINTER_KSCAN)) {
		risk_contacts_button = lv_btn_create(display_center_pane, NULL);
		lv_btn_set_fit(risk_contacts_button, LV_FIT_TIGHT);
		display_risk_contacts_label = lv_label_create(risk_contacts_button, NULL);
	} else {
		display_risk_contacts_label = lv_label_create(display_center_pane, NULL);
	}

	lv_obj_set_height_margin(display_risk_contacts_label, 20);
	lv_label_set_text(display_risk_contacts_label, "");

    display_memory_label = lv_label_create(display_top_bar, NULL);
	lv_obj_set_width_margin(display_memory_label, 10);
	lv_label_set_text(display_memory_label, "");

	display_clock_label = lv_label_create(display_top_bar, NULL);
	lv_obj_set_width_margin(display_clock_label, 10);
	lv_label_set_text(display_clock_label, "");

	display_battery_label = lv_label_create(display_top_bar, NULL);
	lv_obj_set_width_margin(display_battery_label, 10);
	lv_label_set_text(display_battery_label, "");

	display_msg_label = lv_label_create(display_bot_bar, NULL);
	lv_label_set_text(display_msg_label, "");

    static struct k_thread display_thread_data;
    k_thread_create(&display_thread_data, display_stack_area, K_THREAD_STACK_SIZEOF(display_stack_area), display_thread, NULL, NULL, NULL, 0, 0, K_NO_WAIT);

	display_blanking_off(display_dev);

	return 0;
}

int display_set_message(char* msg) {
	lv_label_set_text(display_msg_label, msg);
	return 0;
}

int display_set_time(int time) {
	lv_label_set_text_fmt(display_clock_label, "%d:%d", time / 100, time % 100)
	return 0;
}

int display_set_bat(int bat) {
	lv_label_set_text_fmt(display_battery_label, "Bat: %d%%", bat);
	return 0;
}

int display_set_mem(int mem) {
	lv_label_set_text_fmt(display_memory_label, "Mem: %d%%", mem);
	return 0;
}

int display_set_contacts(int contacts) {
	lv_label_set_text_fmt(display_contacts_label, "Number of contacts: %d,\nfrom which", contacts);
	return 0;
}

int display_set_risk_contacts(int risk_contacts) {
	lv_label_set_text_fmt(display_risk_contacts_label, "%d are risk contacts.", risk_contacts);
	
	if (risk_contacts == 0) {
		// Set Button green
		lv_obj_reset_style_list(risk_contacts_button, LV_OBJ_PART_MAIN);
		lv_obj_add_style(risk_contacts_button, LV_BTN_PART_MAIN, &green_button_style);
	} else if (risk_contacts < 5) {
		// Set Button yellow
		lv_obj_reset_style_list(risk_contacts_button, LV_OBJ_PART_MAIN);
		lv_obj_add_style(risk_contacts_button, LV_BTN_PART_MAIN, &yellow_button_style);
	} else {
		// Set Button red
		lv_obj_reset_style_list(risk_contacts_button, LV_OBJ_PART_MAIN);
		lv_obj_add_style(risk_contacts_button, LV_BTN_PART_MAIN, &red_button_style);
	}
	return 0;
}

#else

void display_thread(void* arg1, void* arg2, void* arg3) {
	// Do nothing
}

int init_styles() {
	return 0;
}

int update_display() {
	return 0;
}

int init_display() {
	return 0;
}

int display_set_message(char* msg) {
	return 0;
}

int display_set_time(int time) {
	return 0;
}

int display_set_bat(int bat) {
	return 0;
}

int display_set_mem(int mem) {
	return 0;
}

int display_set_contacts(int contacts) {
	return 0;
}

int display_set_risk_contacts(int risk_contacts) {
	return 0;
}

#endif
