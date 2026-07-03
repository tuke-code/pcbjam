#include "kicad_stubs_3d.h"

#include "render3d_test_accessor.h"

#include "3d_rendering/opengl/render_3d_opengl.h"

// Member-pointer private-access technique, same as gal_test_accessor.cpp
// (https://bloglitb.blogspot.com/2010/07/access-to-private-members-thats-easy.html).

template <typename Tag>
struct result
{
    typedef typename Tag::type type;
    static type ptr;
};

template <typename Tag>
typename result<Tag>::type result<Tag>::ptr;

template <typename Tag, typename Tag::type p>
struct rob : result<Tag>
{
    struct filler
    {
        filler() { result<Tag>::ptr = p; }
    };
    static filler filler_obj;
};

template <typename Tag, typename Tag::type p>
typename rob<Tag, p>::filler rob<Tag, p>::filler_obj;

// Tags — the typedef's signature picks the right overload of the member.
struct R3D_initializeOpenGL
{
    typedef bool ( RENDER_3D_OPENGL::*type )();
};
template struct rob<R3D_initializeOpenGL, &RENDER_3D_OPENGL::initializeOpenGL>;

struct R3D_generateCylinder
{
    typedef void ( RENDER_3D_OPENGL::*type )( const SFVEC2F&, float, float, float, float,
                                              unsigned int, TRIANGLE_DISPLAY_LIST* );
};
template struct rob<R3D_generateCylinder, &RENDER_3D_OPENGL::generateCylinder>;

struct R3D_generateInvCone
{
    typedef void ( RENDER_3D_OPENGL::*type )( const SFVEC2F&, float, float, float, float,
                                              unsigned int, TRIANGLE_DISPLAY_LIST*, EDA_ANGLE );
};
template struct rob<R3D_generateInvCone, &RENDER_3D_OPENGL::generateInvCone>;

struct R3D_generateDisk
{
    typedef void ( RENDER_3D_OPENGL::*type )( const SFVEC2F&, float, float, unsigned int,
                                              TRIANGLE_DISPLAY_LIST*, bool );
};
template struct rob<R3D_generateDisk, &RENDER_3D_OPENGL::generateDisk>;

struct R3D_generateDimple
{
    typedef void ( RENDER_3D_OPENGL::*type )( const SFVEC2F&, float, float, float, unsigned int,
                                              TRIANGLE_DISPLAY_LIST*, bool );
};
template struct rob<R3D_generateDimple, &RENDER_3D_OPENGL::generateDimple>;

struct R3D_generateRing
{
    typedef void ( RENDER_3D_OPENGL::*type )( const SFVEC2F&, float, float, unsigned int,
                                              std::vector<SFVEC2F>&, std::vector<SFVEC2F>&,
                                              bool );
};
template struct rob<R3D_generateRing, &RENDER_3D_OPENGL::generateRing>;

struct R3D_addObj_Circle
{
    typedef void ( RENDER_3D_OPENGL::*type )( const FILLED_CIRCLE_2D*, TRIANGLE_DISPLAY_LIST*,
                                              float, float );
};
template struct rob<R3D_addObj_Circle, &RENDER_3D_OPENGL::addObjectTriangles>;

struct R3D_addObj_Ring
{
    typedef void ( RENDER_3D_OPENGL::*type )( const RING_2D*, TRIANGLE_DISPLAY_LIST*, float,
                                              float );
};
template struct rob<R3D_addObj_Ring, &RENDER_3D_OPENGL::addObjectTriangles>;

struct R3D_addObj_Poly4
{
    typedef void ( RENDER_3D_OPENGL::*type )( const POLYGON_4PT_2D*, TRIANGLE_DISPLAY_LIST*,
                                              float, float );
};
template struct rob<R3D_addObj_Poly4, &RENDER_3D_OPENGL::addObjectTriangles>;

struct R3D_addObj_Tri
{
    typedef void ( RENDER_3D_OPENGL::*type )( const TRIANGLE_2D*, TRIANGLE_DISPLAY_LIST*, float,
                                              float );
};
template struct rob<R3D_addObj_Tri, &RENDER_3D_OPENGL::addObjectTriangles>;

struct R3D_addObj_Seg
{
    typedef void ( RENDER_3D_OPENGL::*type )( const ROUND_SEGMENT_2D*, TRIANGLE_DISPLAY_LIST*,
                                              float, float );
};
template struct rob<R3D_addObj_Seg, &RENDER_3D_OPENGL::addObjectTriangles>;

struct R3D_appendPostMachining
{
    typedef bool ( RENDER_3D_OPENGL::*type )( TRIANGLE_DISPLAY_LIST*, const SFVEC2F&,
                                              PAD_DRILL_POST_MACHINING_MODE, int, int, float,
                                              float, bool, float, float, float* );
};
template struct rob<R3D_appendPostMachining, &RENDER_3D_OPENGL::appendPostMachiningGeometry>;

struct R3D_createBoard
{
    typedef OPENGL_RENDER_LIST* ( RENDER_3D_OPENGL::*type )( const SHAPE_POLY_SET&,
                                                             const BVH_CONTAINER_2D* );
};
template struct rob<R3D_createBoard, &RENDER_3D_OPENGL::createBoard>;

struct R3D_generate3dGrid
{
    typedef void ( RENDER_3D_OPENGL::*type )( GRID3D_TYPE );
};
template struct rob<R3D_generate3dGrid, &RENDER_3D_OPENGL::generate3dGrid>;

struct R3D_setupMaterials
{
    typedef void ( RENDER_3D_OPENGL::*type )();
};
template struct rob<R3D_setupMaterials, &RENDER_3D_OPENGL::setupMaterials>;

struct R3D_setLayerMaterial
{
    typedef void ( RENDER_3D_OPENGL::*type )( PCB_LAYER_ID );
};
template struct rob<R3D_setLayerMaterial, &RENDER_3D_OPENGL::setLayerMaterial>;

struct R3D_setArrowMaterial
{
    typedef void ( RENDER_3D_OPENGL::*type )();
};
template struct rob<R3D_setArrowMaterial, &RENDER_3D_OPENGL::setArrowMaterial>;

struct R3D_m_grid
{
    typedef GLuint RENDER_3D_OPENGL::*type;
};
template struct rob<R3D_m_grid, &RENDER_3D_OPENGL::m_grid>;

// ---- public wrappers ----

bool R3D_InitializeOpenGL( RENDER_3D_OPENGL& aRenderer )
{
    return ( aRenderer.*result<R3D_initializeOpenGL>::ptr )();
}

void R3D_GenerateCylinder( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter,
                           float aInnerRadius, float aOuterRadius, float aZtop, float aZbot,
                           unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst )
{
    ( aRenderer.*result<R3D_generateCylinder>::ptr )( aCenter, aInnerRadius, aOuterRadius, aZtop,
                                                      aZbot, aNrSides, aDst );
}

void R3D_GenerateInvCone( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter,
                          float aInnerRadius, float aOuterRadius, float aZtop, float aZbot,
                          unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst, EDA_ANGLE aAngle )
{
    ( aRenderer.*result<R3D_generateInvCone>::ptr )( aCenter, aInnerRadius, aOuterRadius, aZtop,
                                                     aZbot, aNrSides, aDst, aAngle );
}

void R3D_GenerateDisk( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aRadius,
                       float aZ, unsigned int aNrSides, TRIANGLE_DISPLAY_LIST* aDst, bool aTop )
{
    ( aRenderer.*result<R3D_generateDisk>::ptr )( aCenter, aRadius, aZ, aNrSides, aDst, aTop );
}

void R3D_GenerateDimple( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aRadius,
                         float aZ, float aDepth, unsigned int aNrSides,
                         TRIANGLE_DISPLAY_LIST* aDst, bool aTop )
{
    ( aRenderer.*result<R3D_generateDimple>::ptr )( aCenter, aRadius, aZ, aDepth, aNrSides, aDst,
                                                    aTop );
}

void R3D_GenerateRing( RENDER_3D_OPENGL& aRenderer, const SFVEC2F& aCenter, float aInnerRadius,
                       float aOuterRadius, unsigned int aNrSides,
                       std::vector<SFVEC2F>& aInnerContour, std::vector<SFVEC2F>& aOuterContour,
                       bool aInvertOrder )
{
    ( aRenderer.*result<R3D_generateRing>::ptr )( aCenter, aInnerRadius, aOuterRadius, aNrSides,
                                                  aInnerContour, aOuterContour, aInvertOrder );
}

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const FILLED_CIRCLE_2D* aCircle,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot )
{
    ( aRenderer.*result<R3D_addObj_Circle>::ptr )( aCircle, aDst, aZtop, aZbot );
}

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const RING_2D* aRing,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot )
{
    ( aRenderer.*result<R3D_addObj_Ring>::ptr )( aRing, aDst, aZtop, aZbot );
}

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const POLYGON_4PT_2D* aPoly,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot )
{
    ( aRenderer.*result<R3D_addObj_Poly4>::ptr )( aPoly, aDst, aZtop, aZbot );
}

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const TRIANGLE_2D* aTri,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot )
{
    ( aRenderer.*result<R3D_addObj_Tri>::ptr )( aTri, aDst, aZtop, aZbot );
}

void R3D_AddObjTriangles( RENDER_3D_OPENGL& aRenderer, const ROUND_SEGMENT_2D* aSeg,
                          TRIANGLE_DISPLAY_LIST* aDst, float aZtop, float aZbot )
{
    ( aRenderer.*result<R3D_addObj_Seg>::ptr )( aSeg, aDst, aZtop, aZbot );
}

bool R3D_AppendPostMachining( RENDER_3D_OPENGL& aRenderer, TRIANGLE_DISPLAY_LIST* aDst,
                              const SFVEC2F& aHoleCenter, PAD_DRILL_POST_MACHINING_MODE aMode,
                              int aSizeIU, int aDepthIU, float aHoleInnerRadius, float aZSurface,
                              bool aIsFront, float aPlatingThickness3d, float aUnitScale,
                              float* aZEnd )
{
    return ( aRenderer.*result<R3D_appendPostMachining>::ptr )(
            aDst, aHoleCenter, aMode, aSizeIU, aDepthIU, aHoleInnerRadius, aZSurface, aIsFront,
            aPlatingThickness3d, aUnitScale, aZEnd );
}

OPENGL_RENDER_LIST* R3D_CreateBoard( RENDER_3D_OPENGL& aRenderer,
                                     const SHAPE_POLY_SET& aBoardPoly,
                                     const BVH_CONTAINER_2D* aThroughHoles )
{
    return ( aRenderer.*result<R3D_createBoard>::ptr )( aBoardPoly, aThroughHoles );
}

void R3D_Generate3dGrid( RENDER_3D_OPENGL& aRenderer, GRID3D_TYPE aGridType )
{
    ( aRenderer.*result<R3D_generate3dGrid>::ptr )( aGridType );
}

unsigned int R3D_GetGridList( RENDER_3D_OPENGL& aRenderer )
{
    return aRenderer.*result<R3D_m_grid>::ptr;
}

void R3D_SetupMaterials( RENDER_3D_OPENGL& aRenderer )
{
    ( aRenderer.*result<R3D_setupMaterials>::ptr )();
}

void R3D_SetLayerMaterial( RENDER_3D_OPENGL& aRenderer, PCB_LAYER_ID aLayerID )
{
    ( aRenderer.*result<R3D_setLayerMaterial>::ptr )( aLayerID );
}

void R3D_SetArrowMaterial( RENDER_3D_OPENGL& aRenderer )
{
    ( aRenderer.*result<R3D_setArrowMaterial>::ptr )();
}
