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

#include "JCVDiagramMap.h"

FJCVDiagramMap::FJCVDiagramMap(FJCVDiagramContext& d) : Diagram(d)
{
    Init(EJCVCellFeature::UNDEFINED, 0);
}

FJCVDiagramMap::FJCVDiagramMap(FJCVDiagramContext& d, uint8 FeatureType, int32 FeatureIndex) : Diagram(d)
{
    Init(FeatureType, FeatureIndex);
}

FJCVDiagramMap::FJCVDiagramMap(const FJCVDiagramMap& SrcMap)
    : Diagram(SrcMap.Diagram)
{
    // Copy cells

    Cells = SrcMap.Cells;

    // Copy feature groups

    const int32 FeatureGroupCount = SrcMap.GetFeatureCount();
    FeatureGroups.SetNum(FeatureGroupCount);

    for (int32 fgi=0; fgi<FeatureGroupCount; ++fgi)
    {
        const FJCVFeatureGroup& SrcFeatureGroup(*SrcMap.GetFeatureGroup(fgi));
        FJCVFeatureGroup& DstFeatureGroup(FeatureGroups[fgi]);

        const int32 CellGroupCount = SrcFeatureGroup.CellGroups.Num();
        DstFeatureGroup.CellGroups.SetNum(CellGroupCount);

        for (int32 cgi=0; cgi<CellGroupCount; ++cgi)
        {
            const FJCVCellGroup& SrcCellGroup(SrcFeatureGroup.CellGroups[cgi]);
            FJCVCellGroup& DstCellGroup(DstFeatureGroup.CellGroups[cgi]);

            DstCellGroup.SetNumUninitialized(SrcCellGroup.Num());

            for (int32 ci=0; ci<SrcCellGroup.Num(); ++ci)
            {
                DstCellGroup[ci] = GetCell(SrcCellGroup[ci]->Site);
            }
        }
    }
}

void FJCVDiagramMap::Init(uint8 FeatureType, int32 FeatureIndex)
{
    // Diagram is empty, no further action required
    if (Diagram.IsEmpty())
    {
        return;
    }

    const int32 CellCount = Diagram.GetSiteNum();
    const FJCVSite* Sites = Diagram.GetSites();

    Cells.SetNumUninitialized(CellCount);

    for (int32 i=0; i<CellCount; ++i)
    {
        bool bIsBorder = false;
        const FJCVSite& s(Sites[i]);
        const FJCVEdge* g(s.edges);

        while (g)
        {
            if (! g->neighbor)
            {
                bIsBorder = true;
                break;
            }

            g = g->next;
        }

        Cells[s.index] = FJCVCell(s, 0.f, bIsBorder, FeatureType, FeatureIndex);
    }
}

// -- FEATURE OPERATIONS

void FJCVDiagramMap::ClearFeatures()
{
    FeatureGroups.Empty();
}

void FJCVDiagramMap::ResetFeatures(uint8 FeatureType, int32 FeatureIndex)
{
    if (FeatureIndex < 0)
    {
        const FJCVFeatureGroup* pft = GetFeatureGroup(FeatureType);
        if (! pft)
            return;
        const FJCVFeatureGroup& ft( *pft );

        for (int32 i=0; i<ft.GetGroupCount(); ++i)
        {
            bool bResult = true;
            const FJCVCellGroup& fg( ft.CellGroups[i] );
            for (FJCVCell* c : fg)
                if (c)
                    c->SetType(EJCVCellFeature::UNDEFINED);
        }
    }
    else
    {
        FJCVCellGroup* f = GetCellsByFeature(FeatureType, FeatureIndex);
        if (f)
        {
            const FJCVCellGroup& fg( *f );
            for (FJCVCell* c : fg)
                if (c)
                    c->SetType(EJCVCellFeature::UNDEFINED);
        }
    }
}

void FJCVDiagramMap::GroupByFeatures()
{
    const int32 cellN = Num();

    FeatureGroups.Reset();

    for (FJCVCell& c : Cells)
    {
        if (c.HasFeatureType())
        {
            if (! FeatureGroups.IsValidIndex(c.FeatureType))
            {
                FeatureGroups.SetNum(c.FeatureType+1);
            }

            FeatureGroups[c.FeatureType].AddCell(c, cellN);
        }
    }

    for (int32 i=0; i<GetFeatureCount(); ++i)
    {
        FeatureGroups[i].SetType((uint8) i);
    }

    for (FJCVFeatureGroup& fg : FeatureGroups)
    {
        fg.Shrink();
    }
}

void FJCVDiagramMap::ExpandFeature(uint8 ft, int32 fi)
{
    if (! HasCells(ft, fi))
        return;

    FJCVCellGroup& cellG( FeatureGroups[ft].CellGroups[fi] );
    FJCVCellSet cellS( cellG );

    for (FJCVCell* c : cellG)
    {
        check(c);
        FJCVEdge* g = c->GetEdge();
        while (g)
        {
            FJCVCell* n = GetCell(g->neighbor);
            if (n && ! cellS.Contains(n))
            {
                cellS.Emplace(n);
                n->SetType(ft, fi);
            }
            g = g->next;
        }
    }
}

void FJCVDiagramMap::ExpandFeature(uint8 ft)
{
    if (! HasCellGroups(ft))
        return;

    FJCVFeatureGroup& featureGroup( FeatureGroups[ft] );

    for (int32 fi=0; fi<featureGroup.GetGroupCount(); ++fi)
    {
        if (! featureGroup.HasCells(fi))
        {
            continue;
        }

        FJCVCellGroup& cellG( featureGroup.CellGroups[fi] );
        FJCVCellSet cellS( cellG );

        for (FJCVCell* c : cellG)
        {
            check(c);
            FJCVEdge* g = c->GetEdge();
            while (g)
            {
                FJCVCell* n = GetCell(g->neighbor);
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

void FJCVDiagramMap::ExpandFeature(FJCVCellSet& cellS, uint8 ft, int32 fi)
{
    FJCVCellGroup cellG( cellS.Array() );
    for (FJCVCell* c : cellG)
    {
        if (! c)
            continue;
        FJCVEdge* g = c->GetEdge();
        while (g)
        {
            FJCVCell* n = GetCell(g->neighbor);
            if (n && ! cellS.Contains(n))
            {
                cellS.Emplace(n);
                n->SetType(ft, fi);
            }
            g = g->next;
        }
    }
}

void FJCVDiagramMap::GenerateNeighbourList()
{
    for (FJCVFeatureGroup& fg : FeatureGroups)
    {
        for (const FJCVCellGroup& cg : fg.CellGroups)
        {
            for (const FJCVCell* c : cg)
            {
                const FJCVEdge* g = c->GetEdge();
                do
                {
                    const FJCVCell* n = GetCell(g->neighbor);
                    if (n)
                        fg.AddNeighbour(*n);
                }
                while ((g=g->next) != nullptr);
            }
        }
    }
}

void FJCVDiagramMap::MarkFiltered(const FJCVSite* InSite, uint8 FeatureType, int32 FeatureIndex, TSet<const FJCVSite*>& FilterSet, bool bAddToFilterIfMarked)
{
    if (InSite && ! FilterSet.Contains(InSite))
    {
        FJCVCell* c = GetCell(InSite);

        if (c)
        {
            c->SetType(FeatureType, FeatureIndex);

            if (bAddToFilterIfMarked)
            {
                FilterSet.Emplace(InSite);
            }
        }
    }
}

void FJCVDiagramMap::MergeGroup(FJCVFeatureGroup& SrcFeatureGroup, FJCVFeatureGroup& DstFeatureGroup)
{
    const int32 cellN = Num();
    const uint8 srcft = SrcFeatureGroup.FeatureType;
    const uint8 dstft = DstFeatureGroup.FeatureType;

    // Add cells from SrcFeatureGroup to DstFeatureGroup, then clear SrcFeatureGroup
    for (int32 fi=0; fi<SrcFeatureGroup.GetGroupCount(); ++fi)
    {
        FJCVCellGroup& cg(SrcFeatureGroup.CellGroups[fi]);

        for (FJCVCell* c : cg)
        {
            c->SetType(dstft, fi);
            DstFeatureGroup.AddCell(*c, cellN);
        }

        cg.Empty();
    }

    // Clear merged cell group container
    SrcFeatureGroup.Empty();
}

void FJCVDiagramMap::ConvertIsolated(uint8 ft0, uint8 ft1, int32 fi, bool bGroupFeatures)
{
    const FJCVFeatureGroup* pfg = GetFeatureGroup(ft0);

    if (! pfg)
    {
        return;
    }

    const FJCVFeatureGroup& fg(*pfg);

    for (int32 i=0; i<fg.GetGroupCount(); ++i)
    {
        bool bResult = true;
        const FJCVCellGroup& cg(fg.CellGroups[i]);
        for (FJCVCell* c : cg)
        {
            if (! c)
                continue;
            const FJCVEdge* g = c->GetEdge();
            while (g)
            {
                const FJCVCell* n = GetCell(g->neighbor);
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
            for (FJCVCell* c : cg)
                c->SetType(ft1, fi);
    }

    if (bGroupFeatures)
    {
        GroupByFeatures();
    }
}

void FJCVDiagramMap::MergeNeighbourList(FJCVFeatureGroup& SrcFg, FJCVFeatureGroup& DstFg)
{
    const uint8 DstFt = DstFg.FeatureType;
    const uint8 SrcFt = SrcFg.FeatureType;

    // Update group connections
    for (uint8 ft2 : SrcFg.Neighbours)
    {
        if (ft2 != DstFt && ! DstFg.HasNeighbour(ft2))
        {
            // Converts neighbour's feature group reference to self
            FJCVFeatureGroup* fg2 = GetFeatureGroup(ft2);
            if (fg2)
            {
                fg2->Neighbours.Remove(SrcFt);
                fg2->Neighbours.Emplace(DstFt);
            }
            // Merges neighbour feature group
            DstFg.Neighbours.Emplace(ft2);
        }
    }

    // Converts merged group type
    SrcFg.Neighbours.Empty();

    // Removes stale groups from neighbour list
    TSet<uint8> nftS = DstFg.Neighbours;
    for (uint8 n : nftS)
        if (! HasCellGroups(n))
            DstFg.Neighbours.Remove(n);
}

void FJCVDiagramMap::ShrinkGroups()
{
    // No feature groups, abort
    if (FeatureGroups.Num() == 0)
    {
        return;
    }

    const int32 LastFeatureGroupCount = FeatureGroups.Num();

    // Remove empty feature groups
    FeatureGroups.RemoveAllSwap(
        [&](const FJCVFeatureGroup& fg) { return ! fg.HasCells(); },
        true
        );

    // Feature group count unchanged, no further update required, return
    if (LastFeatureGroupCount == FeatureGroups.Num())
    {
        return;
    }

    // Map of the original to the current feature type
    TMap<uint8, uint8> MergeGroupMap;

    // Map new feature type and update cell feature type if required
    for (int32 i=0; i<FeatureGroups.Num(); ++i)
    {
        FJCVFeatureGroup& fg(FeatureGroups[i]);
        const uint8 ft0 = fg.FeatureType;
        const uint8 ft1 = i;

        MergeGroupMap.Emplace(ft0, ft1);

        // Feature type unchanged, skip
        if (ft0 == ft1)
        {
            continue;
        }

        // Update cell feature type
        for (int32 fi=0; fi<fg.GetGroupCount(); ++fi)
        {
            for (FJCVCell* c : fg.CellGroups[fi])
            {
                c->SetType(ft1, fi);
            }
        }
    }

    // Remap neighbour list
    for (FJCVFeatureGroup& fg : FeatureGroups)
    {
        TSet<uint8> neighbours = fg.Neighbours;
        fg.Neighbours.Empty();
        for (uint8 n : neighbours)
        {
            if (MergeGroupMap.Contains(n))
            {
                fg.Neighbours.Emplace(MergeGroupMap.FindChecked(n));
            }
        }
    }
}

// -- FEATURE MODIFICATION OPERATIONS (BORDERS)

void FJCVDiagramMap::GenerateSortedBorderEdges(
    const FJCVConstEdgeSet& es,
    TArray<FJCVCellEdgeList>& EdgeLists
    )
{
    if (es.Num() == 0)
    {
        return;
    }

    // Create graph set copy
    FJCVConstEdgeSet s(es);

    // Find all edge point groups

    while (s.Num() > 0)
    {
        const int32 egIndex = EdgeLists.Num();
        EdgeLists.SetNum(egIndex+1, true);

        FJCVCellEdgeList& EdgeList(EdgeLists[egIndex]);
        FJCVCellEdgeList::FEdgePair& ep(EdgeList.EdgePair);
        FJCVCellEdgeList::FPointList& eg(EdgeList.PointList);

        const FJCVEdge* g = *s.CreateConstIterator();

        check(g != nullptr);

        s.Remove(g);
        eg.AddTail(FJCVMathUtil::ToVector2D(g->pos[0]));
        eg.AddTail(FJCVMathUtil::ToVector2D(g->pos[1]));

        // Find head connection until no connection is found or edge set is empty
        while (s.Num() > 0)
        {
            check(eg.GetHead() != nullptr);

            const FVector2D& pt(eg.GetHead()->GetValue());
            const FJCVEdge* n = nullptr;
            int32 order = -1;

            for (const FJCVEdge* e : s)
                if ((order = connected(pt, *(n=e))) >= 0)
                    break;

            if (order < 0)
                break;

            g = n;
            s.Remove(g);
            eg.AddHead(FJCVMathUtil::ToVector2D(g->pos[(order+1)%2]));
        }

        // Add last valid graph node as head end point edge
        check(g != nullptr);
        ep.Get<0>() = g;

        // Find tail connection until no connection is found or edge set is empty
        while (s.Num() > 0)
        {
            check(eg.GetTail() != nullptr);

            const FVector2D& pt(eg.GetTail()->GetValue());
            const FJCVEdge* n = nullptr;
            int32 order = -1;

            for (const FJCVEdge* e : s)
                if ((order = connected(pt, *(n=e))) >= 0)
                    break;

            if (order < 0)
                break;

            g = n;
            s.Remove(g);
            eg.AddTail(FJCVMathUtil::ToVector2D(g->pos[(order+1)%2]));
        }

        // Add last valid graph node as tail end point edge
        check(g != nullptr);
        ep.Get<1>() = g;
    }

    // Merge edge point groups that have connection at each head/tail points

#if 0
    int32 EdgeIt = 0;
    while (EdgeIt < EdgeGroups.Num())
    {
        TDoubleLinkedList<FVector2D>& egList0(EdgeGroups[EdgeIt]);
        bool bHasConnection = (EdgeIt+1) < EdgeGroups.Num();

        // Find valid candidate to merge with the current edge group
        // until none is available
        while (bHasConnection)
        {
            bHasConnection = false;
            int32 ListIt = EdgeIt+1;

            while (ListIt < EdgeGroups.Num())
            {
                TDoubleLinkedList<FVector2D>& egList1(EdgeGroups[ListIt]);

                // Remove and skip empty list
                if (egList1.Num() < 1)
                {
                    EdgeGroups.RemoveAtSwap(ListIt, 1, false);
                    continue;
                }

                // Find connection between merge candidate and the current
                // edge group, if connection found merge edge group and break loop.
                // Otherwise, advance loop iteration.

                const FVector2D& egHead0(egList0.GetHead()->GetValue());
                const FVector2D& egTail0(egList0.GetTail()->GetValue());
                const FVector2D& egHead1(egList1.GetHead()->GetValue());
                const FVector2D& egTail1(egList1.GetTail()->GetValue());

                // Tail0 <- Head1
                if (egTail0.Equals(egHead1, JCV_EQUAL_THRESHOLD))
                {
                    auto* egNode1 = egList1.GetHead() != nullptr
                        ? egList1.GetHead()->GetNextNode()
                        : nullptr;

                    while (egNode1)
                    {
                        egList0.AddTail(egNode1->GetValue());
                        egNode1 = egNode1->GetNextNode();
                    }

                    bHasConnection = true;
                    break;
                }

                // Tail0 <- Tail1
                if (egTail0.Equals(egTail1, JCV_EQUAL_THRESHOLD))
                {
                    auto* egNode1 = egList1.GetTail() != nullptr
                        ? egList1.GetTail()->GetPrevNode()
                        : nullptr;

                    while (egNode1)
                    {
                        egList0.AddTail(egNode1->GetValue());
                        egNode1 = egNode1->GetPrevNode();
                    }

                    bHasConnection = true;
                    break;
                }

                // Head0 <- Tail1
                if (egHead0.Equals(egTail1, JCV_EQUAL_THRESHOLD))
                {
                    auto* egNode1 = egList1.GetTail() != nullptr
                        ? egList1.GetTail()->GetPrevNode()
                        : nullptr;

                    while (egNode1)
                    {
                        egList0.AddHead(egNode1->GetValue());
                        egNode1 = egNode1->GetPrevNode();
                    }

                    bHasConnection = true;
                    break;
                }

                // Head0 <- Head1
                if (egHead0.Equals(egHead1, JCV_EQUAL_THRESHOLD))
                {
                    auto* egNode1 = egList1.GetHead() != nullptr
                        ? egList1.GetHead()->GetNextNode()
                        : nullptr;

                    while (egNode1)
                    {
                        egList0.AddHead(egNode1->GetValue());
                        egNode1 = egNode1->GetNextNode();
                    }

                    bHasConnection = true;
                    break;
                }

                // No valid connection found, advance iteration index
                ListIt++;
            }

            // If connection found, edge group has been merged, safe to remove
            if (bHasConnection)
            {
                EdgeGroups.RemoveAtSwap(ListIt, 1, false);
            }
        }

        // Current edge group has no valid merge candidate left, advance iteration index
        EdgeIt++;
    }
#endif

    EdgeLists.Shrink();
}

// -- FEATURE QUERY OPERATIONS (JUNCTIONS)

void FJCVDiagramMap::GetJunctionCells(FJCVCell& c, TArray<FJCVCellJunction>& Junctions)
{
    const uint8 t = c.FeatureType;

    FJCVCell* n0 = nullptr;
    FJCVCell* n1 = nullptr;

    const FJCVEdge* g0 = nullptr;
    const FJCVEdge* g1 = c.GetEdge();

    while (g1)
    {
        n1 = GetCell(g1->neighbor);

        if (n1 && ! n1->IsType(t))
        {
            if (g0 && n0 && ! n1->IsType(n0->FeatureType))
            {
                int32 conn0 = connected(*g0, *g1);
                int32 conn1 = connectedRev(*g0, *g1);
                if (conn0 > -1 || conn1 > -1)
                {
                    FVector2D Point = conn0 > -1
                        ? FJCVMathUtil::ToVector2D(g0->pos[1])
                        : FJCVMathUtil::ToVector2D(g0->pos[0]);
                    Junctions.Emplace(Point, FJCVCellGroup({&c, n0, n1}));
                }
            }

            n0 = n1;
            g0 = g1;
        }
        else
        {
            n0 = nullptr;
            g0 = nullptr;
        }

        g1 = g1->next;
    }

    // Check last edge connection against the first edge
    if (g0 && n0)
    {
        check(c.GetEdge() != nullptr);

        g1 = c.GetEdge();
        n1 = GetCell(g1->neighbor);

        if (n1 && ! n1->IsType(t) && ! n1->IsType(n0->FeatureType))
        {
            int32 conn0 = connected(*g0, *g1);
            int32 conn1 = connectedRev(*g0, *g1);
            if (conn0 > -1 || conn1 > -1)
            {
                FVector2D Point = conn0 > -1
                    ? FJCVMathUtil::ToVector2D(g0->pos[1])
                    : FJCVMathUtil::ToVector2D(g0->pos[0]);
                Junctions.Emplace(Point, FJCVCellGroup({&c, n0, n1}));
            }
        }
    }
}
