#pragma once

#include "zboss_cpp.h"

#define ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG 0xFC01

#define ZB_ZCL_GESTURE_CONFIG_CLUSTER_REVISION_DEFAULT 1

enum zb_zcl_gesture_config_attr_e {
	ZB_ZCL_GESTURE_CONFIG_ATTR_NUM_CHANNELS_ID = 0x0000,
	ZB_ZCL_GESTURE_CONFIG_ATTR_POLL_MS_ID = 0x0001,
	ZB_ZCL_GESTURE_CONFIG_ATTR_LONG_PRESS_MS_ID = 0x0002,
	ZB_ZCL_GESTURE_CONFIG_ATTR_MULTI_GAP_MS_ID = 0x0003,
	ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_MAX_TAPS_ID = 0x0004,
	ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_HOLD_ENABLED_ID = 0x0005,
	ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_MAX_TAPS_ID = 0x0006,
	ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_HOLD_ENABLED_ID = 0x0007,
};

/* clang-format off */
#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_NUM_CHANNELS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_NUM_CHANNELS_ID, ZB_ZCL_ATTR_TYPE_U8, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_POLL_MS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_POLL_MS_ID, ZB_ZCL_ATTR_TYPE_U16, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_LONG_PRESS_MS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_LONG_PRESS_MS_ID, ZB_ZCL_ATTR_TYPE_U16, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_MULTI_GAP_MS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_MULTI_GAP_MS_ID, ZB_ZCL_ATTR_TYPE_U16, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_MAX_TAPS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_MAX_TAPS_ID, ZB_ZCL_ATTR_TYPE_U8, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_HOLD_ENABLED_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_HOLD_ENABLED_ID, ZB_ZCL_ATTR_TYPE_BOOL, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_MAX_TAPS_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_MAX_TAPS_ID, ZB_ZCL_ATTR_TYPE_U8, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_HOLD_ENABLED_ID(data_ptr) \
	{ ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_HOLD_ENABLED_ID, ZB_ZCL_ATTR_TYPE_BOOL, \
	  ZB_ZCL_ATTR_ACCESS_READ_WRITE, (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *)data_ptr }
/* clang-format on */

#define ZB_ZCL_DECLARE_GESTURE_CONFIG_ATTRIB_LIST(                                                        \
		attr_list, num_channels, poll_ms, long_press_ms, multi_gap_ms,                             \
		ch1_max_taps, ch1_hold_enabled, ch2_max_taps, ch2_hold_enabled)                            \
	ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_GESTURE_CONFIG)              \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_NUM_CHANNELS_ID, (num_channels))                 \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_POLL_MS_ID, (poll_ms))                           \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_LONG_PRESS_MS_ID, (long_press_ms))               \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_MULTI_GAP_MS_ID, (multi_gap_ms))                 \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_MAX_TAPS_ID, (ch1_max_taps))                 \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_CH1_HOLD_ENABLED_ID, (ch1_hold_enabled))         \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_MAX_TAPS_ID, (ch2_max_taps))                 \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_GESTURE_CONFIG_ATTR_CH2_HOLD_ENABLED_ID, (ch2_hold_enabled))         \
	ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST

#define ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG_SERVER_ROLE_INIT ((zb_zcl_cluster_init_t)0)
#define ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG_CLIENT_ROLE_INIT ((zb_zcl_cluster_init_t)0)

#define ZB_GESTURE_CONFIG_DEVICE_ID 0x0000
#define ZB_DEVICE_VER_GESTURE_CONFIG 0

#define ZB_GESTURE_CONFIG_IN_CLUSTER_NUM 3
#define ZB_GESTURE_CONFIG_OUT_CLUSTER_NUM 0

#define ZB_DECLARE_GESTURE_CONFIG_CLUSTER_LIST(                              \
		cluster_list_name,                                            \
		basic_attr_list,                                              \
		identify_attr_list,                                           \
		gesture_config_attr_list)                                     \
zb_zcl_cluster_desc_t cluster_list_name[] =                                   \
{                                                                             \
	ZB_ZCL_CLUSTER_DESC(                                                  \
		ZB_ZCL_CLUSTER_ID_BASIC,                                      \
		ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),            \
		(basic_attr_list),                                            \
		ZB_ZCL_CLUSTER_SERVER_ROLE,                                   \
		ZB_ZCL_MANUF_CODE_INVALID                                     \
	),                                                                     \
	ZB_ZCL_CLUSTER_DESC(                                                  \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,                                   \
		ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),         \
		(identify_attr_list),                                         \
		ZB_ZCL_CLUSTER_SERVER_ROLE,                                   \
		ZB_ZCL_MANUF_CODE_INVALID                                     \
	),                                                                     \
	ZB_ZCL_CLUSTER_DESC(                                                  \
		ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG,                             \
		ZB_ZCL_ARRAY_SIZE(gesture_config_attr_list, zb_zcl_attr_t),   \
		(gesture_config_attr_list),                                   \
		ZB_ZCL_CLUSTER_SERVER_ROLE,                                   \
		ZB_ZCL_MANUF_CODE_INVALID                                     \
	)                                                                      \
}

#define ZB_DECLARE_GESTURE_CONFIG_SIMPLE_DESC_TYPE() \
	ZB_DECLARE_SIMPLE_DESC(3, 0)

#define ZB_ZCL_DECLARE_GESTURE_CONFIG_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =            \
	{                                                                                       \
		ep_id,                                                                         \
		ZB_AF_HA_PROFILE_ID,                                                           \
		ZB_GESTURE_CONFIG_DEVICE_ID,                                                   \
		ZB_DEVICE_VER_GESTURE_CONFIG,                                                  \
		0,                                                                              \
		in_clust_num,                                                                  \
		out_clust_num,                                                                 \
		{                                                                               \
			ZB_ZCL_CLUSTER_ID_BASIC,                                               \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,                                            \
			ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG,                                      \
		}                                                                               \
	}

#define ZB_DECLARE_GESTURE_CONFIG_EP(ep_name, ep_id, cluster_list)                    \
	ZB_ZCL_DECLARE_GESTURE_CONFIG_SIMPLE_DESC(ep_name, ep_id,                     \
		ZB_GESTURE_CONFIG_IN_CLUSTER_NUM, ZB_GESTURE_CONFIG_OUT_CLUSTER_NUM); \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,     \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                    \
		0, NULL,                                                              \
		0, NULL)
