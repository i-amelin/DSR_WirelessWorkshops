#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

static void send_data(zb_buf_t *buf);
void data_indication(zb_uint8_t param) ZB_CALLBACK;

void get_ieee_addr(zb_uint8_t param);
void ieee_addr_callback(zb_uint8_t param) ZB_CALLBACK;
void get_active_ep(zb_uint8_t param);
void active_ep_callback(zb_uint8_t param) ZB_CALLBACK;
void get_power_mode(zb_uint8_t param);
void power_mode_callback(zb_uint8_t param) ZB_CALLBACK;
void get_simple_desc(zb_uint8_t param);
void simple_desc_callback(zb_uint8_t param) ZB_CALLBACK;

typedef struct device_descriptor {
    zb_uint8_t endpoint;
    zb_uint8_t profile_id;
    zb_uint16_t cluster_count;
    zb_uint16_t cluster_id;
    zb_uint16_t *clusters;
} device_descriptor_t;

device_descriptor_t controller_device;


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
  ZG->nwk.nib.security_level = 1;
#endif

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

    zb_uint8_t ieee_param = ZB_REF_FROM_BUF(zb_get_out_buf());
    get_ieee_addr(ieee_param);

    zb_af_set_data_indication(data_indication);
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
  zb_uint8_t *ptr = NULL;
  zb_short_t i;

  ZB_BUF_INITIAL_ALLOC(buf, ZB_TEST_DATA_SIZE, ptr);
  req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = 0;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = controller_device.profile_id;
  req->clusterid = controller_device.cluster_id;
  req->src_endpoint = 0;
  req->dst_endpoint = controller_device.endpoint;

  buf->u.hdr.handle = 0x11;

  for (i = 0; i < ZB_TEST_DATA_SIZE; ++i)
  {
    ptr[i] = i % 32 + '0';
  }
  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, ZB_REF_FROM_BUF(buf));
}

#ifndef APS_RETRANSMIT_TEST
void data_indication(zb_uint8_t param)
{
  zb_ushort_t i;
  zb_uint8_t *ptr;
  zb_buf_t *asdu = (zb_buf_t *)ZB_BUF_FROM_REF(param);

  ZB_APS_HDR_CUT_P(asdu, ptr);

  TRACE_MSG(TRACE_APS3, "data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
                         asdu, (int)ZB_BUF_LEN(asdu), asdu->u.hdr.status));

  for (i = 0; i < ZB_BUF_LEN(asdu); ++i)
  {
    TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
    if (ptr[i] != i % 32 + '0')
    {
      TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                              (int)(i % 32 + '0'), (char)i % 32 + '0'));
    }
  }

  zb_free_buf(asdu);
}
#endif

void get_ieee_addr(zb_uint8_t param) {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zdo_ieee_addr_req_t *req = NULL;

    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_ieee_addr_req_t), req);
    req->nwk_addr = 0;
    req->request_type = ZB_ZDO_SINGLE_DEV_RESPONSE;
    req->start_index = 0;
    zb_zdo_ieee_addr_req(ZB_REF_FROM_BUF(buf), ieee_addr_callback);
}

void ieee_addr_callback(zb_uint8_t param) ZB_CALLBACK {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zdo_nwk_addr_resp_head_t *resp;

    zb_ieee_addr_t ieee_addr;
    zb_uint16_t nwk_addr;
    zb_address_ieee_ref_t addr_ref;

    resp = (zb_zdo_nwk_addr_resp_head_t *)ZB_BUF_BEGIN(buf);
    ZB_DUMP_IEEE_ADDR(resp->ieee_addr);
    if (resp->status == ZB_ZDP_STATUS_SUCCESS) {
        ZB_LETOH64(ieee_addr, resp->ieee_addr);
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);
        zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);

        TRACE_MSG(TRACE_ZDO2, "IEEE Address: %d %d %d %d", (FMT__D_D_D_D,
                    ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3]));
        TRACE_MSG(TRACE_ZDO2, "IEEE Address: %d %d %d %d", (FMT__D_D_D_D,
                    ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]));
    }

    get_power_mode(param);
}

void get_active_ep(zb_uint8_t param) {
    zb_zdo_active_ep_req_t *req = NULL;
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_active_ep_req_t), req);
    req->nwk_addr = 0;
    zb_zdo_active_ep_req(ZB_REF_FROM_BUF(buf), active_ep_callback);
}

void active_ep_callback(zb_uint8_t param) ZB_CALLBACK {
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t*)zdp_cmd;

    zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);
    TRACE_MSG(TRACE_APS1, "active_ep_callback status %hd, addr 0x%x",
        (FMT__H, resp->status, resp->nwk_addr));

    if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0) {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "ep count %hd, ep %hd", (FMT__H_H, resp->ep_count, *
        ep_list));
    if (resp->ep_count != 1) {
        TRACE_MSG(TRACE_APS3, "Error incorrect ep count or ep value", (FMT__0));
    } else {
        controller_device.endpoint = *ep_list;
        get_simple_desc(param);
    } 
}

void get_power_mode(zb_uint8_t param) {
    zb_zdo_power_desc_req_t *req = NULL;
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_power_desc_req_t), req);
    req->nwk_addr = 0;
    zb_zdo_power_desc_req(ZB_REF_FROM_BUF(buf), power_mode_callback);
}

void power_mode_callback(zb_uint8_t param) ZB_CALLBACK {
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zdo_power_desc_resp_t *resp = (zb_zdo_power_desc_resp_t *)ZB_BUF_BEGIN(buf);

    if (resp->hdr.status == ZB_ZDP_STATUS_SUCCESS && resp->hdr.nwk_addr == 0x0) {
        TRACE_MSG(TRACE_APS2, "Power Mode: %hd", (FMT__H, ZB_GET_POWER_DESC_CUR_POWER_MODE(&resp->power_desc)));
    } else {
        TRACE_MSG(TRACE_APS2, "Power Mode: ERROR", (FMT__0));
    }

    get_active_ep(param); 
}

void get_simple_desc(zb_uint8_t param) {
    zb_buf_t *simple_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zdo_simple_desc_req_t *simple_req = NULL;
    ZB_BUF_INITIAL_ALLOC(simple_buf, sizeof(zb_zdo_simple_desc_req_t), simple_req);
    simple_req->nwk_addr = 0;
    simple_req->endpoint = controller_device.endpoint;
    zb_zdo_simple_desc_req(ZB_REF_FROM_BUF(simple_buf), simple_desc_callback);
}

void simple_desc_callback(zb_uint8_t param) ZB_CALLBACK {
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)ZB_BUF_BEGIN(buf);

    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0) {
        TRACE_MSG(TRACE_APS1, "Simple Desc: Error", (FMT__0));
    } else {
        TRACE_MSG(TRACE_APS1, "Endpoint: %hd", (FMT__H, resp->simple_desc.endpoint));
        controller_device.endpoint = resp->simple_desc.endpoint;
        TRACE_MSG(TRACE_APS1, "Profile ID: %d, device ID: %d",
                (FMT__D_D, resp->simple_desc.app_profile_id, resp->simple_desc.app_device_id));
        controller_device.profile_id = resp->simple_desc.app_profile_id;
        TRACE_MSG(TRACE_APS1, "Device version: %hd", (FMT__H, resp->simple_desc.app_device_version));

        controller_device.clusters = resp->simple_desc.app_cluster_list;
        controller_device.cluster_id = *controller_device.clusters;

        send_data(buf);
    }
}
