#include "test_board_data.h"

#include <cmath>
#include <cstddef>

const BOARD_ITEM& DummyBoardItem()
{
    // The reference is stored by OBJECT_2D but never dereferenced (object_2d.h:114);
    // aligned opaque storage stands in so no pcbnew types need to link.
    alignas( 16 ) static unsigned char storage[256] = {};
    return *reinterpret_cast<const BOARD_ITEM*>( storage );
}


// ---- S3DMODEL widget -------------------------------------------------------
// Static storage: S3DMODEL/SMESH point into these arrays (the struct carries
// raw pointers; MODEL_3D copies everything into VBOs on construction).

// Axis-aligned box: 24 vertices (4 per face, flat normals), 36 indices.
static void fillBox( SFVEC3F* aPos, SFVEC3F* aNorm, unsigned int* aIdx, const SFVEC3F& aMin,
                     const SFVEC3F& aMax )
{
    const SFVEC3F n[6] = {
        { 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 },
    };

    // 4 corners per face, CCW seen from outside.
    const SFVEC3F c[6][4] = {
        // +Z
        { { aMin.x, aMin.y, aMax.z }, { aMax.x, aMin.y, aMax.z }, { aMax.x, aMax.y, aMax.z },
          { aMin.x, aMax.y, aMax.z } },
        // -Z
        { { aMin.x, aMin.y, aMin.z }, { aMin.x, aMax.y, aMin.z }, { aMax.x, aMax.y, aMin.z },
          { aMax.x, aMin.y, aMin.z } },
        // +X
        { { aMax.x, aMin.y, aMin.z }, { aMax.x, aMax.y, aMin.z }, { aMax.x, aMax.y, aMax.z },
          { aMax.x, aMin.y, aMax.z } },
        // -X
        { { aMin.x, aMin.y, aMin.z }, { aMin.x, aMin.y, aMax.z }, { aMin.x, aMax.y, aMax.z },
          { aMin.x, aMax.y, aMin.z } },
        // +Y
        { { aMin.x, aMax.y, aMin.z }, { aMin.x, aMax.y, aMax.z }, { aMax.x, aMax.y, aMax.z },
          { aMax.x, aMax.y, aMin.z } },
        // -Y
        { { aMin.x, aMin.y, aMin.z }, { aMax.x, aMin.y, aMin.z }, { aMax.x, aMin.y, aMax.z },
          { aMin.x, aMin.y, aMax.z } },
    };

    for( int f = 0; f < 6; f++ )
    {
        for( int v = 0; v < 4; v++ )
        {
            aPos[f * 4 + v] = c[f][v];
            aNorm[f * 4 + v] = n[f];
        }

        aIdx[f * 6 + 0] = f * 4 + 0;
        aIdx[f * 6 + 1] = f * 4 + 1;
        aIdx[f * 6 + 2] = f * 4 + 2;
        aIdx[f * 6 + 3] = f * 4 + 0;
        aIdx[f * 6 + 4] = f * 4 + 2;
        aIdx[f * 6 + 5] = f * 4 + 3;
    }
}


// Octahedron: 8 triangular faces, 24 vertices (flat normals), 24 indices.
static void fillOctahedron( SFVEC3F* aPos, SFVEC3F* aNorm, unsigned int* aIdx,
                            const SFVEC3F& aCenter, float aRadius )
{
    const SFVEC3F apexTop = aCenter + SFVEC3F( 0, 0, aRadius );
    const SFVEC3F apexBot = aCenter - SFVEC3F( 0, 0, aRadius );

    const SFVEC3F equator[4] = {
        aCenter + SFVEC3F( aRadius, 0, 0 ),
        aCenter + SFVEC3F( 0, aRadius, 0 ),
        aCenter + SFVEC3F( -aRadius, 0, 0 ),
        aCenter + SFVEC3F( 0, -aRadius, 0 ),
    };

    unsigned int v = 0;

    for( int i = 0; i < 4; i++ )
    {
        const SFVEC3F& e0 = equator[i];
        const SFVEC3F& e1 = equator[( i + 1 ) % 4];

        // top face (CCW from outside)
        SFVEC3F nTop = glm::normalize( glm::cross( e1 - e0, apexTop - e0 ) );
        aPos[v] = e0;
        aPos[v + 1] = e1;
        aPos[v + 2] = apexTop;
        aNorm[v] = aNorm[v + 1] = aNorm[v + 2] = nTop;
        aIdx[v] = v;
        aIdx[v + 1] = v + 1;
        aIdx[v + 2] = v + 2;
        v += 3;

        // bottom face
        SFVEC3F nBot = glm::normalize( glm::cross( apexBot - e0, e1 - e0 ) );
        aPos[v] = e1;
        aPos[v + 1] = e0;
        aPos[v + 2] = apexBot;
        aNorm[v] = aNorm[v + 1] = aNorm[v + 2] = nBot;
        aIdx[v] = v;
        aIdx[v + 1] = v + 1;
        aIdx[v + 2] = v + 2;
        v += 3;
    }
}


const S3DMODEL& TestS3DModel()
{
    // mesh 0: opaque red plastic box
    static SFVEC3F      boxPos[24];
    static SFVEC3F      boxNorm[24];
    static unsigned int boxIdx[36];

    // mesh 1: transparent blue octahedron
    static SFVEC3F      octPos[24];
    static SFVEC3F      octNorm[24];
    static unsigned int octIdx[24];

    // mesh 2: per-vertex-colored box (m_Color array exercises GL_COLOR_ARRAY)
    static SFVEC3F      colBoxPos[24];
    static SFVEC3F      colBoxNorm[24];
    static SFVEC3F      colBoxColor[24];
    static unsigned int colBoxIdx[36];

    static SMESH     meshes[3];
    static SMATERIAL materials[3];
    static S3DMODEL  model;
    static bool      initialized = false;

    if( !initialized )
    {
        initialized = true;

        fillBox( boxPos, boxNorm, boxIdx, SFVEC3F( -2.2f, -1.2f, 0.0f ),
                 SFVEC3F( -0.2f, 1.2f, 1.2f ) );

        fillOctahedron( octPos, octNorm, octIdx, SFVEC3F( 1.4f, 0.0f, 0.9f ), 1.1f );

        fillBox( colBoxPos, colBoxNorm, colBoxIdx, SFVEC3F( -0.6f, -2.4f, 0.0f ),
                 SFVEC3F( 1.0f, -1.2f, 0.7f ) );

        for( int i = 0; i < 24; i++ )
        {
            colBoxColor[i] = SFVEC3F( ( i % 3 ) == 0 ? 1.0f : 0.2f, ( i % 3 ) == 1 ? 1.0f : 0.2f,
                                      ( i % 3 ) == 2 ? 1.0f : 0.2f );
        }

        meshes[0] = { 24, boxPos, boxNorm, nullptr, nullptr, 36, boxIdx, 0 };
        meshes[1] = { 24, octPos, octNorm, nullptr, nullptr, 24, octIdx, 1 };
        meshes[2] = { 24, colBoxPos, colBoxNorm, nullptr, colBoxColor, 36, colBoxIdx, 2 };

        // { Ambient, Diffuse, Emissive, Specular, Shininess, Transparency }
        materials[0] = { { 0.30f, 0.05f, 0.05f }, { 0.80f, 0.10f, 0.10f }, { 0, 0, 0 },
                         { 0.30f, 0.30f, 0.30f }, 0.30f, 0.00f };
        materials[1] = { { 0.05f, 0.05f, 0.30f }, { 0.15f, 0.25f, 0.90f }, { 0, 0, 0 },
                         { 0.60f, 0.60f, 0.70f }, 0.80f, 0.50f };
        materials[2] = { { 0.15f, 0.15f, 0.15f }, { 0.70f, 0.70f, 0.70f }, { 0, 0, 0 },
                         { 0.90f, 0.90f, 0.90f }, 0.90f, 0.00f };

        model = { 3, meshes, 3, materials };
    }

    return model;
}


std::vector<SFVEC2F> MakeSquareContour( float aHalf, float aCenterX, float aCenterY )
{
    // CCW, closed (first point repeated last) — AddToMiddleContours processes
    // size-1 segments (layer_triangles.cpp:123-133).
    return {
        SFVEC2F( aCenterX - aHalf, aCenterY - aHalf ),
        SFVEC2F( aCenterX + aHalf, aCenterY - aHalf ),
        SFVEC2F( aCenterX + aHalf, aCenterY + aHalf ),
        SFVEC2F( aCenterX - aHalf, aCenterY + aHalf ),
        SFVEC2F( aCenterX - aHalf, aCenterY - aHalf ),
    };
}


std::vector<SFVEC2F> MakeCircleContour( float aRadius, int aSides, float aCenterX,
                                        float aCenterY )
{
    std::vector<SFVEC2F> points;
    points.reserve( aSides + 1 );

    for( int i = 0; i < aSides; i++ )
    {
        const float a = 2.0f * static_cast<float>( M_PI ) * i / aSides;
        points.emplace_back( aCenterX + aRadius * std::cos( a ),
                             aCenterY + aRadius * std::sin( a ) );
    }

    points.push_back( points.front() );
    return points;
}
