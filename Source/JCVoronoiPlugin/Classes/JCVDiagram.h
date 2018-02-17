// 

#pragma once

#include "SharedPointer.h"
#include "UnrealMemory.h"
#include "JCVDiagramTypes.h"

typedef jcv_diagram     FJCVDiagram;
typedef jcv_site        FJCVSite;
typedef jcv_graphedge   FJCVGraphEdge;
typedef jcv_point       FJCVPoint;

class FJCVDiagramContext
{
    typedef TSharedPtr<FJCVDiagram> TPSDiagram;
    typedef TWeakPtr<FJCVDiagram>   TPWDiagram;

    TPSDiagram Diagram;
    FBox2D DiagramExtents;
    const FJCVSite* Sites;

public:

    FJCVDiagramContext()
    {
        ResetDiagram();
    }

    FJCVDiagramContext(FBox2D Extents, TArray<FVector2D>& Points)
    {
        ResetDiagram();
        GenerateDiagram(Extents, Points);
    }

    FJCVDiagramContext(FVector2D Size, TArray<FVector2D>& Points)
    {
        ResetDiagram();
        GenerateDiagram(Size, Points);
    }

    void ResetDiagram()
    {
        Diagram = TPSDiagram( new FJCVDiagram(), [=](FJCVDiagram* d){
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

    void GenerateDiagram(FBox2D Extents, TArray<FVector2D>& InOutPoints)
    {
        check(Extents.bIsValid);
        check(Diagram.IsValid());
        
        if (InOutPoints.Num() == 0)
            return;

        DiagramExtents = Extents;

        jcv_rect ext;
        ext.min.x = Extents.Min.X;
        ext.min.y = Extents.Min.Y;
        ext.max.x = Extents.Max.X;
        ext.max.y = Extents.Max.Y;
        const int32 pointCount = InOutPoints.Num();

        FJCVPoint* points = nullptr;
        {
            void* p_points = FMemory::Malloc(sizeof(FJCVPoint) * pointCount);
            points = static_cast<FJCVPoint*>( p_points );
            if (! points)
                return;

            // Memcpy if point/vector data size are equals
            if (sizeof(FJCVPoint) == InOutPoints.GetTypeSize())
            {
                FMemory::Memcpy(points, InOutPoints.GetData(), pointCount*InOutPoints.GetTypeSize());
            }
            // Otherwise, standard array assign
            else
            {
                for (int32 i=0; i<pointCount; ++i)
                {
                    points[i].x = InOutPoints[i].X;
                    points[i].y = InOutPoints[i].Y;
                }
            }
        }
        jcv_diagram_generate_useralloc(pointCount, points, &ext, 0, jcv_alloc_fn, jcv_free_fn, Diagram.Get());
        Sites = jcv_diagram_get_sites(Diagram.Get());
        FMemory::Free(points);
    }

    FORCEINLINE const FJCVSite* Find(const FVector2D& pos) const
    {
        const FJCVSite* s = FindClosest(pos);
        if (s)
            return Find(pos, *s);
        return nullptr;
    }

    const FJCVSite* Find(const FVector2D& pos, const FJCVSite& s) const
    {
        if (IsWithin(s, pos))
            return &s;
        const FJCVGraphEdge* g = s.edges;
        if (g)
        do
        {
            const FJCVSite* n = g->neighbor;
            if (n && IsWithin(*n, pos))
                return n;
        }
        while ((g=g->next) != nullptr);
        return FindCloser(s, FJCVMathUtil::AsPt(pos));
    }

    template<class ContainerType>
    void FindAllTo(const FVector2D& pos, const FJCVSite& s, ContainerType& OutSites) const
    {
        if (IsWithin(s, pos))
        {
            OutSites.Emplace(&s);
            return;
        }
        const FJCVGraphEdge* g = s.edges;
        // Point is not within site, search neighbours
        if (g)
        do
        {
            const FJCVSite* n = g->neighbor;
            if (n && IsWithin(*n, pos))
            {
                OutSites.Emplace(n);
                return;
            }
        }
        while ((g=g->next) != nullptr);
        // Point is not within site or its neighbour, expand search
        const FJCVSite* closerSite = FindCloser(s, FJCVMathUtil::AsPt(pos));
        if (closerSite)
        {
            OutSites.Emplace(closerSite);
            FindAllTo(pos, *closerSite, OutSites);
        }
    }

    const FJCVSite* FindClosest(const FVector2D& pos) const
    {
        if (IsEmpty())
        {
            return nullptr;
        }

        const FJCVPoint p( FJCVMathUtil::AsPt(pos) );
        const FJCVSite* s0 = nullptr;
        const FJCVSite* s1 = GetSites();

        while (s1)
        {
            s0 = s1;
            s1 = FindCloser(*s0, p);
        }

        return s0;
    }

    const FJCVSite* FindCloser(const FJCVSite& s0, const FJCVPoint& p) const
    {
        const FJCVSite* s1 = nullptr;
        const FJCVGraphEdge* e = s0.edges;
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

    FORCEINLINE bool IsWithin(const FJCVSite& s, const FVector2D& pos) const
    {
        const FJCVGraphEdge* g = s.edges;
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
        const FJCVGraphEdge* g = s.edges;
        do
        {
            pts.Emplace(g->pos[0]);
        }
        while ((g=g->next) != nullptr);
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

    FORCEINLINE const FJCVGraphEdge* Graph(int32 i) const
    {
        return Site(i).edges;
    }

    FORCEINLINE const FBox2D& GetExtents() const
    {
        return DiagramExtents;
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

