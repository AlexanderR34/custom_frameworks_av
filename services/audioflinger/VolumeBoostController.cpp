/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VolumeBoostController"

#include "VolumeBoostController.h"
#include <log/log.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace android {

static inline float softSaturate(float x) {
    if (x > 0.8f) {
        float excess = x - 0.8f;
        return 0.8f + 0.2f * std::tanh(excess * 2.5f);
    } else if (x < -0.8f) {
        float excess = -x - 0.8f;
        return -(0.8f + 0.2f * std::tanh(excess * 2.5f));
    }
    return x;
}

VolumeBoostController::VolumeBoostController() {
    mGain.store(1.0f, std::memory_order_relaxed);
    refreshGain();
}

void VolumeBoostController::refreshGain() {
    char propVal[PROPERTY_VALUE_MAX] = {0};
    if (property_get("persist.sys.volume_boost_gain", propVal, "") > 0 && propVal[0] != '\0') {
        float gain = static_cast<float>(std::atof(propVal));
        if (gain >= 1.0f && gain <= 6.0f) {
            mGain.store(gain, std::memory_order_relaxed);
        }
    }
}

void VolumeBoostController::setBoostMultiplier(float multiplier) {
    if (multiplier < 1.0f) multiplier = 1.0f;
    if (multiplier > 6.0f) multiplier = 6.0f;
    mGain.store(multiplier, std::memory_order_relaxed);
}

void VolumeBoostController::processPcm(void* buffer, size_t bytes, audio_format_t format, size_t /*channelCount*/) {
    if (buffer == nullptr || bytes == 0) {
        return;
    }

    const float gain = mGain.load(std::memory_order_relaxed);
    if (gain <= 1.001f) {
        return; // Bypass inmediato con 0 sobrecarga de CPU cuando está al 100%
    }

    if (format == AUDIO_FORMAT_PCM_FLOAT) {
        float* samples = static_cast<float*>(buffer);
        const size_t sampleCount = bytes / sizeof(float);

        for (size_t i = 0; i < sampleCount; ++i) {
            float s = samples[i] * gain;
            samples[i] = softSaturate(s);
        }
    } else if (format == AUDIO_FORMAT_PCM_16_BIT) {
        int16_t* samples = static_cast<int16_t*>(buffer);
        const size_t sampleCount = bytes / sizeof(int16_t);

        for (size_t i = 0; i < sampleCount; ++i) {
            float s = (static_cast<float>(samples[i]) / 32768.0f) * gain;
            s = softSaturate(s);
            samples[i] = static_cast<int16_t>(std::clamp(s * 32767.0f, -32768.0f, 32767.0f));
        }
    }
}

} // namespace android

