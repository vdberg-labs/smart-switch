#include "../app_task.h"
#include "matter_gesture_config_cluster.h"

#include "shared/config/config_store.h"
#include "shared/config/gesture_config.h"

#include <app-common/zap-generated/attribute-type.h>
#include <app/ConcreteAttributePath.h>
#include <app/util/attribute-table.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

/*
 * GestureConfig (0xfff1fc01) is a custom/external cluster - see EXTERNAL_CLUSTERS in
 * ../../CMakeLists.txt. NCS v3.3.0's data-model codegen only generates typed C++
 * Attributes::Get/Set accessors for clusters known to the SDK's own checked-in
 * zzz_generated database; a project-local custom cluster gets none, so attribute
 * access here goes through the generic ember emberAfReadAttribute/WriteAttribute API
 * against the raw cluster/attribute MEI codes from light_switch.matter instead.
 */
namespace
{
constexpr EndpointId kGestureConfigEndpointId = 3;
constexpr ClusterId kGestureConfigClusterId = 0xfff1fc01;

constexpr AttributeId kAttrNumChannels = 0xfff10000;
constexpr AttributeId kAttrPollMs = 0xfff10001;
constexpr AttributeId kAttrLongPressMs = 0xfff10002;
constexpr AttributeId kAttrMultiGapMs = 0xfff10003;
constexpr AttributeId kAttrCh1MaxTaps = 0xfff10004;
constexpr AttributeId kAttrCh1HoldEnabled = 0xfff10005;
constexpr AttributeId kAttrCh2MaxTaps = 0xfff10006;
constexpr AttributeId kAttrCh2HoldEnabled = 0xfff10007;

using Protocols::InteractionModel::Status;

/* Set while ApplyStoredConfigToAttributes() is pushing the engine's own state into the ZAP
 * attribute table, so MatterPostAttributeChangeCallback() knows these writes are a mirror of
 * already-authoritative state, not an incoming client write - see the comment there. */
bool sMirroringEngineState = false;

Status WriteU8(AttributeId attribute, uint8_t value)
{
	return emberAfWriteAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute, &value,
				      ZCL_INT8U_ATTRIBUTE_TYPE);
}

Status WriteU16(AttributeId attribute, uint16_t value)
{
	return emberAfWriteAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute,
				      reinterpret_cast<uint8_t *>(&value), ZCL_INT16U_ATTRIBUTE_TYPE);
}

Status WriteBool(AttributeId attribute, bool value)
{
	uint8_t raw = value ? 1 : 0;
	return emberAfWriteAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute, &raw,
				      ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}

Status ReadU8(AttributeId attribute, uint8_t &value)
{
	return emberAfReadAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute, &value, sizeof(value));
}

Status ReadU16(AttributeId attribute, uint16_t &value)
{
	return emberAfReadAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute,
				     reinterpret_cast<uint8_t *>(&value), sizeof(value));
}

Status ReadBool(AttributeId attribute, bool &value)
{
	uint8_t raw = 0;
	Status status = emberAfReadAttribute(kGestureConfigEndpointId, kGestureConfigClusterId, attribute, &raw, sizeof(raw));
	value = (raw != 0);
	return status;
}

void ApplyStoredConfigToAttributes(const SmartSwitch::EngineConfig &cfg)
{
	sMirroringEngineState = true;
	WriteU8(kAttrNumChannels, cfg.numChannels);
	WriteU16(kAttrPollMs, static_cast<uint16_t>(cfg.pollMs));
	WriteU16(kAttrLongPressMs, static_cast<uint16_t>(cfg.longPressMs));
	WriteU16(kAttrMultiGapMs, static_cast<uint16_t>(cfg.multiGapMs));
	WriteU8(kAttrCh1MaxTaps, cfg.channels[0].maxTaps);
	WriteBool(kAttrCh1HoldEnabled, cfg.channels[0].holdEnabled);
	WriteU8(kAttrCh2MaxTaps, cfg.channels[1].maxTaps);
	WriteBool(kAttrCh2HoldEnabled, cfg.channels[1].holdEnabled);
	sMirroringEngineState = false;
}

bool ReadConfigFromAttributes(SmartSwitch::EngineConfig &cfg)
{
	uint8_t numChannels = 0;
	uint16_t pollMs = 0;
	uint16_t longPressMs = 0;
	uint16_t multiGapMs = 0;
	uint8_t ch1MaxTaps = 0;
	uint8_t ch2MaxTaps = 0;
	bool ch1Hold = false;
	bool ch2Hold = false;

	if (ReadU8(kAttrNumChannels, numChannels) != Status::Success) {
		return false;
	}
	if (ReadU16(kAttrPollMs, pollMs) != Status::Success) {
		return false;
	}
	if (ReadU16(kAttrLongPressMs, longPressMs) != Status::Success) {
		return false;
	}
	if (ReadU16(kAttrMultiGapMs, multiGapMs) != Status::Success) {
		return false;
	}
	if (ReadU8(kAttrCh1MaxTaps, ch1MaxTaps) != Status::Success) {
		return false;
	}
	if (ReadBool(kAttrCh1HoldEnabled, ch1Hold) != Status::Success) {
		return false;
	}
	if (ReadU8(kAttrCh2MaxTaps, ch2MaxTaps) != Status::Success) {
		return false;
	}
	if (ReadBool(kAttrCh2HoldEnabled, ch2Hold) != Status::Success) {
		return false;
	}

	cfg.numChannels = numChannels;
	cfg.pollMs = pollMs;
	cfg.longPressMs = longPressMs;
	cfg.multiGapMs = multiGapMs;
	cfg.channels[0] = SmartSwitch::ChannelConfig{ ch1MaxTaps, ch1Hold };
	cfg.channels[1] = SmartSwitch::ChannelConfig{ ch2MaxTaps, ch2Hold };
	return true;
}
} /* namespace */

void emberAfGestureConfigClusterInitCallback(EndpointId endpoint)
{
	if (endpoint != kGestureConfigEndpointId) {
		return;
	}
	/* Only seeds ZAP's own static defaults at this point - GestureEngine::Start()/ApplyConfig()
	 * (and therefore the real persisted config) haven't run yet this early in boot. AppTask::Init()
	 * calls SmartSwitch::RefreshGestureConfigAttributes() again once they have. */
	ApplyStoredConfigToAttributes(AppTask::Instance().GetGestureEngine().GetConfig());
}

namespace SmartSwitch
{
void RefreshGestureConfigAttributes()
{
	ApplyStoredConfigToAttributes(AppTask::Instance().GetGestureEngine().GetConfig());
}
} /* namespace SmartSwitch */

/* Referenced (unconditionally, not weak) by the generated PluginApplicationCallbacks.h
 * INIT/SHUTDOWN macros - every custom cluster must provide these, same as Nordic's own
 * manufacturer_specific sample does for its NordicDevKit cluster. */
void MatterGestureConfigPluginServerInitCallback()
{
	/* Intentionally empty - real init happens in emberAfGestureConfigClusterInitCallback(). */
}

void MatterGestureConfigPluginServerShutdownCallback()
{
	/* Intentionally empty */
}

void MatterPostAttributeChangeCallback(const ConcreteAttributePath &attributePath, uint8_t /* type */,
					uint16_t /* size */, uint8_t * /* value */)
{
	if (attributePath.mEndpointId != kGestureConfigEndpointId || attributePath.mClusterId != kGestureConfigClusterId) {
		return;
	}

	/* emberAfWriteAttribute() (used by ApplyStoredConfigToAttributes() to mirror the engine's own
	 * state into these attributes) invokes this callback synchronously for every attribute it
	 * writes, indistinguishably from a real incoming client write. Reacting to our own mirroring
	 * writes here would call GestureEngine::ApplyConfig() - which, the first time this happens, is
	 * from deep inside emberAfGestureConfigClusterInitCallback(), before GestureEngine::Start() has
	 * run and initialized mApplyWork, i.e. k_work_submit() on an uninitialized k_work: a silent
	 * abort()/kernel panic, confirmed via addr2line against the crashing device's own build - not
	 * just a theoretical race. */
	if (sMirroringEngineState) {
		return;
	}

	SmartSwitch::EngineConfig cfg{};
	if (!ReadConfigFromAttributes(cfg)) {
		LOG_WRN("GestureConfig: failed reading back attributes after write");
		return;
	}

	if (!cfg.IsValid()) {
		LOG_WRN("GestureConfig: write produced an invalid configuration, not applying");
		return;
	}

	SmartSwitch::ConfigStore::Save(AppTask::Instance().GetPersistentStore(), cfg);
	AppTask::Instance().GetGestureEngine().ApplyConfig(cfg);
}
