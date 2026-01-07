/**
 * BITMAP_BASE Test Patterns for GAL Test
 *
 * Provides helper functions to create wxImage test patterns that can be
 * used with KiCad's real BITMAP_BASE class for testing DrawBitmap().
 *
 * Usage:
 *   wxImage img = CreateCheckerboardImage(64, 64);
 *   BITMAP_BASE bitmap;
 *   bitmap.SetImage(img);
 *   gal->DrawBitmap(bitmap);
 */

#ifndef BITMAP_BASE_STUB_H
#define BITMAP_BASE_STUB_H

#include <wx/image.h>
#include <bitmap_base.h>
#include <cmath>
#include <memory>

//=============================================================================
// Image pattern generation functions
//=============================================================================

/**
 * Create a solid color image
 */
inline wxImage CreateSolidImage(int width, int height,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            alpha[y * width + x] = a;
        }
    }

    return img;
}

/**
 * Create a checkerboard pattern image
 */
inline wxImage CreateCheckerboardImage(int width, int height, int squareSize = 8,
                                       uint8_t r1 = 255, uint8_t g1 = 255, uint8_t b1 = 255,
                                       uint8_t r2 = 0, uint8_t g2 = 0, uint8_t b2 = 0)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            bool isLight = ((x / squareSize) + (y / squareSize)) % 2 == 0;

            if (isLight)
            {
                data[idx + 0] = r1;
                data[idx + 1] = g1;
                data[idx + 2] = b1;
            }
            else
            {
                data[idx + 0] = r2;
                data[idx + 1] = g2;
                data[idx + 2] = b2;
            }
            alpha[y * width + x] = 255;
        }
    }

    return img;
}

/**
 * Create a horizontal gradient image
 */
inline wxImage CreateGradientHImage(int width, int height,
                                    uint8_t r1, uint8_t g1, uint8_t b1,
                                    uint8_t r2, uint8_t g2, uint8_t b2)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            float t = (float)x / (width - 1);

            data[idx + 0] = (uint8_t)(r1 + t * (r2 - r1));
            data[idx + 1] = (uint8_t)(g1 + t * (g2 - g1));
            data[idx + 2] = (uint8_t)(b1 + t * (b2 - b1));
            alpha[y * width + x] = 255;
        }
    }

    return img;
}

/**
 * Create a vertical gradient image
 */
inline wxImage CreateGradientVImage(int width, int height,
                                    uint8_t r1, uint8_t g1, uint8_t b1,
                                    uint8_t r2, uint8_t g2, uint8_t b2)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int y = 0; y < height; y++)
    {
        float t = (float)y / (height - 1);

        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;

            data[idx + 0] = (uint8_t)(r1 + t * (r2 - r1));
            data[idx + 1] = (uint8_t)(g1 + t * (g2 - g1));
            data[idx + 2] = (uint8_t)(b1 + t * (b2 - b1));
            alpha[y * width + x] = 255;
        }
    }

    return img;
}

/**
 * Create a radial gradient image
 */
inline wxImage CreateRadialGradientImage(int width, int height,
                                         uint8_t r1, uint8_t g1, uint8_t b1,
                                         uint8_t r2, uint8_t g2, uint8_t b2)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    float cx = width / 2.0f;
    float cy = height / 2.0f;
    float maxDist = std::sqrt(cx * cx + cy * cy);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float t = std::min(1.0f, dist / maxDist);

            data[idx + 0] = (uint8_t)(r1 + t * (r2 - r1));
            data[idx + 1] = (uint8_t)(g1 + t * (g2 - g1));
            data[idx + 2] = (uint8_t)(b1 + t * (b2 - b1));
            alpha[y * width + x] = 255;
        }
    }

    return img;
}

/**
 * Create a striped pattern image
 */
inline wxImage CreateStripedImage(int width, int height, int stripeWidth, bool horizontal,
                                  uint8_t r1, uint8_t g1, uint8_t b1,
                                  uint8_t r2, uint8_t g2, uint8_t b2)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            int pos = horizontal ? y : x;
            bool isFirst = (pos / stripeWidth) % 2 == 0;

            if (isFirst)
            {
                data[idx + 0] = r1;
                data[idx + 1] = g1;
                data[idx + 2] = b1;
            }
            else
            {
                data[idx + 0] = r2;
                data[idx + 1] = g2;
                data[idx + 2] = b2;
            }
            alpha[y * width + x] = 255;
        }
    }

    return img;
}

/**
 * Create a "K" logo-style image (simplified KiCad logo)
 */
inline wxImage CreateKiCadLogoImage(int width, int height,
                                    uint8_t bgR = 30, uint8_t bgG = 60, uint8_t bgB = 30,
                                    uint8_t fgR = 255, uint8_t fgG = 200, uint8_t fgB = 50)
{
    wxImage img(width, height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    // Fill background
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int idx = (y * width + x) * 3;
            data[idx + 0] = bgR;
            data[idx + 1] = bgG;
            data[idx + 2] = bgB;
            alpha[y * width + x] = 255;
        }
    }

    // Draw a simplified "K" shape
    int centerX = width / 2;
    int centerY = height / 2;
    int thickness = std::max(2, std::min(width, height) / 8);
    int halfH = height / 3;
    int halfW = width / 3;

    // Vertical bar of K
    for (int y = centerY - halfH; y <= centerY + halfH; y++)
    {
        if (y < 0 || y >= height) continue;
        for (int dx = -thickness/2; dx <= thickness/2; dx++)
        {
            int x = centerX - halfW/2 + dx;
            if (x < 0 || x >= width) continue;
            int idx = (y * width + x) * 3;
            data[idx + 0] = fgR;
            data[idx + 1] = fgG;
            data[idx + 2] = fgB;
        }
    }

    // Upper diagonal of K
    for (int i = 0; i <= halfH; i++)
    {
        int y = centerY - i;
        int x = centerX - halfW/2 + (halfW * i / halfH);
        if (y < 0 || y >= height) continue;

        for (int dy = -thickness/2; dy <= thickness/2; dy++)
        {
            for (int dx = -thickness/2; dx <= thickness/2; dx++)
            {
                int py = y + dy;
                int px = x + dx;
                if (py < 0 || py >= height) continue;
                if (px < 0 || px >= width) continue;
                int idx = (py * width + px) * 3;
                data[idx + 0] = fgR;
                data[idx + 1] = fgG;
                data[idx + 2] = fgB;
            }
        }
    }

    // Lower diagonal of K
    for (int i = 0; i <= halfH; i++)
    {
        int y = centerY + i;
        int x = centerX - halfW/2 + (halfW * i / halfH);
        if (y < 0 || y >= height) continue;

        for (int dy = -thickness/2; dy <= thickness/2; dy++)
        {
            for (int dx = -thickness/2; dx <= thickness/2; dx++)
            {
                int py = y + dy;
                int px = x + dx;
                if (py < 0 || py >= height) continue;
                if (px < 0 || px >= width) continue;
                int idx = (py * width + px) * 3;
                data[idx + 0] = fgR;
                data[idx + 1] = fgG;
                data[idx + 2] = fgB;
            }
        }
    }

    return img;
}

/**
 * Add a border to an existing image
 */
inline void AddBorderToImage(wxImage& img, int borderWidth,
                             uint8_t r, uint8_t g, uint8_t b)
{
    int width = img.GetWidth();
    int height = img.GetHeight();
    unsigned char* data = img.GetData();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            bool isBorder = (x < borderWidth || x >= width - borderWidth ||
                            y < borderWidth || y >= height - borderWidth);

            if (isBorder)
            {
                int idx = (y * width + x) * 3;
                data[idx + 0] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
            }
        }
    }
}

//=============================================================================
// Factory functions to create BITMAP_BASE with test patterns
//=============================================================================

/**
 * Create a BITMAP_BASE with checkerboard pattern
 */
inline std::unique_ptr<BITMAP_BASE> CreateCheckerboardBitmap(int width, int height, int squareSize = 8)
{
    auto bitmap = std::make_unique<BITMAP_BASE>();
    wxImage img = CreateCheckerboardImage(width, height, squareSize);
    bitmap->SetImage(img);
    return bitmap;
}

/**
 * Create a BITMAP_BASE with horizontal gradient (red to blue)
 */
inline std::unique_ptr<BITMAP_BASE> CreateGradientBitmap(int width, int height)
{
    auto bitmap = std::make_unique<BITMAP_BASE>();
    wxImage img = CreateGradientHImage(width, height, 255, 50, 50, 50, 50, 255);
    bitmap->SetImage(img);
    return bitmap;
}

/**
 * Create a BITMAP_BASE with KiCad logo pattern
 */
inline std::unique_ptr<BITMAP_BASE> CreateKiCadLogoBitmap(int width, int height)
{
    auto bitmap = std::make_unique<BITMAP_BASE>();
    wxImage img = CreateKiCadLogoImage(width, height);
    AddBorderToImage(img, 2, 200, 150, 50);
    bitmap->SetImage(img);
    return bitmap;
}

/**
 * Create a BITMAP_BASE with radial gradient
 */
inline std::unique_ptr<BITMAP_BASE> CreateRadialBitmap(int width, int height)
{
    auto bitmap = std::make_unique<BITMAP_BASE>();
    wxImage img = CreateRadialGradientImage(width, height, 255, 255, 200, 50, 50, 150);
    bitmap->SetImage(img);
    return bitmap;
}

/**
 * Create a BITMAP_BASE with stripes
 */
inline std::unique_ptr<BITMAP_BASE> CreateStripedBitmap(int width, int height, bool horizontal = true)
{
    auto bitmap = std::make_unique<BITMAP_BASE>();
    wxImage img = CreateStripedImage(width, height, 8, horizontal, 100, 150, 200, 200, 100, 50);
    bitmap->SetImage(img);
    return bitmap;
}

#endif  // BITMAP_BASE_STUB_H
