/*****************************************************************************
* | File      	:	LCD_Driver.c
* | Author      :   Waveshare team
* | Function    :   LCD driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2018-12-18
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "LCD_Driver.h"
/*******************************************************************************
function:
	Hardware reset
*******************************************************************************/
static void LCD_Reset(void)
{
	DEV_Delay_ms(200);
	DEV_Digital_Write(DEV_RST_PIN, 0);
	DEV_Delay_ms(200);
	DEV_Digital_Write(DEV_RST_PIN, 1);
	DEV_Delay_ms(200);
}

/*******************************************************************************
function:
		Write data and commands
*******************************************************************************/
static void LCD_Write_Command(UBYTE data)	 
{	
	DEV_Digital_Write(DEV_CS_PIN, 0);
	DEV_Digital_Write(DEV_DC_PIN, 0);
	DEV_SPI_WRITE(data);
}

static void LCD_WriteData_Byte(UBYTE data) 
{	
	DEV_Digital_Write(DEV_CS_PIN, 0);
	DEV_Digital_Write(DEV_DC_PIN, 1);
	DEV_SPI_WRITE(data);  
	DEV_Digital_Write(DEV_CS_PIN,1);
}  

void LCD_WriteData_Word(UWORD data)
{
	DEV_Digital_Write(DEV_CS_PIN, 0);
	DEV_Digital_Write(DEV_DC_PIN, 1);
	DEV_SPI_WRITE((data>>8) & 0xff);
	DEV_SPI_WRITE(data);
	DEV_Digital_Write(DEV_CS_PIN, 1);
}	  


/******************************************************************************
function:	
		Common register initialization
******************************************************************************/
void LCD_Init(void)
{
	LCD_Reset();

	LCD_Write_Command(0x11); //Sleep out
	
	LCD_Write_Command(0xCF);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0xC1);
	LCD_WriteData_Byte(0X30);
	LCD_Write_Command(0xED);
	LCD_WriteData_Byte(0x64);
	LCD_WriteData_Byte(0x03);
	LCD_WriteData_Byte(0X12);
	LCD_WriteData_Byte(0X81);
	LCD_Write_Command(0xE8);
	LCD_WriteData_Byte(0x85);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0x79);
	LCD_Write_Command(0xCB);
	LCD_WriteData_Byte(0x39);
	LCD_WriteData_Byte(0x2C);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0x34);
	LCD_WriteData_Byte(0x02);
	LCD_Write_Command(0xF7);
	LCD_WriteData_Byte(0x20);
	LCD_Write_Command(0xEA);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0x00);
	LCD_Write_Command(0xC0); //Power control
	LCD_WriteData_Byte(0x1D); //VRH[5:0]
	LCD_Write_Command(0xC1); //Power control
	LCD_WriteData_Byte(0x12); //SAP[2:0];BT[3:0]
	LCD_Write_Command(0xC5); //VCM control
	LCD_WriteData_Byte(0x33);
	LCD_WriteData_Byte(0x3F);
	LCD_Write_Command(0xC7); //VCM control
	LCD_WriteData_Byte(0x92);
	LCD_Write_Command(0x3A); // Memory Access Control
	LCD_WriteData_Byte(0x55);
	LCD_Write_Command(0x36); // Memory Access Control
	LCD_WriteData_Byte(0x08);
	LCD_Write_Command(0xB1);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0x12);
	LCD_Write_Command(0xB6); // Display Function Control
	LCD_WriteData_Byte(0x0A);
	LCD_WriteData_Byte(0xA2);

	LCD_Write_Command(0x44);
	LCD_WriteData_Byte(0x02);

	LCD_Write_Command(0xF2); // 3Gamma Function Disable
	LCD_WriteData_Byte(0x00);
	LCD_Write_Command(0x26); //Gamma curve selected
	LCD_WriteData_Byte(0x01);
	LCD_Write_Command(0xE0); //Set Gamma
	LCD_WriteData_Byte(0x0F);
	LCD_WriteData_Byte(0x22);
	LCD_WriteData_Byte(0x1C);
	LCD_WriteData_Byte(0x1B);
	LCD_WriteData_Byte(0x08);
	LCD_WriteData_Byte(0x0F);
	LCD_WriteData_Byte(0x48);
	LCD_WriteData_Byte(0xB8);
	LCD_WriteData_Byte(0x34);
	LCD_WriteData_Byte(0x05);
	LCD_WriteData_Byte(0x0C);
	LCD_WriteData_Byte(0x09);
	LCD_WriteData_Byte(0x0F);
	LCD_WriteData_Byte(0x07);
	LCD_WriteData_Byte(0x00);
	LCD_Write_Command(0XE1); //Set Gamma
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(0x23);
	LCD_WriteData_Byte(0x24);
	LCD_WriteData_Byte(0x07);
	LCD_WriteData_Byte(0x10);
	LCD_WriteData_Byte(0x07);
	LCD_WriteData_Byte(0x38);
	LCD_WriteData_Byte(0x47);
	LCD_WriteData_Byte(0x4B);
	LCD_WriteData_Byte(0x0A);
	LCD_WriteData_Byte(0x13);
	LCD_WriteData_Byte(0x06);
	LCD_WriteData_Byte(0x30);
	LCD_WriteData_Byte(0x38);
	LCD_WriteData_Byte(0x0F);
	LCD_Write_Command(0x29); //Display on
}

/******************************************************************************
function:	Set the cursor position
parameter	:
	  Xstart: 	Start UWORD x coordinate
	  Ystart:	Start UWORD y coordinate
	  Xend  :	End UWORD coordinates
	  Yend  :	End UWORD coordinatesen
******************************************************************************/
void LCD_SetWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD  Yend)
{ 
	LCD_Write_Command(0x2a);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(Xstart & 0xff);
	LCD_WriteData_Byte((Xend - 1) >> 8);
	LCD_WriteData_Byte((Xend - 1) & 0xff);

	LCD_Write_Command(0x2b);
	LCD_WriteData_Byte(0x00);
	LCD_WriteData_Byte(Ystart & 0xff);
	LCD_WriteData_Byte((Yend - 1) >> 8);
	LCD_WriteData_Byte((Yend - 1) & 0xff);

	LCD_Write_Command(0x2C);
}

/******************************************************************************
function:	Settings window
parameter	:
	  Xstart: 	Start UWORD x coordinate
	  Ystart:	Start UWORD y coordinate

******************************************************************************/
void LCD_SetCursor(UWORD X, UWORD Y)
{ 
	LCD_Write_Command(0x2a);
	LCD_WriteData_Byte(X >> 8);
	LCD_WriteData_Byte(X);
	LCD_WriteData_Byte(X >> 8);
	LCD_WriteData_Byte(X);

	LCD_Write_Command(0x2b);
	LCD_WriteData_Byte(Y >> 8);
	LCD_WriteData_Byte(Y);
	LCD_WriteData_Byte(Y >> 8);
	LCD_WriteData_Byte(Y);

	LCD_Write_Command(0x2C);
}

/******************************************************************************
function:	Clear screen function, refresh the screen to a certain color
parameter	:
	  Color :		The color you want to clear all the screen
******************************************************************************/
void LCD_Clear(UWORD Color)
{
	unsigned int i,j;  	
	LCD_SetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
	DEV_Digital_Write(DEV_DC_PIN, 1);
	for(i = 0; i < LCD_WIDTH; i++){
		for(j = 0; j < LCD_HEIGHT; j++){
			DEV_SPI_WRITE((Color>>8)&0xff);
			DEV_SPI_WRITE(Color);
		}
	 }
}

/******************************************************************************
function:	Refresh a certain area to the same color
parameter	:
	  Xstart: Start UWORD x coordinate
	  Ystart:	Start UWORD y coordinate
	  Xend  :	End UWORD coordinates
	  Yend  :	End UWORD coordinates
	  color :	Set the color
******************************************************************************/
void LCD_ClearWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,UWORD color)
{          
	UWORD i,j; 
	LCD_SetWindow(Xstart, Ystart, Xend-1,Yend-1);
	for(i = Ystart; i <= Yend-1; i++){													   	 	
		for(j = Xstart; j <= Xend-1; j++){
			LCD_WriteData_Word(color);
		}
	} 					  	    
}

/******************************************************************************
function: Draw a point
parameter	:
	    X	: 	Set the X coordinate
	    Y	:	Set the Y coordinate
	  Color :	Set the color
******************************************************************************/
void LCD_DrawPaint(UWORD x, UWORD y, UWORD Color)
{
	LCD_SetCursor(x, y);
	LCD_WriteData_Word(Color); 	    
}
