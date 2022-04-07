#include "stm32f0xx_hal.h"

void LCD_init (void);   // initialize LCD

void LCD_send_cmd (char cmd);  // send command to the LCD

void LCD_send_data (char data);  // send data to the LCD

void LCD_send_string (char *str);  // send string to the LCD

void LCD_set_cursor_to_line(int row);  // put cursor at the entered line's start;

void LCD_start_routine(); //default start up display

void LCD_clear (void);
