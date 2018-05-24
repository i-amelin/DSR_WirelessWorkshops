#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include <string.h>

#include "led.h"

#include "commands.h"

zb_ieee_addr_t g_zc_addr = { 0xAA, 0xAA, 0xAA, 0xA, 0xAA, 0xAA, 0xAA, 0xAA };

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


void data_indication(zb_uint8_t param) ZB_CALLBACK;

void zc_toggle_bulb();
void zc_toggle_color();
void zc_step_up();

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

    InitLeds();

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
  
  command command_id;
  uint8_t i;

  /* Remove APS header from the packet */
  ZB_APS_HDR_CUT_P(asdu, ptr);

  TRACE_MSG(TRACE_APS3, "apsde_data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
                         asdu, (int)ZB_BUF_LEN(asdu), asdu->u.hdr.status));

  TRACE_MSG(TRACE_APS3, "The packet contains the following data:", (FMT__0));
  for (i = 0; i < ZB_BUF_LEN(asdu); i++) {
      TRACE_MSG(TRACE_APS3, "%d", (FMT__D, (int)ptr[i]));
  }

  command_id = ptr[0];
  switch (command_id) {
      case TOGGLE_BULB:
          zc_toggle_bulb();
          break;
      case TOGGLE_COLOR:
          zc_toggle_color();
          break;
      case STEP_UP:
          zc_step_up();
          break;
      default:
          TRACE_MSG(TRACE_APS3, "Unknown Command", (FMT__0));
          break;
  }
  zb_free_buf(asdu);
}

void zc_toggle_bulb() {
    TRACE_MSG(TRACE_APS3, "Bulb was toggled", (FMT__0));
}

void zc_toggle_color() {
    SetToNextColor();
    LightLeds();
	TRACE_MSG(TRACE_APS3, "Color was toggled", (FMT__0));
}

void zc_step_up() {
    IncrementCurrentColor();
    LightLeds();
}
