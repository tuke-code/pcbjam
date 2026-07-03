/**
 * Include shim for the native 3D-renderer test build.
 * kiglad.h must precede wx/glcanvas.h so GL symbols come from the vendored
 * glad loader instead of the (deprecated) Apple GL headers.
 */

#ifndef KICAD_STUBS_3D_H
#define KICAD_STUBS_3D_H

#include <kicad_gl/kiglad.h>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#endif // KICAD_STUBS_3D_H
