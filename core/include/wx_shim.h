/*
 * wx_shim.h - Minimal wxWidgets compatibility layer for kicad-core
 *
 * Provides drop-in replacements for wx utility macros/functions used in
 * KiCad's core computation libraries. This is NOT a port of wxWidgets -
 * just ~100 lines of standard C++ that replaces debug/logging utilities.
 *
 * Used macros in kimath:
 *   - wxASSERT, wxASSERT_MSG
 *   - wxCHECK, wxCHECK_MSG, wxCHECK2_MSG, wxCHECK_RET
 *   - wxFAIL_MSG
 *   - wxLogTrace, wxLogDebug, wxLogWarning
 *   - wxString, wxString::Format
 *   - wxT()
 */

#ifndef KICAD_WX_SHIM_H
#define KICAD_WX_SHIM_H

#include <cassert>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <sstream>

// =============================================================================
// Assertions
// =============================================================================

#define wxASSERT(cond) assert(cond)
#define wxASSERT_MSG(cond, msg) assert(cond)  // msg ignored in release

// wxCHECK variants - check condition, return if false
#define wxCHECK(cond, ret) do { if(!(cond)) return ret; } while(0)
#define wxCHECK_MSG(cond, ret, msg) do { if(!(cond)) return ret; } while(0)
#define wxCHECK_RET(cond, msg) do { if(!(cond)) return; } while(0)
#define wxCHECK2(cond, op) do { if(!(cond)) { op; } } while(0)
#define wxCHECK2_MSG(cond, op, msg) do { if(!(cond)) { op; } } while(0)

#define wxFAIL_MSG(msg) assert(false)  // msg ignored in standalone build

// =============================================================================
// Logging - mostly no-ops for core library
// =============================================================================

// Trace logging - typically disabled in release builds anyway
#define wxLogTrace(...) ((void)0)
#define wxLogDebug(...) ((void)0)

// Warnings - optionally print to stderr
#ifdef KICAD_CORE_VERBOSE
    #define wxLogWarning(fmt, ...) fprintf(stderr, "Warning: " fmt "\n", ##__VA_ARGS__)
#else
    #define wxLogWarning(...) ((void)0)
#endif

// Variable argument version
inline void wxVLogWarning(const char* format, va_list args) {
#ifdef KICAD_CORE_VERBOSE
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#else
    (void)format;
    (void)args;
#endif
}

// Log level checking - always returns false (logging disabled)
namespace wxLog {
    inline bool IsLevelEnabled(int, const std::string&) { return false; }
    inline void EnableLogging(bool enable = true) { (void)enable; }
    inline bool IsLoggingEnabled() { return false; }
}

// Log level constants
constexpr int wxLOG_Debug = 0;

// Log component macro
#ifndef wxLOG_COMPONENT
    #define wxLOG_COMPONENT "kicad-core"
#endif

// =============================================================================
// String utilities
// =============================================================================

// wxChar type
using wxChar = char;

// wxT() macro - pass through (we're always using UTF-8)
#define wxT(x) x

// wxASCII_STR for older wx compatibility
#define wxASCII_STR(s) std::string(s)

// Helper to convert args for snprintf - strings need c_str()
template<typename T>
struct FormatArg {
    static auto convert(const T& arg) { return arg; }
};

template<>
struct FormatArg<std::string> {
    static const char* convert(const std::string& arg) { return arg.c_str(); }
};

// Forward declaration for wxString specialization
class wxString;

template<>
struct FormatArg<wxString> {
    static const char* convert(const wxString& arg);
};

// wxString class with static Format method
class wxString : public std::string {
public:
    // Inherit constructors
    using std::string::string;

    // Additional constructors for compatibility
    wxString() : std::string() {}
    wxString(const std::string& s) : std::string(s) {}
    wxString(const char* s) : std::string(s ? s : "") {}

    // Static Format method - converts string args to c_str()
    template<typename... Args>
    static wxString Format(const char* fmt, Args... args) {
        char buf[2048];
        snprintf(buf, sizeof(buf), fmt, FormatArg<Args>::convert(args)...);
        return wxString(buf);
    }

    static wxString Format(const wxString& fmt) {
        return fmt;
    }

    // FromAscii static method
    static wxString FromAscii(const char* s) {
        return wxString(s ? s : "");
    }

    // c_str() for compatibility (inherited from std::string but explicit)
    const char* c_str() const { return std::string::c_str(); }

    // RemoveLast - removes last n characters
    void RemoveLast(size_t n = 1) {
        if (n <= size()) {
            resize(size() - n);
        } else {
            clear();
        }
    }

    // Truncate - truncate to given length
    void Truncate(size_t len) {
        if (len < size()) {
            resize(len);
        }
    }
};

// Define the wxString FormatArg specialization now that wxString is defined
inline const char* FormatArg<wxString>::convert(const wxString& arg) {
    return arg.c_str();
}

// Empty string constant
inline const wxString wxEmptyString = wxString("");

// =============================================================================
// File I/O utilities
// =============================================================================

#include <fstream>

// wxFFile - File wrapper class
class wxFFile {
    FILE* m_fp = nullptr;
    bool m_close = false;

public:
    wxFFile() = default;
    wxFFile(const wxString& filename, const char* mode = "r") {
        Open(filename, mode);
    }
    ~wxFFile() { Close(); }

    bool Open(const wxString& filename, const char* mode = "r") {
        Close();
        m_fp = fopen(filename.c_str(), mode);
        m_close = (m_fp != nullptr);
        return m_fp != nullptr;
    }

    bool IsOpened() const { return m_fp != nullptr; }

    void Close() {
        if (m_fp && m_close) {
            fclose(m_fp);
        }
        m_fp = nullptr;
        m_close = false;
    }

    size_t Read(void* buffer, size_t count) {
        if (!m_fp) return 0;
        return fread(buffer, 1, count, m_fp);
    }

    size_t Write(const void* buffer, size_t count) {
        if (!m_fp) return 0;
        return fwrite(buffer, 1, count, m_fp);
    }

    bool ReadAll(wxString* str) {
        if (!m_fp || !str) return false;
        // Get file size
        long pos = ftell(m_fp);
        fseek(m_fp, 0, SEEK_END);
        long size = ftell(m_fp);
        fseek(m_fp, pos, SEEK_SET);
        // Read content
        str->resize(size);
        size_t read = fread(&(*str)[0], 1, size, m_fp);
        str->resize(read);
        return read > 0;
    }

    size_t Length() const {
        if (!m_fp) return 0;
        long pos = ftell(m_fp);
        fseek(m_fp, 0, SEEK_END);
        long size = ftell(m_fp);
        fseek(m_fp, pos, SEEK_SET);
        return static_cast<size_t>(size);
    }

    bool Eof() const {
        return m_fp ? feof(m_fp) != 0 : true;
    }

    bool Seek(long offset, int origin = SEEK_SET) {
        return m_fp ? fseek(m_fp, offset, origin) == 0 : false;
    }

    long Tell() const {
        return m_fp ? ftell(m_fp) : -1;
    }
};

// From_UTF8 - passthrough for UTF-8 strings
inline wxString From_UTF8(const char* s) {
    return wxString(s ? s : "");
}

// =============================================================================
// Replacement for wx headers
// =============================================================================

// Empty stubs for wx includes - these get included but do nothing
// Create empty headers in core/include/wx/

#endif // KICAD_WX_SHIM_H
