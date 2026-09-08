/*
 * This file is part of SimITL, a Betaflight SITL wrapper.
 *
 * SimITL and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>.
 */

// Shadows betaflight/src/main/build/atomic.h for the SimITL target (fpv-followcam-sim #23).
// The upstream header manipulates the Cortex-M BASEPRI register; the simulator has no interrupts,
// Betaflight runs single-threaded inside the sim loop, so the atomic blocks become plain blocks.
// Needed since drivers/dshot.c (prepareDshotPacket) entered the build.

#pragma once

#include <stdint.h>

#define ATOMIC_BLOCK(prio)    for (uint8_t __atomic_once __attribute__((__unused__)) = 1; __atomic_once; __atomic_once = 0)
#define ATOMIC_BLOCK_NB(prio) for (uint8_t __atomic_once __attribute__((__unused__)) = 1; __atomic_once; __atomic_once = 0)

#define ATOMIC_BARRIER_ENTER(dataPtr, refStr) do { (void)(dataPtr); } while (0)
#define ATOMIC_BARRIER_LEAVE(dataPtr, refStr) do { (void)(dataPtr); } while (0)
#define ATOMIC_BARRIER(data)  do { (void)(data); } while (0)

#define ATOMIC_OR(ptr, val)  __sync_fetch_and_or(ptr, val)
#define ATOMIC_AND(ptr, val) __sync_fetch_and_and(ptr, val)
