// Stub wx/confbase.h - minimal config base for kicad-core
#ifndef _WX_CONFBASE_H_
#define _WX_CONFBASE_H_

#include "wx_shim.h"

// Minimal wxConfigBase stub
class wxConfigBase {
public:
    virtual ~wxConfigBase() = default;

    // Minimal interface - returns false/empty for everything
    virtual bool Read(const wxString& key, wxString* str) const { return false; }
    virtual bool Read(const wxString& key, long* val) const { return false; }
    virtual bool Read(const wxString& key, double* val) const { return false; }
    virtual bool Read(const wxString& key, bool* val) const { return false; }

    virtual bool Write(const wxString& key, const wxString& value) { return false; }
    virtual bool Write(const wxString& key, long value) { return false; }
    virtual bool Write(const wxString& key, double value) { return false; }
    virtual bool Write(const wxString& key, bool value) { return false; }

    static wxConfigBase* Get(bool createOnDemand = true) { return nullptr; }
    static wxConfigBase* Set(wxConfigBase* config) { return nullptr; }
};

#endif // _WX_CONFBASE_H_
