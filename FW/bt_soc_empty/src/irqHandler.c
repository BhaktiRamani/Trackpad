/*
 * irq.c
 *
 *  Created on: Oct 31, 2025
 *      Author: Bhakti Ramani
 */


#include "../inc/irq.h"
#include <stdatomic.h>  // For atomic operations


static volatile bool trackpad_ready_flag = false;

void GPIO_ODD_IRQHandler(void)
{
  uint32_t flags = GPIO_IntGetEnabled();
  GPIO_IntClear(flags);

  if(flags & TRACKPAD_MCU_RDY_MASK)
  {
      GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);
      trackpad_ready_flag = true;

  }
}

void GPIO_EVEN_IRQHander(void){
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);
  
}


bool gpio_irq_is_trackpad_ready(void)
{
  return trackpad_ready_flag;
}

void gpio_irq_clear_trackpad_ready(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_ATOMIC();
  GPIO_PinOutClear(TEST_LED_PORT, TEST_LED_PIN_R);
  trackpad_ready_flag = false;
  CORE_EXIT_ATOMIC();
}
