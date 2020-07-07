// LAF OS Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SYSTEM_H_INCLUDED
#define OS_SYSTEM_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "os/app_mode.h"
#include "os/capabilities.h"
#include "os/color_space.h"
#include "os/keys.h"
#include "os/ref.h"

#include <memory>
#include <stdexcept>

namespace os {

  class ColorSpaceConversion;
  class Display;
  class EventQueue;
  class Font;
  class FontManager;
  class Logger;
  class Menus;
  class NativeDialogs;
  class Surface;
  class System;

  using SystemRef = Ref<System>;

  // TODO why we just don't return nullptr if the display creation fails?
  //      maybe an error handler function?
  class DisplayCreationException : public std::runtime_error {
  public:
    DisplayCreationException(const char* msg) throw()
      : std::runtime_error(msg) { }
  };

  // API to use to get tablet input information.
  enum class TabletAPI {
    // Default tablet API to use in the system (Windows Ink on
    // Windows; only valid value on other systems).
    Default = 0,

    // Use Windows 8/10 pointer messages (Windows Ink).
    WindowsPointerInput = 0,

    // Use the Wintab API to get pressure information from packets but
    // mouse movement from Windows system messages
    // (WM_MOUSEMOVE).
    Wintab = 1,

    // Use the Wintab API processing packets directly (pressure and
    // stylus movement information). With this we might get more
    // precision from the device (but still work-in-progress, some
    // messages might be mixed up).
    WintabPackets = 2,
  };

  class System : public RefCount {
  protected:
    virtual ~System() { }
  public:

    // Windows-specific: The app name at the moment is used to receive
    // DDE messages (WM_DDE_INITIATE) and convert WM_DDE_EXECUTE
    // messages into Event::DropFiles. This allows to the user
    // double-click files in the File Explorer and open the file in a
    // running instance of your app.
    //
    // To receive DDE messages you have to configure the registry in
    // this way (HKCR=HKEY_CLASSES_ROOT):
    //
    //   HKCR\.appfile  (Default)="AppFile"
    //   HKCR\AppFile   (Default)="App File"
    //   HKCR\AppFile\shell\open\command             (Default)="C:\\...\\AppName.EXE"
    //   HKCR\AppFile\shell\open\ddeexec             (Default)="[open(\"%1\")]"
    //   HKCR\AppFile\shell\open\ddeexec\application (Default)="AppName"
    //   HKCR\AppFile\shell\open\ddeexec\topic       (Default)="system"
    //
    // The default value of "HKCR\AppFile\shell\open\ddeexec\application"
    // must match the "appName" given in this function.
    virtual void setAppName(const std::string& appName) = 0;

    // We can use this function to create an application that can run
    // in CLI and GUI mode depending on the given arguments, and in
    // this way avoid to showing the app in the macOS dock bar if we
    // are running in CLI only.
    virtual void setAppMode(AppMode appMode) = 0;

    // We might need to call this function when the app is launched
    // from Steam. It appears that there is a bug on OS X Steam client
    // where the app is launched, activated, and then the Steam client
    // is activated again.
    virtual void activateApp() = 0;

    virtual void finishLaunching() = 0;

    virtual Capabilities capabilities() const = 0;
    bool hasCapability(Capabilities c) const {
      return (int(capabilities()) & int(c)) == int(c);
    }

    // Sets the specific API to use to process tablet/stylus/pen
    // messages.
    //
    // It can be used to avoid loading wintab32.dll too (sometimes a
    // program can be locked when we load the wintab32.dll, so we need
    // a way to opt-out loading this library.)
    virtual void setTabletAPI(TabletAPI api) = 0;
    virtual TabletAPI tabletAPI() const = 0;

    // Sub-interfaces
    virtual Logger* logger() = 0;
    virtual Menus* menus() = 0;
    virtual NativeDialogs* nativeDialogs() = 0;
    virtual EventQueue* eventQueue() = 0;

    virtual bool gpuAcceleration() const = 0;
    virtual void setGpuAcceleration(bool state) = 0;
    virtual gfx::Size defaultNewDisplaySize() = 0;
    virtual Display* defaultDisplay() = 0;
    virtual Ref<Display> makeDisplay(int width, int height, int scale) = 0;
    virtual Ref<Surface> makeSurface(int width, int height, const os::ColorSpaceRef& colorSpace = nullptr) = 0;
    virtual Ref<Surface> makeRgbaSurface(int width, int height, const os::ColorSpaceRef& colorSpace = nullptr) = 0;
    virtual Ref<Surface> loadSurface(const char* filename) = 0;
    virtual Ref<Surface> loadRgbaSurface(const char* filename) = 0;

    // New font manager
    virtual FontManager* fontManager() = 0;

    // Old font functions (to be removed)
    virtual Ref<Font> loadSpriteSheetFont(const char* filename, int scale = 1) = 0;
    virtual Ref<Font> loadTrueTypeFont(const char* filename, int height) = 0;

    // Returns true if the the given scancode key is pressed/actived.
    virtual bool isKeyPressed(KeyScancode scancode) = 0;

    // Returns the active pressed modifiers.
    virtual KeyModifiers keyModifiers() = 0;

    // Returns the latest unicode character that activated the given
    // scancode.
    virtual int getUnicodeFromScancode(KeyScancode scancode) = 0;

    // Indicates if you want to use dead keys or not. By default it's
    // false, which behaves as regular shortcuts. You should set this
    // to true when you're inside a text field in your app.
    virtual void setTranslateDeadKeys(bool state) = 0;

    // Color management
    virtual void listColorSpaces(
      std::vector<os::ColorSpaceRef>& list) = 0;
    virtual os::ColorSpaceRef makeColorSpace(
      const gfx::ColorSpaceRef& colorSpace) = 0;
    virtual Ref<ColorSpaceConversion> convertBetweenColorSpace(
      const os::ColorSpaceRef& src,
      const os::ColorSpaceRef& dst) = 0;

    // Set a default color profile for all displays (nullptr to use
    // the active monitor color profile and change it dynamically when
    // the window changes to another monitor).
    virtual void setDisplaysColorSpace(const os::ColorSpaceRef& cs) = 0;
    virtual os::ColorSpaceRef displaysColorSpace() = 0;
  };

  SystemRef make_system();
  System* instance();
  void set_instance(System* system);

} // namespace os

#endif
