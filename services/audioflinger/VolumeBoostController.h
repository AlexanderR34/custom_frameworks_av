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

#pragma once

#include <system/audio.h>
#include <cutils/properties.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstdlib>

namespace android {

/**
 * VolumeBoostController:
 * Native PCM volume amplification controller (+12 dB / 200%).
 * Applies single-stage gain scaling with smooth hyperbolic tangent (tanh)
 * soft-saturation limiter to prevent clipping and protect hardware SmartPA.
 */
class VolumeBoostController {
public:
    static inline float softSaturate(float x) {
        if (x > 0.80f) {
            float excess = x - 0.80f;
            return 0.80f + 0.20f * std::tanh(excess * 1.5f);
        } else if (x < -0.80f) {
            float excess = -x - 0.80f;
            return -(0.80f + 0.20f * std::tanh(excess * 1.5f));
        }
        return x;
    }

    static inline std::atomic<float>& getGainRef() {
        static std::atomic<float> sGain{[]() {
            char propVal[PROPERTY_VALUE_MAX] = {0};
            if (property_get("persist.sys.volume_boost_gain", propVal, "") > 0 && propVal[0] != '\0') {
                float gain = static_cast<float>(std::atof(propVal));
                if (gain >= 1.0f && gain <= 6.0f) {
                    return gain;
                }
            }
            return 1.0f;
        }()};
        return sGain;
    }

    static inline void refreshGain() {
        char propVal[PROPERTY_VALUE_MAX] = {0};
        if (property_get("persist.sys.volume_boost_gain", propVal, "") > 0 && propVal[0] != '\0') {
            float gain = static_cast<float>(std::atof(propVal));
            if (gain >= 1.0f && gain <= 6.0f) {
                getGainRef().store(gain, std::memory_order_relaxed);
            }
        }
    }

    static inline void setBoostMultiplier(float multiplier) {
        if (multiplier < 1.0f) multiplier = 1.0f;
        if (multiplier > 6.0f) multiplier = 6.0f;
        getGainRef().store(multiplier, std::memory_order_relaxed);
    }

    static inline float getBoostMultiplier() {
        return getGainRef().load(std::memory_order_relaxed);
    }

    static inline void processPcm(void* buffer, size_t bytes, audio_format_t format, float customGain = 0.0f) {
        if (buffer == nullptr || bytes == 0) {
            return;
        }

        const float gain = (customGain > 1.0f) ? customGain : getGainRef().load(std::memory_order_relaxed);
        if (gain <= 1.001f) {
            return; // Clean bypass when boost is disabled / standard 100%
        }

        if (format == AUDIO_FORMAT_PCM_FLOAT) {
            float* samples = static_cast<float*>(buffer);
            const size_t sampleCount = bytes / sizeof(float);

            for (size_t i = 0; i < sampleCount; ++i) {
                float s = samples[i] * gain;
                samples[i] = std::clamp(softSaturate(s), -1.0f, 1.0f);
            }
        } else if (format == AUDIO_FORMAT_PCM_16_BIT) {
            int16_t* samples = static_cast<int16_t*>(buffer);
            const size_t sampleCount = bytes / sizeof(int16_t);

            for (size_t i = 0; i < sampleCount; ++i) {
                float s = (static_cast<float>(samples[i]) / 32768.0f) * gain;
                s = std::clamp(softSaturate(s), -1.0f, 1.0f);
                samples[i] = static_cast<int16_t>(s * 32767.0f);
            }
        }
    }
};

} // namespace android
