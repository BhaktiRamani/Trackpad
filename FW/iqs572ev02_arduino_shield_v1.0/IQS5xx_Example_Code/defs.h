#ifndef defs_h
#define defs_h

#define IQS5xx_ADDR          	0x74
#define RDY_PIN              	2		// Digital I/O 2
#define LED_TOP					10		// Digital I/O 10
#define	LED_BOTTOM				6		// Digital I/O 6
#define LED_LEFT				5		// Digital I/O 5
#define LED_RIGHT				9		// Digital I/O 9
#define LED_LATCH_TIME			300 	// milliseconds

#define	END_WINDOW				(uint16_t)0xEEEE

#define BitIsSet(VAR,Index)		(VAR & (1<<Index)) != 0
#define BitIsNotSet(VAR,Index)	(VAR & (1<<Index)) == 0

#define SetBit(VAR,Index)		VAR |= (1<<Index)
#define ClearBit(VAR,Index)		VAR &= (uint8_t)(~(1<<Index))

#define FALSE 					0
#define	TRUE 					!FALSE



void Process_XY();
void IQS5xx_AcknowledgeReset(); 
void IQS5xx_CheckVersion();
void SetEdgeLEDs(uint8_t ui8LEDsToSet);
void HandleLEDs(void);

#endif