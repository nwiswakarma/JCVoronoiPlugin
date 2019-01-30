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

#include "JCVFeatureUtility.h"

void FJCVFeatureUtility::PointFill(FJCVDiagramMap& Map, const TArray<FJCVCell*>& seeds, uint8 FeatureTypeFilter)
{
    if (Map.IsEmpty() || seeds.Num() < 1)
    {
        return;
    }

    TQueue<FJCVCell*> CellQ;
    TSet<FJCVCell*> ExclusionSet;

    ExclusionSet.Reserve(Map.Num());

    for (FJCVCell* c : seeds)
    {
        if (c && ! ExclusionSet.Contains(c))
        {
            CellQ.Enqueue(c);
            ExclusionSet.Emplace(c);
        }
    }

    while (! CellQ.IsEmpty())
    {
        FJCVCell* Cell;
        CellQ.Dequeue(Cell);

        FJCVEdge* g = Cell->GetEdge();
        check(g);

        do
        {
            FJCVCell* n = Map.GetCellNeighbour(g);

            // Visited or invalid neighbour cell, skip
            if (! n || ExclusionSet.Contains(n))
            {
                continue;
            }

            // Add cell to the visited set
            ExclusionSet.Emplace(n);

            // Cell feature type does not pass the feature filter, skip cell type assignment
            if (FeatureTypeFilter != 255 && n->FeatureType != FeatureTypeFilter)
            {
                continue;
            }

            n->SetType(*Cell);
            CellQ.Enqueue(n);
        }
        while ((g = g->next) != nullptr);
    }
}

void FJCVFeatureUtility::GenerateSegmentExpands(
    FJCVDiagramMap& Map,
    const TArray<FVector2D>& Origins,
    int32 SegmentCount,
    FRandomStream& Rand,
    TArray<FJCVCell*>& OutOrigins,
    TArray<FJCVCell*>& OutSegments
    )
{
    if (Map.IsEmpty())
    {
        return;
    }

    TArray<FJCVCell*> OriginCells;
    const FBox2D Bounds(Map.GetBounds());
    const int32 OriginCount(Origins.Num());

    // Generates segmented plate origins
    for (int32 i=0; i<OriginCount; ++i)
    {
        FJCVCell* c = Map.GetCell(Map->Find(Origins[i]));
        if (c)
        {
            c->SetType(i, 0);
            OriginCells.Emplace(c);
        }
    }

    check(OriginCells.Num() > 0);
    OutSegments = OriginCells;

    // Generates segmented plate features by point fill expand the origins
    PointFill(Map, OriginCells);

    // Group cells by their feature type and generate neighbour list
    // for each feature group
    Map.GroupByFeatures();
    Map.GenerateNeighbourList();

    TQueue<uint8> plateQ;
    TSet<uint8> plateS;
    // Clamp the number of plate to be generated
    // if there is not enough plate segments
    const int32 plateN = FMath::Min(SegmentCount, OriginCount);

    // Generate plate origins
    for (int32 i=0; i<plateN; ++i)
    {
        FRotator randRot( FRotator(0.f,Rand.GetFraction()*360.f,0.f) );
        FVector2D randDir( randRot.Vector() );
        FVector2D randPos( Bounds.GetCenter() + randDir*Bounds.GetExtent()*2.f );
        FJCVCell* plateCell = nullptr;
        float dist0 = TNumericLimits<float>::Max();
        for (FJCVCell* c : OriginCells)
        {
            float dist1 = (randPos-c->ToVector2D()).Size();
            if (! plateS.Contains(c->FeatureType) && dist1 < dist0)
            {
                plateCell = c;
                dist0 = dist1;
            }
        }
        if (plateCell)
        {
            FJCVFeatureGroup* fg = Map.GetFeatureGroup(plateCell->FeatureType);
            if (fg)
            {
                plateQ.Enqueue(fg->FeatureType);
                plateS.Emplace(fg->FeatureType);
            }
            OutOrigins.Emplace(plateCell);
        }
    }

    // Merges plate segments
    while (! plateQ.IsEmpty())
    {
        uint8 ft0;
        plateQ.Dequeue(ft0);

        FJCVFeatureGroup* fg0 = Map.GetFeatureGroup(ft0);
        if (fg0)
        {
            bool bMerged = false;
            for (uint8 ft1 : fg0->Neighbours)
            {
                if (plateS.Contains(ft1))
                    continue;
                FJCVFeatureGroup* fg1 = Map.GetFeatureGroup(ft1);
                if (fg1)
                {
                    Map.MergeGroup(*fg1, *fg0);
                    Map.MergeNeighbourList(*fg1, *fg0);
                    bMerged = true;
                    break;
                }
            }
            if (bMerged)
            {
                plateQ.Enqueue(ft0);
            }
        }
    }

    // Clear empty feature groups
    Map.ShrinkGroups();
}

void FJCVFeatureUtility::PointFillSubdivideFeatures(
    FJCVDiagramMap& Map,
    const uint8 FeatureType,
    const TArray<int32>& OriginCellIndices,
    int32 SegmentCount,
    FRandomStream& Rand
    )
{
    // Empty feature group or zero cell count specified, abort
    if (SegmentCount < 1 || ! Map.HasCells(FeatureType))
    {
        return;
    }

    const FJCVFeatureGroup& fg(*Map.GetFeatureGroup(FeatureType));
    const int32 GroupCellCount = fg.GetCellCount();

    SegmentCount = FMath::Min(SegmentCount, GroupCellCount);

    // Segment count equals feature group cell count, assign each
    // cell within the feature group as new feature types and return
    if (SegmentCount == GroupCellCount)
    {
        FJCVCellGroup SegmentCells;
        fg.GetCells(SegmentCells);

        int32 FeatureCount = Map.GetFeatureCount();

        for (FJCVCell* c : SegmentCells)
        {
            if (c)
            {
                c->SetType(FeatureCount++);
            }
        }

        // Generate group and neighbour list then shrink feature types
        Map.GroupByFeatures();
        Map.GenerateNeighbourList();

        return;
    }

    // Remove origin cells with invalid feature type
    FJCVCellGroup OriginCells;
    OriginCells.Reserve(OriginCellIndices.Num());

    for (int32 i : OriginCellIndices)
    {
        if (Map.IsValidIndex(i))
        {
            FJCVCell& c(Map.GetCell(i));

            if (c.FeatureType == FeatureType)
            {
                OriginCells.Emplace(&c);
            }
        }
    }

    const int32 OriginCount = OriginCells.Num();

    // No more than one valid origin, no further update required, abort
    if (OriginCount < 2)
    {
        return;
    }

    // Generate new feature type each origin

    TSet<uint8> FeatureTypeSet;
    TSet<uint8> VisitedFeatureSet;

    // Exclude existing feature types from the feature merge list
    for (int32 i=0; i<Map.GetFeatureCount(); ++i)
    {
        VisitedFeatureSet.Emplace(i);
    }

    int32 FeatureCount = Map.GetFeatureCount();

    for (FJCVCell* c : OriginCells)
    {
        if (c)
        {
            const uint8 NewFeatureType = FeatureCount++;
            FeatureTypeSet.Emplace(NewFeatureType);
            c->SetType(NewFeatureType);
        }
    }

    // Generates segmented plate features by point fill expand the origins
    PointFill(Map, OriginCells, FeatureType);

    // Group cells by their feature type and generate neighbour list
    // for each feature group
    Map.GroupByFeatures();
    Map.GenerateNeighbourList();

    TQueue<uint8> plateQ;

    // Clamp the number of plate to be generated
    // if there is not enough plate segments
    const int32 plateN = FMath::Min(SegmentCount, OriginCount);

    const FBox2D Bounds(Map.GetBounds());
    const FVector2D MapCenter(Bounds.GetCenter());
    const FVector2D MapExtent(Bounds.GetExtent());

    // Generate plate origins
    for (int32 i=0; i<plateN; ++i)
    {
        FRotator randRot(0.f, Rand.GetFraction()*360.f, 0.f);
        FVector2D randDir(randRot.Vector());
        FVector2D randPos(MapCenter + randDir*MapExtent*2.f);

        float distSq0 = TNumericLimits<float>::Max();
        FJCVCell* plateCell = nullptr;

        for (FJCVCell* c : OriginCells)
        {
            float distSq1 = (randPos-c->ToVector2D()).SizeSquared();

            if (! VisitedFeatureSet.Contains(c->FeatureType) && distSq1 < distSq0)
            {
                plateCell = c;
                distSq0 = distSq1;
            }
        }

        if (plateCell)
        {
            FJCVFeatureGroup* fg = Map.GetFeatureGroup(plateCell->FeatureType);

            if (fg)
            {
                plateQ.Enqueue(fg->FeatureType);
                VisitedFeatureSet.Emplace(fg->FeatureType);
            }
        }
    }

    // Merges plate segments
    while (! plateQ.IsEmpty())
    {
        uint8 ft0;
        plateQ.Dequeue(ft0);

        FJCVFeatureGroup* fg0 = Map.GetFeatureGroup(ft0);

        if (fg0)
        {
            bool bMerged = false;

            for (uint8 ft1 : fg0->Neighbours)
            {
                if (VisitedFeatureSet.Contains(ft1))
                {
                    continue;
                }

                FJCVFeatureGroup* fg1 = Map.GetFeatureGroup(ft1);

                if (fg1)
                {
                    Map.MergeGroup(*fg1, *fg0);
                    Map.MergeNeighbourList(*fg1, *fg0);
                    bMerged = true;
                    break;
                }
            }

            if (bMerged)
            {
                plateQ.Enqueue(ft0);
            }
        }
    }
}

void FJCVFeatureUtility::GetRandomCellWithinFeature(
    FJCVCellGroup& OutCells,
    const FJCVDiagramMap& Map,
    uint8 FeatureType,
    int32 CellCount,
    FRandomStream& Rand,
    bool bAllowBorder,
    int32 MinCellDistance
    )
{
    OutCells.Empty(CellCount);

    // Empty feature group or zero cell count specified, abort
    if (CellCount < 1 || ! Map.HasCells(FeatureType))
    {
        return;
    }

    const FJCVFeatureGroup& fg(*Map.GetFeatureGroup(FeatureType));
    const int32 GroupCellCount = fg.GetCellCount();

    MinCellDistance = FMath::Max(0, MinCellDistance);
    CellCount = FMath::Min(CellCount, GroupCellCount);

    // Feature cell count is less than or equal the requested
    // number of cell count and minimum cell distance is zero,
    // output all cell and return
    if (MinCellDistance < 1 && CellCount >= fg.GetCellCount())
    {
        fg.GetCells(OutCells);
        return;
    }

    FJCVConstCellSet VisitedCellSet;
    FJCVCellGroup GroupCells;
    fg.GetCells(GroupCells);

    while (OutCells.Num() < CellCount && GroupCells.Num() > 0)
    {
        const int32 CellIndex = Rand.RandHelper(GroupCells.Num());
        FJCVCell* RandCell = GroupCells[CellIndex];
        bool bValidCell = (RandCell != nullptr);

        if (bValidCell)
        {
            check(RandCell != nullptr);

            // Cell has been visited but not removed possibly due to
            // minimum cell distance check, remove from cell group
            // and skip
            if (VisitedCellSet.Contains(RandCell))
            {
                bValidCell = false;
            }
            // If border cell is not allowed, check whether current cell
            // is a feature border (Adjacent to another cell with different
            // feature type and is not diagram border cell)
            else if (! bAllowBorder && Map.IsFeatureBorder(*RandCell, true))
            {
                bValidCell = false;
            }
        }

        // Invalid cell, remove from cell group and skip
        if (! bValidCell)
        {
            GroupCells.RemoveAtSwap(CellIndex, 1, false);
            continue;
        }

        // If minimum distance between cell is specified,
        // mark neighbouring cells as visited to prevent 
        if (MinCellDistance > 0)
        {
            TQueue<const FJCVCell*> CellQueue;
            CellQueue.Enqueue(RandCell);

            for (int32 i=0; i<MinCellDistance; ++i)
            {
                FJCVConstCellSet NeighbourCellSet;

                // Find neighbours of cells in queue
                while (! CellQueue.IsEmpty())
                {
                    const FJCVCell* Cell;
                    CellQueue.Dequeue(Cell);

                    check(Cell);

                    Map.GetNeighbourCells(*Cell, NeighbourCellSet);
                }

                // Add unvisited neighbour cells to the queue
                // if further distance check is required
                if (i < (MinCellDistance-1))
                {
                    for (const FJCVCell* nc : NeighbourCellSet)
                    {
                        check(nc);

                        if (! VisitedCellSet.Contains(nc))
                        {
                            CellQueue.Enqueue(nc);
                        }
                    }
                }

                // Add (union) neighbour cells to visited cell set
                VisitedCellSet.Append(NeighbourCellSet);
            }
        }

        // Add current cell to output and remove it from group cells
        OutCells.Emplace(RandCell);
        GroupCells.RemoveAtSwap(CellIndex, 1, false);
    }
}

void FJCVFeatureUtility::GenerateDepthMap(FJCVDiagramMap& SrcMap, FJCVDiagramMap& DstMap, uint8 FeatureType, int32 FeatureIndex)
{
    // Make sure target feature type have any cells
    if ((FeatureIndex >= 0 && ! SrcMap.HasCells(FeatureType, FeatureIndex)) || ! SrcMap.HasCells(FeatureType))
    {
        return;
    }

    check(SrcMap.Num() == DstMap.Num());

    // Clear target map features
    DstMap.ClearFeatures();

    // Feature type cell group indices to evaluate
    TArray<int32> FeatureIndices;
    SrcMap.GetFeatureIndices(FeatureIndices, FeatureType, FeatureIndex, true);

    for (int32 i=0; i<FeatureIndices.Num(); ++i)
    {
        const int32 CurFeatureIndex = FeatureIndices[i];

        FJCVConstCellSet VisitedCellSet;
        FJCVConstCellSet PrevCellSet;
        FJCVConstCellSet CurCellSet;

        // Find feature border cells to use as initial cells to evaluate
        SrcMap.GetBorderCells(CurCellSet, FeatureType, -1, CurFeatureIndex, true, true);

        VisitedCellSet = CurCellSet;
        PrevCellSet = CurCellSet;

        int32 VisitedCellCount = 0;

        // Callback for every visited cells
        TFunctionRef<void(const FJCVCell*)> VisitCallback(
            [&](const FJCVCell* Cell)
            {
                check(Cell != nullptr);

                if (Cell->IsType(FeatureType, CurFeatureIndex) && ! VisitedCellSet.Contains(Cell))
                {
                    CurCellSet.Emplace(Cell);
                }
            } );

        // Set border cells as lowest depth feature
        for (const FJCVCell* Cell : CurCellSet)
        {
            FJCVCell& DstCell(DstMap.GetCell(Cell->GetIndex()));
            DstCell.SetType(i, 0);
        }

        int32 Depth = 1;

        while (PrevCellSet.Num() > 0)
        {
            CurCellSet.Reset();

            for (const FJCVCell* Cell : PrevCellSet)
            {
                check(Cell != nullptr);
                SrcMap.VisitNeighbours(*Cell, VisitCallback);
            }

            VisitedCellSet.Append(CurCellSet);
            PrevCellSet = CurCellSet;

            for (const FJCVCell* Cell : CurCellSet)
            {
                FJCVCell& DstCell(DstMap.GetCell(Cell->GetIndex()));
                DstCell.SetType(i, Depth);
            }

            ++Depth;
        }
    }

    DstMap.GroupByFeatures();
}

void UJCVFeatureUtility::GenerateDepthMap(UJCVDiagramAccessor* SrcAccessor, UJCVDiagramAccessor* DstAccessor, FJCVFeatureId FeatureId)
{
    if (! IsValid(SrcAccessor))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GenerateDepthMap() ABORTED, INVALID SrcAccessor"));
        return;
    }

    if (! IsValid(DstAccessor))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GenerateDepthMap() ABORTED, INVALID DstAccessor"));
        return;
    }

    if (! SrcAccessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GenerateDepthMap() ABORTED, INVALID SrcAccessor MAP"));
        return;
    }

    if (! DstAccessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GenerateDepthMap() ABORTED, INVALID DstAccessor MAP"));
        return;
    }

    FJCVDiagramMap& SrcMap(SrcAccessor->GetMap());
    FJCVDiagramMap& DstMap(DstAccessor->GetMap());

    // Make sure source and target map have the same cell count
    if (SrcMap.Num() != DstMap.Num())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GenerateDepthMap() ABORTED, SOURCE AND TARGET MAP HAVE DIFFERENT CELL COUNT"));
        return;
    }

    FJCVFeatureUtility::GenerateDepthMap(SrcMap, DstMap, FeatureId.Type, FeatureId.Index);
}

FJCVCellRef UJCVFeatureUtility::FindDepthMapCellOutsidePointRadius(UJCVDiagramAccessor* Accessor, int32 Seed, FVector2D Origin, float Radius, uint8 FeatureType, int32 FromIndex, int32 ToIndex)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::FindDepthMapCellOutsideRadius() ABORTED, INVALID ACCESSOR"));
        return FJCVCellRef();
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::FindDepthMapCellOutsideRadius() ABORTED, INVALID ACCESSOR MAP"));
        return FJCVCellRef();
    }

    FJCVDiagramMap& Map(Accessor->GetMap());

    if (! Map.HasFeatureType(FeatureType))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::FindDepthMapCellOutsideRadius() ABORTED, INVALID FEATURE TYPE"));
        return FJCVCellRef();
    }

    FJCVFeatureGroup& FeatureGroup(*Map.GetFeatureGroup(FeatureType));

    FRandomStream Rand(Seed);
    const int32 GroupCount = FeatureGroup.GetGroupCount();
    const int32 Index0 = FMath::Clamp(FromIndex, 0, GroupCount-1);
    const int32 Index1 = FMath::Clamp(ToIndex  , 0, GroupCount-1);

    TArray<int32> FeatureIndices;

    if (FromIndex <= ToIndex)
    {
        for (int32 i=Index0; i<=Index1; ++i)
        {
            FeatureIndices.Emplace(i);
        }
    }
    else
    {
        for (int32 i=Index0; i>=Index1; --i)
        {
            FeatureIndices.Emplace(i);
        }
    }

    const float RadiusSq = Radius * Radius;
    FJCVCellRef CellResult;

    for (int32 FeatureIndex : FeatureIndices)
    {
        FJCVCellGroup CellGroup(FeatureGroup.CellGroups[FeatureIndex]);
        int32 CellCount = CellGroup.Num();

        // Shuffle cell group copy
		for (int32 i=0; i<CellCount; ++i)
		{
			int32 SwapIndex = Rand.RandHelper(CellCount);

			if (i != SwapIndex)
			{
                CellGroup.Swap(i, SwapIndex);
			}
		}

        // Find cell outside of point radius
        for (const FJCVCell* Cell : CellGroup)
        {
            FVector2D CellOrigin(Cell->ToVector2DUnsafe());
            float CellDistSq = (Origin-CellOrigin).SizeSquared();

            if (CellDistSq > RadiusSq)
            {
                CellResult = FJCVCellRef(Cell);
                break;
            }
        }

        if (CellResult.HasValidCell())
        {
            break;
        }
    }

    return CellResult;
}
