/**
 * KIFONT Helpers for GAL Test
 *
 * Provides factory functions to create STROKE_GLYPH instances for testing
 * DrawGlyph() and DrawGlyphs() APIs.
 *
 * Uses KiCad's actual KIFONT::GLYPH and KIFONT::STROKE_GLYPH classes from
 * font/glyph.h. This file only provides helper functions to create test glyphs.
 *
 * STROKE_GLYPH inherits from std::vector<std::vector<VECTOR2D>>, where each
 * inner vector represents a stroke path (pen down to pen up).
 */

#ifndef KIFONT_STUB_H
#define KIFONT_STUB_H

#include <font/glyph.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <vector>
#include <memory>

namespace KIFONT
{

/**
 * Helper function to create a stroke glyph from raw polyline data
 */
inline std::unique_ptr<STROKE_GLYPH> MakeStrokeGlyph(
    const std::vector<std::vector<VECTOR2D>>& aStrokes)
{
    auto glyph = std::make_unique<STROKE_GLYPH>();

    for (const auto& stroke : aStrokes)
    {
        for (size_t i = 0; i < stroke.size(); i++)
        {
            glyph->AddPoint(stroke[i]);
        }
        glyph->RaisePen();
    }

    glyph->Finalize();
    return glyph;
}


/**
 * Create letter "F" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterF(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Vertical stroke
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(0, 100) * scale + offset },
        // Top horizontal
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(60, 0) * scale + offset },
        // Middle horizontal
        { VECTOR2D(0, 40) * scale + offset, VECTOR2D(40, 40) * scale + offset }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "L" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterL(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Vertical stroke
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(0, 100) * scale + offset },
        // Bottom horizontal
        { VECTOR2D(0, 100) * scale + offset, VECTOR2D(50, 100) * scale + offset }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "K" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterK(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Vertical stroke
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(0, 100) * scale + offset },
        // Upper diagonal
        { VECTOR2D(0, 50) * scale + offset, VECTOR2D(50, 0) * scale + offset },
        // Lower diagonal
        { VECTOR2D(0, 50) * scale + offset, VECTOR2D(50, 100) * scale + offset }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "I" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterI(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Top horizontal
        { VECTOR2D(-20, 0) * scale + offset, VECTOR2D(20, 0) * scale + offset },
        // Vertical stroke
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(0, 100) * scale + offset },
        // Bottom horizontal
        { VECTOR2D(-20, 100) * scale + offset, VECTOR2D(20, 100) * scale + offset }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "C" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterC(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    // Approximate C with connected line segments
    std::vector<std::vector<VECTOR2D>> strokes = {
        {
            VECTOR2D(50, 10) * scale + offset,
            VECTOR2D(30, 0) * scale + offset,
            VECTOR2D(10, 10) * scale + offset,
            VECTOR2D(0, 30) * scale + offset,
            VECTOR2D(0, 70) * scale + offset,
            VECTOR2D(10, 90) * scale + offset,
            VECTOR2D(30, 100) * scale + offset,
            VECTOR2D(50, 90) * scale + offset
        }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "A" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterA(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Left diagonal
        { VECTOR2D(0, 100) * scale + offset, VECTOR2D(30, 0) * scale + offset },
        // Right diagonal
        { VECTOR2D(30, 0) * scale + offset, VECTOR2D(60, 100) * scale + offset },
        // Crossbar
        { VECTOR2D(15, 60) * scale + offset, VECTOR2D(45, 60) * scale + offset }
    };
    return MakeStrokeGlyph(strokes);
}


/**
 * Create letter "D" as a stroke glyph
 */
inline std::unique_ptr<STROKE_GLYPH> MakeLetterD(double scale = 1.0, VECTOR2D offset = VECTOR2D(0, 0))
{
    std::vector<std::vector<VECTOR2D>> strokes = {
        // Vertical stroke
        { VECTOR2D(0, 0) * scale + offset, VECTOR2D(0, 100) * scale + offset },
        // Curved part (approximated)
        {
            VECTOR2D(0, 0) * scale + offset,
            VECTOR2D(30, 0) * scale + offset,
            VECTOR2D(50, 20) * scale + offset,
            VECTOR2D(50, 80) * scale + offset,
            VECTOR2D(30, 100) * scale + offset,
            VECTOR2D(0, 100) * scale + offset
        }
    };
    return MakeStrokeGlyph(strokes);
}

}  // namespace KIFONT

#endif  // KIFONT_STUB_H
