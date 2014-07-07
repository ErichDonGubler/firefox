//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// generatemip.h: Defines the GenerateMip function, templated on the format
// type of the image for which mip levels are being generated.

#ifndef LIBGLESV2_RENDERER_GENERATEMIP_H_
#define LIBGLESV2_RENDERER_GENERATEMIP_H_

#include "common/mathutil.h"
#include "imageformats.h"

namespace rx
{

namespace priv
{

template <typename T>
static inline T *GetPixel(void *data, unsigned int x, unsigned int y, unsigned int z, unsigned int rowPitch, unsigned int depthPitch)
{
    return reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(data) + (x * sizeof(T)) + (y * rowPitch) + (z * depthPitch));
}

template <typename T>
static inline const T *GetPixel(const void *data, unsigned int x, unsigned int y, unsigned int z, unsigned int rowPitch, unsigned int depthPitch)
{
    return reinterpret_cast<const T*>(reinterpret_cast<const unsigned char*>(data) + (x * sizeof(T)) + (y * rowPitch) + (z * depthPitch));
}

template <typename T>
static void GenerateMip_Y(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                          const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                          unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                          unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth == 1);
    ASSERT(sourceHeight > 1);
    ASSERT(sourceDepth == 1);

    for (unsigned int y = 0; y < destHeight; y++)
    {
        const T *src0 = GetPixel<T>(sourceData, 0, y * 2, 0, sourceRowPitch, sourceDepthPitch);
        const T *src1 = GetPixel<T>(sourceData, 0, y * 2 + 1, 0, sourceRowPitch, sourceDepthPitch);
        T *dst = GetPixel<T>(destData, 0, y, 0, destRowPitch, destDepthPitch);

        T::average(dst, src0, src1);
    }
}

template <typename T>
static void GenerateMip_X(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                          const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                          unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                          unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth > 1);
    ASSERT(sourceHeight == 1);
    ASSERT(sourceDepth == 1);

    for (unsigned int x = 0; x < destWidth; x++)
    {
        const T *src0 = GetPixel<T>(sourceData, x * 2, 0, 0, sourceRowPitch, sourceDepthPitch);
        const T *src1 = GetPixel<T>(sourceData, x * 2 + 1, 0, 0, sourceRowPitch, sourceDepthPitch);
        T *dst = GetPixel<T>(destData, x, 0, 0, destRowPitch, destDepthPitch);

        T::average(dst, src0, src1);
    }
}

template <typename T>
static void GenerateMip_Z(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                          const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                          unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                          unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth == 1);
    ASSERT(sourceHeight == 1);
    ASSERT(sourceDepth > 1);

    for (unsigned int z = 0; z < destDepth; z++)
    {
        const T *src0 = GetPixel<T>(sourceData, 0, 0, z * 2, sourceRowPitch, sourceDepthPitch);
        const T *src1 = GetPixel<T>(sourceData, 0, 0, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
        T *dst = GetPixel<T>(destData, 0, 0, z, destRowPitch, destDepthPitch);

        T::average(dst, src0, src1);
    }
}

template <typename T>
static void GenerateMip_XY(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                           const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                           unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                           unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth > 1);
    ASSERT(sourceHeight > 1);
    ASSERT(sourceDepth == 1);

    for (unsigned int y = 0; y < destHeight; y++)
    {
        for (unsigned int x = 0; x < destWidth; x++)
        {
            const T *src0 = GetPixel<T>(sourceData, x * 2, y * 2, 0, sourceRowPitch, sourceDepthPitch);
            const T *src1 = GetPixel<T>(sourceData, x * 2, y * 2 + 1, 0, sourceRowPitch, sourceDepthPitch);
            const T *src2 = GetPixel<T>(sourceData, x * 2 + 1, y * 2, 0, sourceRowPitch, sourceDepthPitch);
            const T *src3 = GetPixel<T>(sourceData, x * 2 + 1, y * 2 + 1, 0, sourceRowPitch, sourceDepthPitch);
            T *dst = GetPixel<T>(destData, x, y, 0, destRowPitch, destDepthPitch);

            T tmp0, tmp1;

            T::average(&tmp0, src0, src1);
            T::average(&tmp1, src2, src3);
            T::average(dst, &tmp0, &tmp1);
        }
    }
}

template <typename T>
static void GenerateMip_YZ(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                           const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                           unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                           unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth == 1);
    ASSERT(sourceHeight > 1);
    ASSERT(sourceDepth > 1);

    for (unsigned int z = 0; z < destDepth; z++)
    {
        for (unsigned int y = 0; y < destHeight; y++)
        {
            const T *src0 = GetPixel<T>(sourceData, 0, y * 2, z * 2, sourceRowPitch, sourceDepthPitch);
            const T *src1 = GetPixel<T>(sourceData, 0, y * 2, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
            const T *src2 = GetPixel<T>(sourceData, 0, y * 2 + 1, z * 2, sourceRowPitch, sourceDepthPitch);
            const T *src3 = GetPixel<T>(sourceData, 0, y * 2 + 1, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
            T *dst = GetPixel<T>(destData, 0, y, z, destRowPitch, destDepthPitch);

            T tmp0, tmp1;

            T::average(&tmp0, src0, src1);
            T::average(&tmp1, src2, src3);
            T::average(dst, &tmp0, &tmp1);
        }
    }
}

template <typename T>
static void GenerateMip_XZ(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                           const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                           unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                           unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth > 1);
    ASSERT(sourceHeight == 1);
    ASSERT(sourceDepth > 1);

    for (unsigned int z = 0; z < destDepth; z++)
    {
        for (unsigned int x = 0; x < destWidth; x++)
        {
            const T *src0 = GetPixel<T>(sourceData, x * 2, 0, z * 2, sourceRowPitch, sourceDepthPitch);
            const T *src1 = GetPixel<T>(sourceData, x * 2, 0, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
            const T *src2 = GetPixel<T>(sourceData, x * 2 + 1, 0, z * 2, sourceRowPitch, sourceDepthPitch);
            const T *src3 = GetPixel<T>(sourceData, x * 2 + 1, 0, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
            T *dst = GetPixel<T>(destData, x, 0, z, destRowPitch, destDepthPitch);

            T tmp0, tmp1;

            T::average(&tmp0, src0, src1);
            T::average(&tmp1, src2, src3);
            T::average(dst, &tmp0, &tmp1);
        }
    }
}

template <typename T>
static void GenerateMip_XYZ(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                            const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                            unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                            unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    ASSERT(sourceWidth > 1);
    ASSERT(sourceHeight > 1);
    ASSERT(sourceDepth > 1);

    for (unsigned int z = 0; z < destDepth; z++)
    {
        for (unsigned int y = 0; y < destHeight; y++)
        {
            for (unsigned int x = 0; x < destWidth; x++)
            {
                const T *src0 = GetPixel<T>(sourceData, x * 2, y * 2, z * 2, sourceRowPitch, sourceDepthPitch);
                const T *src1 = GetPixel<T>(sourceData, x * 2, y * 2, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
                const T *src2 = GetPixel<T>(sourceData, x * 2, y * 2 + 1, z * 2, sourceRowPitch, sourceDepthPitch);
                const T *src3 = GetPixel<T>(sourceData, x * 2, y * 2 + 1, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
                const T *src4 = GetPixel<T>(sourceData, x * 2 + 1, y * 2, z * 2, sourceRowPitch, sourceDepthPitch);
                const T *src5 = GetPixel<T>(sourceData, x * 2 + 1, y * 2, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
                const T *src6 = GetPixel<T>(sourceData, x * 2 + 1, y * 2 + 1, z * 2, sourceRowPitch, sourceDepthPitch);
                const T *src7 = GetPixel<T>(sourceData, x * 2 + 1, y * 2 + 1, z * 2 + 1, sourceRowPitch, sourceDepthPitch);
                T *dst = GetPixel<T>(destData, x, y, z, destRowPitch, destDepthPitch);

                T tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;

                T::average(&tmp0, src0, src1);
                T::average(&tmp1, src2, src3);
                T::average(&tmp2, src4, src5);
                T::average(&tmp3, src6, src7);

                T::average(&tmp4, &tmp0, &tmp1);
                T::average(&tmp5, &tmp2, &tmp3);

                T::average(dst, &tmp4, &tmp5);
            }
        }
    }
}


typedef void (*MipGenerationFunction)(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                                      const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                                      unsigned int destWidth, unsigned int destHeight, unsigned int destDepth,
                                      unsigned char *destData, int destRowPitch, int destDepthPitch);

template <typename T>
static MipGenerationFunction GetMipGenerationFunction(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth)
{
    unsigned char index = ((sourceWidth > 1)  ? 1 : 0) |
                          ((sourceHeight > 1) ? 2 : 0) |
                          ((sourceDepth > 1)  ? 4 : 0);

    switch (index)
    {
      case 1:  return GenerateMip_X<T>;   // W x 1 x 1
      case 2:  return GenerateMip_Y<T>;   // 1 x H x 1
      case 3:  return GenerateMip_XY<T>;  // W x H x 1
      case 4:  return GenerateMip_Z<T>;   // 1 x 1 x D
      case 5:  return GenerateMip_XZ<T>;  // W x 1 x D
      case 6:  return GenerateMip_YZ<T>;  // 1 x H x D
      case 7:  return GenerateMip_XYZ<T>; // W x H x D
      default: return NULL;
    }
}

}

template <typename T>
static void GenerateMip(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                        const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                        unsigned char *destData, int destRowPitch, int destDepthPitch)
{
    unsigned int mipWidth = std::max(1U, sourceWidth >> 1);
    unsigned int mipHeight = std::max(1U, sourceHeight >> 1);
    unsigned int mipDepth = std::max(1U, sourceDepth >> 1);

    priv::MipGenerationFunction generationFunction = priv::GetMipGenerationFunction<T>(sourceWidth, sourceHeight, sourceDepth);
    ASSERT(generationFunction != NULL);

    generationFunction(sourceWidth, sourceHeight, sourceDepth, sourceData, sourceRowPitch, sourceDepthPitch,
                       mipWidth, mipHeight, mipDepth, destData, destRowPitch, destDepthPitch);
}

}

#endif // LIBGLESV2_RENDERER_GENERATEMIP_H_
