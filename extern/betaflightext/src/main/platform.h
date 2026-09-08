/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define NOINLINE __attribute__((noinline))

#if !defined(UNIT_TEST) && !defined(SIMULATOR_BUILD) && !(USBD_DEBUG_LEVEL > 0)
#pragma GCC poison sprintf snprintf
#endif

#ifdef USE_CONFIG
#include "config.h"
#endif

#include "target/common_pre.h"

// MCU specific platform from platform/X
#include "platform/platform.h"

#include "target.h"
#include "target/common_post.h"

// SimITL (fpv-followcam-sim #23): this file shadows betaflight/src/main/platform.h via the include
// path. target/common_post.h drops USE_DYN_NOTCH_FILTER for SIMULATOR_BUILD with the comment that
// it needs arm_math.h; since Betaflight 4.3 the dynamic notch runs on common/sdft.c (plain C),
// so the guard is stale. Restore what the target asked for.
#if defined(SIMULATOR_BUILD) && defined(SIMITL_WANT_DYN_NOTCH_FILTER)
#define USE_DYN_NOTCH_FILTER
#endif
#include "target/common_defaults_post.h"
