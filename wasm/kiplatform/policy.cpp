/*
 * WASM implementation of kiplatform/policy.h
 * Policies are not configured in browser environment
 */

#include <kiplatform/policy.h>
#include <wx/string.h>

namespace KIPLATFORM
{
namespace POLICY
{

PBOOL GetPolicyBool( const wxString& aKey )
{
    // No enterprise policies in browser environment
    return PBOOL::NOT_CONFIGURED;
}

std::uint32_t GetPolicyEnumUInt( const wxString& aKey )
{
    // No enterprise policies in browser environment
    return 0;
}

} // namespace POLICY
} // namespace KIPLATFORM
