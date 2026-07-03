/**
 * Hand-built input DATA for the scenarios (geometry containers, S3DMODEL,
 * contours). Data only — everything that draws must be real KiCad code.
 */

#ifndef TEST_BOARD_DATA_H
#define TEST_BOARD_DATA_H

#include <plugins/3dapi/c3dmodel.h> // S3DMODEL / SMESH / SMATERIAL
#include <plugins/3dapi/xv3d_types.h>

#include <vector>

// The shapes2D constructors take a BOARD_ITEM& that is only stored for later
// identification, never dereferenced by any code the suite runs
// (object_2d.h:114). Only the forward declaration exists here.
class BOARD_ITEM;

/// An opaque never-dereferenced BOARD_ITEM reference for the shapes2D ctors.
const BOARD_ITEM& DummyBoardItem();

/// A small multi-mesh, multi-material widget: opaque box + transparent
/// octahedron + per-vertex-colored box. Arrays live in static storage; the
/// returned struct stays valid for the process lifetime.
const S3DMODEL& TestS3DModel();

/// Closed CCW square contour (first point repeated last) for AddToMiddleContours.
std::vector<SFVEC2F> MakeSquareContour( float aHalf, float aCenterX = 0.0f,
                                        float aCenterY = 0.0f );

/// Closed regular-polygon contour approximating a circle.
std::vector<SFVEC2F> MakeCircleContour( float aRadius, int aSides, float aCenterX = 0.0f,
                                        float aCenterY = 0.0f );

#endif // TEST_BOARD_DATA_H
