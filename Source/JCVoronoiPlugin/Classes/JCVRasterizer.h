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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "JCVRasterizer.generated.h"

class FJCVRasterizer
{
    class Edge
    {
        public:
            int32 X1, Y1, X2, Y2;

            Edge(int32 x1, int32 y1, int32 x2, int32 y2)
            {
                if(y1 < y2) {
                    X1 = x1;
                    Y1 = y1;
                    X2 = x2;
                    Y2 = y2;
                } else {
                    X1 = x2;
                    Y1 = y2;
                    X2 = x1;
                    Y2 = y1;
                }
            }
    };

    class Span
    {
        public:
            int32 X1, X2;

            Span(int32 x1, int32 x2)
            {
                if(x1 < x2) {
                    X1 = x1;
                    X2 = x2;
                } else {
                    X1 = x2;
                    X2 = x1;
                }
            }
    };

    TArray<FIntPoint> m_IndexBuffer;
    uint32 m_Width, m_Height;

    void DrawSpan(const Span &span, int32 y)
    {
        int32 xdiff = span.X2 - span.X1;
        if(xdiff == 0)
            return;

        float factor = 0.0f;
        float factorStep = 1.0f / (float)xdiff;

        // draw each pixel in the span
        for(int32 x = span.X1; x < span.X2; x++) {
            SetPixel(x, y);
            factor += factorStep;
        }
    }

    void DrawSpansBetweenEdges(const Edge &e1, const Edge &e2)
    {
        // calculate difference between the y coordinates
        // of the first edge and return if 0
        float e1ydiff = (float)(e1.Y2 - e1.Y1);
        if(FMath::IsNearlyZero(e1ydiff))
            return;

        // calculate difference between the y coordinates
        // of the second edge and return if 0
        float e2ydiff = (float)(e2.Y2 - e2.Y1);
        if(FMath::IsNearlyZero(e2ydiff))
            return;

        // calculate differences between the x coordinates
        float e1xdiff = (float)(e1.X2 - e1.X1);
        float e2xdiff = (float)(e2.X2 - e2.X1);

        // calculate factors to use for interpolation
        // with the edges and the step values to increase
        // them by after drawing each span
        float factor1 = (float)(e2.Y1 - e1.Y1) / e1ydiff;
        float factorStep1 = 1.0f / e1ydiff;
        float factor2 = 0.0f;
        float factorStep2 = 1.0f / e2ydiff;

        // loop through the lines between the edges and draw spans
        for(int32 y = e2.Y1; y < e2.Y2; y++) {
            // create and draw span
            Span span(e1.X1 + (int32)(e1xdiff * factor1),
                      e2.X1 + (int32)(e2xdiff * factor2));
            DrawSpan(span, y);

            // increase factors
            factor1 += factorStep1;
            factor2 += factorStep2;
        }
    }

public:

    TArray<FIntPoint>& GetIndexBuffer()
    {
        return m_IndexBuffer;
    }

    void SetIndexBuffer(uint32 w, uint32 h)
    {
        m_Width = w;
        m_Height = h;
        m_IndexBuffer.Empty( w*h );
    }

    void SetPixel(uint32 x, uint32 y)
    {
        if(x >= m_Width || y >= m_Height)
            return;

        m_IndexBuffer.Emplace(x, y);
    }

    void SetPixel(int32 x, int32 y)
    {
        SetPixel((uint32)x, (uint32)y);
    }

    void SetPixel(float x, float y)
    {
        if(x < 0.0f || y < 0.0f)
            return;

        SetPixel((uint32)x, (uint32)y);
    }

    void Clear(bool bShrink = true)
    {
        if (bShrink)
        {
            m_IndexBuffer.Empty();
        }
        else
        {
            m_IndexBuffer.Empty(m_Width*m_Height);
        }
    }

    FORCEINLINE void DrawTriangle(const FVector2D& v1, const FVector2D& v2, const FVector2D& v3)
    {
        DrawTriangle(v1.X, v1.Y, v2.X, v2.Y, v3.X, v3.Y);
    }

    void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        // create edges for the triangle
        Edge edges[3] = {
            Edge((int32)x1, (int32)y1, (int32)x2, (int32)y2),
            Edge((int32)x2, (int32)y2, (int32)x3, (int32)y3),
            Edge((int32)x3, (int32)y3, (int32)x1, (int32)y1)
        };

        int32 maxLength = 0;
        int32 longEdge = 0;

        // find edge with the greatest length in the y axis
        for(int32 i = 0; i < 3; i++) {
            int32 length = edges[i].Y2 - edges[i].Y1;
            if(length > maxLength) {
                maxLength = length;
                longEdge = i;
            }
        }

        int32 shortEdge1 = (longEdge + 1) % 3;
        int32 shortEdge2 = (longEdge + 2) % 3;

        // draw spans between edges; the long edge can be drawn
        // with the shorter edges to draw the full triangle
        DrawSpansBetweenEdges(edges[longEdge], edges[shortEdge1]);
        DrawSpansBetweenEdges(edges[longEdge], edges[shortEdge2]);
    }
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVRasterizerRef
{
	GENERATED_BODY()

    FJCVRasterizer Rasterizer;

	UPROPERTY()
    TArray<FIntPoint> IndexBuffer;
};

UCLASS()
class JCVORONOIPLUGIN_API UJCVRasterizerFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="JCV")
    static void Tri(const FVector2D& v1, const FVector2D& v2, const FVector2D& v3, TArray<FIntPoint>& Indices)
    {
        FJCVRasterizer r;
        r.SetIndexBuffer(256, 256);
        r.DrawTriangle(v1.X, v1.Y, v2.X, v2.Y, v3.X, v3.Y);
        Indices = r.GetIndexBuffer();
    }

	UFUNCTION(BlueprintCallable, Category="JCV")
    static void SetIndexBuffer(FJCVRasterizerRef& RasterizerRef, FIntPoint Size)
    {
        RasterizerRef.Rasterizer.SetIndexBuffer(Size.X, Size.Y);
    }

	UFUNCTION(BlueprintCallable, Category="JCV")
    static void DrawTriangle(FJCVRasterizerRef& RasterizerRef, const FVector2D& v1, const FVector2D& v2, const FVector2D& v3)
    {
        FJCVRasterizer& r(RasterizerRef.Rasterizer);
        r.DrawTriangle(v1.X, v1.Y, v2.X, v2.Y, v3.X, v3.Y);
    }

	UFUNCTION(BlueprintCallable, Category="JCV")
    static void DrawTriangles(FJCVRasterizerRef& RasterizerRef, const TArray<FVector2D>& Vertices, const TArray<int32>& Indices)
    {
        FJCVRasterizer& r(RasterizerRef.Rasterizer);
        const int32 TriCount = Indices.Num() / 3;

        for (int32 i=0; i<TriCount; ++i)
        {
            const int32 idx = i * 3;
            const FVector2D& v1(Vertices[Indices[idx  ]]);
            const FVector2D& v2(Vertices[Indices[idx+1]]);
            const FVector2D& v3(Vertices[Indices[idx+2]]);
            r.DrawTriangle(v1.X, v1.Y, v2.X, v2.Y, v3.X, v3.Y);
        }
    }

	UFUNCTION(BlueprintCallable, Category="JCV")
    static void StoreIndexBuffer(FJCVRasterizerRef& RasterizerRef)
    {
        RasterizerRef.IndexBuffer = RasterizerRef.Rasterizer.GetIndexBuffer();
    }

	UFUNCTION(BlueprintCallable, Category="JCV")
    static TArray<FIntPoint> GetIndexBuffer(FJCVRasterizerRef& RasterizerRef)
    {
        return RasterizerRef.Rasterizer.GetIndexBuffer();
    }
};
