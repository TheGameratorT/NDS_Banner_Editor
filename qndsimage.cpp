#include "qndsimage.h"

QNDSImage::QNDSImage() {}

QNDSImage::QNDSImage(const QImage& img, const QVector<u16>& pal, int alphaThreshold) {
    replace(img, pal, alphaThreshold);
}

QNDSImage::QNDSImage(const QImage& img, int colorCount, int alphaThreshold) {
    replace(img, colorCount, alphaThreshold);
}

QNDSImage::QNDSImage(const QVector<u8>& ncg, const QVector<u16>& ncl, bool is4bpp) {
    replace(ncg, ncl, is4bpp);
}

u16 QNDSImage::toRgb15(u32 rgb24)
{
    u8 r, g, b;

    r = (rgb24 >> 16) & 0xFF;
    g = (rgb24 >> 8) & 0xFF;
    b = (rgb24 >> 0) & 0xFF;

    r = qRound(r * 31.0 / 255.0);
    g = qRound(g * 31.0 / 255.0);
    b = qRound(b * 31.0 / 255.0);

    return (b << 10) | (g << 5) | r;
}

u32 QNDSImage::toRgb24(u16 rgb15)
{
    u8 r, g, b;

    r = (rgb15 >> 0) & 0x1F;
    g = (rgb15 >> 5) & 0x1F;
    b = (rgb15 >> 10) & 0x1F;

    r = qRound(r * 255.0 / 31.0);
    g = qRound(g * 255.0 / 31.0);
    b = qRound(b * 255.0 / 31.0);

    return (r << 16) | (g << 8) | b;
}

void QNDSImage::replace(const QImage& img, const QVector<u16>& pal, int alphaThreshold)
{
    palette = pal;

    const int newPalSize = pal.size();
    QVector<QColor> newPal24(newPalSize);
    for (int i = 0; i < newPalSize; i++)
        newPal24[i] = toRgb24(pal[i]);

    const int width = img.width();
    const int height = img.height();

    texture.resize(width * height);

    if(img.depth() == 8 && img.colorCount() <= 16)
    {
        for (int i = 0, y = 0; y < height; y++)
            for (int x = 0; x < width; x++, i++)
                texture[i] = img.pixelIndex(x, y);
    }
    else
    {
        for (int i = 0, y = 0; y < height; y++)
            for (int x = 0; x < width; x++, i++)
                texture[i] = closestMatch(img.pixelColor(x, y), newPal24, alphaThreshold);
    }

    texture = getTiled(width / 8, true);
}

void QNDSImage::replace(const QImage& img, int colorCount, int alphaThreshold)
{
    const int width = img.width();
    const int height = img.height();

    QVector<QColor> pal;
    pal.append(QColor(0xFF, 0x00, 0xFF, 0x00)); //Make transparent the first color

    if(img.depth() == 8)
    {
        // We already have a palette, just use it
        if(img.colorCount() <= 16) // Valid palette, don't add transparent
            pal.clear();

        for(const QRgb &color : img.colorTable())
            pal.append(color);
    }
    else
    {
        // No palette, get all the colors
        QColor magenta = QColor(0xFF, 0x00, 0xFF, 0xFF);
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++)
            {
                QColor c = img.pixelColor(x, y);
                if(!pal.contains(c) && c.alpha() >= alphaThreshold && c != magenta)
                    pal.append(c);
            }
        }
    }

    QVector<u16> newPal = createPalette(pal, colorCount);
    replace(img, newPal, alphaThreshold);
}

void QNDSImage::replace(const QVector<u8>& ncg, const QVector<u16>& ncl, bool is4bpp)
{
    if(is4bpp)
    {
        const int texSize = ncg.size() << 1;
        texture.resize(texSize);

        for (int i = 0, j = 0; i < texSize; i += 2, j++)
        {
            texture[i] = ncg[j] & 0xF;
            texture[i + 1] = (ncg[j] >> 4) & 0xF;
        }
    }
    else
        texture = ncg;

    palette = ncl;
}

QVector<u8> QNDSImage::getTiled(int tileWidth, bool inverse)
{
    const int textureSize = texture.size();
    const int width = tileWidth * 8;
    const int height = textureSize / width;

    QVector<u8> out(textureSize);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
        {
            u32 bf = x + y * width;

            u32 tf = bf % 64;
            u32 tx = tf % 8;
            u32 ty = tf / 8;

            u32 gf = bf / 64;
            u32 gx = (gf % tileWidth) * 8;
            u32 gy = (gf / tileWidth) * 8;

            u32 dsti, srci;
            if (inverse)
            {
                dsti = x + y * width;
                srci = (gx + tx) + (gy + ty) * width;
            }
            else
            {
                dsti = (gx + tx) + (gy + ty) * width;
                srci = x + y * width;
            }
            out[dsti] = texture[srci];
        }
    }

    return out;
}

#define _QNDSIMAGE_PLTT_IMAGE_DEBUG 0

QImage QNDSImage::toImage(int tileWidth)
{
#if _QNDSIMAGE_PLTT_IMAGE_DEBUG
    QImage out(16, 16, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    for (int i = 0, y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++, i++)
        {
            if (i == palette.size())
                break;
            out.setPixelColor(x, y, toRgb24(palette[i]));
        }
    }
    return out;
#else
    const int width = tileWidth * 8;
    const int height = texture.size() / width;

    QImage out(width, height, QImage::Format_Indexed8);

    QVector<QRgb> pal;
    for(u16 color : this->palette)
        pal.append(0xFF000000 | toRgb24(color));
    pal[0] &= 0x00FFFFFF; // Make color 0 transparent
    out.setColorTable(pal);

    QVector<u8> tiled = getTiled(tileWidth, false);
    for(int i = 0, y = 0; y < height; y++) {
        for(int x = 0; x < width; x++, i++) {
            out.setPixel(x, y, tiled[i]);
        }
    }

    return out;
#endif
}

void QNDSImage::toNitro(QVector<u8>& ncg, QVector<u16>& ncl, bool is4bpp)
{
    if(is4bpp)
    {
        const int texSize = texture.size() >> 1;
        ncg.resize(texSize);

        for (int i = 0, j = 0; i < texSize; i++, j += 2)
        {
            ncg[i] = texture[j] & 0xF;
            ncg[i] |= (texture[j + 1] & 0xF) << 4;
        }
    }
    else
        ncg = texture;

    ncl = palette;
}

QVector<u16> QNDSImage::createPalette(QVector<QColor> pal, int colorCount)
{
    if(pal.size() < colorCount)
        pal.resize(colorCount);

    // Sort colors and reduce to needed amount
    if(pal.size() > colorCount)
    {
        // For finding color channel that has the most wide range,
        // we need to keep their lower and upper bound.
        int lower_red = pal[0].red(),
            lower_green = pal[0].green(),
            lower_blue = pal[0].blue();
        int upper_red = 0,
            upper_green = 0,
            upper_blue = 0;

        // Loop trough all the colors
        for (QColor c : pal)
        {
            lower_red = std::min(lower_red, c.red());
            lower_green = std::min(lower_green, c.green());
            lower_blue = std::min(lower_blue, c.blue());

            upper_red = std::max(upper_red, c.red());
            upper_green = std::max(upper_green, c.green());
            upper_blue = std::max(upper_blue, c.blue());
        }

        int red = upper_red - lower_red;
        int green = upper_green - lower_green;
        int blue = upper_blue - lower_blue;
        int max = std::max(std::max(red, green), blue);

        // Compare two rgb color according to our selected color channel.
        std::sort(pal.begin(), pal.end(),
        [max, red, green/*, blue*/](const QColor& c1, const QColor& c2)
        {
            // Always keep transparent at the start
            if(c1.alpha() == 0)
                return true;
            else if(c2.alpha() == 0)
                return false;

            if (max == red)  // if red is our color that has the widest range
                return c1.red() < c2.red(); // just compare their red channel
            else if (max == green) //...
                return c1.green() < c2.green();
            else //if (max == blue)
                return c1.blue() < c2.blue();
        });

        // Reduce to the desired number of colors
        double groupSize = pal.size() / colorCount;
        for (int i = 0; i < colorCount; ++i)
            palette.append(toRgb15(pal[qRound((groupSize * i) + (groupSize / 2))].rgb()));
    }
    else
    {
        // Already a valid palette, just use it
        for (int i = 0; i < colorCount; ++i) {
            palette.append(toRgb15(pal[i].rgb()));
        }

    }

    return palette;
}

inline int QNDSImage::pixelDistance(QColor p1, QColor p2)
{
    int r1 = p1.red();
    int g1 = p1.green();
    int b1 = p1.blue();
    int a1 = p1.alpha();

    int r2 = p2.red();
    int g2 = p2.green();
    int b2 = p2.blue();
    int a2 = p2.alpha();

    return abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2) + abs(a1 - a2);
}

inline int QNDSImage::closestMatch(QColor pixel, const QVector<QColor>& clut, int alphaThreshold)
{
    // If it's transparency, index 0
    if(pixel.alpha() < alphaThreshold || pixel == QColor(0xFF, 0x00, 0xFF, 0xFF)) {
        return 0;
    }

    // Otherwise find the closest match
    int idx = 0;
    int current_distance = INT_MAX;
    for (int i = 1; i < clut.size(); ++i)
    {
        int dist = pixelDistance(pixel, clut[i]);
        if (dist < current_distance)
        {
            current_distance = dist;
            idx = i;
        }
    }
    return idx;
}
