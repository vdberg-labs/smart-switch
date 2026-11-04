#pragma once

/* Project-local extension of the SDK's fixed <app-common/zap-generated/callback.h> (found first
 * because default_zap is earlier on the include path than zzz_generated/app-common) - that file
 * only forward-declares emberAf<Cluster>ClusterInitCallback for clusters known to the SDK's own
 * master data model. Without this, callback-stub.cpp's use of
 * emberAfGestureConfigClusterInitCallback() (before its own later weak definition of it) has no
 * prior declaration. #include_next chains to the real one unmodified; see CLAUDE.md's
 * "ZAP-Generated Files" section for the full trace. */
#include_next <app-common/zap-generated/callback.h>

void emberAfGestureConfigClusterInitCallback(chip::EndpointId endpoint);
