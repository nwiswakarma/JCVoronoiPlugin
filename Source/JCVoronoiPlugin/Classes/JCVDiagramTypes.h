// 

#pragma once

#include "jc_voronoi.h"
#include "UnrealMathUtility.h"

typedef jcv_diagram     FJCVDiagram;
typedef jcv_site        FJCVSite;
typedef jcv_graphedge   FJCVGraphEdge;
typedef jcv_point       FJCVPoint;

class FJCVMathUtil
{
public:

    FORCEINLINE static float DistSqr(const FJCVPoint& p0, const FJCVPoint& p1)
    {
        float X = p0.x-p1.x;
        float Y = p0.y-p1.y;
        return X*X + Y*Y;
    }

    FORCEINLINE static FJCVPoint AsPt(const FVector2D& v)
    {
        FJCVPoint p;
        p.x = v.X;
        p.y = v.Y;
        return p;
    }

    FORCEINLINE static FVector2D AsV2D(const FJCVPoint& p)
    {
        return FVector2D(p.x, p.y);
    }

    FORCEINLINE static float MidF(float f0, float f1)
    {
        return f0+(f1-f0)*.5f;
    }

    FORCEINLINE static FVector2D MidV2D(const FVector2D& v0, const FVector2D& v1)
    {
        return FVector2D( MidF(v0.X, v1.X), MidF(v0.Y, v1.Y) );
    }
};
