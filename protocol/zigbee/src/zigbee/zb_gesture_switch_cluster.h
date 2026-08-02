#pragma once

#include "zboss_cpp.h"

#define ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH 0xFC00

#define ZB_ZCL_GESTURE_SWITCH_CLUSTER_REVISION_DEFAULT 1

enum zb_zcl_gesture_switch_cmd_e {
	ZB_ZCL_GESTURE_SWITCH_CMD_REPORT_ID = 0x01,
};

enum zb_zcl_gesture_switch_gesture_e {
	ZB_ZCL_GESTURE_SWITCH_GESTURE_INITIAL_PRESS = 0,
	ZB_ZCL_GESTURE_SWITCH_GESTURE_SHORT_RELEASE = 1,
	ZB_ZCL_GESTURE_SWITCH_GESTURE_TAP = 2,
	ZB_ZCL_GESTURE_SWITCH_GESTURE_LONG_PRESS = 3,
	ZB_ZCL_GESTURE_SWITCH_GESTURE_LONG_RELEASE = 4,
};

typedef ZB_PACKED_PRE struct zb_zcl_gesture_switch_report_s {
	zb_uint8_t channel;
	zb_uint8_t gesture;
	zb_uint8_t count;
} ZB_PACKED_STRUCT zb_zcl_gesture_switch_report_t;

#define ZB_ZCL_GESTURE_SWITCH_SEND_REPORT_REQ(buffer, src_ep, cb, channel_val, gesture_val, count_val) \
	{                                                                                            \
		zb_addr_u gesture_switch_report_addr = { 0 };                                       \
		zb_uint8_t *ptr = (zb_uint8_t *)ZB_ZCL_START_PACKET_REQ(buffer)                     \
		ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE) \
		ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(                                                \
			ptr, ZB_ZCL_GET_SEQ_NUM(), ZB_ZCL_GESTURE_SWITCH_CMD_REPORT_ID);            \
		ZB_ZCL_PACKET_PUT_DATA8(ptr, (channel_val));                                        \
		ZB_ZCL_PACKET_PUT_DATA8(ptr, (gesture_val));                                        \
		ZB_ZCL_PACKET_PUT_DATA8(ptr, (count_val));                                          \
		ZB_ZCL_FINISH_PACKET((buffer), ptr)                                                 \
		ZB_ZCL_SEND_COMMAND_SHORT(                                                          \
			(buffer), gesture_switch_report_addr, ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT, 0, (src_ep), \
			ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH, (cb));               \
	}

#define ZB_ZCL_DECLARE_GESTURE_SWITCH_CLIENT_ATTRIB_LIST(attr_list)                       \
	ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_GESTURE_SWITCH) \
	ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST

#define ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH_CLIENT_ROLE_INIT ((zb_zcl_cluster_init_t)0)
#define ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH_SERVER_ROLE_INIT ((zb_zcl_cluster_init_t)0)

#define ZB_GESTURE_SWITCH_DEVICE_ID 0x0000
#define ZB_DEVICE_VER_GESTURE_SWITCH 0

#define ZB_GESTURE_SWITCH_IN_CLUSTER_NUM 2
#define ZB_GESTURE_SWITCH_OUT_CLUSTER_NUM 1

#define ZB_DECLARE_GESTURE_SWITCH_CLUSTER_LIST(                            \
		cluster_list_name,                                          \
		basic_attr_list,                                             \
		identify_attr_list,                                          \
		gesture_switch_attr_list)                                    \
zb_zcl_cluster_desc_t cluster_list_name[] =                                  \
{                                                                             \
	ZB_ZCL_CLUSTER_DESC(                                                 \
		ZB_ZCL_CLUSTER_ID_BASIC,                                     \
		ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),           \
		(basic_attr_list),                                           \
		ZB_ZCL_CLUSTER_SERVER_ROLE,                                  \
		ZB_ZCL_MANUF_CODE_INVALID                                    \
	),                                                                    \
	ZB_ZCL_CLUSTER_DESC(                                                 \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,                                  \
		ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),        \
		(identify_attr_list),                                        \
		ZB_ZCL_CLUSTER_SERVER_ROLE,                                  \
		ZB_ZCL_MANUF_CODE_INVALID                                    \
	),                                                                    \
	ZB_ZCL_CLUSTER_DESC(                                                 \
		ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH,                            \
		ZB_ZCL_ARRAY_SIZE(gesture_switch_attr_list, zb_zcl_attr_t),  \
		(gesture_switch_attr_list),                                  \
		ZB_ZCL_CLUSTER_CLIENT_ROLE,                                  \
		ZB_ZCL_MANUF_CODE_INVALID                                    \
	)                                                                     \
}

#define ZB_DECLARE_GESTURE_SWITCH_SIMPLE_DESC_TYPE() \
	ZB_DECLARE_SIMPLE_DESC(2, 1)

#define ZB_ZCL_DECLARE_GESTURE_SWITCH_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =          \
	{                                                                                    \
		ep_id,                                                                      \
		ZB_AF_HA_PROFILE_ID,                                                        \
		ZB_GESTURE_SWITCH_DEVICE_ID,                                                \
		ZB_DEVICE_VER_GESTURE_SWITCH,                                               \
		0,                                                                           \
		in_clust_num,                                                               \
		out_clust_num,                                                              \
		{                                                                            \
			ZB_ZCL_CLUSTER_ID_BASIC,                                            \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,                                         \
			ZB_ZCL_CLUSTER_ID_GESTURE_SWITCH,                                   \
		}                                                                            \
	}

#define ZB_DECLARE_GESTURE_SWITCH_EP(ep_name, ep_id, cluster_list)                    \
	ZB_ZCL_DECLARE_GESTURE_SWITCH_SIMPLE_DESC(ep_name, ep_id,                     \
		ZB_GESTURE_SWITCH_IN_CLUSTER_NUM, ZB_GESTURE_SWITCH_OUT_CLUSTER_NUM); \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,     \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                            \
		0, NULL,                                                              \
		0, NULL)
