
/** Put this in the src folder **/

#include "i2c-lcd.h"
extern I2C_HandleTypeDef hi2c1;  // change your handler here accordingly

#define SLAVE_ADDRESS_LCD 0x4E // change this according to ur setup

void LCD_send_cmd (char cmd)
{
  char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0
	HAL_I2C_Master_Transmit (&hi2c1, SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4, 100);
}

void LCD_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	data_t[0] = data_u|0x0D;  //en=1, rs=0
	data_t[1] = data_u|0x09;  //en=0, rs=0
	data_t[2] = data_l|0x0D;  //en=1, rs=0
	data_t[3] = data_l|0x09;  //en=0, rs=0
	HAL_I2C_Master_Transmit (&hi2c1, SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4, 100);
}

void LCD_clear (void)
{
	LCD_send_cmd (0x01);
	/*for (int i=0; i<70; i++)
	{
		LCD_send_data (' ');
	}*/
}

//set the cursor to line start of line number row
void LCD_set_cursor_to_line(int row)
{
	int com;
    switch (row)
    {
			case 1:
				com = 0x80|0x00;
				break;
      case 2:
        com = 0x80|0x40;
        break;
      case 3:
        com = 0x80|0x14;
        break;
			case 4:
				com = 0x80|0x54;
    }

    LCD_send_cmd (com);
}


void LCD_init (void)
{
	// 4 bit initialisation
	HAL_Delay(50);  // wait for >40ms
	LCD_send_cmd (0x30);
	HAL_Delay(5);  // wait for >4.1ms
	LCD_send_cmd (0x30);
	HAL_Delay(1);  // wait for >100us
	LCD_send_cmd (0x30);
	HAL_Delay(10);
	LCD_send_cmd (0x20);  // 4bit mode
	HAL_Delay(10);

  // dislay initialisation
	LCD_send_cmd (0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	HAL_Delay(1);
	LCD_send_cmd (0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	HAL_Delay(1);
	LCD_send_cmd (0x01);  // clear display
	HAL_Delay(1);
	HAL_Delay(1);
	LCD_send_cmd (0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	HAL_Delay(1);
	LCD_send_cmd (0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
}

void LCD_send_string (char *str)
{
	while (*str) LCD_send_data (*str++);
}

/*

	char heart_row1[] = {0x20,0xff,0xff,0x20,0xff,0xff,0x20};
	char heart_row2[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	char heart_row3[] = {0x20,0xff,0xff,0xff,0xff,0xff,0x20};
	char heart_row4[] = {0x20,0x20,0x20,0xff,0x20,0x20,0x20};
	char weiny_row1[] = {0x20,0x20,0xff,0xff,0x20,0x20,0x20};
	char weiny_row2[] = {0x20,0x20,0xff,0xff,0x20,0x20,0x20};
	char weiny_row3[] = {0x20,0x20,0xff,0xff,0x20,0x20,0x20};
	char weiny_row4[] = {0xff,0xff,0xff,0xff,0xff,0xff,0x20};
	char empty_row[]	= {0x20,0x20,0x20,0x20,0x20,0x20,0x20};

	char *display[20] = {heart_row1,
											heart_row2,
											heart_row3,
											heart_row4,
											empty_row,
											empty_row,
											"       ",
											"       ",
											"       ",
											weiny_row1,
											weiny_row2,
											weiny_row3,
											weiny_row4,
											weiny_row4,
											"       ",
											"       ",
											"       ",
											"       ",
											"       ",
											"sendnudes"};
											
	LCD_send_cmd(0x80|0x0b);
  LCD_send_string (display[0]);
	LCD_send_cmd(0x80|0x4b);
	LCD_send_string (display[1]);
	LCD_send_cmd(0x80|0x1f);
  LCD_send_string (display[2]);
	LCD_send_cmd(0x80|0x5f);
	LCD_send_string (display[3]);
	

	
	
	HAL_Delay(2000);
	int n_shifts = 20-4;
	
	for (int i=0; i<n_shifts; i++)
	  {
			LCD_send_cmd(0x80|0x0b);
			LCD_send_string (display[i+1]);
			LCD_send_cmd(0x80|0x4b);
			LCD_send_string (display[i+2]);
			LCD_send_cmd(0x80|0x1f);
			LCD_send_string (display[i+3]);
			if (i ==19-4) 
			{
				HAL_Delay(2000);
			}
			LCD_send_cmd(0x80|0x5f);
			LCD_send_string (display[i+4]);
			if (i == 8)
			{
				HAL_Delay(800);
			}
			else if (i ==19-4) 
			{
				HAL_Delay(50);
			}
			else HAL_Delay(300);
	
	  }
		
	LCD_clear();

	*/