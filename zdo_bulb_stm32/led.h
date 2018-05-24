/*!
 \file
 \brief libled Header File
 
 This file contains the functions using for control the leds on the DSR's extension board.
 */

#ifndef LED_H
#define LED_H

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

/*! Pediod of the timer used for controlling the brightness. */
#define PERIOD 1600

/*! Prescaler of the timer. */
#define PRESCALER 1680 

/*! Pin of the red led on the extension board. */
#define RED GPIO_Pin_8 

/*! Pin of the green led on the extension board. */
#define GREEN GPIO_Pin_9 

/*! Pin of the blue led on the extension board. */
#define BLUE GPIO_Pin_10 

/*! Pins of all leds on the extension board. */
#define LEDS RED | GREEN | BLUE

/*! Pin of the red led on the main board. */
#define BOARD_RED GPIO_Pin_14 

/*! Pin of the green led on the main board. */
#define BOARD_GREEN GPIO_Pin_12 

/*! Pin of the blue led on the main board. */
#define BOARD_BLUE GPIO_Pin_15 

/*! Pins of all leds on the main board. */
#define BOARD_LEDS BOARD_RED | BOARD_GREEN | BOARD_BLUE 

/*!
 * Initializes the timer, the leds on the ext-board and the main board.
 */
void InitLeds(void);

/*!
 * Changes the color that is controlled at current time.
 */
void SetToNextColor(void);

/*!
 * Increments the brightness of the current led.
 */
void IncrementCurrentColor(void);

/*!
 * Turnes on the current led and turnes off other leds.
 */
void LightLeds(void);

#endif /* LED_H */
