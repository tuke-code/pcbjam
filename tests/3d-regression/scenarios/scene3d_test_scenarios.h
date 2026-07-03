/**
 * Scenario registry for the 3D-renderer regression suite.
 *
 * Scenario names are the single source of truth: they become the committed
 * baseline filenames (3d-<name>.png), the entries of manifest.json and later
 * the WebGL-port test IDs. Names are append-only — never renumber or rename.
 */

#ifndef SCENE3D_TEST_SCENARIOS_H
#define SCENE3D_TEST_SCENARIOS_H

struct SCENE3D_CTX;

namespace Scene3DTest
{

int         GetScenarioCount();
const char* GetScenarioName( int aIndex );

/// Render scenario aIndex. The scenario body calls aCtx.BeginFrame(...) itself
/// (some scenarios use custom background colors) and then real KiCad 3D code.
void RenderScenario( SCENE3D_CTX& aCtx, int aIndex );

} // namespace Scene3DTest

#endif // SCENE3D_TEST_SCENARIOS_H
