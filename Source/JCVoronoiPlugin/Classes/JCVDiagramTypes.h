////////////////////////////////////////////////////////////////////////////////
//
// MIT License
// 
// Copyright (c) 2018-2019 Nuraga Wiswakarma
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
// 

#pragma once

#include "jc_voronoi.h"
#include "UnrealMathUtility.h"

#define FJCV_INT3_SCALE     1000.f
#define FJCV_INT3_SCALE_INV .001f

#define JCV_EQUAL_THRESHOLD FJCV_INT3_SCALE_INV
#define JCV_INT_CONVERSION_SCALE FJCV_INT3_SCALE

typedef jcv_diagram     FJCVDiagram;
typedef jcv_site        FJCVSite;
typedef jcv_graphedge   FJCVEdge;
typedef jcv_point       FJCVPoint;

// Math Utility

class FJCVMathUtil
{
public:

    FORCEINLINE static bool IsEqual(const FJCVPoint& Pt0, const FJCVPoint& Pt1)
    {
        return FMath::IsNearlyEqual(Pt0.x, Pt1.x, JCV_EQUAL_THRESHOLD) &&
               FMath::IsNearlyEqual(Pt0.y, Pt1.y, JCV_EQUAL_THRESHOLD);
    }

    FORCEINLINE static float DistSqr(const FJCVPoint& p0, const FJCVPoint& p1)
    {
        float X = p0.x-p1.x;
        float Y = p0.y-p1.y;
        return X*X + Y*Y;
    }

    FORCEINLINE static FJCVPoint ToPt(const FVector2D& v)
    {
        FJCVPoint p;
        p.x = v.X;
        p.y = v.Y;
        return p;
    }

    FORCEINLINE static FVector2D ToVector2D(const FJCVPoint& p)
    {
        return FVector2D(p.x, p.y);
    }

    FORCEINLINE static FIntPoint ToIntPointScaled(const FJCVPoint& p, float Scale = JCV_INT_CONVERSION_SCALE)
    {
        return FIntPoint(
            FMath::RoundHalfFromZero(p.x * JCV_INT_CONVERSION_SCALE),
            FMath::RoundHalfFromZero(p.y * JCV_INT_CONVERSION_SCALE)
            );
    }

    FORCEINLINE static FIntPoint ToIntPointScaled(float X, float Y, float Scale = JCV_INT_CONVERSION_SCALE)
    {
        return FIntPoint(
            FMath::RoundHalfFromZero(X * JCV_INT_CONVERSION_SCALE),
            FMath::RoundHalfFromZero(Y * JCV_INT_CONVERSION_SCALE)
            );
    }

    FORCEINLINE static float GetMidValue(float f0, float f1)
    {
        return f0+(f1-f0)*.5f;
    }

    FORCEINLINE static FVector2D GetMidPoint(const FVector2D& v0, const FVector2D& v1)
    {
        return FVector2D( GetMidValue(v0.X, v1.X), GetMidValue(v0.Y, v1.Y) );
    }
};
