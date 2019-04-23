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

#include "SharedPointer.h"
#include "UnrealMemory.h"
#include "JCVDiagramTypes.h"

typedef jcv_diagram     FJCVDiagram;
typedef jcv_site        FJCVSite;
typedef jcv_graphedge   FJCVEdge;
typedef jcv_point       FJCVPoint;

class FJCVDiagramContext
{
    typedef TSharedPtr<FJCVDiagram> FPSDiagram;
    typedef TWeakPtr<FJCVDiagram>   FPWDiagram;

    FPSDiagram Diagram;
    FBox2D DiagramBounds;
    const FJCVSite* Sites;

    FJCVDiagramContext(const FJCVDiagramContext& Other) = default;
    FJCVDiagramContext& operator=(const FJCVDiagramContext& Other) = default;

public:

    FJCVDiagramContext()
    {
        ResetDiagram();
    }

    FJCVDiagramContext(FBox2D Bounds, TArray<FVector2D>& Points)
    {
        ResetDiagram();
        GenerateDiagram(Bounds, Points);
    }

    FJCVDiagramContext(FVector2D Size, TArray<FVector2D>& Points)
    {
        ResetDiagram();
        GenerateDiagram(Size, Points);
    }

    void ResetDiagram()
    {
        Diagram = FPSDiagram( new FJCVDiagram(), [=](FJCVDiagram* d){
            if (d->internal)
                jcv_diagram_free(d);
        } );
        FMemory::Memset(Diagram.Get(), 0, sizeof(jcv_diagram));
    }

    FORCEINLINE void GenerateDiagram(FVector2D Size, TArray<FVector2D>& InOutPoints)
    {
        check(Size.X > 0.f && Size.Y > 0.f);
        GenerateDiagram(FBox2D(FVector2D(0.f, 0.f), Size), InOutPoints);
    }

    void GenerateDiagram(FBox2D Bounds, TArray<FVector2D>& InOutPoints)
    {
        check(Bounds.bIsValid);
        check(Diagram.IsValid());
        
        if (InOutPoints.Num() == 0)
            return;

        DiagramBounds = Bounds;

        jcv_rect JCVBounds;
        JCVBounds.min.x = Bounds.Min.X;
        JCVBounds.min.y = Bounds.Min.Y;
        JCVBounds.max.x = Bounds.Max.X;
        JCVBounds.max.y = Bounds.Max.Y;

        const int32 PointCount = InOutPoints.Num();

        FJCVPoint* Points = nullptr;
        {
            void* p_points = FMemory::Malloc(sizeof(FJCVPoint) * PointCount);
            Points = static_cast<FJCVPoint*>( p_points );
            if (! Points)
                return;

            // Memcpy if point/vector data size are equals
            if (sizeof(FJCVPoint) == InOutPoints.GetTypeSize())
            {
                FMemory::Memcpy(Points, InOutPoints.GetData(), PointCount*InOutPoints.GetTypeSize());
            }
            // Otherwise, standard array assign
            else
            {
                for (int32 i=0; i<PointCount; ++i)
                {
                    Points[i].x = InOutPoints[i].X;
                    Points[i].y = InOutPoints[i].Y;
                }
            }
        }

        jcv_diagram_generate_useralloc(
            PointCount,
            Points,
            &JCVBounds,
            0,
            jcv_alloc_fn,
            jcv_free_fn,
            Diagram.Get()
            );

        Sites = jcv_diagram_get_sites(Diagram.Get());
        FMemory::Free(Points);
    }

    /**
     * Find a site that contain the specified point.
     *
     * Return a valid site if found. Otherwise, return nullptr.
     */
    FORCEINLINE const FJCVSite* Find(const FVector2D& pos) const
    {
        const FJCVSite* s = FindClosest(pos);
        if (s)
            return Find(pos, *s);
        return nullptr;
    }

    /**
     * Find a site that contain the specified point.
     * Search is started at specified site.
     *
     * Return a valid site if found. Otherwise, return nullptr.
     */
    const FJCVSite* FindFrom(const FVector2D& pos, const FJCVSite& s) const
    {
        if (IsEmpty())
        {
            return nullptr;
        }

        const FJCVPoint p( FJCVMathUtil::ToPt(pos) );
        const FJCVSite* s0 = nullptr;
        const FJCVSite* s1 = &s;

        while (s1)
        {
            s0 = s1;
            s1 = FindCloser(*s0, p);
        }

        if (s0)
        {
            return Find(pos, *s0);
        }

        return nullptr;
    }

    /**
     * Find a site that contain the specified point.
     * Search is started with at specified site.
     *
     * Return a valid site if found. Otherwise, return nullptr.
     */
    const FJCVSite* Find(const FVector2D& pos, const FJCVSite& s) const
    {
        if (IsWithin(s, pos))
            return &s;
        const FJCVEdge* g = s.edges;
        if (g)
        do
        {
            const FJCVSite* n = g->neighbor;
            if (n && IsWithin(*n, pos))
                return n;
        }
        while ((g=g->next) != nullptr);
        return FindCloser(s, FJCVMathUtil::ToPt(pos));
    }

    template<class ContainerType>
    const FJCVSite* FindAllTo(const FVector2D& pos, const FJCVSite& s, ContainerType& OutSites) const
    {
        // Point is within starting search site, return immediately
        if (IsWithin(s, pos))
        {
            OutSites.Emplace(&s);
            return &s;
        }

        const FJCVEdge* g = s.edges;
        // Point is not within site, search neighbours
        if (g)
        do
        {
            const FJCVSite* n = g->neighbor;
            if (n && IsWithin(*n, pos))
            {
                OutSites.Emplace(n);
                return n;
            }
        }
        while ((g=g->next) != nullptr);

        // Point is not within site or its neighbour, expand search
        const FJCVSite* closerSite = FindCloser(s, FJCVMathUtil::ToPt(pos));
        if (closerSite)
        {
            OutSites.Emplace(closerSite);
            return FindAllTo(pos, *closerSite, OutSites);
        }

        return nullptr;
    }

    /**
     * Find a site which origin is closest to the specified point.
     *
     * Return a valid site if found. Otherwise, return nullptr.
     */
    const FJCVSite* FindClosest(const FVector2D& pos) const
    {
        if (IsEmpty())
        {
            return nullptr;
        }

        const FJCVPoint p( FJCVMathUtil::ToPt(pos) );
        const FJCVSite* s0 = nullptr;
        const FJCVSite* s1 = GetSites();

        while (s1)
        {
            s0 = s1;
            s1 = FindCloser(*s0, p);
        }

        return s0;
    }

    /**
     * Find a site with closer origin to a point than the specified site.
     *
     * Return a valid site if found. Otherwise, return nullptr.
     */
    const FJCVSite* FindCloser(const FJCVSite& s0, const FJCVPoint& p) const
    {
        const FJCVSite* s1 = nullptr;
        const FJCVEdge* e = s0.edges;
        float d0 = FJCVMathUtil::DistSqr(s0.p, p);
        while (e)
        {
            if (e->neighbor)
            {
                float d1 = FJCVMathUtil::DistSqr(e->neighbor->p, p);
                if (d1 < d0)
                {
                    d0 = d1;
                    s1 = e->neighbor;
                }
            }
            e = e->next;
        }
        return s1;
    }

    template<class ContainerType>
    void FindAllWithin(const FBox2D& r, const FJCVSite& s, ContainerType& OutSites) const
    {
        if (! r.bIsValid)
        {
            return;
        }

        bool bStartOnMin = IsWithin(s, r.Min);
        bool bStartOnMax = IsWithin(s, r.Max);

        if (bStartOnMin && bStartOnMax)
        {
            OutSites.Emplace(&s);
            return;
        }

        TSet<const FJCVSite*> SiteSet;
        TQueue<const FJCVSite*> SiteQueue;

        // Find rect border cells

        if (bStartOnMin)
        {
            const FJCVSite* Search = &s;
            Search = FindAllTo(FVector2D(r.Max.X, r.Min.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Max.X, r.Max.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Min.X, r.Max.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Min.X, r.Min.Y), *Search, SiteSet);
        }
        else if (bStartOnMax)
        {
            const FJCVSite* Search = &s;
            Search = FindAllTo(FVector2D(r.Min.X, r.Max.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Min.X, r.Min.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Max.X, r.Min.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Max.X, r.Max.Y), *Search, SiteSet);
        }
        else
        {
            const FJCVSite* Search = FindClosest(r.Min);
            Search = FindAllTo(FVector2D(r.Max.X, r.Min.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Max.X, r.Max.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Min.X, r.Max.Y), *Search, SiteSet);
            Search = FindAllTo(FVector2D(r.Min.X, r.Min.Y), *Search, SiteSet);
        }

        // Enqueue site expand search

        for (const FJCVSite* Site : SiteSet)
        {
            SiteQueue.Enqueue(Site);
        }

        TSet<const FJCVSite*> VisitedSiteSet(SiteSet);

        // Boundary expand search

        while (! SiteQueue.IsEmpty())
        {
            const FJCVSite* Site;
            SiteQueue.Dequeue(Site);

            check(Site);

            if (const FJCVEdge* g = Site->edges)
            do
            {
                const FJCVSite* n = g->neighbor;

                // Skip invalid site or registered site
                if (! n || VisitedSiteSet.Contains(n))
                {
                    continue;
                }

                // Mark visited
                VisitedSiteSet.Emplace(n);

                // Check if site center is within rect
                if (r.IsInside(FJCVMathUtil::ToVector2D(n->p)))
                {
                    SiteSet.Emplace(n);
                    SiteQueue.Enqueue(n);
                }
                // Check if site edge points is within rect
                else
                {
                    if (const FJCVEdge* ng = n->edges)
                    do
                    {
                        if (r.IsInside(FJCVMathUtil::ToVector2D(ng->pos[0])))
                        {
                            SiteSet.Emplace(n);
                            SiteQueue.Enqueue(n);
                            break;
                        }
                    }
                    while ((ng=ng->next) != nullptr);
                }
            }
            while ((g=g->next) != nullptr);
        }

        OutSites.Reserve(SiteSet.Num());

        for (const FJCVSite* Site : SiteSet)
        {
            OutSites.Emplace(Site);
        }
    }

    FORCEINLINE bool IsWithin(const FJCVSite& s, const FVector2D& pos) const
    {
        const FJCVEdge* g = s.edges;
        const FJCVPoint& sp( s.p );
        if (g)
        do
        {
            if (pntri(pos.X, pos.Y, sp, g->pos[0], g->pos[1]))
                return true;
        }
        while ((g=g->next) != nullptr);
        return false;
    }

    FORCEINLINE void GetPoints(const FJCVSite& s, TArray<FJCVPoint>& pts) const
    {
        if (const FJCVEdge* g = s.edges)
        do
        {
            pts.Emplace(g->pos[0]);
        }
        while ((g=g->next) != nullptr);
    }

    FORCEINLINE void GetPoints(const FJCVSite& s, TArray<FVector2D>& pts) const
    {
        if (const FJCVEdge* g = s.edges)
        do
        {
            pts.Emplace(FJCVMathUtil::ToVector2D(g->pos[0]));
        }
        while ((g=g->next) != nullptr);
    }

    FORCEINLINE void GetSiteBounds(const FJCVSite& s, FBox2D& Bounds) const
    {
        Bounds.Init();
        if (const FJCVEdge* g = s.edges)
        do
        {
            Bounds += FJCVMathUtil::ToVector2D(g->pos[0]);
        }
        while ((g=g->next) != nullptr);
    }

    template<class ContainerType>
    FORCEINLINE void GetNeighbours(const FJCVSite& s, ContainerType& OutSites) const
    {
        if (const FJCVEdge* g = s.edges)
        do
        {
            if (g->neighbor)
            {
                OutSites.Emplace(g->neighbor);
            }
        }
        while ((g=g->next) != nullptr);
    }

    template<class FCallback>
    FORCEINLINE void VisitNeighbours(const FJCVSite& s, const FCallback& Callback) const
    {
        if (const FJCVEdge* g = s.edges)
        do
        {
            if (g->neighbor)
            {
                Callback(g->neighbor);
            }
        }
        while ((g=g->next) != nullptr);
    }

    FORCEINLINE float GetShortestMidPoint(const FJCVSite& s, FVector2D& OutPoint) const
    {
        FVector2D sp(s.p.x, s.p.y);
        float ShortestDistSq = BIG_NUMBER;
        if (const FJCVEdge* g = s.edges)
        do
        {
            const FJCVPoint& gp0(g->pos[0]);
            const FJCVPoint& gp1(g->pos[1]);
            FVector2D gpm(gp0.x+(gp1.x-gp0.x)*0.5, gp0.y+(gp1.y-gp0.y)*0.5);
            float gpmDistSq = (gpm-sp).SizeSquared();

            if (gpmDistSq < ShortestDistSq)
            {
                ShortestDistSq = gpmDistSq;
                OutPoint = gpm;
            }
        }
        while ((g=g->next) != nullptr);
        return ShortestDistSq;
    }

    FORCEINLINE float GetShortestMidPoint(const FJCVSite& s) const
    {
        FVector2D MidPt;
        return GetShortestMidPoint(s, MidPt);
    }

    FORCEINLINE bool pntri(float px, float py, const FJCVPoint& p0, const FJCVPoint& p1, const FJCVPoint& p2) const
    {
        float dX = px-p2.x;
        float dY = py-p2.y;
        float dX21 = p2.x-p1.x;
        float dY12 = p1.y-p2.y;
        float D = dY12*(p0.x-p2.x) + dX21*(p0.y-p2.y);
        float s = dY12*dX + dX21*dY;
        float t = (p2.y-p0.y)*dX + (p0.x-p2.x)*dY;
        if (D<0.f) return s<=0.f && t<=0.f && s+t>=D;
        return s>=0.f && t>=0.f && s+t<=D;
    }

    FORCEINLINE int32 GetSiteNum() const
    {
        return HasValidDiagram() ? SiteNum() : 0;
    }

    FORCEINLINE bool HasValidDiagram() const
    {
        return Diagram.IsValid();
    }

    FORCEINLINE bool IsEmpty() const
    {
        return GetSiteNum() == 0;
    }

    FORCEINLINE bool IsValidIndex(int32 i) const
    {
        return HasValidDiagram() ? i<SiteNum() : false;
    }

    FORCEINLINE const FJCVDiagram* GetData() const
    {
        return HasValidDiagram() ? Diagram.Get() : nullptr;
    }

    FJCVDiagram* GetData()
    {
        return HasValidDiagram() ? Diagram.Get() : nullptr;
    }

    FORCEINLINE const FJCVSite* GetSites() const
    {
        return HasValidDiagram() ? Sites : nullptr;
    }

    FORCEINLINE const FJCVSite* GetSite(int32 i) const
    {
        return HasValidDiagram()
            ? i<GetSiteNum() ? &Site(i) : nullptr
            : nullptr;
    }

    FORCEINLINE int32 SiteNum() const
    {
        return Diagram.Get()->numsites;
    }

    FORCEINLINE const FJCVSite& Site(int32 i) const
    {
        return Sites[i];
    }

    FORCEINLINE const FJCVEdge* Graph(int32 i) const
    {
        return Site(i).edges;
    }

    FORCEINLINE const FBox2D& GetDiagramBounds() const
    {
        return DiagramBounds;
    }

private:

    FORCEINLINE static void* jcv_alloc_fn(void* memctx, size_t size)
    {
        (void) memctx;
        return FMemory::Malloc(size);
    }

    FORCEINLINE static void jcv_free_fn(void* memctx, void* p)
    {
        (void) memctx;
        FMemory::Free(p);
    }
};
