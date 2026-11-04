#pragma once

namespace SmartSwitch
{

/* Pushes the gesture engine's current (real, persisted) config into the GestureConfig cluster's
 * RAM attribute storage. Must be called after GestureEngine::Start()/ApplyConfig() have run -
 * emberAfGestureConfigClusterInitCallback() runs too early for that (before ConfigStore::Load()),
 * so AppTask::Init() calls this separately once the real config is loaded. */
void RefreshGestureConfigAttributes();

}
