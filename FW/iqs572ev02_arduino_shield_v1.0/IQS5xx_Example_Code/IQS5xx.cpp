#include "Arduino.h"
#include "defs.h"
#include "I2C.h"
#include "IQS5xx.h"

//
// Local function prototypes
//
void Print_signed(int16_t num);
void Print_unsigned(uint16_t num);

unsigned long LatchStartTime = 0;
unsigned long FlashTime = 0;
uint8_t	ui8LEDState = 0;
uint8_t	ui8CurrLEDState = 0;

extern uint8_t 	Data_Buff[44];
extern uint16_t	ui16SnapStatus[15], ui16PrevSnap[15];

typedef enum
{
	eNone, eSingleTap, eTapAndHold, eSwipeXNeg, eSwipeXPos, eSwipeYPos,
	eSwipeYNeg, eTwoFingerTap, eVerticalScroll, eHorizontalScroll, eZoomIn,
	eZoomOut
}
eGesture;

eGesture CurrGesture = eNone, PrevGesture = eNone; 

//*****************************************************************************
//
//! Acknowledge the reset flag
//!
//! This function simply sets the ACK_RESET bit found in the SYSTEM_CONTROL_0 
//! register.  By setting this bit the SHOW_RESET flag is cleared in the 
//! SYSTEM_INFO_0 register.  During normal operation, the SHOW_RESET bit can be 
//! monitored and if it becomes set, then an unexpected reset has occurred.  
//! If any device configuration is needed, it must then be repeated.
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void IQS5xx_AcknowledgeReset(void) 
{
    static  uint8_t System_ctrl_0 = ACK_RESET;  

	I2C_Write(SystemControl0_adr, &System_ctrl_0, 1);
}

//*****************************************************************************
//
//! Read and display version
//!
//! This function reads the version details from the IQS5xx and sends this to 
//! the display port.
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void IQS5xx_CheckVersion(void) 
{
	uint8_t ui8DataBuffer[6];
	//
	// Dont wait for RDY here, since the device could be in EventMode, and then
	// there will be no communication window to complete this.  Rather do a 
	// forced communication, where clock stretching will be done on the IQS5xx
	// until an appropriate time to complete the i2c.
	//
	I2C_Read(ProductNumber_adr, &ui8DataBuffer[0] ,6);
  
	Serial.print("Product "); 
	Serial.print((ui8DataBuffer[0]<<8) + ui8DataBuffer[1]);
	Serial.print("  Project ");
	Serial.print((ui8DataBuffer[2]<<8) + ui8DataBuffer[3]);
	Serial.print("  Version "); 
	Serial.print(ui8DataBuffer[4]); Serial.print("."); 
	Serial.println(ui8DataBuffer[5]);
}

//*****************************************************************************
//
//! Display a snap state change
//!                            
//! If the state of any snap output has changed, then this function can be used
//!	to display which Rx/Tx channel has changed status.  
//!			                         
//! \param None
//!                                           
//! \return None
//                                                      
//*****************************************************************************
void DisplaySnap(void)
{
	uint8_t		ui8Tx, ui8Rx;
	uint16_t	ui16ToggledBits;
	
	for(ui8Tx = 0; ui8Tx < 15; ui8Tx++)
	{
		ui16ToggledBits = ui16PrevSnap[ui8Tx] ^ ui16SnapStatus[ui8Tx];

		for(ui8Rx = 0; ui8Rx < 10; ui8Rx++)
		{
			if(BitIsSet(ui16ToggledBits, ui8Rx))
			{
				if(BitIsSet(ui16SnapStatus[ui8Tx], ui8Rx))
				{
					Serial.print("Snap set on Rx");
				}
				else
				{
					Serial.print("Snap released on Rx");
				}
				Serial.print(ui8Rx);
				Serial.print("/Tx");
				Serial.print(ui8Tx);
				Serial.println(" channel    ");
			}
		}
	}
}

//*****************************************************************************
//
//! Set and clear the 4 LEDs as required
//!                            
//! This function sets and clears the 4 LEDs according to the byte sent, with 
//! the 4 lower bits indicating the LED as follows: 
//! Bit 0 = LED on LEFT
//! Bit 1 = LED on RIGHT
//! Bit 2 = LED on TOP
//! Bit 3 = LED on BOTOM
//!			                         
//! \param ui8LEDsToSet give the 4 bits indicating which LEDs must be ON
//!                                           
//! \return None
//                                                      
//*****************************************************************************
void
SetEdgeLEDs(uint8_t ui8LEDsToSet)
{
	if(BitIsSet(ui8LEDsToSet, 0))
	{
		digitalWrite(LED_LEFT, HIGH);
	}
	else
	{
		digitalWrite(LED_LEFT, LOW);
	}
	
	if(BitIsSet(ui8LEDsToSet, 1))
	{
		digitalWrite(LED_RIGHT, HIGH);
	}
	else
	{
		digitalWrite(LED_RIGHT, LOW);
	}
	
	if(BitIsSet(ui8LEDsToSet, 2))
	{
		digitalWrite(LED_TOP, HIGH);
	}
	else
	{
		digitalWrite(LED_TOP, LOW);
	}
	
	if(BitIsSet(ui8LEDsToSet, 3))
	{
		digitalWrite(LED_BOTTOM, HIGH);
	}
	else
	{
		digitalWrite(LED_BOTTOM, LOW);
	}
}

void
HandleLEDs(void)
{
	unsigned long TimeNow;
	
	TimeNow = millis();
	//
	// Test if LEDs must be switched OFF - after delay time
	//
	if(PrevGesture == eSingleTap)
	{
		// Switch the LEDs around the edges in clockwise animation
		//
		if((TimeNow - LatchStartTime) < ((LED_LATCH_TIME*1)/5))
		{
			SetEdgeLEDs(0x04);
		}
		else if((TimeNow - LatchStartTime) < ((LED_LATCH_TIME*2)/5))
		{
			SetEdgeLEDs(0x02);
		}
		else if((TimeNow - LatchStartTime) < ((LED_LATCH_TIME*3)/5))
		{
			SetEdgeLEDs(0x08);
		}
		else if((TimeNow - LatchStartTime) < ((LED_LATCH_TIME*4)/5)) 
		{
			SetEdgeLEDs(0x01);
		}
		else if((TimeNow - LatchStartTime) < LED_LATCH_TIME) 
		{
			SetEdgeLEDs(0x04);
		}
		else
		{
			SetEdgeLEDs(0x00);
		}
	}
	//
	// Check if latch time has elapsed, then clear the LEDs
	//
	if(TimeNow - LatchStartTime >= LED_LATCH_TIME) 
	{
		SetEdgeLEDs(0x00);
	}
	
	//
	// Toggle the LEDs for continuous gestures
	//
	if(CurrGesture != eNone)
	{
		if((TimeNow - FlashTime) > 30)
		{
			// if gesture LEDs have been set for 30ms, then toggle them, and
			// reset the 'FlashTime' time
			//
			FlashTime = TimeNow;
			ui8CurrLEDState ^= ui8LEDState;
			SetEdgeLEDs(ui8CurrLEDState);
		}
	}
}

//*****************************************************************************
//
//! Process the data received
//!                            
//! This function sorts the read bytes from the IQS5xx and prints relevant data 
//! on serial port. 
//! REL_X[n]: Relative X Position of the finger n; n is from (1 to 5)
//! REL_Y[n]: Relative X Position of the finger n; n is from (1 to 5)
//! ABS_X[n]: Absolute X Position of the finger n; n is from (1 to 5)
//! ABS_Y[n]: Absolute Y Position of the finger n; n is from (1 to 5)
//! ui16TouchStrength[n]   : Touch strength of finger n; n is from (1 to 5)
//! ui8Area[n]   : Touch area of finger n; this is number of channels under touch 
//! for a particular finger; 
//! Where 'n' is from (1 to 5)
//!			                         
//! \param None
//!                                           
//! \return None
//                                                      
//*****************************************************************************
void Process_XY(void) 
{ 
	uint8_t 	i; 
	static uint8_t ui8FirstTouch = 0;
	uint8_t 	ui8NoOfFingers;
	uint8_t 	ui8SystemFlags[2];
	int16_t 	i16RelX;
	int16_t 	i16RelY;
	uint16_t 	ui16AbsX[6];
	uint16_t 	ui16AbsY[6];
	uint16_t 	ui16TouchStrength[6];
	uint8_t  	ui8Area[6];
	eGesture	tempGesture;
	
	ui8LEDState = 0x00;
	tempGesture = CurrGesture;
	CurrGesture = eNone;
 
	ui8SystemFlags[0] = Data_Buff[2];
	ui8SystemFlags[1] = Data_Buff[3];
	ui8NoOfFingers = Data_Buff[4];
	
	i16RelX = ((Data_Buff[5] << 8) | (Data_Buff[6]));
	i16RelY = ((Data_Buff[7] << 8) | (Data_Buff[8]));
	//
	// Re-initialize the device if unexpected RESET detected
	//
	if((ui8SystemFlags[0] & SHOW_RESET) != 0)
	{
		Serial.println("RESET DETECTED");
		IQS5xx_AcknowledgeReset(); 
		return;
	}

	if((ui8SystemFlags[1] & SNAP_TOGGLE) != 0)
	{
		// A snap state has changed, thus indicate which channel
		//
		DisplaySnap();
		return;
	}
	//
	// Taps are handled here because the gesture only becomes set after the 
	// finger is removed (ie ui8NoOfFingers = 0)
	//
	if((Data_Buff[0]) == SINGLE_TAP) 
	{		
		Serial.println("Single Tap  "); 
		CurrGesture = eSingleTap;
		ui8LEDState = 0x04;
	}
	else if((Data_Buff[1]) == TWO_FINGER_TAP)   
	{
		Serial.println("2 Finger Tap"); 
		CurrGesture = eTwoFingerTap;
		ui8LEDState = 0x0F;
	}		

	if(ui8NoOfFingers != 0) 
	{
		if (!(ui8FirstTouch)) 
		{
			Serial.print("Gestures:    ");
			Serial.print(" RelX: ");
			Serial.print("RelY: ");
			Serial.print("Fig: ");
			Serial.print("X1:  "); Serial.print("Y1:  "); Serial.print("TS1: "); Serial.print("TA1: ");
			Serial.print("X2:  "); Serial.print("Y2:  "); Serial.print("TS2: "); Serial.print("TA2: ");
			Serial.print("X3:  "); Serial.print("Y3:  "); Serial.print("TS3: "); Serial.print("TA3: ");
			Serial.print("X4:  "); Serial.print("Y4:  "); Serial.print("TS4: "); Serial.print("TA4: ");
			Serial.print("X5:  "); Serial.print("Y5:  "); Serial.print("TS5: "); Serial.println("TA5: ");
			ui8FirstTouch = 1;
		}

		switch (Data_Buff[0])
		{
			case TAP_AND_HOLD	:  	Serial.print("Tap & Hold  "); 
									CurrGesture = eTapAndHold; 
									ui8LEDState = 0x0F;
									break;
			case SWIPE_X_NEG	:  	Serial.print("Swipe X-    "); 
									CurrGesture = eSwipeXNeg; 
									ui8LEDState = 0x01;
									break;
			case SWIPE_X_POS	:  	Serial.print("Swipe X+    "); 
									CurrGesture = eSwipeXPos;  
									ui8LEDState = 0x02;			
									break;
			case SWIPE_Y_NEG	:  	Serial.print("Swipe Y-    "); 
									CurrGesture = eSwipeYNeg; 
									ui8LEDState = 0x04;
									break;
			case SWIPE_Y_POS	:  	Serial.print("Swipe Y+    "); 
									CurrGesture = eSwipeYPos;  
									ui8LEDState = 0x08;
									break;
		}

		switch (Data_Buff[1])
		{
			case SCROLL			:  	Serial.print("Scroll      "); 
									if(i16RelX != 0)
									{
										ui8LEDState = 0x0C; 
										CurrGesture = eHorizontalScroll;
									}
									else if(i16RelY != 0)
									{
										ui8LEDState = 0x03; 
										CurrGesture = eVerticalScroll;
									}
									break;
			case ZOOM			:  	Serial.print("Zoom        "); 
									if(i16RelX > 0)
									{
										ui8LEDState = 0x05; 
										CurrGesture = eZoomIn;
									}
									else
									{
										ui8LEDState = 0x0A; 
										CurrGesture = eZoomOut;
									}
									break;
		}
		
		if((Data_Buff[0] | Data_Buff[1]) == 0) 
		{
			Serial.print("            ");
		}

		Print_signed(i16RelX);
		Print_signed(i16RelY);
		Print_unsigned(ui8NoOfFingers);    

		for (i = 0; i < 5; i++)
		{
			ui16AbsX[i+1] = ((Data_Buff[(7*i)+9] << 8) | (Data_Buff[(7*i)+10])); //9-16-23-30-37//10-17-24-31-38
			ui16AbsY[i+1] = ((Data_Buff[(7*i)+11] << 8) | (Data_Buff[(7*i)+12])); //11-18-25-32-39//12-19-26-33-40
			ui16TouchStrength[i+1] = ((Data_Buff[(7*i)+13] << 8) | (Data_Buff[(7*i)+14])); //13-20-27-34-11/14-21-28-35-42
			ui8Area[i+1] = (Data_Buff[7*i+15]); //15-22-29-36-43
			Print_unsigned(ui16AbsX[i+1]);
			Print_unsigned(ui16AbsY[i+1]);
			Print_unsigned(ui16TouchStrength[i+1]);
			Print_unsigned(ui8Area[i+1]);
		}
		Serial.println("");
	} 
	else 
	{
		ui8FirstTouch = 0;
	}
	
	if(CurrGesture != tempGesture)
	{
		PrevGesture = tempGesture;
	}
	
	if(CurrGesture != eNone)
	{
		// reset the delay timer to latch LEDs on after gesture
		//
		LatchStartTime = millis();
		
		if(CurrGesture != tempGesture)
		{
			// Only update on gesture change
			//
			SetEdgeLEDs(ui8LEDState);
			ui8CurrLEDState = ui8LEDState;
			//
			// Also when a new gesture is set, save the time so that the 
			// flashing can be managed from this time for continuous gestures
			//
			FlashTime = LatchStartTime;
		}
	}
}

//*****************************************************************************
//
//! Print a signed value on serial display
//!                            
//! Print the signed integer on the serial port with adjusted tabs 
//! on serial port for easy column reading. 
//!			                         
//! \param None
//!                                           
//! \return None
//                                                      
//*****************************************************************************
void Print_signed(int16_t i16Num)
{
	if(i16Num < (-99))
	{
		Serial.print(" ");
	}
	else if(i16Num < (-9))
	{
		Serial.print("  ");
	}
	else if(i16Num < 0)
	{
		Serial.print("   ");
	}
	else if(i16Num < 10)
	{
		Serial.print("    ");
	}
	else if(i16Num < 100)
	{
		Serial.print("   ");
	}
	else if(i16Num < 1000)
	{
		Serial.print("  ");
	}
	Serial.print(i16Num);
}

//*****************************************************************************
//
//! Print an unsigned value on serial display
//!                            
//! Print the unsigned integer on the serial port with adjusted tabs 
//! on serial port for easy column reading. 
//!			                         
//! \param None
//!                                           
//! \return None
//                                                      
//*****************************************************************************
void Print_unsigned(uint16_t ui16Num)
{
	if(ui16Num < 10)
	{
		Serial.print("    ");
	}
	else if(ui16Num < 100)
	{
		Serial.print("   ");
	}
	else if(ui16Num < 1000)
	{
		Serial.print("  ");
	}
	else if(ui16Num < 10000)
	{
		Serial.print(" ");
	}

	if(ui16Num > 10000)
	{
		Serial.print("  xxx");
	}
	else
	{
		Serial.print(ui16Num);
	}
}


