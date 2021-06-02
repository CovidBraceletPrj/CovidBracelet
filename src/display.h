#ifndef DISPLAY_H
#define DISPLAY_H

int init_display();

int update_display();

int display_set_message(char* msg);

int display_set_time();

int display_set_bat(int bat);

int display_set_mem(int mem);

int display_set_contacts(int contacts);

int display_set_risk_contacts(int risk_contacts);

#endif