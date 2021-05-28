#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

const struct device* display_dev;
lv_obj_t* display_center_pane;
lv_obj_t* display_top_bar;
lv_obj_t* display_bot_bar;
lv_obj_t* display_contacts_label;
lv_obj_t* display_risk_contacts_label;
lv_obj_t* risk_contacts_button;
lv_obj_t* display_count_label;
lv_obj_t* display_clock_label;
lv_obj_t* display_battery_label;
lv_obj_t* display_memory_label;
lv_obj_t* display_msg_label;
lv_style_t green_button_style;
lv_style_t yellow_button_style;
lv_style_t red_button_style;

int get_battery_percentage() {
	return 74;
}

int get_memory_percentage() {
	return 32;
}

int get_time() {
	return 1200;
}

int get_contacts() {
	return 314;
}

int risk_contacts = 0;

int get_risk_contacts() {
	return risk_contacts++;
}

int init_styles() {
	lv_style_init(&green_button_style);
	lv_style_init(&yellow_button_style);
	lv_style_init(&red_button_style);

	lv_style_set_bg_opa(&green_button_style, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_pad_top(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_bottom(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_left(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_right(&green_button_style, LV_STATE_DEFAULT, 10);
	lv_style_set_border_width(&green_button_style, LV_STATE_DEFAULT, 1);
	lv_style_set_radius(&green_button_style, LV_STATE_DEFAULT, 10);

	lv_style_copy(&yellow_button_style, &green_button_style);
	lv_style_copy(&red_button_style, &green_button_style);
	
	lv_style_set_bg_color(&green_button_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	lv_style_set_text_color(&green_button_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_bg_color(&yellow_button_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
	lv_style_set_text_color(&yellow_button_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_color(&red_button_style, LV_STATE_DEFAULT, LV_COLOR_RED);
	lv_style_set_text_color(&red_button_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);

	return 0;
}

int update_display() {
	static char display_contacts_str[30] = {0};
	static char display_risk_contacts_str[30] = {0};
	static char display_clock_str[6] = {0};
	static char display_battery_str[10] = {0};
	static char display_memory_str[10] = {0};

	printk("Update display\n");
	sprintf(display_contacts_str, "%d Kontakte erkannt, davon", get_contacts());
	lv_label_set_text(display_contacts_label, display_contacts_str);

	int risk_contacts = get_risk_contacts();
	if (risk_contacts == 0) {
		sprintf(display_risk_contacts_str, "keine Risiko-Kontakte");
	} else if (risk_contacts == 1) {
		sprintf(display_risk_contacts_str, "1 Risiko-Kontakt");
	} else {
		sprintf(display_risk_contacts_str, "%d Risiko-Kontakte", risk_contacts);
	}
	lv_label_set_text(display_risk_contacts_label, display_risk_contacts_str);
	
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


	int time = get_time();
	sprintf(display_clock_str, "%d:%02d", (time / 100) % 100, time % 100);
	lv_label_set_text(display_clock_label, display_clock_str);

	sprintf(display_battery_str, "Bat: %d%%", get_battery_percentage());
	lv_label_set_text(display_battery_label, display_battery_str);

	sprintf(display_memory_str, "Mem: %d%%", get_memory_percentage());
	lv_label_set_text(display_memory_label, display_memory_str);

    lv_task_handler();

	return 0;
}

int init_display() {

	init_styles();

	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
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
	
	update_display();

	lv_task_handler();
	display_blanking_off(display_dev);

	return 0;
}

int display_set_message(char* msg) {
	lv_label_set_text(display_msg_label, msg);
	return 0;
}