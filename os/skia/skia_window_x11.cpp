// LAF OS Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/skia/skia_window_x11.h"

#include "gfx/size.h"
#include "os/common/event_queue_with_resize_display.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/skia/skia_display.h"
#include "os/skia/skia_surface.h"
#include "os/x11/x11.h"

#include "SkBitmap.h"

namespace os {

namespace {

bool convert_skia_bitmap_to_ximage(const SkBitmap& bitmap, XImage& image)
{
  memset(&image, 0, sizeof(image));
  int bpp = 8*bitmap.bytesPerPixel();
  image.width = bitmap.width();
  image.height = bitmap.height();
  image.format = ZPixmap;
  image.data = (char*)bitmap.getPixels();
  image.byte_order = LSBFirst;
  image.bitmap_unit = bpp;
  image.bitmap_bit_order = LSBFirst;
  image.bitmap_pad = bpp;
  image.depth = 24;
  image.bytes_per_line = bitmap.rowBytes() - 4*bitmap.width();
  image.bits_per_pixel = bpp;

  return (XInitImage(&image) ? true: false);
}

} // anonymous namespace

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display,
                       int width, int height, int scale)
  : X11Window(X11::instance()->display(), width, height, scale)
  , m_queue(queue)
  , m_display(display)
{
}

SkiaWindow::~SkiaWindow()
{
}

void SkiaWindow::setVisible(bool visible)
{
  // TODO
}

void SkiaWindow::maximize()
{
  // TODO
}

bool SkiaWindow::isMaximized() const
{
  return false;
}

bool SkiaWindow::isMinimized() const
{
  return false;
}

void SkiaWindow::onQueueEvent(Event& ev)
{
  ev.setDisplay(m_display);
  m_queue->queueEvent(ev);
}

void SkiaWindow::onPaint(const gfx::Rect& rc)
{
  auto surface = static_cast<SkiaSurface*>(m_display->surface());
  const SkBitmap& bitmap = surface->bitmap();

  int scale = this->scale();
  if (scale == 1) {
    XImage image;
    if (convert_skia_bitmap_to_ximage(bitmap, image)) {
      XPutImage(
        x11display(), handle(), gc(), &image,
        rc.x, rc.y,
        rc.x, rc.y,
        rc.w, rc.h);
    }
  }
  else {
    SkBitmap scaled;
    const SkImageInfo info =
      SkImageInfo::Make(rc.w, rc.h,
                        bitmap.info().colorType(),
                        bitmap.info().alphaType());

    // Increase m_buffer for "scaled" pixels if needed.
    const size_t rowBytes = info.minRowBytes();
    const size_t requiredSize = info.computeByteSize(rowBytes);
    if (requiredSize > m_buffer.size())
      m_buffer.resize(requiredSize);

    if (scaled.installPixels(info, (void*)&m_buffer[0], rowBytes)) {
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);

      SkCanvas canvas(scaled);
      SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(rc.x/scale, rc.y/scale, rc.w/scale, rc.h/scale));
      SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, rc.w, rc.h));
      canvas.drawBitmapRect(bitmap, srcRect, dstRect, &paint,
                            SkCanvas::kStrict_SrcRectConstraint);

      XImage image;
      if (convert_skia_bitmap_to_ximage(scaled, image)) {
        XPutImage(
          x11display(), handle(), gc(), &image,
          0, 0,
          rc.x, rc.y,
          rc.w, rc.h);
      }
    }
  }
}

void SkiaWindow::onResize(const gfx::Size& sz)
{
  // Set the ResizeDisplay event that will be sent in the near time
  // (150ms) by the EventQueueWithResizeDisplay.
  Event ev;
  ev.setType(Event::ResizeDisplay);
  ev.setDisplay(m_display);
  const bool isNewEvent =
    static_cast<EventQueueWithResizeDisplay*>(EventQueue::instance())
      ->setResizeDisplayEvent(ev);

  if (isNewEvent) m_resizeSurface.make(m_display);
  m_display->resize(sz);
  if (!isNewEvent) m_resizeSurface.draw(m_display);
}

} // namespace os
