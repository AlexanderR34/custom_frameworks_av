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

namespace android {

/**
 * VolumeBoostController:
 * Controlador nativo e independiente de sobreamplificación PCM (+12 dB / 200%).
 * Opera a nivel de muestras de audio en memoria en AudioFlinger / FastMixer,
 * garantizando amplificación para motores de juegos (Unity, Unreal, AAudio FAST/RAW)
 * y audio estándar con limitador de saturación suave (Soft-Limiter).
 */
class VolumeBoostController {
public:
    static VolumeBoostController& getInstance() {
        static VolumeBoostController sInstance;
        return sInstance;
    }

    // Procesa el buffer PCM según su formato nativo (Float o 16-bit)
    void processPcm(void* buffer, size_t bytes, audio_format_t format, size_t channelCount);

    // Consulta y actualiza el multiplicador desde persist.sys.volume_boost_gain
    void refreshGain();

    // Establece el multiplicador de ganancia directamente (1.0f = normal, 2.0f = 200%)
    void setBoostMultiplier(float multiplier);

    float getBoostMultiplier() const {
        return mGain.load(std::memory_order_relaxed);
    }

private:
    VolumeBoostController();
    ~VolumeBoostController() = default;
    VolumeBoostController(const VolumeBoostController&) = delete;
    VolumeBoostController& operator=(const VolumeBoostController&) = delete;

    std::atomic<float> mGain{1.0f};
    std::atomic<uint32_t> mCheckCounter{0};
};

} // namespace android
