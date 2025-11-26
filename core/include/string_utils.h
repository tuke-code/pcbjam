/*
 * string_utils.h - Minimal stub for kicad-core
 *
 * The sexpr library only uses From_UTF8() which is defined in wx_shim.h
 */

#ifndef KICAD_CORE_STRING_UTILS_H
#define KICAD_CORE_STRING_UTILS_H

#include "wx_shim.h"

// From_UTF8 is already defined in wx_shim.h

// Additional string utilities that might be needed
inline std::string To_UTF8(const wxString& str) {
    return str;
}

#endif // KICAD_CORE_STRING_UTILS_H
