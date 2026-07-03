/**
 * Access to RENDER_3D_OPENGL's private geometry generators / material setters
 * for the Tier-2 scenarios, using the same member-pointer technique as
 * tests/gal-regression/native/gal_test_accessor.cpp (no KiCad header edits).
 * Overloads are disambiguated by the tag's member-function-pointer typedef.
 */

#ifndef RENDER3D_TEST_ACCESSOR_H
#define RENDER3D_TEST_ACCESSOR_H

#include <plugins/3dapi/xv3d_types.h>

#include <3d_enums.h>
#include <geometry/eda_angle.h>
#include <layer_ids.h>
#include <padstack.h> // PAD_DRILL_POST_MACHINING_MODE

#include <vector>

class RENDER_3D_OPENGL;
class TRIANGLE_DISPLAY_LIST;
class OPENGL_RENDER_LIST;
class FILLED_CIRCLE_2D;
class RING_2D;
class POLYGON_4PT_2D;
class TRIANGLE_2D;
class ROUND_SEGMENT_2D;
class SHAPE_POLY_SET;
class BVH_CONTAINER_2D;

bool R3D_InitializeOpenGL( RENDER_3D_OPENGL& aRenderer );

void R3D_GenerateCylinder( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter,
                           float aInnerRadius, float aOuterRadius, float aZtop, float aZbot,
                           unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst );

void R3D_GenerateInvCone( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter,
                          float aInnerRadius, float aOuterRadius, float aZtop, float aZbot,
                          unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst, EDA_ANGLE aAngle );

void R3D_GenerateDisk( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aRadius,
                       float aZ, unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst, bool aTop );

void R3D_GenerateDimple( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aRadius,
                         float aZ, float aDepth, unsigned int aNrSides,
                         TRIANGLE_DISPLAY_LIST* aDst, bool aTop );

void R3D_GenerateRing( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aInnerRadius,
                       float aOuterRadius, unsigned int aNrSides,
                       std::vector<SFVEC2F>& aInnerContour, std::vector<SFVEC2F>& aOuterContour,
                       bool aInvertOrder );

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const FILLED_CIRCLE_2D* aCircle,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot );
void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const RING_2D* aRing,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot );
void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const POLYGON_4PT_2D* aPoly,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot );
void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const TRIANGLE_2D* aTri,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot );
void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const ROUND_SEGMENT_2D* aSeg,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot );

bool R3D_AppendPostMachining( RENDER_3D_OPENGL& aRenderer, TRIANGLE_DISPLAY_LIST* aDst,
                              const SFVEC2F& aHoleCenter, PAD_DRILL_POST_MACHINING_MODE aMode,
                              int aSizeIU, int aDepthIU, float aHoleInnerRadius, float aZSurface,
                              bool aIsFront, float aPlatingThickness3d, float aUnitScale,
                              float* aZEnd );

OPENGL_RENDER_LIST* R3D_CreateBoard( RENDER_3D_OPENGL& aRenderer,
                                     const SHAPE_POLY_SET& aBoardPoly,
                                     const BVH_CONTAINER_2D* aThroughHoles = nullptr );

void R3D_Generate3dGrid( RENDER_3D_OPENGL& aRenderer, GRID3D_TYPE aGridType );

/// The compiled grid display-list id (private m_grid).
unsigned int R3D_GetGridList( RENDER_3D_OPENGL& aRenderer );

void R3D_SetupMaterials( RENDER_3D_OPENGL& aRenderer );
void R3D_SetLayerMaterial( RENDER_3D_OPENGL& aRenderer, PCB_LAYER_ID aLayerID );
void R3D_SetArrowMaterial( RENDER_3D_OPENGL& aRenderer );

#endif // RENDER3D_TEST_ACCESSOR_H
