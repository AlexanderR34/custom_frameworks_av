/*
 * Copyright (C) 2021-2022 The LineageOS Project
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

#include <string>
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <utils/String8.h>
#include <utils/String16.h>

namespace android {
namespace media {

class AppVolume : public Parcelable {
public:
    AppVolume() :
        packageName(String8("")),
        volume(1.0f),
        muted(false),
        active(false) {}
    AppVolume(const String8& pkg, float vol, bool mute, bool act) :
        packageName(pkg),
        volume(vol),
        muted(mute),
        active(act) {}
    virtual ~AppVolume() = default;

    bool operator<(const AppVolume& other) const {
        return packageName < other.packageName;
    }

    virtual status_t writeToParcel(Parcel *parcel) const override {
        parcel->writeString8(packageName);
        parcel->writeFloat(volume);
        parcel->writeBool(muted);
        parcel->writeBool(active);
        return NO_ERROR;
    }

    virtual status_t readFromParcel(const Parcel *parcel) override {
        packageName = parcel->readString8();
        volume = parcel->readFloat();
        muted = parcel->readBool();
        active = parcel->readBool();
        return NO_ERROR;
    }

    String8 packageName;
    float volume;
    bool muted;
    bool active;
};

} // namespace media
} // namespace android
