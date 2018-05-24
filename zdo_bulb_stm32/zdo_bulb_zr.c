#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "stm32f4xx_tim.h"
#include <string.h>
#include "commands.h"

static void send_data(zb_buf_t *buf);
#ifndef APS_RETRANSMIT_TEST
//void data_indication(zb_uint8_t param) ZB_CALLBACK;
#endif

#define LEFT_BUTTON GPIO_Pin_0
#define RIGHT_BUTTON GPIO_Pin_1

#define DEBOUNCE_DELAY 200

#define DEBOUNCE_TIMER_PERIOD 24000
#define DEBOUNCE_TIMER_PRESCALER 1000

int left_button_delay, right_button_delay;

void zr_toggle_bulb(zb_uint8_t param);
void zr_toggle_color(zb_uint8_t param);
void zr_step_up(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC|| defined ZB_IAR)
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */


#ifndef ZB8051
  ZB_INIT("zdo_zr", argv[1], argv[2]);
#else
  ZB_INIT("zdo_zr", "2", "2");
#endif
#ifdef ZB_SECURITY
  ZG->nwk.nib.security_level = 0;
#endif
  /* FIXME: temporary, until neighbor table is not in nvram */
  /* add extended address of potential parent to emulate that we've already
   * been connected to it */
  {
    zb_ieee_addr_t ieee_address;
    zb_address_ieee_ref_t ref;

    ZB_64BIT_ADDR_ZERO(ieee_address);
    ieee_address[7] = 8;

    zb_address_update(ieee_address, 0, ZB_FALSE, &ref);
  }

  ZB_PIB_RX_ON_WHEN_IDLE() = ZB_FALSE;
  ZB_AIB().aps_channel_mask = (1l << 20);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param) ZB_CALLBACK
{
  initTIM();
  initEXTI();
  initNVIC();
  initButtons();

  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
#ifndef APS_RETRANSMIT_TEST
    //zb_af_set_data_indication(data_indication);
#endif
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }
}

static void send_data(zb_buf_t *buf)
{
  zb_apsde_data_req_t *req;
  req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = 0;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;
  buf->u.hdr.handle = 0x11;

  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, ZB_REF_FROM_BUF(buf));
}

void initTIM() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    TIM_TimeBaseInitTypeDef tim_struct;
    tim_struct.TIM_Period = (uint16_t)(DEBOUNCE_TIMER_PERIOD - 1);
    tim_struct.TIM_Prescaler = (uint16_t)(DEBOUNCE_TIMER_PRESCALER - 1);
    TIM_TimeBaseInit(TIM6, &tim_struct);
    TIM_CtrlPWMOutputs(TIM6, ENABLE);
    TIM_Cmd(TIM6, ENABLE);

    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    TIM_ITConfig(TIM6, TIM_DIER_UIE, ENABLE);
}

void initNVIC() {
    NVIC_InitTypeDef nvic_struct;

    // Init NVIC for a external interrupt
    nvic_struct.NVIC_IRQChannel = EXTI0_IRQn;
    nvic_struct.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_struct.NVIC_IRQChannelSubPriority = 0;
    nvic_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_struct);

    nvic_struct.NVIC_IRQChannel = EXTI1_IRQn;
    nvic_struct.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&nvic_struct);
}

void initEXTI() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource0);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource1);
    
    EXTI_InitTypeDef exti_struct;
    exti_struct.EXTI_Line = EXTI_Line0;
    exti_struct.EXTI_LineCmd = ENABLE;
    exti_struct.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_struct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&exti_struct);

    exti_struct.EXTI_Line = EXTI_Line1;
    EXTI_Init(&exti_struct);
}

void initButtons() {
    GPIO_InitTypeDef gpio_struct;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    gpio_struct.GPIO_Pin = LEFT_BUTTON | RIGHT_BUTTON;
    gpio_struct.GPIO_Mode = GPIO_Mode_IN;
    gpio_struct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &gpio_struct);
}

void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        //if (left_button_delay > DEBOUNCE_DELAY) {
            left_button_delay = 0;

            zb_uint8_t step_up_param = (zb_uint8_t)ZB_REF_FROM_BUF(zb_get_out_buf());
            zr_step_up(step_up_param);
        //}
    }
    EXTI_ClearITPendingBit(EXTI_Line0);
}

void EXTI1_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
        //if (right_button_delay > DEBOUNCE_DELAY) {
            right_button_delay = 0;
            
            zb_uint8_t toggle_color_param = (zb_uint8_t)ZB_REF_FROM_BUF(zb_get_out_buf());
            zr_toggle_color(toggle_color_param);
        //}
    }
    EXTI_ClearITPendingBit(EXTI_Line1);
}

void TIM6_DAC_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
  {
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    left_button_delay++;
    right_button_delay++;
  }
}

void zr_toggle_bulb(zb_uint8_t param) {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr = NULL;
    ZB_BUF_INITIAL_ALLOC(buf, 1, ptr);
    *ptr = TOGGLE_BULB;
    send_data(buf);
}

void zr_toggle_color(zb_uint8_t param) {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr = NULL;
    ZB_BUF_INITIAL_ALLOC(buf, 1, ptr);
    *ptr = TOGGLE_COLOR;
    send_data(buf);
}

void zr_step_up(zb_uint8_t param) {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr = NULL;
    ZB_BUF_INITIAL_ALLOC(buf, 1, ptr);
    *ptr = STEP_UP;
    send_data(buf);
}
