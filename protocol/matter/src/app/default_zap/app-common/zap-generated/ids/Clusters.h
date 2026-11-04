#pragma once

/* Project-local extension of the SDK's fixed <app-common/zap-generated/ids/Clusters.h> (found
 * first because default_zap is earlier on the include path than zzz_generated/app-common) - that
 * file only aggregates ClusterId.h for clusters known to the SDK's own master data model, never a
 * project-local custom cluster. #include_next chains to the real one unmodified; see CLAUDE.md's
 * "ZAP-Generated Files" section for the full trace. */
#include_next <app-common/zap-generated/ids/Clusters.h>

#include <zap-generated/clusters/GestureConfig/ClusterId.h>
