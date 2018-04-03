#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include <string.h>

#define ZB_TEST_DUMMY_DATA_SIZE 10

zb_ieee_addr_t g_zc_addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*
  The test is: ZC starts PAN, ZR joins to it by association and send APS data packet, when ZC
  received packet, it sends packet to ZR, when ZR received packet, it sends
  packet to ZC etc.
 */

#define MAX_LEVEL 100
#define LEVEL_STEP 10

typedef enum {
    ON_BULB = 2,
    OFF_BULB,
    TOGGLE_BULB,
    SET_LEVEL,
    STEP_UP,
    STEP_DOWN
} command;

uint8_t is_bulb_lighting;
uint8_t lighting_level;

static void zc_send_data(zb_buf_t *buf, zb_uint16_t addr, char *message);
void data_indication(zb_uint8_t param) ZB_CALLBACK;

void zc_on_bulb();
void zc_off_bulb();
void zc_toggle_bulb();
void zc_set_level(uint8_t level);
void zc_step_up();
void zc_step_down();

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif


  /* Init device, load IB values from nvram or set it to default */
  is_bulb_lighting = 0;
  lighting_level = 50;

#ifndef ZB8051
  ZB_INIT("zdo_zc", argv[1], argv[2]);
#else
  ZB_INIT("zdo_zc", "1", "1");
#endif
#ifdef ZB_SECURITY
  ZG->nwk.nib.security_level = 0;
#endif
  ZB_IEEE_ADDR_COPY(ZB_PIB_EXTENDED_ADDRESS(), &g_zc_addr);
  MAC_PIB().mac_pan_id = 0x1aaa;

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;

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
  TRACE_MSG(TRACE_APS3, ">>zb_zdo_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));
  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
    zb_af_set_data_indication(data_indication);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
  }
  zb_free_buf(buf);
}


void data_indication(zb_uint8_t param) ZB_CALLBACK
{
  uint8_t new_level;
  zb_uint8_t *ptr;
  zb_buf_t *asdu = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_apsde_data_indication_t *ind = ZB_GET_BUF_PARAM(asdu, zb_apsde_data_indication_t);
  command command_id;
  uint8_t i;

  /* Remove APS header from the packet */
  ZB_APS_HDR_CUT_P(asdu, ptr);

  TRACE_MSG(TRACE_APS3, "apsde_data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
                         asdu, (int)ZB_BUF_LEN(asdu), asdu->u.hdr.status));

  /* send packet back to ZR */
  /*zc_send_data(asdu, ind->src_addr);*/
  TRACE_MSG(TRACE_APS3, "The packet contains the following data:", (FMT__0));
  for (i = 0; i < ZB_BUF_LEN(asdu); i++) {
      TRACE_MSG(TRACE_APS3, "%d", (FMT__D, (int)ptr[i]));
  }

  command_id = ptr[0];
  switch (command_id) {
      case ON_BULB:
          zc_off_bulb();
          break;
      case OFF_BULB:
          zc_on_bulb();
          break;
      case TOGGLE_BULB:
          zc_toggle_bulb();
          break;
      case SET_LEVEL:
          new_level = ptr[1];
          zc_set_level(new_level);
          break;
      case STEP_UP:
          zc_step_up();
          break;
      case STEP_DOWN:
          zc_step_down();
          break;
      default:
          TRACE_MSG(TRACE_APS3, "Unknown Command", (FMT__0));
          break;
  }
  zb_free_buf(asdu);
}

static void zc_send_data(zb_buf_t *buf, zb_uint16_t addr, char *message)
{
    zb_apsde_data_req_t *req;
    zb_uint8_t *ptr = NULL;
    zb_short_t i;
    int message_length = strlen(message);

    ZB_BUF_INITIAL_ALLOC(buf, message_length, ptr);
    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
    req->dst_addr.addr_short = addr; /* send to ZR */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;
    buf->u.hdr.handle = 0x11;
    for (i = 0 ; i < message_length; i++)
    {
      ptr[i] = message[i];
    }
    TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, ZB_REF_FROM_BUF(buf));
}

void zc_on_bulb() {
    is_bulb_lighting = 1;
    TRACE_MSG(TRACE_APS3, "Bulb is on", (FMT__0));
}

void zc_off_bulb() {
    is_bulb_lighting = 0;
    TRACE_MSG(TRACE_APS3, "Bulb is off", (FMT__0));
}

void zc_toggle_bulb() {
    is_bulb_lighting ^= 1;
    TRACE_MSG(TRACE_APS3, "Bulb was toggled", (FMT__0));
}

void zc_set_level(uint8_t level) {
    if (level <= 100 && level >= 0) {
        lighting_level = level;
        TRACE_MSG(TRACE_APS3, "New level: %d", (FMT__D, level));
    } else {
        TRACE_MSG(TRACE_APS3, "Error: incorrect value of lighting level", (FMT__0));
    }
}

void zc_step_up() {
    if (lighting_level + LEVEL_STEP <= MAX_LEVEL) {
        lighting_level += LEVEL_STEP;
        TRACE_MSG(TRACE_APS3, "Step up. New level: %d", (FMT__D, lighting_level));
    } else {
        TRACE_MSG(TRACE_APS3, "Error: max value of level", (FMT__0));
    }
}

void zc_step_down() {
    if (lighting_level - LEVEL_STEP >= 0) {
        lighting_level -= LEVEL_STEP;
        TRACE_MSG(TRACE_APS3, "Step down. New level: %d", (FMT__D, lighting_level));
    } else {
        TRACE_MSG(TRACE_APS3, "Error: min value of level", (FMT__0));
    }
}
