#include <stdint.h>
#include "PLL.h"
#include "LCD.h"
#include "os.h"
#include "joystick.h"
#include "FIFO.h"
#include "PORTE.h"
#include "tm4c123gh6pm.h"

// Constants
#define BGCOLOR     					LCD_BLACK
#define CROSSSIZE            			5

//------------------Defines and Variables-------------------
uint16_t origin[2]; // the original ADC value of x,y if the joystick is not touched
int16_t x = 63;  // horizontal position of the crosshair, initially 63
int16_t y = 63;  // vertical position of the crosshair, initially 63
int16_t prevx = 63;
int16_t	prevy = 63;
uint8_t select;  // joystick push

//---------------------User debugging-----------------------

#define TEST_TIMER 0		// Change to 1 if testing the timer
#define TEST_PERIOD 800000  // Defined by user
#define PERIOD 4000000  	// Defined by user

unsigned long Count;   		// number of times thread loops


//--------------------------------------------------------------
void CrossHair_Init(void){
	BSP_LCD_FillScreen(BGCOLOR);	// Draw a black screen
	BSP_Joystick_Input(&origin[0], &origin[1], &select); // Initial values of the joystick, used as reference
}

//******** Producer ***************
// Helper Function
int UpdatePosition(uint16_t rawx, uint16_t rawy, rxDataType* data){
	if (rawx > origin[0]){
		x = x + ((rawx - origin[0]) >> 9);
	}
	else{
		x = x - ((origin[0] - rawx) >> 9);
	}
	if (rawy < origin[1]){
		y = y + ((origin[1] - rawy) >> 9);
	}
	else{
		y = y - ((rawy - origin[1]) >> 9);
	}
	if (x > 127){
		x = 127;}
	if (x < 0){
		x = 0;}
	if (y > 120 - CROSSSIZE){
		y = 120 - CROSSSIZE;}
	if (y < 0){
		y = 0;}
	data->x = x; data->y = y;
	return 1;
}

void Producer(void){
#if TEST_TIMER
	PE1 ^= 0x02;	// heartbeat
	Count++;	// Increment dummy variable			
#else
	uint16_t rawX,rawY; // raw adc value
	uint8_t select;
	rxDataType data;
	BSP_Joystick_Input(&rawX,&rawY,&select);
	UpdatePosition(rawX,rawY,&data); // calculation work
	RxFifo_Put(data);
#endif
}

//******** Consumer *************** 
void Consumer(void){
		rxDataType data;
		RxFifo_Get(&data);
		BSP_LCD_DrawCrosshair(prevx, prevy, BGCOLOR); // Draw a black crosshair
		BSP_LCD_DrawCrosshair(data.x, data.y, LCD_RED); // Draw a red crosshair
		BSP_LCD_Message(1, 5, 3, "X: ", x);		
		BSP_LCD_Message(1, 5, 12, "Y: ", y);
		prevx = data.x; 
		prevy = data.y;
}

//******** Main *************** 
int main(void){
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
#if TEST_TIMER
	PortE_Init();       // profile user threads
	Count = 0;
	OS_AddPeriodicThread(&Producer, TEST_PERIOD, 1);
	while(1){}
#else
  BSP_LCD_Init();        // initialize LCD
	BSP_Joystick_Init();   // initialize Joystick
  CrossHair_Init();      
 	RxFifo_Init();
	OS_AddPeriodicThread(&Producer,PERIOD, 1);
	while(1){
		Consumer();
	}
#endif
} 
