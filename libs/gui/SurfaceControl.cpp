/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "SurfaceControl"

#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <android/native_window.h>

#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/threads.h>

#include <binder/IPCThreadState.h>

#include <ui/DisplayInfo.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>

#include <gui/BufferQueueCore.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>

namespace android {

// ============================================================================
//  SurfaceControl
// ============================================================================

SurfaceControl::SurfaceControl(
        const sp<SurfaceComposerClient>& client,
        const sp<IBinder>& handle,
        const sp<IGraphicBufferProducer>& gbp)
    : mClient(client), mHandle(handle), mGraphicBufferProducer(gbp)
{
}

SurfaceControl::~SurfaceControl()
{
    destroy();
}

void SurfaceControl::destroy()
{
    if (isValid()) {
        mClient->destroySurface(mHandle);
    }
    // clear all references and trigger an IPC now, to make sure things
    // happen without delay, since these resources are quite heavy.
    mClient.clear();
    mHandle.clear();
    mGraphicBufferProducer.clear();
    IPCThreadState::self()->flushCommands();
}

void SurfaceControl::clear()
{
    // here, the window manager tells us explicitly that we should destroy
    // the surface's resource. Soon after this call, it will also release
    // its last reference (which will call the dtor); however, it is possible
    // that a client living in the same process still holds references which
    // would delay the call to the dtor -- that is why we need this explicit
    // "clear()" call.
    destroy();
}

void SurfaceControl::disconnect() {
    if (mGraphicBufferProducer != nullptr) {
        mGraphicBufferProducer->disconnect(
                BufferQueueCore::CURRENTLY_CONNECTED_API);
    }
}

bool SurfaceControl::isSameSurface(
        const sp<SurfaceControl>& lhs, const sp<SurfaceControl>& rhs)
{
    if (lhs == nullptr || rhs == nullptr)
        return false;
    return lhs->mHandle == rhs->mHandle;
}

status_t SurfaceControl::setLayerStack(uint32_t layerStack) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setLayerStack(mHandle, layerStack);
}

status_t SurfaceControl::setLayer(int32_t layer) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setLayer(mHandle, layer);
}

status_t SurfaceControl::setRelativeLayer(const sp<IBinder>& relativeTo, int32_t layer) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setRelativeLayer(mHandle, relativeTo, layer);
}

status_t SurfaceControl::setPosition(float x, float y) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setPosition(mHandle, x, y);
}
status_t SurfaceControl::setGeometryAppliesWithResize() {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setGeometryAppliesWithResize(mHandle);
}
status_t SurfaceControl::setSize(uint32_t w, uint32_t h) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setSize(mHandle, w, h);
}
status_t SurfaceControl::hide() {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->hide(mHandle);
}
status_t SurfaceControl::show() {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->show(mHandle);
}
status_t SurfaceControl::setFlags(uint32_t flags, uint32_t mask) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setFlags(mHandle, flags, mask);
}
status_t SurfaceControl::setTransparentRegionHint(const Region& transparent) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setTransparentRegionHint(mHandle, transparent);
}
status_t SurfaceControl::setAlpha(float alpha) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setAlpha(mHandle, alpha);
}
status_t SurfaceControl::setMatrix(float dsdx, float dtdx, float dtdy, float dsdy) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setMatrix(mHandle, dsdx, dtdx, dtdy, dsdy);
}
status_t SurfaceControl::setCrop(const Rect& crop) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setCrop(mHandle, crop);
}
status_t SurfaceControl::setFinalCrop(const Rect& crop) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setFinalCrop(mHandle, crop);
}

status_t SurfaceControl::deferTransactionUntil(const sp<IBinder>& handle,
        uint64_t frameNumber) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->deferTransactionUntil(mHandle, handle, frameNumber);
}

status_t SurfaceControl::deferTransactionUntil(const sp<Surface>& handle,
        uint64_t frameNumber) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->deferTransactionUntil(mHandle, handle, frameNumber);
}

status_t SurfaceControl::reparentChildren(const sp<IBinder>& newParentHandle) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->reparentChildren(mHandle, newParentHandle);
}

status_t SurfaceControl::detachChildren() {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->detachChildren(mHandle);
}

status_t SurfaceControl::setOverrideScalingMode(int32_t overrideScalingMode) {
    status_t err = validate();
    if (err < 0) return err;
    return mClient->setOverrideScalingMode(mHandle, overrideScalingMode);
}

status_t SurfaceControl::clearLayerFrameStats() const {
    status_t err = validate();
    if (err < 0) return err;
    const sp<SurfaceComposerClient>& client(mClient);
    return client->clearLayerFrameStats(mHandle);
}

status_t SurfaceControl::getLayerFrameStats(FrameStats* outStats) const {
    status_t err = validate();
    if (err < 0) return err;
    const sp<SurfaceComposerClient>& client(mClient);
    return client->getLayerFrameStats(mHandle, outStats);
}

status_t SurfaceControl::validate() const
{
    if (mHandle==nullptr || mClient==nullptr) {
        ALOGE("invalid handle (%p) or client (%p)",
                mHandle.get(), mClient.get());
        return NO_INIT;
    }
    return NO_ERROR;
}

status_t SurfaceControl::writeSurfaceToParcel(
        const sp<SurfaceControl>& control, Parcel* parcel)
{
    sp<IGraphicBufferProducer> bp;
    if (control != nullptr) {
        bp = control->mGraphicBufferProducer;
    }
    return parcel->writeStrongBinder(IInterface::asBinder(bp));
}

sp<Surface> SurfaceControl::generateSurfaceLocked() const
{
    // This surface is always consumed by SurfaceFlinger, so the
    // producerControlledByApp value doesn't matter; using false.
    mSurfaceData = new Surface(mGraphicBufferProducer, false);

    return mSurfaceData;
}

sp<Surface> SurfaceControl::getSurface() const
{
    Mutex::Autolock _l(mLock);
    if (mSurfaceData == nullptr) {
        return generateSurfaceLocked();
    }
    return mSurfaceData;
}

sp<Surface> SurfaceControl::createSurface() const
{
    Mutex::Autolock _l(mLock);
    return generateSurfaceLocked();
}

sp<IBinder> SurfaceControl::getHandle() const
{
    Mutex::Autolock lock(mLock);
    return mHandle;
}

// ----------------------------------------------------------------------------
}; // namespace android
