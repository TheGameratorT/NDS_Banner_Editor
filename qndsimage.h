#ifndef QNDSIMAGE_H
#define QNDSIMAGE_H

#include "types.h"

#include <QVector>
#include <QImage>
#include <QColor>

class QNDSImage
{
public:
    QNDSImage();
    QNDSImage(const QImage& img, const QVector<u16>& pal, bool col0Transparent);
    QNDSImage(const QImage& img, int colorCount, int alphaThreshold);
    QNDSImage(const QVector<u8>& ncg, const QVector<u16>& ncl, bool is4bpp, bool col0Transparent);

    static u16 toRgb15(u32 rgb24);
    static u32 toRgb24(u16 rgb15);

    void replace(const QImage& img, const QVector<u16>& pal, bool col0Transparent);
    void replace(const QImage& img, int colorCount, int alphaThreshold);
    void replace(const QVector<u8>& ncg, const QVector<u16>& ncl, bool is4bpp, bool col0Transparent);

    QVector<u8> getTiled(int tileWidth, bool inverse);
    QImage toImage(int tileWidth);
    void toNitro(QVector<u8>& ncg, QVector<u16>& ncl, bool is4bpp);

private:
    bool col0Transparent;

    QVector<u8> texture;
    QVector<u16> palette;

    int pixelDistance(QColor p1, QColor p2);
    int closestMatch(QColor pixel, const QVector<QColor> &clut);

    QVector<u16> createPalette(const QVector<QColor>& pal, int colorCount);
};

#endif // QNDSIMAGE_H
