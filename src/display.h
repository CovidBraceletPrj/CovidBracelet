#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * @brief Initializes all display elements and adds them to the canvas.
 *
 * @return int
 */
int init_display();

/**
 * @brief Updates all information on the display using the get functions, defined by `display.c`. Alternatively the
 * values of the displayed elements can be set explicitly via set `display_set_<property>`-functions.
 *
 * @return int
 */
int update_display();

/**
 * @brief This is the entry point for the display thread. It peridocially updates the display and handles input events.
 *
 */
void display_thread(void*, void*, void*);

/**
 * @brief Displays a message at the bottom of the display. The message will persist until it is removed expicitly, by calling `display_set_message("")`.
 * 
 * @param msg A pointer to the string that should be displayed
 * @return int 
 */
int display_set_message(char* msg);

/**
 * @brief Sets the on display clock. The time should have the format `h * 100 + m`. So `display_set_time(1145)` sets the clock to 11:45.
 * 
 * @param time A number representation of the time
 * @return int 
 */
int display_set_time(int time);

/**
 * @brief Set the battery level percentage.
 * 
 * @param bat The battery level in percent
 * @return int 
 */
int display_set_bat(int bat);

/**
 * @brief Set the non-volatile memory state.
 * 
 * @param mem The occupied memory in percent
 * @return int 
 */
int display_set_mem(int mem);

/**
 * @brief Set the number of total registered contacts in the last 14 days.
 * 
 * @param contacts The number of contacts
 * @return int 
 */
int display_set_contacts(int contacts);

/**
 * @brief Set the number of contacts that were tested positively on Covid and registered in the last 14 days.
 * 
 * @param risk_contacts The number of risk contacts
 * @return int 
 */
int display_set_risk_contacts(int risk_contacts);

#endif