// 

#pragma once

#include "SharedPointer.h"
#include "UnrealMathUtility.h"
#include "UnrealMemory.h"
#include "Queue.h"
#include "Set.h"

#include "JCVDiagram.h"

typedef TSharedPtr<struct FJCVIslandContext> TPSJCVIslandContext;
typedef TWeakPtr<struct FJCVIslandContext>   TPWJCVIslandContext;

typedef TSharedPtr<struct FJCVIsland> TPSJCVIsland;
typedef TWeakPtr<struct FJCVIsland>   TPWJCVIsland;

enum EJCVCellFeature
{
    UNDEFINED   = 255,
    OCEAN       = 0,
    ISLAND      = 1,
    LAKE        = 2
};

struct FJCVCell
{
    const FJCVSite* Site = nullptr;
    float Value          = 0.f;
    bool bIsBorder       = false;
    uint8 FeatureType    = EJCVCellFeature::UNDEFINED;
    int32 FeatureIndex   = 0;

    FJCVCell(const FJCVSite& s, float h, bool b)
        : Site(&s)
        , Value(h)
        , bIsBorder(b)
        , FeatureType(EJCVCellFeature::UNDEFINED)
        , FeatureIndex(0)
    {
    }

    FJCVCell(const FJCVSite& s, float h, bool b, uint8 t, uint8 n)
        : Site(&s)
        , Value(h)
        , bIsBorder(b)
        , FeatureType(t)
        , FeatureIndex(n)
    {
    }

    FORCEINLINE int32 index() const
    {
        return Site ? Site->index : -1;
    }

    FORCEINLINE FVector2D V2D() const
    {
        if (Site)
            return FVector2D(Site->p.x, Site->p.y);
        return FVector2D(0.f,0.f);
    }

    FORCEINLINE FJCVGraphEdge* Graph() const
    {
        return Site ? Site->edges : nullptr;
    }

    FORCEINLINE bool IsBorder() const
    {
        return bIsBorder;
    }

    FORCEINLINE bool IsType(uint8 t, int32 i = -1) const
    {
        return FeatureType == t && (i<0 || FeatureIndex == i);
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

struct FJCVGraphNode
{
    const FJCVGraphEdge* Graph;
    FJCVPoint MidPt;
    int32 Order;

    FJCVGraphNode() = default;

    FJCVGraphNode(const FJCVGraphEdge* g, int32 c) : Graph(g) , Order(c)
    {
        const FJCVPoint& p0( (*this)[0] );
        const FJCVPoint& p1( (*this)[1] );
        MidPt.x = p0.x+(p1.x-p0.x)*.5f;
        MidPt.y = p0.y+(p1.y-p0.y)*.5f;
    }

    FORCEINLINE const FJCVPoint& mid() const
    {
        return MidPt;
    }

    FORCEINLINE FVector2D vmid() const
    {
        return FJCVMathUtil::AsV2D(MidPt);
    }

    FORCEINLINE const FJCVPoint& operator()() const
    {
        return Graph->pos[Order];
    }

    FORCEINLINE const FJCVGraphEdge* operator->() const
    {
        return Graph;
    }

    FORCEINLINE const FJCVPoint& operator[](int32 i) const
    {
        if (i == 0)
            return Graph->pos[Order==0?0:1];
        if (i == 1)
            return Graph->pos[Order==0?1:0];
        return MidPt;
    }
};

typedef TArray<FJCVCell*> FJCVCellGroup;
typedef TSet<FJCVCell*>   FJCVCellSet;

typedef TArray<const FJCVGraphEdge*> FJCVGraphGroup;
typedef TSet<const FJCVGraphEdge*>   FJCVGraphSet;

struct FJCVFeatureGroup
{
    typedef FJCVCell        FCell;
    typedef FJCVCellGroup   FCellGroup;

    uint8 FeatureType;
    TArray<FCellGroup> CellGroups;
    TSet<uint8> Neighbours;

    FJCVFeatureGroup() = default;

    FORCEINLINE int32 GetGroupCount() const
    {
        return CellGroups.Num();
    }

    FORCEINLINE int32 GetCellCount() const
    {
        int32 count = 0;
        for (const FCellGroup& cg : CellGroups)
            count += cg.Num();
        return count;
    }

    FORCEINLINE bool HasNeighbour(uint8 n) const
    {
        return Neighbours.Contains(n);
    }

    FORCEINLINE bool HasCellGroups() const
    {
        return GetGroupCount() > 0;
    }

    FORCEINLINE bool HasCells() const
    {
        if (! HasCellGroups())
            return false;
        for (const FCellGroup& cg : CellGroups)
            if (cg.Num() > 0)
                return true;
        return false;
    }

    FORCEINLINE bool HasCells(int32 i) const
    {
        if (i < 0)
            return HasCells();
        if (CellGroups.IsValidIndex(i))
            return CellGroups[i].Num() > 0;
        return false;
    }

    FORCEINLINE void SetType(uint8 t)
    {
        FeatureType = t;
    }

    FORCEINLINE void AddCell(FCell& c, int32 r = -1)
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
        for (FCellGroup& g : CellGroups)
            g.Shrink();
    }

    FORCEINLINE void Empty()
    {
        CellGroups.Empty();
    }
};

struct FJCVIsland
{
    typedef FJCVSite         FSite;
    typedef FJCVGraphEdge    FGraph;
    typedef FJCVCell         FCell;
    typedef FJCVCellGroup    FCellGroup;
    typedef FJCVFeatureGroup FFeatureGroup;

    FJCVIsland(FJCVDiagramContext& d)
        : Diagram(d)
    {
        if (Diagram.IsEmpty()) return;

        const int32 CellCount = Diagram.GetSiteNum();
        const FSite* sites( Diagram.GetSites() );

        Cells.SetNumUninitialized( CellCount );

        for (int32 i=0; i<CellCount; ++i)
        {
            bool bIsBorder = false;
            const FSite& s( sites[i] );
            const FGraph* g( s.edges );
            while (g)
            {
                if (! g->neighbor)
                {
                    bIsBorder = true;
                    break;
                }
                g = g->next;
            }
            Cells[s.index] = FCell(s, 0.f, bIsBorder);
        }
    }

    // -------------------------------------------------------------
    // FEATURE OPERATIONS
    // -------------------------------------------------------------

    void ResetFeatures(uint8 FeatureType, int32 FeatureIndex)
    {
        if (FeatureIndex < 0)
        {
            const FFeatureGroup* pft = GetFeatureGroup(FeatureType);
            if (! pft)
                return;
            const FFeatureGroup& ft( *pft );

            for (int32 i=0; i<ft.GetGroupCount(); ++i)
            {
                bool bResult = true;
                const FCellGroup& fg( ft.CellGroups[i] );
                for (FCell* c : fg)
                    if (c)
                        c->SetType(EJCVCellFeature::UNDEFINED);
            }
        }
        else
        {
            FCellGroup* f = GetCellsByFeature(FeatureType, FeatureIndex);
            if (f)
            {
                const FCellGroup& fg( *f );
                for (FCell* c : fg)
                    if (c)
                        c->SetType(EJCVCellFeature::UNDEFINED);
            }
        }
    }

    void GroupByFeatures()
    {
        FeatureGroups.Reset();
        const int32 cellN = Num();
        for (FCell& c : Cells)
        {
            if (c.FeatureIndex < 0)
                continue;
            if (! FeatureGroups.IsValidIndex(c.FeatureType))
                FeatureGroups.SetNum(c.FeatureType+1);
            FeatureGroups[c.FeatureType].AddCell(c, cellN);
        }
        for (int32 i=0; i<GetFeatureCount(); ++i)
            FeatureGroups[i].SetType((uint8) i);
        for (FFeatureGroup& fg : FeatureGroups)
            fg.Shrink();
    }

    void ExpandFeature(uint8 ft, int32 fi)
    {
        if (! HasCells(ft, fi))
            return;

        FCellGroup& cellG( FeatureGroups[ft].CellGroups[fi] );
        FJCVCellSet cellS( cellG );

        for (FCell* c : cellG)
        {
            check(c);
            FGraph* g( c->Graph() );
            while (g)
            {
                FCell* n( Cell(g->neighbor) );
                if (n && ! cellS.Contains(n))
                {
                    cellS.Emplace(n);
                    n->SetType(ft, fi);
                }
                g = g->next;
            }
        }
    }

    void ExpandFeature(uint8 ft)
    {
        if (! HasCellGroups(ft))
            return;

        FFeatureGroup& featureGroup( FeatureGroups[ft] );

        for (int32 fi=0; fi<featureGroup.GetGroupCount(); ++fi)
        {
            if (! featureGroup.HasCells(fi))
                continue;

            FCellGroup& cellG( featureGroup.CellGroups[fi] );
            FJCVCellSet cellS( cellG );

            for (FCell* c : cellG)
            {
                check(c);
                FGraph* g( c->Graph() );
                while (g)
                {
                    FCell* n( Cell(g->neighbor) );
                    if (n && ! cellS.Contains(n))
                    {
                        cellS.Emplace(n);
                        n->SetType(ft, fi);
                    }
                    g = g->next;
                }
            }
        }
    }

    void ExpandFeature(FJCVCellSet& cellS, uint8 ft, int32 fi)
    {
        FJCVCellGroup cellG( cellS.Array() );
        for (FCell* c : cellG)
        {
            if (! c)
                continue;
            FGraph* g( c->Graph() );
            while (g)
            {
                FCell* n( Cell(g->neighbor) );
                if (n && ! cellS.Contains(n))
                {
                    cellS.Emplace(n);
                    n->SetType(ft, fi);
                }
                g = g->next;
            }
        }
    }

    void ConnectFeatures()
    {
        for (FFeatureGroup& fg : FeatureGroups)
        {
            for (const FCellGroup& cg : fg.CellGroups)
            {
                for (const FCell* c : cg)
                {
                    const FGraph* g = c->Graph();
                    do
                    {
                        const FCell* n( Cell(g->neighbor) );
                        if (n)
                            fg.AddNeighbour(*n);
                    }
                    while ((g=g->next) != nullptr);
                }
            }
        }
    }

    FORCEINLINE bool MarkFiltered(const FSite* InSite, uint8 FeatureType, int32 FeatureIndex, TSet<const FSite*> FilterSet, bool bAddToFilterIfMarked = false)
    {
        if (InSite && ! FilterSet.Contains(InSite))
        {
            FCell* c = Cell(InSite);
            if (c)
            {
                c->SetType(FeatureType, FeatureIndex);
                if (bAddToFilterIfMarked)
                    FilterSet.Emplace(InSite);
                return true;
            }
        }
        return false;
    }

    void MergeGroup(FFeatureGroup& fg0, FFeatureGroup& fg1)
    {
        const int32 cellN = Num();
        const uint8 ft0 = fg0.FeatureType;
        const uint8 ft1 = fg1.FeatureType;
        // Merges cells
        for (int32 fi=0; fi<fg1.GetGroupCount(); ++fi)
        {
            FCellGroup& cg( fg1.CellGroups[fi] );
            for (FCell* c : cg)
            {
                c->SetType(ft0, fi);
                fg0.AddCell(*c, cellN);
            }
            cg.Empty();
        }
        // Empties merged cell groups
        fg1.Empty();
    }

    void ConvertIsolated(uint8 ft0, uint8 ft1, int32 fi, bool bGroupFeatures=false)
    {
        const FFeatureGroup* pft = GetFeatureGroup(ft0);
        if (! pft)
            return;
        const FFeatureGroup& ft( *pft );

        for (int32 i=0; i<ft.GetGroupCount(); ++i)
        {
            bool bResult = true;
            const FCellGroup& fg( ft.CellGroups[i] );
            for (FCell* c : fg)
            {
                if (! c)
                    continue;
                const FGraph* g( c->Graph() );
                while (g)
                {
                    const FCell* n( Cell(g->neighbor) );
                    if (! n || (! n->IsType(ft0, i) && ! n->IsType(ft1)))
                    {
                        bResult = false;
                        break;
                    }
                    g = g->next;
                }
                if (! bResult)
                    break;
            }
            if (bResult)
                for (FCell* c : fg)
                    c->SetType(ft1, fi);
        }
        if (bGroupFeatures)
            GroupByFeatures();
    }

    void ConvertConnections(FFeatureGroup& fg0, FFeatureGroup& fg1)
    {
        const uint8 ft0 = fg0.FeatureType;
        const uint8 ft1 = fg1.FeatureType;
        // Update group connections
        for (uint8 ft2 : fg1.Neighbours)
        {
            if (ft2 != ft0 && ! fg0.HasNeighbour(ft2))
            {
                // Converts neighbour's feature group reference to self
                FFeatureGroup* fg2 = GetFeatureGroup(ft2);
                if (fg2)
                {
                    fg2->Neighbours.Remove(ft1);
                    fg2->Neighbours.Emplace(ft0);
                }
                // Merges neighbour feature group
                fg0.Neighbours.Emplace(ft2);
            }
        }
        // Converts merged group type
        fg1.Neighbours.Empty();
        // Removes stale groups from neighbour list
        TSet<uint8> nftS = fg0.Neighbours;
        for (uint8 n : nftS)
            if (! HasCellGroups(n))
                fg0.Neighbours.Remove(n);
    }

    void ShrinkGroups()
    {
        TArray<int32> validGroups;
        for (const FFeatureGroup& fg : FeatureGroups)
            if (fg.HasCells())
                validGroups.Emplace(fg.FeatureType);

        // No empty feature group, no changes required
        if (validGroups.Num() < 1 || validGroups.Num() == FeatureGroups.Num())
            return;

        const int32 groupN = validGroups.Num();
        int32 fgi = 0;
        // Map containing original to converted indices
        TMap<uint8, uint8> convertGroup;

        for (int32 i=0; i<groupN; ++i)
        {
            const int32 vgi = validGroups[i];
            // Filter groups with index already within shrink count
            if (vgi < groupN)
            {
                convertGroup.Emplace(vgi, vgi);
                continue;
            }
            while (validGroups.Contains(fgi))
            {
                fgi++;
            }

            check(FeatureGroups.IsValidIndex(fgi));

            FFeatureGroup& fg0( FeatureGroups[vgi] );
            FFeatureGroup& fg1( FeatureGroups[fgi] );

            // Merges cells
            MergeGroup(fg1, fg0);
            convertGroup.Emplace(vgi, fgi);

            // Merges neighbour indices
            fg1.Neighbours.Empty();
            for (uint8 n : fg0.Neighbours)
                fg1.Neighbours.Emplace(n);

            fgi++;
        }
        FeatureGroups.SetNum(groupN, true);

        for (FFeatureGroup& fg : FeatureGroups)
        {
            TSet<uint8> neighbours = fg.Neighbours;
            fg.Neighbours.Empty();
            for (uint8 n : neighbours)
                if (convertGroup.Contains(n))
                    fg.Neighbours.Emplace(convertGroup.FindChecked(n));
        }
    }

    FORCEINLINE bool HasCells(uint8 Type, int32 Index=-1) const
    {
        const FFeatureGroup* fg = GetFeatureGroup(Type);
        if (fg)
            return fg->HasCells(Index);
        return false;
    }

    FORCEINLINE bool HasCellGroups(uint8 Type) const
    {
        const FFeatureGroup* fg = GetFeatureGroup(Type);
        if (fg)
            return fg->HasCellGroups();
        return false;
    }

    FORCEINLINE int32 GetFeatureCount() const
    {
        return FeatureGroups.Num();
    }

    FORCEINLINE const FFeatureGroup* GetFeatureGroup(uint8 Type) const
    {
        if (FeatureGroups.IsValidIndex(Type))
            return &FeatureGroups[Type];
        return nullptr;
    }

    FORCEINLINE FFeatureGroup* GetFeatureGroup(uint8 Type)
    {
        if (FeatureGroups.IsValidIndex(Type))
            return &FeatureGroups[Type];
        return nullptr;
    }

    FCellGroup* GetCellsByFeature(uint8 Type, int32 Index)
    {
        if (FeatureGroups.IsValidIndex(Type))
            if (FeatureGroups[Type].CellGroups.IsValidIndex(Index))
                return &FeatureGroups[Type].CellGroups[Index];
        return nullptr;
    }

    template<class ContainerType>
    void GetBorderCells(ContainerType& g, uint8 t0, uint8 t1, bool bAllowBorder=false, bool bAgainstAnyType=false)
    {
        FCellGroup* f;
        int32 i=0;
        do
        {
            f = GetCellsByFeature(t0, i++);
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
    void GetBorderCells(ContainerType& g, uint8 t0, uint8 t1, int32 fi, bool bAllowBorder=false, bool bAgainstAnyType=false)
    {
        FCellGroup* f = GetCellsByFeature(t0, fi);
        if (f)
        {
            g.Reserve(f->Num());
            GetBorderCells(g, t1, *f, bAllowBorder, bAgainstAnyType);
            g.Shrink();
        }
    }

    template<class CellContainer, class GraphContainer>
    void GetBorderEdges(const CellContainer& cg, GraphContainer& eg, uint8 t, bool bAllowBorder=false)
    {
        for (FCell* c : cg)
        {
            const FGraph* g = c->Graph();
            do
            {
                const FCell* n( Cell(g->neighbor) );
                if ((bAllowBorder && !n) || (n && n->IsType(t)))
                    eg.Emplace(g);
            }
            while ((g=g->next) != nullptr);
        }
    }

    template<class CellContainer, class GraphContainer>
    void GetBorderEdges(const CellContainer& cg, GraphContainer& eg, bool bAllowBorder=false)
    {
        for (FCell* c : cg)
        {
            const FGraph* g = c->Graph();
            const uint8 t = c->FeatureType;
            do
            {
                const FCell* n( Cell(g->neighbor) );
                if ((bAllowBorder && !n) || (n && ! n->IsType(t)))
                    eg.Emplace(g);
            }
            while ((g=g->next) != nullptr);
        }
    }

    void SortEdges(const FJCVGraphSet& es, TArray<FJCVGraphNode>& eg)
    {
        if (es.Num() == 0)
        {
            return;
        }

        const FGraph* g = *es.CreateConstIterator();

        // Creates transient graph set copy
        FJCVGraphSet s( es );

        s.Remove(g);
        eg.Emplace(g, 0);

        while (s.Num() > 0)
        {
            const FGraph* n = nullptr;
            int32 order = -1;
            for (const FGraph* e : s)
                if ((order = connected(*g, *(n=e))) >= 0)
                    break;
            if (order < 0)
                break;
            g = n;
            s.Remove(g);
            eg.Emplace(g, order);
        }
    }

    // -------------------------------------------------------------
    // QUERY OPERATIONS
    // -------------------------------------------------------------

    // CONST OPERATIONS

    FORCEINLINE bool HasNeighbourType(const FCell& c, uint8 t, bool bAllowBorder=false) const
    {
        const FGraph* g( c.Graph() );
        while (g)
        {
            const FCell* n( Cell(g->neighbor) );
            if ((bAllowBorder && !n) || (n && n->IsType(t)))
                return true;
            g = g->next;
        }
        return false;
    }

    FORCEINLINE bool HasNeighbourType(const FCell& c, bool bAllowBorder=false) const
    {
        const FGraph* g( c.Graph() );
        const uint8 t = c.FeatureType;
        while (g)
        {
            const FCell* n( Cell(g->neighbor) );
            if ((bAllowBorder && !n) || (n && ! n->IsType(t)))
                return true;
            g = g->next;
        }
        return false;
    }

    FORCEINLINE const FJCVDiagramContext& GetDiagram() const
    {
        return Diagram;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Diagram.IsEmpty();
    }

    FORCEINLINE bool IsValidIndex(int32 i) const
    {
        return Cells.IsValidIndex(i);
    }

    FORCEINLINE int32 Num() const
    {
        return Cells.Num();
    }

    FORCEINLINE const FSite* Site(int32 i) const
    {
        return Cells[i].Site;
    }

    FORCEINLINE const FCell* CellData() const
    {
        return Cells.GetData();
    }

    FORCEINLINE const FCell* Cell(const FSite* s) const
    {
        return s ? &Cell(s->index) : nullptr;
    }

    FORCEINLINE const FCell* Cell(const FGraph* g) const
    {
        return g ? Cell(g->neighbor) : nullptr;
    }

    FORCEINLINE const FCell& Cell(int32 i) const
    {
        return Cells[i];
    }

    FORCEINLINE const FCell& operator[](int32 i) const
    {
        return Cell(i);
    }

    FORCEINLINE const FJCVDiagramContext* operator->() const
    {
        return &Diagram;
    }

    // NON-CONST OPERATIONS

    FORCEINLINE FJCVDiagramContext& GetDiagram()
    {
        return Diagram;
    }

    FORCEINLINE FCell* CellData()
    {
        return Cells.GetData();
    }

    FORCEINLINE FCell* Cell(const FSite* s)
    {
        return s ? &Cell(s->index) : nullptr;
    }

    FORCEINLINE FCell* Cell(const FGraph* g)
    {
        return g ? Cell(g->neighbor) : nullptr;
    }

    FORCEINLINE FCell& Cell(int32 i)
    {
        return Cells[i];
    }

    FORCEINLINE FCell& operator[](int32 i)
    {
        return Cell(i);
    }

    FORCEINLINE FJCVDiagramContext* operator->()
    {
        return &Diagram;
    }

private:

    FJCVDiagramContext& Diagram;
    TArray<FCell> Cells;
    TArray<FFeatureGroup> FeatureGroups;

    template<class ContainerType>
    FORCEINLINE void GetBorderCells(ContainerType& g, uint8 t, FCellGroup& f, bool bAllowBorder=false, bool bAgainstAnyType=false)
    {
        if (bAgainstAnyType)
        {
            for (FCell* c : f)
                if (HasNeighbourType(*c, bAllowBorder))
                    g.Emplace(c);
        }
        else
        {
            for (FCell* c : f)
                if (HasNeighbourType(*c, t, bAllowBorder))
                    g.Emplace(c);
        }
    }

    FORCEINLINE static int32 connected(const FGraph& g0, const FGraph& g1)
    {
        if (FMath::IsNearlyEqual(g0.pos[1].x, g1.pos[0].x, .001f) && FMath::IsNearlyEqual(g0.pos[1].y, g1.pos[0].y, .001f))
            return 0;
        if (FMath::IsNearlyEqual(g0.pos[1].x, g1.pos[1].x, .001f) && FMath::IsNearlyEqual(g0.pos[1].y, g1.pos[1].y, .001f))
            return 1;
        return -1;
    }
};

struct FJCVIslandContext
{
    FJCVIslandContext() = default;

    FJCVIslandContext(FVector2D Size, TArray<FVector2D>& Points)
    {
        GenerateDiagram(Size, Points);
    }

    void GenerateDiagram(FVector2D Size, TArray<FVector2D>& Points)
    {
        Diagram.GenerateDiagram(Size, Points);
    }

    FORCEINLINE bool HasIsland(int32 i) const
    {
        return IslandGroups.IsValidIndex(i) ? IslandGroups[i].IsValid() : false;
    }

    FORCEINLINE FJCVIsland& ResetIsland(int32 i)
    {
        if (HasIsland(i))
            IslandGroups[i] = MakeShareable( new FJCVIsland(Diagram) );
    }

    FORCEINLINE FJCVIsland& CreateIsland(int32 i=0, bool bReset = false)
    {
        const int32 index = i<0 ? 0 : i;
        if (! IslandGroups.IsValidIndex(index))
            IslandGroups.SetNum(index+1);
        if (bReset || ! IslandGroups[index].IsValid())
            IslandGroups[index] = MakeShareable( new FJCVIsland(Diagram) );
        return *IslandGroups[index].Get();
    }

    FORCEINLINE FJCVIsland& GetIsland(int32 i = 0)
    {
        return *IslandGroups[i].Get();
    }

private:

    FJCVDiagramContext Diagram;
    TArray<TPSJCVIsland> IslandGroups;

};
