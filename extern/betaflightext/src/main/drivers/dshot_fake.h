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

// Fake DSHOT motor driver for the SimITL target (fpv-followcam-sim #23).
//
// Betaflight's DSHOT output is mapped onto the simulation's motor input (motorsPwm, 0..1000),
// and the simulation's motor speed is handed back as bidirectional DSHOT telemetry, encoded
// exactly like an ESC would send it, so that Betaflight's own decoder, RPM filter, dynamic idle
// and blackbox eRPM fields see the same data path as on real hardware.

#pragma once

#include "platform.h"
#include "drivers/motor_types.h"

// Written by the simulation (src/sim/bf.cpp) before every scheduler pass: mechanical RPM per motor.
extern float simDshotMotorRpm[MAX_SUPPORTED_MOTORS];
