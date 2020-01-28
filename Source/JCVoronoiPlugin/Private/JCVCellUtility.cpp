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

#include "JCVCellUtility.h"
#include "JCVDiagramMap.h"

void FJCVCellUtility::PointFillVisit(
    FJCVDiagramMap& Map,
    const TArray<FJCVCell*>& OriginCells,
    const FJCVNeighbourVisitCallback& VisitCallback
    )
{
    if (Map.IsEmpty() || OriginCells.Num() < 1)
    {
        return;
    }

    TQueue<FJCVCell*> CellVisitQueue;
    TSet<FJCVCell*> VisitedCellSet;

    // Visit starting cells, filter invalid and duplicate cells
    for (FJCVCell* c : OriginCells)
    {
        if (c && ! VisitedCellSet.Contains(c))
        {
            CellVisitQueue.Enqueue(c);
            VisitedCellSet.Emplace(c);
        }
    }

    // Visit cells in queue
    while (! CellVisitQueue.IsEmpty())
    {
        FJCVCell* Cell;
        CellVisitQueue.Dequeue(Cell);

        check(Cell != nullptr);

        FJCVEdge* g = Cell->GetEdge();
        check(g);

        do
        {
            FJCVCell* NeighbourCell = Map.GetCellNeighbour(g);

            //  Invalid neighbour or already visited cell, skip
            if (! NeighbourCell || VisitedCellSet.Contains(NeighbourCell))
            {
                continue;
            }

            check(g != nullptr);

            // Add cell to the visited set
            VisitedCellSet.Emplace(NeighbourCell);

            // Call visit callback
            bool bEnqueueCellVisit = VisitCallback(*Cell, *NeighbourCell, *g);

            // If visit callback return true, enqueue current neighbouring cell
            if (bEnqueueCellVisit)
            {
                CellVisitQueue.Enqueue(NeighbourCell);
            }
        }
        while ((g = g->next) != nullptr);
    }
}

void FJCVCellUtility::ExpandVisit(
    FJCVDiagramMap& Map,
    int32 ExpandCount,
    const TArray<FJCVCell*>& OriginCells,
    const FJCVNeighbourVisitCallback& VisitCallback
    )
{
    if (Map.IsEmpty() || OriginCells.Num() < 1)
    {
        return;
    }

    TSet<FJCVCell*> VisitedCellSet;
    TArray<FJCVCell*> CellVisitList;
    TArray<FJCVCell*> CellVisitNextList;

    // Visit starting cells, filter invalid and duplicate cells
    for (FJCVCell* c : OriginCells)
    {
        if (c && ! VisitedCellSet.Contains(c))
        {
            CellVisitList.Emplace(c);
            VisitedCellSet.Emplace(c);
        }
    }

    for (int32 It=0; It<ExpandCount; ++It)
    {
        // Visit cells in list
        for (int32 i=0; i<CellVisitList.Num(); ++i)
        {
            FJCVCell* Cell = CellVisitList[i];

            check(Cell != nullptr);

            FJCVEdge* g = Cell->GetEdge();
            check(g);

            do
            {
                FJCVCell* NeighbourCell = Map.GetCellNeighbour(g);

                //  Invalid neighbour or already visited cell, skip
                if (! NeighbourCell || VisitedCellSet.Contains(NeighbourCell))
                {
                    continue;
                }

                // Add cell to the visited set
                VisitedCellSet.Emplace(NeighbourCell);

                // Call visit callback
                bool bEnqueueCellVisit = VisitCallback(*Cell, *NeighbourCell, *g);

                // If visit callback return true, enqueue current neighbouring cell
                if (bEnqueueCellVisit)
                {
                    CellVisitNextList.Emplace(NeighbourCell);
                }
            }
            while ((g = g->next) != nullptr);
        }

        CellVisitList = MoveTemp(CellVisitNextList);
    }
}

float FJCVCellUtility::GetClosestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    const FJCVFeatureId& FeatureId,
    bool bAgainstAnyType
    )
{
    const uint8 FeatureType = FeatureId.Type;
    const int32 FeatureIndex = FeatureId.Index;

    check(Map.HasFeature(FeatureType));
    check(Map.IsValidCell(&OriginCell));

    const FVector2D Origin = OriginCell.ToVector2D();
    float DistanceToFeatureSq = BIG_NUMBER;

    TFunctionRef<void(FJCVCell& Cell)> CellCallback(
        [&](FJCVCell& Cell)
        {
            const FVector2D CellPoint = Cell.ToVector2D();
            const float CellDistSq = (CellPoint-Origin).SizeSquared();

            if (CellDistSq < DistanceToFeatureSq)
            {
                DistanceToFeatureSq = CellDistSq;
            }
        } );

    if (bAgainstAnyType)
    {
        Map.VisitCells(CellCallback, &OriginCell);
    }
    else
    {
        Map.VisitFeatureCells(CellCallback, FeatureType, FeatureIndex);
    }

    return DistanceToFeatureSq;
}

float FJCVCellUtility::GetFurthestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    const FJCVFeatureId& FeatureId,
    bool bAgainstAnyType
    )
{
    const uint8 FeatureType = FeatureId.Type;
    const int32 FeatureIndex = FeatureId.Index;

    check(Map.HasFeature(FeatureType));
    check(Map.IsValidCell(&OriginCell));

    const FVector2D Origin = OriginCell.ToVector2D();
    float DistanceToFeatureSq = TNumericLimits<float>::Min();

    TFunctionRef<void(FJCVCell& Cell)> CellCallback(
        [&](FJCVCell& Cell)
        {
            const FVector2D CellPoint = Cell.ToVector2D();
            const float CellDistSq = (CellPoint-Origin).SizeSquared();

            if (CellDistSq > DistanceToFeatureSq)
            {
                DistanceToFeatureSq = CellDistSq;
            }
        } );

    if (bAgainstAnyType)
    {
        Map.VisitCells(CellCallback, &OriginCell);
    }
    else
    {
        Map.VisitFeatureCells(CellCallback, FeatureType, FeatureIndex);
    }

    return DistanceToFeatureSq;
}
