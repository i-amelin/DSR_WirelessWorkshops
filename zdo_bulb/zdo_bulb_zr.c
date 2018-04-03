#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include <string.h>

typedef enum {
    ON_BULB = 1,
    OFF_BULB,
    TOGGLE_BULB,
    SET_LEVEL,
    STEP_UP,
    STEP_DOWN
} command;

static void send_data(zb_uint8_t param, char *message);
#ifndef APS_RETRANSMIT_TEST
//void data_indication(zb_uint8_t param) ZB_CALLBACK;
#endif

void zr_on_bulb(zb_uint8_t param);
void zr_off_bulb(zb_uint8_t param);
void zr_toggle_bulb(zb_uint8_t param);
void zr_set_level(zb_uint8_t param);
void zr_step_up(zb_uint8_t param);
void zr_step_down(zb_uint8_t param);

/*
  ZR joins to ZC, then sends APS packet.
 */


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
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
#ifndef APS_RETRANSMIT_TEST
    //zb_af_set_data_indication(data_indication);
#endif
    zb_uint8_t on_bulb = (zb_uint8_t)ZB_REF_FROM_BUF(zb_get_out_buf());
    ZB_SCHEDULE_ALARM(zr_on_bulb, on_bulb, 5 * ZB_TIME_ONE_SECOND);

    zb_uint8_t toggle_bulb = (zb_uint8_t)ZB_REF_FROM_BUF(zb_get_out_buf());
    ZB_SCHEDULE_ALARM(zr_toggle_bulb, toggle_bulb, 11 * ZB_TIME_ONE_SECOND);

    zb_buf_t *set_level_buf = zb_get_out_buf();
    ZB_SET_BUF_PARAM(set_level_buf, 95, uint8_t);
    zb_uint8_t set_level = (zb_uint8_t)ZB_REF_FROM_BUF(set_level_buf);
    ZB_SCHEDULE_ALARM(zr_set_level, set_level, 12 * ZB_TIME_ONE_SECOND);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }
}


static void send_data(zb_uint8_t param, char *message)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_short_t i;
  zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  int message_length = strlen(message);

  TRACE_MSG(TRACE_APS3, "Message length: %x", (FMT__D, message_length));

  ZB_BUF_INITIAL_ALLOC(buf, message_length, ptr);
  req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = 0;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;

  buf->u.hdr.handle = 0x11;

  for (i = 0; i < message_length; ++i)
  {
    ptr[i] = message[i];
  }
  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, ZB_REF_FROM_BUF(buf));
}

void zr_on_bulb(zb_uint8_t param) {
    char cmd[] = { ON_BULB };
    send_data(param, cmd);
}

void zr_off_bulb(zb_uint8_t param) {
    char cmd[] = { OFF_BULB };
    send_data(param, cmd);
}

void zr_toggle_bulb(zb_uint8_t param) {
    char cmd[] = { TOGGLE_BULB };
    send_data(param, cmd);
}

void zr_set_level(zb_uint8_t param) {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    uint8_t level = ZB_GET_BUF_PARAM(buf, uint8_t);
    char cmd[2] = { SET_LEVEL, level };
    send_data(param, cmd);
}

void zr_step_up(zb_uint8_t param) {
    char cmd[] = { STEP_UP };
    send_data(param, cmd);
}

void zr_step_down(zb_uint8_t param) {
    char cmd[] = { STEP_DOWN };
    send_data(param, cmd);
}
