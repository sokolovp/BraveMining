// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/x11_cursor_factory_ozone.h"

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursors_aura.h"
#include "ui/gfx/geometry/point.h"

namespace ui {

namespace {

X11CursorOzone* ToX11CursorOzone(PlatformCursor cursor) {
  return static_cast<X11CursorOzone*>(cursor);
}

PlatformCursor ToPlatformCursor(X11CursorOzone* cursor) {
  return static_cast<PlatformCursor>(cursor);
}

// Gets default aura cursor bitmap/hotspot and creates a X11CursorOzone with it.
scoped_refptr<X11CursorOzone> CreateAuraX11Cursor(CursorType type) {
  SkBitmap bitmap;
  gfx::Point hotspot;
  if (GetCursorBitmap(type, &bitmap, &hotspot)) {
    return new X11CursorOzone(bitmap, hotspot);
  }
  return nullptr;
}

}  // namespace

X11CursorFactoryOzone::X11CursorFactoryOzone()
    : invisible_cursor_(X11CursorOzone::CreateInvisible()) {}

X11CursorFactoryOzone::~X11CursorFactoryOzone() {}

PlatformCursor X11CursorFactoryOzone::GetDefaultCursor(CursorType type) {
  return ToPlatformCursor(GetDefaultCursorInternal(type).get());
}

PlatformCursor X11CursorFactoryOzone::CreateImageCursor(
    const SkBitmap& bitmap,
    const gfx::Point& hotspot,
    float bitmap_dpi) {
  // There is a problem with custom cursors that have no custom data. The
  // resulting SkBitmap is empty and X crashes when creating a zero size cursor
  // image. Return invisible cursor here instead.
  if (bitmap.drawsNothing()) {
    // The result of |invisible_cursor_| is owned by the caller, and will be
    // Unref()ed by code far away. (Usually in web_cursor.cc in content, among
    // others.) If we don't manually add another reference before we cast this
    // to a void*, we can end up with |invisible_cursor_| being freed out from
    // under us.
    invisible_cursor_->AddRef();
    return ToPlatformCursor(invisible_cursor_.get());
  }

  X11CursorOzone* cursor = new X11CursorOzone(bitmap, hotspot);
  cursor->AddRef();
  return ToPlatformCursor(cursor);
}

PlatformCursor X11CursorFactoryOzone::CreateAnimatedCursor(
    const std::vector<SkBitmap>& bitmaps,
    const gfx::Point& hotspot,
    int frame_delay_ms,
    float bitmap_dpi) {
  X11CursorOzone* cursor = new X11CursorOzone(bitmaps, hotspot, frame_delay_ms);
  cursor->AddRef();
  return ToPlatformCursor(cursor);
}

void X11CursorFactoryOzone::RefImageCursor(PlatformCursor cursor) {
  ToX11CursorOzone(cursor)->AddRef();
}

void X11CursorFactoryOzone::UnrefImageCursor(PlatformCursor cursor) {
  ToX11CursorOzone(cursor)->Release();
}

scoped_refptr<X11CursorOzone> X11CursorFactoryOzone::GetDefaultCursorInternal(
    CursorType type) {
  if (type == CursorType::kNone)
    return invisible_cursor_;

  // TODO(kylechar): Use predefined X cursors here instead.
  if (!default_cursors_.count(type)) {
    // Loads the default aura cursor bitmap for cursor type. Falls back on
    // pointer cursor then invisible cursor if this fails.
    scoped_refptr<X11CursorOzone> cursor = CreateAuraX11Cursor(type);
    if (!cursor.get()) {
      if (type != CursorType::kPointer) {
        cursor = GetDefaultCursorInternal(CursorType::kPointer);
      } else {
        NOTREACHED() << "Failed to load default cursor bitmap";
      }
    }
    default_cursors_[type] = cursor;
  }

  // Returns owned default cursor for this type.
  return default_cursors_[type];
}

}  // namespace ui
