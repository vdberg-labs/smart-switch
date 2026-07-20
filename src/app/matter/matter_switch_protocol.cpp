#include "matter_switch_protocol.h"

#include <app/EventLogging.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <platform/CHIPDeviceLayer.h>

using namespace ::chip;
using namespace ::chip::app;

namespace SmartSwitch
{

void MatterSwitchProtocol::OnGesture(uint8_t channel, Gesture gesture, uint8_t count)
{
	const EndpointId ep = static_cast<EndpointId>(kFirstEndpoint + channel);

	chip::DeviceLayer::SystemLayer().ScheduleLambda([ep, gesture, count]() {
		using namespace chip::app::Clusters;
		chip::EventNumber en = 0;
		switch (gesture) {
		case Gesture::InitialPress: {
			Switch::Attributes::CurrentPosition::Set(ep, 1);
			Switch::Events::InitialPress::Type e{ 1 };
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case Gesture::ShortRelease: {
			Switch::Attributes::CurrentPosition::Set(ep, 0);
			Switch::Events::ShortRelease::Type e{};
			e.previousPosition = 1;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case Gesture::Tap: {
			Switch::Events::MultiPressComplete::Type e{};
			e.previousPosition = 0;
			e.totalNumberOfPressesCounted = count;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case Gesture::LongPress: {
			Switch::Events::LongPress::Type e{ 1 };
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case Gesture::LongRelease: {
			Switch::Attributes::CurrentPosition::Set(ep, 0);
			Switch::Events::LongRelease::Type e{};
			e.previousPosition = 1;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		}
	});
}

} /* namespace SmartSwitch */
