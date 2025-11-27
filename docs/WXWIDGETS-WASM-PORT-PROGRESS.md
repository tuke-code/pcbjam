# wxWidgets 3.2.6 WASM Port Progress

## Build Command
```bash
cd /Users/V/IdeaProjects/kicad-wasm/build-wasm/wxwidgets-universal
emmake make -j1 2>&1 | tail -30
```

## Current Status: BUILD PASSING

Last verified: Build completes successfully with all libraries generated.

---

## COMPLETED CHANGES

### 1. build/wasm/ directory (DONE)
Copied from reference:
- `wxwidgets/build/wasm/common.mk`
- `wxwidgets/build/wasm/wxwasm.mk`
- `wxwidgets/build/wasm/wx.js` (JavaScript glue layer - critical!)
- `wxwidgets/build/wasm/template.html`
- `wxwidgets/build/wasm/httpd.py`

### 2. Locale warning fix (DONE)
**src/common/intl.cpp** - Added `#ifndef __WXWASM__` guard around locale warning at line ~394

**src/common/wxcrt.cpp** - Added `#ifdef __WXWASM__` to return NULL from wxSetlocale at line ~121

### 3. src/common/ modifications (DONE)
**include/wx/config.h** - Added WASM case to include wx/wasm/config.h and define wxConfig as wxLocalStorageConfig

**src/common/config.cpp** - Added WASM case to use wxLocalStorageConfig

**src/common/event.cpp** - Added `m_clickCount = event.m_clickCount;` in wxMouseEvent::Assign() at line ~615

**src/common/fontcmn.cpp** - Multiple changes:
- Added `#if !defined(__WXWASM__)` guard around FromString()/ToString() (lines 735-827)
- Added `m_isRendered = false;` in Init() for WASM (line 839-841)
- Added `#if !defined(__WXWASM__)` guard around setter functions (lines 884-927)

**include/wx/fontutil.h** - Added WASM-specific members at line ~222:
```cpp
#if defined(__WXWASM__)
    mutable bool m_isRendered;
    mutable wxString m_renderedString;
#endif
```

**src/common/combocmn.cpp** - Added WASM configuration block at line ~165:
```cpp
#elif defined(__WXWASM__)
#include "wx/dialog.h"
#define wxComboCtrlGenericTLW   wxDialog
#define USE_TRANSIENT_POPUP           1
#define TRANSIENT_POPUPWIN_IS_PERFECT 1
#define POPUPWIN_IS_PERFECT           1
#define TEXTCTRL_TEXT_CENTERED        0
#define FOCUS_RING                    0
```

### 4. src/univ/ Dialog async modal support (DONE)
**include/wx/univ/dialog.h** - Added:
- `#include <functional>` at top
- `virtual void ShowModal(std::function<void (int)> callback) wxOVERRIDE;` declaration
- `std::function<void (int)> m_modalCallback;` member

**include/wx/dialog.h** (base class) - Added:
- `#include <functional>` at top
- `virtual void ShowModal(std::function<void (int)> callback) = 0;` declaration
- Public `PopupMenu()` callback overloads

**include/wx/window.h** - Added:
- `#include <functional>`
- Public `PopupMenu()` callback overloads
- `virtual void DoPopupMenu(wxMenu *menu, int x, int y, std::function<void (bool)> callback) = 0;`

**src/univ/dialog.cpp** - Added:
- `#include <emscripten.h>`
- `m_modalCallback = NULL;` in Init()
- `EM_JS(int, startModal, ...)` JavaScript bridge function
- Modified ShowModal() to use async approach (returns wxID_CANCEL for now)
- Added `ShowModal(callback)` overload
- Modified EndModal() to call callback when present

### 5. src/univ/ Window modifications (DONE)
**include/wx/univ/window.h** - Added:
- `#include <functional>` at top
- WASM case for wxWindowNative (already present)
- `virtual void DoPopupMenu(wxMenu *menu, int x, int y, std::function<void (bool)> callback) wxOVERRIDE;`
- `std::function<void (int)> m_popupCallback;` member

**src/univ/winuniv.cpp** - Added:
- WASM case in `wxIMPLEMENT_DYNAMIC_CLASS`
- Commented out `EVT_KEY_DOWN(wxWindow::OnKeyDown)`
- `m_popupCallback = NULL;` in Init()
- SetScrollbar assertion already updated

### 6. src/univ/ Control rendering (DONE)
**src/univ/ctrlrend.cpp** - Changed GetLabel() to GetLabelText() in:
- DrawLabel()
- DrawButtonLabel()
- DrawFrame()

**src/univ/stdrend.cpp** - Changed wxBORDER_SIMPLE to use DrawStaticBorder()

### 7. src/univ/ Widget fixes (DONE)
**src/univ/anybutton.cpp** - Fixed:
- Moved Refresh() before Click() in Toggle()
- Changed GetLabel() to GetLabelText() in DoGetBestClientSize()

**src/univ/checkbox.cpp** - Changed GetLabel() to GetLabelText() in:
- DrawCheckButton() call
- GetMultiLineTextExtent() call

**src/univ/stattext.cpp** - Added:
- AutoResizeIfNecessary() call after WXSetVisibleLabel()
- Changed GetLabel() to GetLabelText() in WXGetVisibleLabel()

**src/univ/radiobut.cpp** - Added:
- Toggle() method
- PerformAction() override
- Changed GetLabel() to GetLabelText() in DoDraw()

**include/wx/univ/radiobut.h** - Added:
- Toggle() declaration
- PerformAction() override declaration

**src/univ/textctrl.cpp** - Added:
- SetBackgroundColour(*wxWHITE) in Create()
- DoGetSizeFromTextSize() method

**include/wx/univ/textctrl.h** - Changed:
- GetDefaultBorder() returns wxBORDER_STATIC instead of wxBORDER_SUNKEN
- Added DoGetSizeFromTextSize() declaration

### 8. GetScrollbarArrowSize signature + slider/spinbutt changes (DONE)
**include/wx/univ/renderer.h** - Changed:
- `GetScrollbarArrowSize()` to `GetScrollbarArrowSize(wxOrientation orientation)`
- `DrawSliderShaft` to add `double fracValue` parameter
- Added `GetOverflowHeight()` to wxMenuGeometryInfo
- Added `DrawMenuOverflowArrow()` method
- Updated wxDelegateRenderer wrappers

**src/univ/themes/gtk.cpp, mono.cpp, win32.cpp** - Updated:
- `GetScrollbarArrowSize(wxOrientation WXUNUSED(orientation))` signature
- `DrawSliderShaft` signature with `double WXUNUSED(fracValue)` parameter

**src/univ/scrolbar.cpp** - Added:
- `thumbSize = wxMax(wxMin(thumbSize, range), 0);` in SetScrollbar
- `GetScrollbarArrowSize()` helper method that calls renderer with orientation
- Changed all `m_renderer->GetScrollbarArrowSize()` to `GetScrollbarArrowSize()`
- Changed `size.x = SIZE` to `size.y = 15` for horizontal scrollbar

**include/wx/univ/scrolbar.h** - Added:
- `wxSize GetScrollbarArrowSize() const;` declaration

**src/univ/spinbutt.cpp** - Updated:
- `DoGetBestClientSize()` calls renderer with orientation
- `CalcArrowRects()` rewritten with hardcoded ARROW_WIDTH/HEIGHT

**src/univ/settingsuniv.cpp** - Updated:
- `GetMetric()` calls use orientation parameter

**src/univ/slider.cpp** - Updated:
- Added `IsInverted()` logic in CalcThumbRect
- Added fracValue calculation in DoDraw
- Updated PixelToThumbPos with IsInverted logic
- Simplified OnThumbDragStart/OnThumbDrag/OnThumbDragEnd

**include/wx/univ/slider.h** - Added:
- `bool IsInverted() const { return IsVert() != HasFlag(wxSL_INVERSE); }`

---

### 9. src/univ/menu.cpp (DONE)

Complete rewrite of popup menu handling for WASM:

**Timer-based submenu opening** - In browsers, no modal event loops. Timer allows diagonal mouse movement toward submenus without accidentally closing them.
- `m_subMenuTimer` - delays submenu opening (50ms)
- `m_subMenuPoint` - tracks mouse position when starting submenu tracking
- `IsPointTrackingToSubMenu()` - geometry check if mouse moving toward submenu
- `OnSubMenuTimer()` - timer callback

**Overflow handling** - Browser windows can be smaller than desktop. Menus need to scroll.
- `m_offsetY` - scroll offset
- `m_overflowTimer` - continuous scrolling when hovering arrows
- `HasOverflow()`, `HasOverflowArrowUp()`, `HasOverflowArrowDown()`
- `GetOverflowArrowUpRect()`, `GetOverflowArrowDownRect()`
- `OverflowArrowHitTest()`
- `GetMaxClientHeight()` - available screen height
- `SetOffsetY()` - set scroll and refresh
- `OnOverflowTimer()`, `OnMouseWheel()`

**Async popup menus** - DoPopupMenu skips blocking event loop for WASM
- Wrap blocking code in `#ifndef __WXWASM__`
- Add callback-based overload
- DismissPopupMenu calls callback

**Other changes:**
- Border from `wxBORDER_RAISED` to `wxBORDER_STATIC`
- ClickItem BEFORE DismissAndNotify (was after)
- GetRootWindow uses GetWindow() instead of GetInvokingWindow()
- Detach adds GetParent()->RemoveChild(this)
- OnLeftDown uses IsShowingMenu()/DismissMenu() instead of HasCapture()/OnDismiss()

---

### 10. src/generic/ modifications (DONE - partial)

**src/generic/spinctlg.cpp** - MARGIN=0, null check for m_spinButton in DoMoveWindow
**src/generic/msgdlgg.cpp** - Async ShowModal overload added
**include/wx/generic/msgdlgg.h** - ShowModal callback declaration added
**src/generic/renderg.cpp** - Visual tweaks (3DLIGHT color, transparent pen, highlight color)
**src/generic/treectlg.cpp** - Smaller indent/spacing (10 instead of 15/18), transparent pen
**src/generic/vlbox.cpp** - SetBackgroundColour(*wxWHITE)

Not yet applied (may not be needed for wxWidgets 3.2.6):
- **src/generic/caret.cpp** - Different API in 3.2.6
- **src/generic/grid.cpp** - Visual tweaks (low priority)
- **src/generic/gridctrl.cpp** - 4 lines (low priority)
- **src/generic/filedlgg.cpp** - 2 lines
- **src/generic/listctrl.cpp** - 2 lines
- **src/generic/stattextg.cpp** - 2 lines

---

## REMAINING CHANGES (NOT YET APPLIED)

### Priority 1: include/wx/ header modifications (Low priority - complex)

**include/wx/platinfo.h** - 77 lines (browser detection, wxBrowserInfo class, wxPORT_WASM)
**src/common/platinfo.cpp** - 6 lines (browser info init)
**src/common/utilscmn.cpp** - 57 lines (async wxMessageBox, wxPORT_WASM check)
**src/common/wincmn.cpp** - 19 lines (async popup menu)

Many other headers need `#elif defined(__WXWASM__)` or `#ifdef __WXWASM__` additions.

### Priority 4: Build system files

**Makefile.in** - 897 lines of changes
**autoconf_inc.m4** - 13 lines
**build/bakefiles/files.bkl** - 74 lines
**build/bakefiles/wx.bkl** - 2 lines
**build/cmake/files.cmake** - 71 lines
**build/cmake/setup.cmake** - 2 lines
**build/cmake/toolkit.cmake** - 6 lines
**build/files** - 71 lines

---

## HOW TO CHECK REFERENCE CHANGES

To see what a file changed in the reference:
```bash
cd /Users/V/IdeaProjects/kicad-wasm/wxWidgets-wasm-reference
git show d262364a0a -- path/to/file
```

The last 10 commits in reference (oldest to newest):
1. d262364a0a - Initial commit of wasm sources (main changes)
2. 595b16b855 - Add wasm files (theme, demo makefiles)
3. 0dbfab6b4c - Update README.md
4. 31a467a173 - Update README.md
5. 3c0f3b1954 - Update link to wavacity
6. 57ea7c9a04 - Fix crash if mouse window reset in event handler
7. ae94f56bd3 - Size top window before run
8. 255970e5e2 - Support font size in pixels
9. b44707a19a - Translate touch events to mouse events
10. 293bd9feba - Suppress locale warnings

Bug fixes 6-10 are already applied in wxwidgets/src/wasm/ files.

---

## KEY INSIGHT: Why config.sub in Submodules

wxWidgets 3.2.6 uses git submodules for bundled libraries (pcre, expat, jpeg, png, tiff). Each has its own config.sub that must recognize wasm32. The reference (older ~3.0.x) had libraries directly in-tree without submodules.

Current submodule config.sub files are already updated (showing `m` modified status in git).

---

## SKIPPED CHANGES (Low Priority or Complex)

- **src/common/appcmn.cpp** - Just debug printf (not needed)
- **src/common/init.cpp** - Just debug printf (not needed)
- **src/common/dcbufcmn.cpp** - Different API in 3.2.6 (already correct)
- **Browser info in platinfo.h/cpp** - Complex, adds wxBrowserInfo class (not critical)
- **Async wxMessageBox** - Changes function signature (complex)

---

## NEXT STEPS

1. **Complete menu.cpp changes** - Follow the 20-step list above
2. Apply src/generic/ changes
3. Apply remaining include/wx/ header changes
4. Test with a minimal WASM app

---

## FILES IN wxwidgets/ FOLDER

Key WASM-specific directories already present:
- `wxwidgets/src/wasm/` - 29 source files
- `wxwidgets/include/wx/wasm/` - 31 header files
- `wxwidgets/src/univ/themes/wasm.cpp` - WASM theme (98KB)
- `wxwidgets/build/wasm/` - Build support files (copied from reference)
