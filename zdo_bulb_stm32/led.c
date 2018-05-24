#include "led.h"

static int currentColor; ///< The order number of the color turning on at current time.
static const int brightness[] = { 0, 320, 640, 960, 1280, 1600 }; ///< Array of possible values of led brightness.
static int currentBrightnesses[] = { 0, 0, 0 }; ///< Stores the indexes of the brightness array for each led.

/*!
 * Initializes a timer used for brightness controlling.
 */
static void InitTIM(void);

/*!
 * Initializes Output Compares needed for the timer.
 */
static void InitOC(void);

/*!
 * Increments the brightness of the color.
 * \param[in] color The order number of color which brightness will be changed.
 */
static void IncrementLed(int color);

void InitLeds(void) {
    currentColor = 0;

    InitTIM();
    InitOC();

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    GPIO_InitTypeDef gpio_struct;
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);
    gpio_struct.GPIO_Pin = LEDS;
    gpio_struct.GPIO_Mode = GPIO_Mode_AF;
    gpio_struct.GPIO_OType = GPIO_OType_PP;
    gpio_struct.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio_struct);

    gpio_struct.GPIO_Pin = BOARD_LEDS;
    gpio_struct.GPIO_Mode = GPIO_Mode_OUT;
    gpio_struct.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_struct.GPIO_OType = GPIO_OType_PP;
    gpio_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &gpio_struct);

    GPIO_ResetBits(GPIOD, BOARD_LEDS);
    GPIO_SetBits(GPIOD, BOARD_RED);
}

static void InitTIM(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseInitTypeDef tim_struct;
    tim_struct.TIM_Period = (uint16_t)(PERIOD - 1);
    tim_struct.TIM_Prescaler = (uint16_t)(PRESCALER - 1);
    tim_struct.TIM_ClockDivision = 0;
    tim_struct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tim_struct);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

static void InitOC(void) {
    TIM_OCInitTypeDef oc_struct;
    oc_struct.TIM_OCMode = TIM_OCMode_PWM1;
    oc_struct.TIM_OutputState = TIM_OutputState_Enable;
    oc_struct.TIM_OCPolarity = TIM_OCPolarity_Low;
    oc_struct.TIM_Pulse = brightness[0];

    TIM_OC1Init(TIM1, &oc_struct);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM1, &oc_struct);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3Init(TIM1, &oc_struct);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
}

void SetToNextColor(void) {
    currentColor++;
    currentColor %= 3;
}

void IncrementCurrentColor(void) {
    IncrementLed(currentColor);
}

static void IncrementLed(int color) {
    uint8_t currentBrightness = currentBrightnesses[color] + 1;
    currentBrightness %= 6;
    currentBrightnesses[color] = currentBrightness;
}

void LightLeds(void) {
    uint8_t currentBrightness = currentBrightnesses[currentColor];
    switch (currentColor) {
        case 0:
            TIM_SetCompare1(TIM1, brightness[currentBrightness]);
            GPIO_ResetBits(GPIOD, BOARD_BLUE);
            GPIO_SetBits(GPIOD, BOARD_RED);
            break;
        case 1:
            TIM_SetCompare2(TIM1, brightness[currentBrightness]);
            GPIO_ResetBits(GPIOD, BOARD_RED);
            GPIO_SetBits(GPIOD, BOARD_GREEN);
            break;
        case 2:
            TIM_SetCompare3(TIM1, brightness[currentBrightness]);
            GPIO_ResetBits(GPIOD, BOARD_GREEN);
            GPIO_SetBits(GPIOD, BOARD_BLUE);
            break;
        }
}
