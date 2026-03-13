/*
 * GPIO.c
 *
 *  Created on: Oct 31, 2025
 *      Author: Bhakti Ramani
 */


#include "../inc/gpio.h"
#include "sl_iostream.h"


void gpio_init()
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  // TODO : change to external pull ups in future
  GPIO_PinModeSet(TRACKPAD_MCU_RDY_PORT, 
    TRACKPAD_MCU_RDY_PIN, 
    gpioModeInput,
    0);

  GPIO_PinModeSet(HAPTICS_EN_PORT, 
    HAPTICS_EN_PIN, 
    gpioModePushPull, 
    0); // default value to 0 (disable module)
  GPIO_PinModeSet(LED_DI_PORT, 
    LED_DI_PIN, 
    gpioModePushPullAlternate, 
    0); // default value to write
  
  // Test LED's
  GPIO_PinModeSet(TEST_LED_PORT,
    TEST_LED_PIN,
    gpioModePushPullAlternate,
    0); // default value to write

  GPIO_PinModeSet(TEST_LED_PORT,
    TEST_LED_PIN_Y,
    gpioModePushPullAlternate,
    0); // default value to write

  GPIO_PinModeSet(TEST_LED_PORT,
    TEST_LED_PIN_R,
    gpioModePushPullAlternate,
    0); // default value to write

  GPIO_IntClear(1 << TRACKPAD_MCU_RDY_PIN);

  GPIO_ExtIntConfig(
    TRACKPAD_MCU_RDY_PORT,    // Port
    TRACKPAD_MCU_RDY_PIN,     // Pin number
    TRACKPAD_RDY_INT,         // Interrupt number (pin 9 uses interrupt line 9 for ODD handler)
    true,         // Rising edge trigger
    false,        // Falling edge trigger (disabled)
    true          // Enable immediately
  );

  // Enable interrupt in NVIC
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn); //  enable in NVIC

  GPIO_IntEnable(1 << TRACKPAD_MCU_RDY_PIN);


}


void enable_haptics(){

}
void disable_haptics(){

}
