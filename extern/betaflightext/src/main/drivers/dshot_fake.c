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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "platform.h"

#ifdef USE_DSHOT

#include "common/maths.h"
#include "common/time.h"

#include "drivers/dshot.h"
#include "drivers/dshot_command.h"
#include "drivers/motor.h"
#include "drivers/time.h"

#include "pg/motor.h"

#include "drivers/dshot_fake.h"

// Globals other Betaflight units expect from the platform's DSHOT implementation.
FAST_DATA_ZERO_INIT bool useDshotTelemetry = false;
#ifdef USE_DSHOT_TELEMETRY
FAST_DATA_ZERO_INIT dshotTelemetryCycleCounters_t dshotDMAHandlerCycleCounters;  // cli dshot_telemetry_info
FAST_DATA_ZERO_INIT uint32_t inputStampUs;
uint32_t readDoneCount;
#endif

// The simulation's motor input (target.c); 0..1000 like the PWM driver writes it.
extern int16_t motorsPwm[MAX_SUPPORTED_MOTORS];

float simDshotMotorRpm[MAX_SUPPORTED_MOTORS];

typedef struct fakeDshotMotor_s {
    dshotProtocolControl_t protocolControl;
    bool enabled;
} fakeDshotMotor_t;

static fakeDshotMotor_t dshotMotors[MAX_SUPPORTED_MOTORS];

bool isDshotBitbangActive(const motorDevConfig_t *motorDevConfig)
{
    UNUSED(motorDevConfig);
    return false;
}

#ifdef USE_DSHOT_TELEMETRY
// Encode a mechanical RPM the way an ESC answers a bidirectional DSHOT frame:
// eRPM period in microseconds as 9-bit mantissa with 3-bit exponent ("eee mmmmmmmmm"),
// 0x0fff for "not spinning". The result is what Betaflight's dshot_decode_eRPM_telemetry_value()
// consumes, quantisation included.
static uint16_t encodeErpmPeriod(float rpm)
{
    const float erpm = rpm * (motorConfig()->motorPoleCount / 2.0f);
    if (erpm < 1.0f) {
        return 0x0fff;
    }
    uint32_t period = lrintf(60000000.0f / erpm);
    if (period < 1) {
        period = 1;
    }
    unsigned exponent = 0;
    while (period > 0x1ff && exponent < 7) {
        period = (period + 1) >> 1;
        exponent++;
    }
    if (period > 0x1ff) {
        return 0x0fff;   // slower than the format can express: report standstill
    }
    return (uint16_t)((exponent << 9) | period);
}
#endif

static bool fakeDshotDecodeTelemetry(void)
{
#ifdef USE_DSHOT_TELEMETRY
    if (!useDshotTelemetry) {
        return true;
    }
#ifdef USE_DSHOT_TELEMETRY_STATS
    const timeMs_t currentTimeMs = millis();
#endif
    for (unsigned i = 0; i < MAX_SUPPORTED_MOTORS && i < dshotMotorCount; i++) {
        dshotTelemetryState.motorState[i].rawValue = encodeErpmPeriod(simDshotMotorRpm[i]);
        dshotTelemetryState.readCount++;
#ifdef USE_DSHOT_TELEMETRY_STATS
        updateDshotTelemetryQuality(&dshotTelemetryQuality[i], true, currentTimeMs);
#endif
    }
    // updateDshotTelemetry() (fc/core.c, filtering task) decodes the raw values from here on.
    dshotTelemetryState.rawValueState = DSHOT_RAW_VALUE_STATE_NOT_PROCESSED;
#endif
    return true;
}

static void fakeDshotWriteInt(uint8_t index, uint16_t value)
{
    if (index >= dshotMotorCount) {
        return;
    }
    fakeDshotMotor_t *motor = &dshotMotors[index];

    // A pending DSHOT command (beacon, spin direction, ...) replaces the throttle value.
    if (dshotCommandIsProcessing()) {
        value = dshotCommandGetCurrent(index);
        if (value) {
            motor->protocolControl.requestTelemetry = true;
        }
    }
    motor->protocolControl.value = value;
    (void)prepareDshotPacket(&motor->protocolControl);   // keeps the telemetry-request handshake honest

    // DSHOT 48..2047 -> simulation 0..1000; commands (< 48) and MOTOR_STOP mean "off".
    if (value < DSHOT_MIN_THROTTLE) {
        motorsPwm[index] = 0;
    } else {
        motorsPwm[index] = lrintf((value - DSHOT_MIN_THROTTLE) * 1000.0f / DSHOT_RANGE);
    }
}

static void fakeDshotWrite(uint8_t index, float value)
{
    fakeDshotWriteInt(index, lrintf(value));
}

static void fakeDshotUpdateComplete(void)
{
    // Advance the DSHOT command state machine in step with the motor update, like the real drivers.
    if (!dshotCommandQueueEmpty()) {
        if (!dshotCommandOutputIsEnabled(dshotMotorCount)) {
            return;
        }
    }
}

static void fakeDshotPostInit(void)
{
    for (unsigned i = 0; i < MAX_SUPPORTED_MOTORS && i < dshotMotorCount; i++) {
        dshotMotors[i].enabled = true;
    }
}

static bool fakeDshotEnable(void)
{
    return true;
}

static void fakeDshotDisable(void)
{
}

static bool fakeDshotIsMotorEnabled(unsigned index)
{
    return index < dshotMotorCount && dshotMotors[index].enabled;
}

static void fakeDshotShutdown(void)
{
    for (unsigned i = 0; i < MAX_SUPPORTED_MOTORS; i++) {
        motorsPwm[i] = 0;
    }
}

static bool fakeDshotIsMotorIdle(unsigned index)
{
    return index < dshotMotorCount && dshotMotors[index].protocolControl.value == 0;
}

static void fakeDshotRequestTelemetry(unsigned index)
{
    if (index < dshotMotorCount) {
        dshotMotors[index].protocolControl.requestTelemetry = true;
    }
}

static const motorVTable_t fakeDshotVTable = {
    .postInit = fakeDshotPostInit,
    .convertExternalToMotor = dshotConvertFromExternal,
    .convertMotorToExternal = dshotConvertToExternal,
    .enable = fakeDshotEnable,
    .disable = fakeDshotDisable,
    .isMotorEnabled = fakeDshotIsMotorEnabled,
    .telemetryWait = NULL,
    .decodeTelemetry = fakeDshotDecodeTelemetry,
    .updateInit = NULL,
    .write = fakeDshotWrite,
    .writeInt = fakeDshotWriteInt,
    .updateComplete = fakeDshotUpdateComplete,
    .shutdown = fakeDshotShutdown,
    .isMotorIdle = fakeDshotIsMotorIdle,
    .getMotorIO = NULL,
    .requestTelemetry = fakeDshotRequestTelemetry,
};

// Called by motorDevInit() for every DSHOT protocol (motor_pwm_protocol = DSHOT150/300/600).
bool dshotPwmDevInit(motorDevice_t *device, const motorDevConfig_t *motorConfig)
{
    if (!device) {
        return false;
    }
    dshotMotorCount = MIN(device->count, MAX_SUPPORTED_MOTORS);
#ifdef USE_DSHOT_TELEMETRY
    useDshotTelemetry = motorConfig->useDshotTelemetry;
#else
    UNUSED(motorConfig);
#endif
    for (unsigned i = 0; i < MAX_SUPPORTED_MOTORS; i++) {
        dshotMotors[i].protocolControl.value = 0;
        dshotMotors[i].protocolControl.requestTelemetry = false;
        dshotMotors[i].enabled = false;
        motorsPwm[i] = 0;
    }
    device->vTable = &fakeDshotVTable;
    printf("Initialized fake DSHOT motor count %d, bidirectional telemetry %s\n",
           dshotMotorCount, useDshotTelemetry ? "on" : "off");
    return true;
}

#endif // USE_DSHOT
