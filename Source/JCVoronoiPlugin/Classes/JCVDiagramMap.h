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

#include "CoreMinimal.h"

#include "JCVDiagram.h"
#include "JCVParameters.h"

class FJCVDiagramMapContext;
class FJCVDiagramMap;
struct FJCVCell;
struct FJCVEdgeNode;

typedef TSharedPtr<FJCVDiagramMapContext> FPSJCVDiagramMapContext;
typedef TWeakPtr<FJCVDiagramMapContext>   FPWJCVDiagramMapContext;

typedef TSharedPtr<FJCVDiagramMap> FPSJCVDiagramMap;
typedef TWeakPtr<FJCVDiagramMap>   FPWJCVDiagramMap;

typedef TArray<FJCVCell*>       FJCVCellGroup;
typedef TSet<FJCVCell*>         FJCVCellSet;

typedef TArray<FJCVEdgeNode>    FJCVEdgeNodeGroup;
typedef TSet<FJCVEdgeNode>      FJCVEdgeNodeSet;

typedef TArray<FJCVEdge*>       FJCVEdgeGroup;
typedef TSet<FJCVEdge*>         FJCVEdgeSet;

typedef TArray<const FJCVCell*> FJCVConstCellGroup;
typedef TSet<const FJCVCell*>   FJCVConstCellSet;

typedef TArray<const FJCVEdge*> FJCVConstEdgeGroup;
typedef TSet<const FJCVEdge*>   FJCVConstEdgeSet;

enum EJCVCellFeature
{
    JCV_CF_UNMARKED = 0
};

struct FJCVCell
{
    const FJCVSite* Site = nullptr;
    float Value          = 0.f;
    bool bIsBorder       = false;
    uint8 FeatureType    = JCV_CF_UNMARKED;
    int32 FeatureIndex   = 0;

    FJCVCell(const FJCVSite& s, float h, bool b)
        : Site(&s)
        , Value(h)
        , bIsBorder(b)
        , FeatureType(JCV_CF_UNMARKED)
        , FeatureIndex(0)
    {
    }

    FJCVCell(const FJCVSite& s, float h, bool b, uint8 ft, uint8 fi)
        : Site(&s)
        , Value(h)
        , bIsBorder(b)
        , FeatureType(ft)
        , FeatureIndex(fi)
    {
    }

    FORCEINLINE bool IsValid() const
    {
        return Site != nullptr;
    }

    FORCEINLINE int32 GetIndex() const
    {
        return IsValid() ? Site->index : -1;
    }

    FORCEINLINE float GetValue() const
    {
        return Value;
    }

    FORCEINLINE FVector2D ToVector2D() const
    {
        return IsValid() ? ToVector2DUnsafe() : FVector2D::ZeroVector;
    }

    FORCEINLINE FVector2D ToVector2DUnsafe() const
    {
        return FVector2D(Site->p.x, Site->p.y);
    }

    FORCEINLINE FJCVEdge* GetEdge() const
    {
        return IsValid() ? Site->edges : nullptr;
    }

    template<class FContainerType>
    FORCEINLINE void GetPoints(FContainerType& OutPoints) const
    {
        if (IsValid())
        {
            if (const FJCVEdge* g = Site->edges)
            do
            {
                OutPoints.Emplace(g->pos[0]);
            }
            while ((g=g->next) != nullptr);
        }
    }

    FORCEINLINE void GetBounds(FBox2D& OutBounds) const
    {
        if (IsValid())
        {
            const FJCVPoint& Center(Site->p);
            OutBounds.Init();

            if (const FJCVEdge* g = Site->edges)
            do
            {
                const FJCVPoint& CellPoint(g->pos[0]);
                const float PointX = CellPoint.x-Center.x;
                const float PointY = CellPoint.y-Center.y;
                OutBounds += FVector2D(PointX, PointY);
            }
            while ((g=g->next) != nullptr);
        }
    }

    FORCEINLINE bool IsBorder() const
    {
        return bIsBorder;
    }

    FORCEINLINE bool IsType(uint8 t, int32 i = -1) const
    {
        return FeatureType == t && (i < 0 || FeatureIndex == i);
    }


    FORCEINLINE void SetType(uint8 t, int32 i = 0)
    {
        FeatureType  = t;
        FeatureIndex = i;
    }

    FORCEINLINE void SetType(const FJCVCell& rhs)
    {
        SetType(rhs.FeatureType, rhs.FeatureIndex);
    }

    FORCEINLINE void SetValue(float v)
    {
        Value = v;
    }

    FORCEINLINE void SetFeature(float v, uint8 t, int32 i)
    {
        Value = v;
        FeatureType  = t;
        FeatureIndex = i;
    }

    FORCEINLINE void SetFeature(float v, uint8 t)
    {
        SetFeature(v, t, 0);
    }
};

struct FJCVEdgeNode
{
    enum
    {
        HEAD = 0,
        TAIL = 1,
        MID  = 2
    };

    const FJCVEdge* Edge;
    FJCVPoint MidPt;
    int32 Order;

    FJCVEdgeNode() = default;

    FJCVEdgeNode(const FJCVEdge* g, int32 c) : Edge(g) , Order(c)
    {
        const FJCVPoint& p0(GetHead());
        const FJCVPoint& p1(GetTail());
        MidPt.x = p0.x+(p1.x-p0.x)*.5f;
        MidPt.y = p0.y+(p1.y-p0.y)*.5f;
    }

    FORCEINLINE bool IsReversed() const
    {
        return Order != 0;
    }

    FORCEINLINE const FJCVPoint& GetHead() const
    {
        return Edge->pos[IsReversed() ? TAIL : HEAD];
    }

    FORCEINLINE const FJCVPoint& GetTail() const
    {
        return Edge->pos[IsReversed() ? HEAD : TAIL];
    }

    FORCEINLINE const FJCVPoint& GetMid() const
    {
        return MidPt;
    }

    FORCEINLINE FVector2D GetHeadVector2D() const
    {
        return FJCVMathUtil::ToVector2D(GetHead());
    }

    FORCEINLINE FVector2D GetTailVector2D() const
    {
        return FJCVMathUtil::ToVector2D(GetTail());
    }

    FORCEINLINE FVector2D GetMidVector2D() const
    {
        return FJCVMathUtil::ToVector2D(GetMid());
    }

    FORCEINLINE bool IsTailToHead(const FJCVEdgeNode& Node) const
    {
        return FJCVMathUtil::IsEqual(GetTail(), Node.GetHead());
    }

    FORCEINLINE bool IsTailToTail(const FJCVEdgeNode& Node) const
    {
        return FJCVMathUtil::IsEqual(GetTail(), Node.GetTail());
    }

    FORCEINLINE bool IsHeadToTail(const FJCVEdgeNode& Node) const
    {
        return FJCVMathUtil::IsEqual(GetHead(), Node.GetTail());
    }

    FORCEINLINE bool IsHeadToHead(const FJCVEdgeNode& Node) const
    {
        return FJCVMathUtil::IsEqual(GetHead(), Node.GetHead());
    }
};

struct FJCVCellJunction
{
    FVector2D Point;
    FJCVCellGroup Cells;

    FJCVCellJunction() = default;

    FJCVCellJunction(const FVector2D& InPoint, const FJCVCellGroup& InCells)
        : Point(InPoint)
        , Cells(InCells)
    {
    }

    FORCEINLINE bool IsEqual(const FJCVCellJunction& Other) const
    {
        return Point.Equals(Other.Point, JCV_EQUAL_THRESHOLD);
    }
};

struct FJCVCellEdgeList
{
    typedef TPair<const FJCVEdge*, const FJCVEdge*> FEdgePair;
    typedef TDoubleLinkedList<FVector2D> FPointList;

    FPointList PointList;
    FEdgePair EdgePair;
};

struct FJCVFeatureGroup
{
    uint8 FeatureType;
    TArray<FJCVCellGroup> CellGroups;
    TSet<uint8> Neighbours;

    FORCEINLINE bool IsValidIndex(int32 FeatureIndex) const
    {
        return CellGroups.IsValidIndex(FeatureIndex);
    }

    FORCEINLINE int32 GetGroupCount() const
    {
        return CellGroups.Num();
    }

    FORCEINLINE int32 GetCellCount() const
    {
        int32 count = 0;
        for (const FJCVCellGroup& cg : CellGroups)
            count += cg.Num();
        return count;
    }

    FORCEINLINE int32 GetCellCount(int32 FeatureIndex) const
    {
        return CellGroups.IsValidIndex(FeatureIndex)
            ? CellGroups[FeatureIndex].Num()
            : 0;
    }

    FORCEINLINE bool HasNeighbour(uint8 n) const
    {
        return Neighbours.Contains(n);
    }

    FORCEINLINE bool HasCellGroups() const
    {
        return GetGroupCount() > 0;
    }

    /**
     * Check whether any of the cell group contain any cell
     */
    FORCEINLINE bool HasCells() const
    {
        if (HasCellGroups())
        {
            for (const FJCVCellGroup& cg : CellGroups)
                if (cg.Num() > 0)
                    return true;
        }
        return false;
    }

    /**
     * Check whether the specified cell group contain any cell
     */
    FORCEINLINE bool HasCells(int32 FeatureIndex) const
    {
        return GetCellCount(FeatureIndex) > 0;
    }

    FJCVCellGroup* GetCellGroup(int32 FeatureIndex)
    {
        return IsValidIndex(FeatureIndex) ? &CellGroups[FeatureIndex] : nullptr;
    }

    template<class FCellContainer>
    void GetCells(FCellContainer& OutCells, const bool bClearOutput = true) const
    {
        if (bClearOutput)
        {
            OutCells.Empty();
        }

        OutCells.Reserve(GetCellCount());

        for (const FJCVCellGroup& cg : CellGroups)
        {
            OutCells.Append(cg);
        }
    }

    template<class FCellContainer>
    void GetCells(FCellContainer& OutCells, int32 FeatureIndex, const bool bClearOutput = true) const
    {
        if (CellGroups.IsValidIndex(FeatureIndex))
        {
            const FJCVCellGroup& cg(CellGroups[FeatureIndex]);

            if (bClearOutput)
            {
                OutCells.Empty();
            }

            OutCells.Reserve(cg.Num());
            OutCells.Append(cg);
        }
        else if (bClearOutput)
        {
            OutCells.Empty();
        }
    }

    FORCEINLINE void SetType(uint8 t)
    {
        FeatureType = t;
    }

    FORCEINLINE void AddCell(FJCVCell& c, int32 r = -1)
    {
        const int32 i = c.FeatureIndex;
        if (i < 0)
            return;
        if (! CellGroups.IsValidIndex(i))
        {
            CellGroups.SetNum(i+1);
            if (r > 0)
                CellGroups.Reserve(r);
        }
        CellGroups[i].Emplace(&c);
    }

    FORCEINLINE void AddNeighbour(const FJCVCell& c)
    {
        if (! c.IsType(FeatureType) && ! Neighbours.Contains(c.FeatureType))
            Neighbours.Emplace(c.FeatureType);
    }

    FORCEINLINE void Shrink()
    {
        for (FJCVCellGroup& g : CellGroups)
            g.Shrink();
    }

    FORCEINLINE void Empty()
    {
        CellGroups.Empty();
    }

    FJCVCellSet AsSet() const
    {
        FJCVCellSet OutSet;
        OutSet.Reserve(GetCellCount());

        for (const FJCVCellGroup& cg : CellGroups)
        {
            OutSet.Append(cg);
        }

        return MoveTemp(OutSet);
    }

    FJCVCellSet AsSet(int32 FeatureIndex) const
    {
        if (FeatureIndex < 0)
        {
            return AsSet();
        }

        if (HasCells(FeatureIndex))
        {
            FJCVCellSet OutSet(CellGroups[FeatureIndex]);
            return MoveTemp(OutSet);
        }

        return FJCVCellSet();
    }
};

class FJCVDiagramMap
{
public:

    FJCVDiagramMap(FJCVDiagramContext& d);
    FJCVDiagramMap(FJCVDiagramContext& d, uint8 FeatureType, int32 FeatureIndex);
    explicit FJCVDiagramMap(const FJCVDiagramMap& SourceMap);

    // -- DIAGRAM QUERY OPERATIONS

    FORCEINLINE FJCVDiagramContext* operator->()
    {
        return &Diagram;
    }

    FORCEINLINE const FJCVDiagramContext* operator->() const
    {
        return &Diagram;
    }

    FORCEINLINE const FJCVDiagramContext& GetDiagram() const
    {
        return Diagram;
    }

    FORCEINLINE FJCVDiagramContext& GetDiagram()
    {
        return Diagram;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Diagram.IsEmpty();
    }

    FORCEINLINE const FBox2D& GetBounds() const
    {
        return Diagram.GetDiagramBounds();
    }

    // -- FEATURE MODIFICATION OPERATIONS

    void ClearFeatures();

    void ResetFeatures(uint8 FeatureType, int32 FeatureIndex);

    void GroupByFeatures();

    void ExpandFeature(uint8 ft, int32 fi);

    void ExpandFeature(uint8 ft);

    void ExpandFeature(FJCVCellSet& cellS, uint8 ft, int32 fi);

    void ConvertIsolated(
        uint8 ft0,
        uint8 ft1,
        int32 fi,
        bool bGroupFeatures = false
        );

    void MarkFiltered(
        const FJCVSite* InSite,
        uint8 FeatureType,
        int32 FeatureIndex,
        TSet<const FJCVSite*>& FilterSet,
        bool bAddToFilterIfMarked = false
        );

    void MergeGroup(FJCVFeatureGroup& fg0, FJCVFeatureGroup& fg1);

    template<class FCallback>
    void VisitFeatureCells(const FCallback& Callback, uint8 FeatureType, int32 FeatureIndex = -1)
    {
        // Invalid feature type, abort
        if (! HasFeature(FeatureType))
        {
            return;
        }

        const FJCVFeatureGroup& FeatureGroup(FeatureGroups[FeatureType]);

        TArray<int32> FeatureIndices;
        GetFeatureIndices(FeatureIndices, FeatureType, FeatureIndex, true);

        for (const int32 fi : FeatureIndices)
        {
            const FJCVCellGroup& CellGroup(FeatureGroup.CellGroups[fi]);

            for (FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Callback(*Cell);
            }
        }
    }

    void GenerateNeighbourList();

    void MergeNeighbourList(FJCVFeatureGroup& fg0, FJCVFeatureGroup& fg1);

    void ShrinkGroups();

    // -- FEATURE MODIFICATION OPERATIONS (BORDERS)

    void SortEdges(const FJCVConstEdgeSet& es, FJCVEdgeNodeGroup& eg)
    {
        if (es.Num() == 0)
        {
            return;
        }

        const FJCVEdge* g = *es.CreateConstIterator();

        // Creates transient graph set copy
        FJCVConstEdgeSet s(es);

        s.Remove(g);
        eg.Emplace(g, 0);

        while (s.Num() > 0)
        {
            const FJCVEdge* n = nullptr;
            int32 order = -1;
            for (const FJCVEdge* e : s)
                if ((order = connected(*g, *(n=e))) >= 0)
                    break;
            if (order < 0)
                break;
            g = n;
            s.Remove(g);
            eg.Emplace(g, order);
        }
    }

    void GenerateSortedBorderEdges(
        const FJCVConstEdgeSet& es,
        TArray<FJCVCellEdgeList>& EdgeLists
        );

    // -- FEATURE QUERY OPERATIONS

    FORCEINLINE bool HasCells(uint8 Type) const
    {
        const FJCVFeatureGroup* fg = GetFeatureGroup(Type);
        if (fg)
        {
            return fg->HasCells();
        }
        return false;
    }

    FORCEINLINE bool HasCells(uint8 Type, int32 Index) const
    {
        const FJCVFeatureGroup* fg = GetFeatureGroup(Type);
        if (fg)
        {
            return fg->HasCells(Index);
        }
        return false;
    }

    FORCEINLINE bool HasCellGroups(uint8 Type) const
    {
        const FJCVFeatureGroup* fg = GetFeatureGroup(Type);
        if (fg)
            return fg->HasCellGroups();
        return false;
    }

    FORCEINLINE int32 GetFeatureCount() const
    {
        return FeatureGroups.Num();
    }

    FORCEINLINE bool HasFeature(int32 FeatureType) const
    {
        return FeatureGroups.IsValidIndex(FeatureType);
    }

    FORCEINLINE const FJCVFeatureGroup* GetFeatureGroup(uint8 Type) const
    {
        return FeatureGroups.IsValidIndex(Type)
            ? &FeatureGroups[Type]
            : nullptr;
    }

    FORCEINLINE FJCVFeatureGroup* GetFeatureGroup(uint8 Type)
    {
        return FeatureGroups.IsValidIndex(Type)
            ? &FeatureGroups[Type]
            : nullptr;
    }

    FORCEINLINE int32 GetFeatureGroupCount(uint8 Type) const
    {
        return FeatureGroups.IsValidIndex(Type)
            ? FeatureGroups[Type].GetGroupCount()
            : 0;
    }

    FORCEINLINE FJCVCellGroup* GetFeatureCellGroup(uint8 Type, int32 Index)
    {
        return FeatureGroups.IsValidIndex(Type)
            ? FeatureGroups[Type].GetCellGroup(Index)
            : nullptr;
    }

    FORCEINLINE FJCVCellGroup* GetFeatureCellGroup(const FJCVFeatureId& FeatureId)
    {
        return GetFeatureCellGroup(FeatureId.Type, FeatureId.Index);
    }

    void GetFeatureIndices(TArray<int32>& FeatureIndices, uint8 Type, int32 Index, bool bFilterEmpty = false) const
    {
        // Invalid feature type, abort
        if (! FeatureGroups.IsValidIndex(Type))
        {
            return;
        }

        const FJCVFeatureGroup& FeatureGroup(FeatureGroups[Type]);

        if (Index < 0)
        {
            for (int32 fi=0; fi<FeatureGroup.GetGroupCount(); ++fi)
            {
                if (! bFilterEmpty || FeatureGroup.HasCells(fi))
                {
                    FeatureIndices.Emplace(fi);
                }
            }
        }
        else
        if (FeatureGroup.IsValidIndex(Index))
        {
            if (! bFilterEmpty || FeatureGroup.HasCells(Index))
            {
                FeatureIndices.Emplace(Index);
            }
        }
    }

    // -- FEATURE QUERY OPERATIONS (BORDERS)

    template<class ContainerType>
    void GetBorderCells(
        ContainerType& g,
        uint8 t0,
        uint8 t1,
        bool bAllowBorder = false,
        bool bAgainstAnyType = false
        )
    {
        FJCVCellGroup* f;
        int32 i=0;
        do
        {
            f = GetFeatureCellGroup(t0, i++);
            if (f)
            {
                g.Reserve(g.Num()+f->Num());
                GetBorderCells(g, t1, *f, bAllowBorder, bAgainstAnyType);
            }
        }
        while (f);
        g.Shrink();
    }

    template<class ContainerType>
    void GetBorderCells(
            ContainerType& g,
            uint8 t0,
            uint8 t1,
            int32 fi,
            bool bAllowBorder = false,
            bool bAgainstAnyType = false
            )
    {
        FJCVCellGroup* f = GetFeatureCellGroup(t0, fi);
        if (f)
        {
            g.Reserve(f->Num());
            GetBorderCells(g, t1, *f, bAllowBorder, bAgainstAnyType);
            g.Shrink();
        }
    }

    template<class CellContainer, class GraphContainer>
    void GetBorderEdges(
        const CellContainer& cg,
        GraphContainer& eg,
        uint8 t,
        bool bAllowBorder = false
        ) const
    {
        for (const FJCVCell* c : cg)
        {
            const FJCVEdge* g = c->GetEdge();
            check(g != nullptr);
            do
            {
                const FJCVCell* n = GetCell(g->neighbor);
                if ((bAllowBorder && !n) || (n && n->IsType(t)))
                    eg.Emplace(g);
            }
            while ((g=g->next) != nullptr);
        }
    }

    template<class CellContainer, class GraphContainer>
    void GetBorderEdges(
        const CellContainer& cg,
        GraphContainer& eg,
        bool bAllowBorder = false
        ) const
    {
        for (const FJCVCell* c : cg)
        {
            const FJCVEdge* g = c->GetEdge();
            const uint8 t = c->FeatureType;
            check(g != nullptr);
            do
            {
                const FJCVCell* n = GetCell(g->neighbor);
                if ((bAllowBorder && !n) || (n && ! n->IsType(t)))
                    eg.Emplace(g);
            }
            while ((g=g->next) != nullptr);
        }
    }

    // -- FEATURE QUERY OPERATIONS (JUNCTIONS)

    void GetJunctionCells(FJCVCell& c, TArray<FJCVCellJunction>& Junctions);

    // -- CELL QUERY OPERATIONS (CONST)

    FORCEINLINE bool IsValidIndex(int32 i) const
    {
        return Cells.IsValidIndex(i);
    }

    FORCEINLINE bool IsValidCell(const FJCVCell* Cell) const
    {
        if (Cell && Cell->IsValid())
        {
            return IsValidIndex(Cell->GetIndex())
                ? GetCell(Cell->GetIndex()).Site == Cell->Site
                : false;
        }
        return false;
    }

    FORCEINLINE int32 Num() const
    {
        return Cells.Num();
    }

    FORCEINLINE const FJCVSite* GetSite(int32 i) const
    {
        return Cells[i].Site;
    }

    FORCEINLINE const FJCVCell* GetCell(const FJCVSite* s) const
    {
        return s ? &GetCell(s->index) : nullptr;
    }

    FORCEINLINE const FJCVCell* GetCell(const FJCVEdge* g) const
    {
        if (g)
        {
            return (g->edge->sites[0] == g->neighbor)
                ? GetCell(g->edge->sites[1])
                : GetCell(g->edge->sites[0]);
        }
        return nullptr;
    }

    FORCEINLINE const FJCVCell& GetCell(int32 i) const
    {
        return Cells[i];
    }

    FORCEINLINE const FJCVCell* GetCellNeighbour(const FJCVEdge* g) const
    {
        return g ? GetCell(g->neighbor) : nullptr;
    }

    FORCEINLINE int32 GetCellIndex(const FJCVCell* Cell) const
    {
        return Cell ? Cell->GetIndex() : -1;
    }

    FORCEINLINE bool IsFeatureBorder(const FJCVCell& c, bool bTestBorder = false) const
    {
        const FJCVEdge* g = c.GetEdge();
        const uint8 t = c.FeatureType;
        check(g != nullptr);
        while (g)
        {
            const FJCVCell* n = GetCell(g->neighbor);
            if ((bTestBorder && ! n) || (n && ! n->IsType(t)))
                return true;
            g = g->next;
        }
        return false;
    }

    FORCEINLINE bool HasNeighbourType(const FJCVCell& c, uint8 t, bool bTestBorder = false) const
    {
        const FJCVEdge* g = c.GetEdge();
        check(g != nullptr);
        while (g)
        {
            const FJCVCell* n = GetCell(g->neighbor);
            if ((bTestBorder && ! n) || (n && n->IsType(t)))
                return true;
            g = g->next;
        }
        return false;
    }

    template<class ContainerType>
    void GetNeighbourTypes(const FJCVCell& c, ContainerType& Types) const
    {
        const FJCVEdge* g = c.GetEdge();
        const uint8 t = c.FeatureType;

        check(g != nullptr);

        TSet<uint8> NeighbourTypes;

        // Find neighbouring types
        while (g)
        {
            const FJCVCell* n = GetCell(g->neighbor);
            if (n && ! n->IsType(t) && ! NeighbourTypes.Contains(n->FeatureType))
            {
                NeighbourTypes.Emplace(n->FeatureType);
            }
            g = g->next;
        }

        // Store neighbouring types, if any
        Types.Reserve(NeighbourTypes.Num());
        for (uint8 nt : NeighbourTypes)
        {
            Types.Emplace(nt);
        }
    }

    FORCEINLINE bool IsJunctionType(const FJCVCell& c) const
    {
        const FJCVEdge* g = c.GetEdge();
        const uint8 t = c.FeatureType;

        check(g != nullptr);

        TSet<uint8> NeighbourTypes;

        // Find cell with multiple neighbouring types
        while (g)
        {
            const FJCVCell* n = GetCell(g->neighbor);
            if (n && ! n->IsType(t) && ! NeighbourTypes.Contains(n->FeatureType))
            {
                NeighbourTypes.Emplace(n->FeatureType);
            }
            g = g->next;
        }

        return NeighbourTypes.Num() > 1;
    }

    // -- CELL QUERY OPERATIONS

    FORCEINLINE FJCVCell* GetCell(const FJCVSite* s)
    {
        return s ? &GetCell(s->index) : nullptr;
    }

    FORCEINLINE FJCVCell* GetCell(const FJCVEdge* g)
    {
        if (g)
        {
            return (g->edge->sites[0] == g->neighbor)
                ? GetCell(g->edge->sites[1])
                : GetCell(g->edge->sites[0]);
        }
        return nullptr;
    }

    FORCEINLINE FJCVCell& GetCell(int32 i)
    {
        return Cells[i];
    }

    FORCEINLINE FJCVCell* GetCellNeighbour(const FJCVEdge* g)
    {
        return g ? GetCell(g->neighbor) : nullptr;
    }

    void GetAdjacentEdges(
        const FJCVEdge* g,
        const FJCVEdge*& gPrev,
        const FJCVEdge*& gNext
        )
    {
        gPrev = nullptr;
        gNext = nullptr;

        // Invalid source edge, abort
        if (! g)
        {
            return;
        }

        check(GetCell(g) != nullptr);

        const FJCVSite* s = GetCell(g)->Site;
        check(s != nullptr);

        // Find previous edge

        {
            const FJCVEdge* sg = s->edges;

            check(sg != nullptr);

            while (sg->next != nullptr)
            {
                const FJCVEdge* sgn = sg->next;

                if (sgn == g)
                {
                    gPrev = sg;
                    break;
                }

                sg = sgn;
            }

            check(sg != nullptr);

            // Previous edge still not found, check for point connections
            // between the last and first edge
            if (! gPrev && IsConnected(sg, s->edges))
            {
                gPrev = sg;
            }
        }

        // Find next edge, get edge .next property unless it is null
        // which point check against the first site edge is required

        if (g->next)
        {
            gNext = g->next;
        }
        else
        if (IsConnected(g, s->edges))
        {
            gNext = s->edges;
        }
    }

    template<class FContainerType>
    void GetNeighbourCells(const FJCVCell& c, FContainerType& OutCells)
    {
        if (const FJCVSite* Site = c.Site)
        {
            TArray<const FJCVSite*> Neighbours;
            Diagram.GetNeighbours(*Site, Neighbours);

            for (const FJCVSite* ns : Neighbours)
            {
                check(ns != nullptr);
                check(GetCell(ns) != nullptr);

                OutCells.Emplace(GetCell(ns));
            }
        }
    }

    template<class FCallback>
    FORCEINLINE void VisitNeighbours(const FJCVCell& c, const FCallback& Callback) const
    {
        if (const FJCVSite* Site = c.Site)
        {
            TFunctionRef<void(const FJCVSite*)> SiteCallback(
                [&](const FJCVSite* NeighbourSite)
                {
                    check(NeighbourSite != nullptr);
                    check(GetCell(NeighbourSite) != nullptr);
                    Callback(GetCell(NeighbourSite));
                } );

            Diagram.VisitNeighbours(*Site, SiteCallback);
        }
    }

    template<class FCallback>
    void VisitCells(const FCallback& Callback, const FJCVCell* CellToSkip = nullptr)
    {
        for (FJCVCell& Cell : Cells)
        {
            if (CellToSkip && CellToSkip == &Cell)
            {
                continue;
            }

            Callback(Cell);
        }
    }

    template<class FCallback>
    void VisitCellDuals(const FJCVCell& Cell, const FCallback& Callback)
    {
        // Invalid input cell, abort
        if (! Cell.IsValid())
        {
            return;
        }

        const FJCVSite* Site(Cell.Site);
        const FJCVEdge* g0 = Site->edges;
        const FJCVEdge* g1 = nullptr;

        if (g0 && g0->next)
        {
            // Visit site points starting from the second point
            while ((g1=g0->next) != nullptr)
            {
                check(FJCVMathUtil::IsEqual(g0->pos[1], g1->pos[0]));

                const FJCVPoint Point(g0->pos[1]);
                const FJCVCell* Neighbour0(GetCell(g0->neighbor));
                const FJCVCell* Neighbour1(GetCell(g1->neighbor));

                // Make sure edge point have complete dual cells
                if (Neighbour0 && Neighbour1)
                {
                    Callback(Point, Cell, *Neighbour0, *Neighbour1);
                }

                g0 = g1;
            }

            // Visit the first site point
            g1 = Site->edges;
            {
                check(g0 != nullptr);
                check(g1 != nullptr);
                check(FJCVMathUtil::IsEqual(g0->pos[1], g1->pos[0]));

                const FJCVPoint Point(g0->pos[1]);
                const FJCVCell* Neighbour0(GetCell(g0->neighbor));
                const FJCVCell* Neighbour1(GetCell(g1->neighbor));

                // Make sure edge point have complete dual cells
                if (Neighbour0 && Neighbour1)
                {
                    Callback(Point, Cell, *Neighbour0, *Neighbour1);
                }
            }
        }
    }

    template<class FContainerType>
    FORCEINLINE void GetCellPointValues(FContainerType& OutPoints, const FJCVCell& Cell, uint8 FeatureType = 0xFF, int32 FeatureIndex = -1, bool bFilterByType = false) const
    {
        if (! Cell.IsValid())
        {
            return;
        }

        const FJCVSite* Site(Cell.Site);
        const FJCVEdge* g0 = Site->edges;
        const FJCVEdge* g1 = nullptr;

        const float CellValue = Cell.GetValue();

        if (g0 && g0->next)
        {
            // Visit site points starting from the second point
            while ((g1=g0->next) != nullptr)
            {
                check(FJCVMathUtil::IsEqual(g0->pos[1], g1->pos[0]));

                const FJCVPoint Point(g0->pos[1]);
                const FJCVCell* Neighbour0(GetCell(g0->neighbor));
                const FJCVCell* Neighbour1(GetCell(g1->neighbor));

                const float v0 = Neighbour0 ? Neighbour0->GetValue() : 0.f;
                const float v1 = Neighbour1 ? Neighbour1->GetValue() : 0.f;
                float PointValue = (v0+v1+CellValue) / 3.f;

                if (bFilterByType)
                {
                    if ((Neighbour0 && ! Neighbour0->IsType(FeatureType, FeatureIndex)) ||
                        (Neighbour1 && ! Neighbour1->IsType(FeatureType, FeatureIndex)))
                    {
                        PointValue = 0.f;
                    }
                }

                OutPoints.Emplace(Point.x, Point.y, PointValue);

                g0 = g1;
            }

            // Visit the first site point
            g1 = Site->edges;
            {
                check(g0 != nullptr);
                check(g1 != nullptr);
                check(FJCVMathUtil::IsEqual(g0->pos[1], g1->pos[0]));

                const FJCVPoint Point(g0->pos[1]);
                const FJCVCell* Neighbour0(GetCell(g0->neighbor));
                const FJCVCell* Neighbour1(GetCell(g1->neighbor));

                const float v0 = Neighbour0 ? Neighbour0->GetValue() : 0.f;
                const float v1 = Neighbour1 ? Neighbour1->GetValue() : 0.f;
                float PointValue = (v0+v1+CellValue) / 3.f;

                if (bFilterByType)
                {
                    if ((Neighbour0 && ! Neighbour0->IsType(FeatureType, FeatureIndex)) ||
                        (Neighbour1 && ! Neighbour1->IsType(FeatureType, FeatureIndex)))
                    {
                        PointValue = 0.f;
                    }
                }

                OutPoints.Emplace(Point.x, Point.y, PointValue);
            }
        }
    }

    FORCEINLINE const FJCVEdge* FindEdgeConnection(const FJCVCell& c0, const FJCVCell& c1) const
    {
        const FJCVEdge* g0 = c0.GetEdge();
        const FJCVEdge* g1 = c1.GetEdge();

        check(g0 != nullptr);
        check(g1 != nullptr);

        do
        {
            do
            {
                if (connected(*g0, *g1) != -1)
                {
                    return g0;
                }
            }
            while ((g1=g1->next) != nullptr);
        }
        while ((g0=g0->next) != nullptr);

        return nullptr;
    }

    FORCEINLINE bool IsConnected(const FJCVEdge* c0, const FJCVEdge* c1) const
    {
        return (c0 && c1)
            ? connected(*c0, *c1) != -1 || connectedRev(*c0, *c1) != -1
            : false;
    }

    FORCEINLINE bool IsConnected(const FJCVEdge& c0, const FJCVEdge& c1) const
    {
        return connected(c0, c1) != -1 || connectedRev(c0, c1) != -1;
    }

    FORCEINLINE bool IsConnected(const FVector2D& pt, const FJCVEdge& edge) const
    {
        return connected(pt, edge) != -1;
    }

private:

    FJCVDiagramContext& Diagram;
    TArray<FJCVCell> Cells;
    TArray<FJCVFeatureGroup> FeatureGroups;

    void Init(uint8 FeatureType, int32 FeatureIndex);

    template<class ContainerType>
    FORCEINLINE void GetBorderCells(ContainerType& g, uint8 t, FJCVCellGroup& f, bool bAllowBorder=false, bool bAgainstAnyType=false)
    {
        if (bAgainstAnyType)
        {
            for (FJCVCell* c : f)
                if (IsFeatureBorder(*c, bAllowBorder))
                    g.Emplace(c);
        }
        else
        {
            for (FJCVCell* c : f)
                if (HasNeighbourType(*c, t, bAllowBorder))
                    g.Emplace(c);
        }
    }

    FORCEINLINE static int32 connected(const FVector2D& Pt, const FJCVEdge& g)
    {
        if (FMath::IsNearlyEqual(Pt.X, g.pos[0].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(Pt.Y, g.pos[0].y, JCV_EQUAL_THRESHOLD))
            return 0;
        if (FMath::IsNearlyEqual(Pt.X, g.pos[1].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(Pt.Y, g.pos[1].y, JCV_EQUAL_THRESHOLD))
            return 1;
        return -1;
    }

    FORCEINLINE static int32 connected(const FJCVEdge& g0, const FJCVEdge& g1)
    {
        if (FMath::IsNearlyEqual(g0.pos[1].x, g1.pos[0].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(g0.pos[1].y, g1.pos[0].y, JCV_EQUAL_THRESHOLD))
            return 0;
        if (FMath::IsNearlyEqual(g0.pos[1].x, g1.pos[1].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(g0.pos[1].y, g1.pos[1].y, JCV_EQUAL_THRESHOLD))
            return 1;
        return -1;
    }

    FORCEINLINE static int32 connectedRev(const FJCVEdge& g0, const FJCVEdge& g1)
    {
        if (FMath::IsNearlyEqual(g0.pos[0].x, g1.pos[0].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(g0.pos[0].y, g1.pos[0].y, JCV_EQUAL_THRESHOLD))
            return 1;
        if (FMath::IsNearlyEqual(g0.pos[0].x, g1.pos[1].x, JCV_EQUAL_THRESHOLD) &&
            FMath::IsNearlyEqual(g0.pos[0].y, g1.pos[1].y, JCV_EQUAL_THRESHOLD))
            return 0;
        return -1;
    }
};

class FJCVDiagramMapContext
{
public:

    FJCVDiagramMapContext() = default;

    FJCVDiagramMapContext(FVector2D Size, TArray<FVector2D>& Points)
    {
        GenerateDiagram(Size, Points);
    }

    FJCVDiagramMapContext(FBox2D Bounds, TArray<FVector2D>& Points)
    {
        GenerateDiagram(Bounds, Points);
    }

    void GenerateDiagram(FVector2D Size, TArray<FVector2D>& Points)
    {
        Diagram.GenerateDiagram(Size, Points);
    }

    void GenerateDiagram(FBox2D Bounds, TArray<FVector2D>& Points)
    {
        Diagram.GenerateDiagram(Bounds, Points);
    }

    FORCEINLINE bool HasMap(int32 i) const
    {
        return MapGroups.IsValidIndex(i) ? MapGroups[i].IsValid() : false;
    }

    FORCEINLINE FJCVDiagramMap& ResetMap(int32 i)
    {
        if (HasMap(i))
            MapGroups[i] = MakeShareable( new FJCVDiagramMap(Diagram) );
    }

    FORCEINLINE FJCVDiagramMap& CreateMap(int32 MapId = 0, bool bReset = false)
    {
        return CreateMap(MapId, JCV_CF_UNMARKED, 0, bReset);
    }

    FJCVDiagramMap& CreateMap(
        int32 MapId = 0,
        uint8 FeatureType = JCV_CF_UNMARKED,
        int32 FeatureIndex = 0,
        bool bReset = false
        )
    {
        const int32 Index = MapId<0 ? 0 : MapId;

        if (! MapGroups.IsValidIndex(Index))
        {
            MapGroups.SetNum(Index+1);
        }

        if (bReset || ! MapGroups[Index].IsValid())
        {
            MapGroups[Index] = MakeShareable(
                new FJCVDiagramMap(
                    Diagram,
                    FeatureType,
                    FeatureIndex
                    )
                );
        }

        return *MapGroups[Index];
    }

    FORCEINLINE FJCVDiagramMap& CopyMap(int32 SrcId, int32 DstId)
    {
        check(MapGroups.IsValidIndex(SrcId));
        check(SrcId != DstId);

        if (! MapGroups.IsValidIndex(DstId))
        {
            MapGroups.SetNum(DstId+1);
        }

        MapGroups[DstId] = MakeShareable(new FJCVDiagramMap(*MapGroups[SrcId]));

        return *MapGroups[DstId];
    }

    FORCEINLINE FJCVDiagramMap& GetMap(int32 i = 0)
    {
        return *MapGroups[i];
    }

private:

    FJCVDiagramContext Diagram;
    TArray<FPSJCVDiagramMap> MapGroups;
};
