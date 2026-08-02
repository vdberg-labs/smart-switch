#include "zigbee/zb_gesture_config_cluster.h"
#include "zigbee/zb_gesture_switch_cluster.h"
#include "zigbee/zboss_cpp.h"

#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>

#include "zigbee/zigbee_ble_config_mode.h"
#include "zigbee/zigbee_factory_reset.h"
#include "zigbee/zigbee_gesture_config_attrs.h"
#include "zigbee/zigbee_network_removal.h"
#include "zigbee/zigbee_network_info.h"
#include "zigbee/zigbee_persistent_store.h"
#include "zigbee/zigbee_switch_protocol.h"

#include "shared/ble/gesture_config_service.h"
#include "shared/config/config_store.h"
#include "shared/gesture/gesture_engine.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace
{
constexpr zb_uint8_t kFirstEndpoint = 1;
constexpr zb_uint8_t kNumChannels = 2;

constexpr zb_bool_t kErasePersistentConfigAtBoot = ZB_FALSE;

constexpr char kManufacturerName[] = "vdBerg Labs";
constexpr char kModelIdentifier[] = "SmartSwitch Rev1";

struct zb_device_ctx {
	zb_zcl_basic_attrs_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
	zb_char_t manufacturer_name[1 + ZB_ZCL_CLUSTER_ID_BASIC_MANUFACTURER_NAME_MAX_LEN];
	zb_char_t model_identifier[1 + ZB_ZCL_CLUSTER_ID_BASIC_MODEL_IDENTIFIER_MAX_LEN];
};

zb_device_ctx sDevCtx[kNumChannels];

zb_device_ctx sConfigDevCtx;

#define SMART_SWITCH_DECLARE_BASIC_ATTRIB_LIST(attr_list, zcl_version, power_source, manufacturer_name, \
						model_id)                                                  \
	ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_BASIC)                       \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID, (zcl_version))                             \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, (power_source))                           \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (manufacturer_name))                 \
	ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (model_id))                           \
	ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST

ZB_DECLARE_GESTURE_SWITCH_SIMPLE_DESC_TYPE();

SMART_SWITCH_DECLARE_BASIC_ATTRIB_LIST(sBasicAttrList0, &sDevCtx[0].basic_attr.zcl_version, &sDevCtx[0].basic_attr.power_source,
					sDevCtx[0].manufacturer_name, sDevCtx[0].model_identifier);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(sIdentifyAttrList0, &sDevCtx[0].identify_attr.identify_time);
ZB_ZCL_DECLARE_GESTURE_SWITCH_CLIENT_ATTRIB_LIST(sGestureSwitchAttrList0);
ZB_DECLARE_GESTURE_SWITCH_CLUSTER_LIST(sClusterList0, sBasicAttrList0, sIdentifyAttrList0, sGestureSwitchAttrList0);
ZB_DECLARE_GESTURE_SWITCH_EP(sEp0, kFirstEndpoint + 0, sClusterList0);

SMART_SWITCH_DECLARE_BASIC_ATTRIB_LIST(sBasicAttrList1, &sDevCtx[1].basic_attr.zcl_version, &sDevCtx[1].basic_attr.power_source,
					sDevCtx[1].manufacturer_name, sDevCtx[1].model_identifier);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(sIdentifyAttrList1, &sDevCtx[1].identify_attr.identify_time);
ZB_ZCL_DECLARE_GESTURE_SWITCH_CLIENT_ATTRIB_LIST(sGestureSwitchAttrList1);
ZB_DECLARE_GESTURE_SWITCH_CLUSTER_LIST(sClusterList1, sBasicAttrList1, sIdentifyAttrList1, sGestureSwitchAttrList1);
ZB_DECLARE_GESTURE_SWITCH_EP(sEp1, kFirstEndpoint + 1, sClusterList1);

SmartSwitch::ZigbeeGestureConfigAttrs sGestureConfigAttrs;

SMART_SWITCH_DECLARE_BASIC_ATTRIB_LIST(sConfigBasicAttrList, &sConfigDevCtx.basic_attr.zcl_version,
					&sConfigDevCtx.basic_attr.power_source, sConfigDevCtx.manufacturer_name,
					sConfigDevCtx.model_identifier);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(sConfigIdentifyAttrList, &sConfigDevCtx.identify_attr.identify_time);
ZB_ZCL_DECLARE_GESTURE_CONFIG_ATTRIB_LIST(sGestureConfigAttrList, &sGestureConfigAttrs.numChannels,
					   &sGestureConfigAttrs.pollMs, &sGestureConfigAttrs.longPressMs,
					   &sGestureConfigAttrs.multiGapMs, &sGestureConfigAttrs.ch1MaxTaps,
					   &sGestureConfigAttrs.ch1HoldEnabled, &sGestureConfigAttrs.ch2MaxTaps,
					   &sGestureConfigAttrs.ch2HoldEnabled);
ZB_DECLARE_GESTURE_CONFIG_CLUSTER_LIST(sConfigClusterList, sConfigBasicAttrList, sConfigIdentifyAttrList,
					sGestureConfigAttrList);
ZB_DECLARE_GESTURE_CONFIG_SIMPLE_DESC_TYPE();
ZB_DECLARE_GESTURE_CONFIG_EP(sEp2, kFirstEndpoint + kNumChannels, sConfigClusterList);

ZBOSS_DECLARE_DEVICE_CTX_3_EP(sDeviceCtx, sEp0, sEp1, sEp2);

SmartSwitch::ZigbeeSwitchProtocol sSwitchProtocol;
SmartSwitch::ZigbeeBleConfigMode sBleConfigMode;
SmartSwitch::ZigbeePersistentStore sPersistentStore;
SmartSwitch::ZigbeeFactoryReset sFactoryReset;
SmartSwitch::ZigbeeNetworkRemoval sNetworkRemoval;
SmartSwitch::ZigbeeNetworkInfo sCommissioningInfo;
SmartSwitch::GestureEngine sGestureEngine(sSwitchProtocol, sBleConfigMode);

void InitClusterAttrs()
{
	for (auto &ctx : sDevCtx) {
		ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
		ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE;
		ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
		ZB_ZCL_SET_STRING_VAL(ctx.manufacturer_name, kManufacturerName, ZB_ZCL_STRING_CONST_SIZE(kManufacturerName));
		ZB_ZCL_SET_STRING_VAL(ctx.model_identifier, kModelIdentifier, ZB_ZCL_STRING_CONST_SIZE(kModelIdentifier));
	}

	sConfigDevCtx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	sConfigDevCtx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE;
	sConfigDevCtx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
	ZB_ZCL_SET_STRING_VAL(sConfigDevCtx.manufacturer_name, kManufacturerName, ZB_ZCL_STRING_CONST_SIZE(kManufacturerName));
	ZB_ZCL_SET_STRING_VAL(sConfigDevCtx.model_identifier, kModelIdentifier, ZB_ZCL_STRING_CONST_SIZE(kModelIdentifier));
}

void ZclDeviceCb(zb_bufid_t bufid)
{
	zb_zcl_device_callback_param_t *param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

	param->status = RET_OK;

	if (param->device_cb_id != ZB_ZCL_SET_ATTR_VALUE_CB_ID) {
		param->status = RET_NOT_IMPLEMENTED;
		return;
	}

	if (param->cb_param.set_attr_value_param.cluster_id != ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG) {
		param->status = RET_NOT_IMPLEMENTED;
		return;
	}

	sGestureConfigAttrs.OnAttrWritten();
}

zb_ret_t AlwaysAllowAttrWrite(zb_uint16_t /*attr_id*/, zb_uint8_t /*endpoint*/, zb_uint8_t * /*value*/)
{
	return RET_OK;
}

} /* namespace */

extern "C" void zboss_signal_handler(zb_bufid_t bufid)
{
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	if (bufid) {
		zb_buf_free(bufid);
	}
}

int main(void)
{
	LOG_INF("Starting Smart Switch Zigbee firmware");

	zigbee_erase_persistent_storage(kErasePersistentConfigAtBoot);

	InitClusterAttrs();
	ZB_AF_REGISTER_DEVICE_CTX(&sDeviceCtx);
	zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_GESTURE_CONFIG, ZB_ZCL_CLUSTER_SERVER_ROLE, AlwaysAllowAttrWrite,
				    nullptr, nullptr);

	sPersistentStore.Init();
	sGestureEngine.Start();
	sGestureEngine.ApplyConfig(SmartSwitch::ConfigStore::Load(sPersistentStore));
	sGestureConfigAttrs.Init(sGestureEngine, sPersistentStore);
	sBleConfigMode.Init();
	SmartSwitch::GestureConfigService::Init(sGestureEngine, sBleConfigMode, sPersistentStore, sFactoryReset,
						 sCommissioningInfo, sNetworkRemoval);

	ZB_ZCL_REGISTER_DEVICE_CB(ZclDeviceCb);

	zigbee_enable();

	LOG_INF("Smart Switch Zigbee firmware started");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
