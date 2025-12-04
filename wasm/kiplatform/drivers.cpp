/*
 * WASM implementation of kiplatform/drivers.h
 * 3D mouse drivers are not available in browser
 */

#include <kiplatform/drivers.h>

namespace KIPLATFORM
{
namespace DRIVERS
{

bool Valid3DConnexionDriverVersion()
{
    // No 3D mouse support in browser
    return false;
}

} // namespace DRIVERS
} // namespace KIPLATFORM
