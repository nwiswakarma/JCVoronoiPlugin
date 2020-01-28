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
#include "JCVCellUtility.h"
#include "JCVDiagramMap.h"

// Visit Utility

void FJCVFeatureUtility::PointFill(FJCVDiagramMap& Map, const TArray<FJCVCell*>& OriginCells, uint8 FeatureTypeFilter)
{
    FJCVCellUtility::PointFillVisit(
        Map,
        OriginCells,
        [FeatureTypeFilter](
            FJCVCell& CurrentCell,
            FJCVCell& NeighbourCell,
            FJCVEdge& CellEdge
            )
        {
            if (FeatureTypeFilter != 255 && NeighbourCell.FeatureType != FeatureTypeFilter)
            {
                return false;
            }

            NeighbourCell.SetType(CurrentCell);
            return true;
        } );
}

void FJCVFeatureUtility::PointFillIsolated(
    FJCVDiagramMap& Map,
    FJCVFeatureId BoundFeature,
    FJCVFeatureId TargetFeature,
    const TArray<FJCVCell*>& OriginCells
    )
{
    FJCVCellUtility::PointFillVisit(
        Map,
        OriginCells,
        [BoundFeature, TargetFeature](
            FJCVCell& CurrentCell,
            FJCVCell& NeighbourCell,
            FJCVEdge& CellEdge
            )
        {
            if (! NeighbourCell.IsType(BoundFeature.Type, BoundFeature.Index))
            {
                NeighbourCell.SetType(TargetFeature.Type, TargetFeature.Index);
                return true;
            }

            return false;
        } );
}

void FJCVFeatureUtility::ExpandFeatureFromCellGroups(
    FJCVDiagramMap& Map,
    const TArray<FJCVCell*>& OriginCells,
    const FJCVFeatureId& FeatureId,
    int32 ExpandCount
    )
{
    FJCVCellUtility::ExpandVisit(
        Map,
        ExpandCount,
        OriginCells,
        [FeatureId](
            FJCVCell& Cell,
            FJCVCell& NeighbourCell,
            FJCVEdge& CellEdge
            )
        {
            NeighbourCell.SetType(FeatureId.Type, FeatureId.Index);
            return true;
        } );
}

// Feature Segments

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
            FJCVFeatureGroup* plateFeatureGroup = Map.GetFeatureGroup(plateCell->FeatureType);

            if (plateFeatureGroup)
            {
                plateQ.Enqueue(plateFeatureGroup->FeatureType);
                VisitedFeatureSet.Emplace(plateFeatureGroup->FeatureType);
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

// Depth Map Utility

void FJCVFeatureUtility::GenerateDepthMap(FJCVDiagramMap& SrcMap, FJCVDiagramMap& DstMap, const FJCVFeatureId& FeatureId)
{
    const uint8 FeatureType = FeatureId.Type;
    const int32 FeatureIndex = FeatureId.Index;

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
        const int32 DepthFeatureType = i+1;

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
            DstCell.SetType(DepthFeatureType, 0);
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
                DstCell.SetType(DepthFeatureType, Depth);
            }

            ++Depth;
        }
    }

    DstMap.GroupByFeatures();
}

// Cell Query

void FJCVFeatureUtility::GetRandomCellWithinFeature(
    FJCVCellGroup& OutCells,
    FJCVDiagramMap& Map,
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
            // Check whether current cell is a feature border
            // (Adjacent to another cell with different
            // feature type and is not diagram border cell)
            //
            // Mark cell as invalid if feature border cell is being filtered
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
        // mark neighbouring cells as visited to prevent duplicates
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

void FJCVFeatureUtility::GetFeatureCellGroups(
    FJCVDiagramMap& Map,
    const TArray<FJCVFeatureId> FeatureIds,
    TArray<FJCVCellGroup>& OutCellGroups
    )
{
    OutCellGroups.SetNum(FeatureIds.Num());

    for (int32 i=0; i<FeatureIds.Num(); ++i)
    {
        const FJCVFeatureId& FeatureId(FeatureIds[i]);
        FJCVCellGroup* CellGroup = Map.GetFeatureCellGroup(FeatureId);

        if (CellGroup)
        {
            OutCellGroups[i] = *CellGroup;
        }
    }
}

// Blueprint Functions

void UJCVFeatureUtility::K2_GenerateDepthMap(UJCVDiagramAccessor* SrcAccessor, UJCVDiagramAccessor* DstAccessor, FJCVFeatureId FeatureId)
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

    FJCVFeatureUtility::GenerateDepthMap(SrcMap, DstMap, FeatureId);
}

void UJCVFeatureUtility::GetFeatureIdRange(TArray<FJCVFeatureId>& FeatureIds, uint8 FeatureType, int32 FeatureIndexStart, int32 FeatureIndexEnd, bool bInclusiveEnd)
{
    if (FeatureIndexStart <= FeatureIndexEnd)
    {
        for (int32 i=FeatureIndexStart; i<=FeatureIndexEnd; ++i)
        {
            FeatureIds.Emplace(FeatureType, i);
        }
    }
    else
    {
        for (int32 i=FeatureIndexStart; i>=FeatureIndexEnd; --i)
        {
            FeatureIds.Emplace(FeatureType, i);
        }
    }
}

void UJCVFeatureUtility::GetCellsFromFeatures(
    UJCVDiagramAccessor* Accessor,
    TArray<FJCVCellRefGroup>& CellRefGroups,
    const TArray<FJCVFeatureId>& FeatureIds,
    const TArray<FJCVCellRef>& FilterCells,
    bool bUseFilterCells
    )
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GetCellsFromFeatures() ABORTED, INVALID ACCESSOR"));
        return;
    }
    else
    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GetCellsFromFeatures() ABORTED, INVALID ACCESSOR MAP"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    CellRefGroups.SetNum(FeatureIds.Num());

    TSet<const FJCVCell*> FilterCellSet;

    if (bUseFilterCells)
    {
        FilterCellSet.Reserve(FilterCells.Num());

        for (const FJCVCellRef& FilterCell : FilterCells)
        {
            if (FilterCell.HasValidCell())
            {
                FilterCellSet.Emplace(FilterCell.Data);
            }
        }
    }

    for (int32 i=0; i<FeatureIds.Num(); ++i)
    {
        const FJCVFeatureId& FeatureId(FeatureIds[i]);
        TArray<FJCVCellRef>& CellRefs(CellRefGroups[i].Data);

        FJCVCellGroup* CellGroupPtr = Map.GetFeatureCellGroup(FeatureId);

        if (! CellGroupPtr)
        {
            continue;
        }

        FJCVCellGroup& CellGroup(*CellGroupPtr);

        if (bUseFilterCells)
        {
            for (FJCVCell* Cell : CellGroup)
            {
                if (! FilterCellSet.Contains(Cell))
                {
                    CellRefs.Emplace(Cell);
                }
            }
        }
        else
        {
            for (FJCVCell* Cell : CellGroup)
            {
                CellRefs.Emplace(Cell);
            }
        }
    }
}

void UJCVFeatureUtility::GetRandomCellsFromFeaturesByDistanceFromDepthMapEdge(
    UJCVDiagramAccessor* Accessor,
    int32 Seed,
    const TArray<FJCVFeatureId>& FeatureIds,
    TArray<FJCVCellRef>& OutCellRefs,
    TArray<FJCVCellRef>& OutFilteredCellRefs,
    TArray<float>& OutDistances,
    float FilterDistanceRatio,
    float FilterDistanceRatioRandom,
    bool bGenerateFilteredCells
    )
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GetRandomCellsFromFeaturesByDistanceFromDepthMapEdge() ABORTED, INVALID ACCESSOR"));
        return;
    }
    else
    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVFeatureUtility::GetRandomCellsFromFeaturesByDistanceFromDepthMapEdge() ABORTED, INVALID ACCESSOR MAP"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    FRandomStream Rand(Seed);

    // Generate cell candidates
    TArray<FJCVCellGroup> FeatureCellGroups;
    FJCVFeatureUtility::GetFeatureCellGroups(Map, FeatureIds, FeatureCellGroups);

    // Valid cell container
    TArray<FJCVCell*> ValidCells;

    // Reserve filtered cells container space
    TSet<FJCVCell*> FilterCellSet;
    if (bGenerateFilteredCells)
    {
        int32 FeatureCellCount = 0;
        for (const FJCVCellGroup& FeatureCells : FeatureCellGroups)
        {
            FeatureCellCount += FeatureCells.Num();
        }
        FilterCellSet.Reserve(FeatureCellCount);
    }

    for (int32 i=0; i<FeatureIds.Num(); ++i)
    {
        FJCVCellGroup& FeatureCells(FeatureCellGroups[i]);

        while (FeatureCells.Num() > 0)
        {
            int32 RandIndex = Rand.RandHelper(FeatureCells.Num());
            FJCVCell* RandCell = FeatureCells[RandIndex];
            FVector2D RandCellPos = RandCell->ToVector2DUnsafe();

            FeatureCells.RemoveAtSwap(RandIndex, 1, false);

            check(RandCell != nullptr);

            // Get distance from edge

            float DistanceToEdgeSq = FJCVCellUtility::GetClosestDistanceFromCellSq(
                Map,
                *RandCell,
                FJCVFeatureId(0, -1),
                false
                );

            // Filter all cells within distance

            float FilterDistanceSq = DistanceToEdgeSq * FilterDistanceRatio;
            FilterDistanceSq -= FilterDistanceSq * FilterDistanceRatioRandom * Rand.GetFraction();

            // Iterate through feature cell groups to filter cells by distance
            for (int32 j=i; j<FeatureIds.Num(); ++j)
            {
                FJCVCellGroup& FilterCells(FeatureCellGroups[j]);
                int32 FilterIt = 0;

                // Remove all cells within filter distance
                while (FilterIt < FilterCells.Num())
                {
                    FJCVCell* FilterCell = FilterCells[FilterIt];
                    check(FilterCell != nullptr);

                    FVector2D FilterCellPos(FilterCell->ToVector2DUnsafe());
                    float DistToFilterSq = (FilterCellPos-RandCellPos).SizeSquared();

                    if (bGenerateFilteredCells)
                    {
                        FilterCellSet.Emplace(FilterCell);
                    }

                    if (DistToFilterSq < FilterDistanceSq)
                    {
                        FilterCells.RemoveAtSwap(FilterIt, 1, false);
                        continue;
                    }

                    ++FilterIt;
                }
            }

            // Generate output cell ref and distance to edge

            OutCellRefs.Emplace(RandCell);
            OutDistances.Emplace(FMath::Sqrt(DistanceToEdgeSq));
        }
    }

    if (bGenerateFilteredCells)
    {
        OutFilteredCellRefs.Reserve(FilterCellSet.Num());
        for (const FJCVCell* FilteredCell : FilterCellSet)
        {
            OutFilteredCellRefs.Emplace(FilteredCell);
        }
    }
}

void UJCVFeatureUtility::GetRandomCellsFromDepthFeatureRangeByDistanceFromEdge(
    UJCVDiagramAccessor* Accessor,
    int32 Seed,
    uint8 FeatureType,
    int32 FeatureIndexStart,
    int32 FeatureIndexEnd,
    TArray<FJCVCellRef>& OutCellRefs,
    TArray<FJCVCellRef>& OutFilteredCellRefs,
    TArray<float>& OutDistances,
    float FilterDistanceRatio,
    float FilterDistanceRatioRandom,
    bool bGenerateFilteredCells
    )
{
    TArray<FJCVFeatureId> FeatureIds;
    GetFeatureIdRange(FeatureIds, FeatureType, FeatureIndexStart, FeatureIndexEnd);

    GetRandomCellsFromFeaturesByDistanceFromDepthMapEdge(
        Accessor,
        Seed,
        FeatureIds,
        OutCellRefs,
        OutFilteredCellRefs,
        OutDistances,
        FilterDistanceRatio,
        FilterDistanceRatioRandom,
        bGenerateFilteredCells
        );
}
