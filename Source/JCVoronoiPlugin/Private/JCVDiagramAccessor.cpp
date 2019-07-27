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

#include "JCVDiagramAccessor.h"
#include "JCVDiagramMap.h"
#include "JCVCellUtility.h"
#include "JCVFeatureUtility.h"
#include "JCVValueGenerator.h"
#include "JCVPlateGenerator.h"

#include "Poly/GULPolyUtilityLibrary.h"

void UJCVDiagramAccessor::SetMap(FJCVDiagramMap& AccessedMap, int32 InContextId, int32 InMapId)
{
    Map = &AccessedMap;
    ContextId = InContextId;
    MapId = InMapId;
}

FBox2D UJCVDiagramAccessor::K2_GetBounds() const
{
    return HasValidMap() ? Map->GetBounds() : FBox2D();
}

int32 UJCVDiagramAccessor::GetCellIndex(const FJCVCellRef& CellRef) const
{
    if (HasValidMap() && CellRef.Data)
    {
        return Map->GetCellIndex(CellRef.Data);
    }

    return -1;
}

FJCVCellRef UJCVDiagramAccessor::GetCellRef(int32 CellIndex)
{
    return (Map && Map->IsValidIndex(CellIndex))
        ? FJCVCellRef(&Map->GetCell(CellIndex))
        : FJCVCellRef();
}

bool UJCVDiagramAccessor::IsValidCell(const FJCVCellRef& CellRef) const
{
    return (GetCellIndex(CellRef) >= 0);
}

void UJCVDiagramAccessor::GetCellsFromRefs(const TArray<FJCVCellRef>& CellRefs, TArray<FJCVCell*>& Cells) const
{
    check(Map != nullptr);

    Cells.Reserve(CellRefs.Num());

    for (int32 i=0; i<CellRefs.Num(); ++i)
    {
        const FJCVCellRef& CellRef(CellRefs[i]);

        if (Map->IsValidCell(CellRef.Data))
        {
            Cells.Emplace(&Map->GetCell(CellRef.Data->GetIndex()));
        }
    }
}

// MARK FEATURE FUNCTIONS

//void UJCVDiagramAccessor::MarkUnmarkedFeatures(uint8 FeatureType)
//{
//    if (HasValidMap())
//    {
//        FJCVCellTraits_Deprecated CellTraits(JCV_CF_UNMARKED, FeatureType);
//        FJCVValueGenerator::MarkFeatures(*Map, CellTraits);
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkUnmarkedFeatures() ABORTED, INVALID MAP"));
//    }
//}

//void UJCVDiagramAccessor::MarkFeaturesByType(FJCVCellTraits_Deprecated FeatureTraits)
//{
//    if (HasValidMap())
//    {
//        FJCVValueGenerator::MarkFeatures(*Map, FeatureTraits);
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByType() ABORTED, INVALID MAP"));
//    }
//}

//void UJCVDiagramAccessor::MarkFeaturesByValue(FJCVValueTraits_Deprecated ValueTraits)
//{
//    if (HasValidMap())
//    {
//        FJCVValueGenerator::MarkFeatures(*Map, ValueTraits);
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByValue() ABORTED, INVALID MAP"));
//    }
//}

void UJCVDiagramAccessor::MarkPositions(TArray<FJCVCellRef>& VisitedCellRefs, const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId, bool bContiguous, bool bClampPoints, bool bExtractVisitedCells)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkPositions() ABORTED, INVALID MAP"));
        return;
    }

    if (Positions.Num() <= 0)
    {
        return;
    }

    TArray<FJCVCell*> VisitedCells;
    TArray<FJCVCell*>* VisitedCellsPtr = bExtractVisitedCells ? &VisitedCells : nullptr;

    if (bContiguous)
    {
        MarkPositionsContiguous(Positions, FeatureId, VisitedCellsPtr, bClampPoints);
    }
    else
    {
        MarkPositions(Positions, FeatureId, VisitedCellsPtr, bClampPoints);
    }

    if (bExtractVisitedCells)
    {
        VisitedCellRefs.Reserve(VisitedCells.Num());

        for (const FJCVCell* VisitedCell : VisitedCells)
        {
            VisitedCellRefs.Emplace(VisitedCell);
        }
    }
}

void UJCVDiagramAccessor::MarkPoly(const TArray<FVector2D>& Points, FJCVFeatureId FeatureMarkId, bool bClampPoints)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkPoly() ABORTED, INVALID MAP"));
        return;
    }
    if (Points.Num() < 3)
    {
        return;
    }

    TArray<FJCVCell*> BoundingCells;

    MarkPositionsContiguous(Points, FeatureMarkId, &BoundingCells, bClampPoints);
    MarkIsolatedFeatures(BoundingCells, FeatureMarkId);
}

void UJCVDiagramAccessor::MarkIsolatedFeatures(const TArray<FJCVCellRef>& BoundingCellRefs, FJCVFeatureId FeatureMarkId)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkIsolatedFeatures() ABORTED, INVALID MAP"));
        return;
    }
    if (BoundingCellRefs.Num() < 3)
    {
        return;
    }

    TArray<FJCVCell*> BoundingCells;
    GetCellsFromRefs(BoundingCellRefs, BoundingCells);

    MarkIsolatedFeatures(BoundingCells, FeatureMarkId);
}

void UJCVDiagramAccessor::MarkIsolatedFeatures(const TArray<FJCVCell*>& BoundingCells, const FJCVFeatureId& FeatureMarkId)
{
    check(HasValidMap());

    if (BoundingCells.Num() < 3)
    {
        return;
    }

    const int32 BoundingCellCount = BoundingCells.Num();

    // Generate bounding poly

    TArray<FVector2D> BoundingPoly;
    BoundingPoly.Reserve(BoundingCells.Num());

    for (const FJCVCell* BoundingCell : BoundingCells)
    {
        BoundingPoly.Emplace(BoundingCell->ToVector2DUnsafe());
    }

    // Generate expanded cell set

    TSet<FJCVCell*> ExpandCellSet;
    ExpandCellSet.Reserve(BoundingCellCount*3);

    FJCVFeatureUtility::ExpandVisit(
        *Map,
        1,
        BoundingCells,
        [&ExpandCellSet,FeatureMarkId](FJCVCell& Cell, FJCVCell& NeighbourCell)
        {
            ExpandCellSet.Emplace(&NeighbourCell);
            return false;
        } );

    // Iterate through expanded cell set and mark cell within boundin poly

    while (ExpandCellSet.Num() > 0)
    {
        FJCVCell* InitialCell = *ExpandCellSet.CreateIterator();
        FVector2D InitialCellOrigin = InitialCell->ToVector2DUnsafe();

        // Cell is within poly, mark feature by point fill with the cell as origin
        if (UGULPolyUtilityLibrary::IsPointOnPoly(InitialCellOrigin, BoundingPoly))
        {
            // Mark feature and remove cell from candidate set
            InitialCell->SetType(FeatureMarkId.Type, FeatureMarkId.Index);
            ExpandCellSet.Remove(InitialCell);

            FJCVFeatureUtility::PointFillVisit(
                *Map,
                { InitialCell },
                [&ExpandCellSet,FeatureMarkId](FJCVCell& CurrentCell, FJCVCell& NeighbourCell)
                {
                    // Remove neighbour cell from candidate set
                    if (ExpandCellSet.Contains(&NeighbourCell))
                    {
                        ExpandCellSet.Remove(&NeighbourCell);
                    }
                    // If not yet marked, mark neighbour cell and add to visit cell queue
                    if (! NeighbourCell.IsType(FeatureMarkId.Type, FeatureMarkId.Index))
                    {
                        NeighbourCell.SetType(FeatureMarkId.Type, FeatureMarkId.Index);
                        return true;
                    }
                    // Otherwise, don't add to visit cell queue
                    else
                    {
                        return false;
                    }
                } );
        }
        // Otherwise, exclude the cell and any neighbouring cells
        // from the feature candidate set
        else
        {
            TQueue<FJCVCell*> ExclusionQueue;
            ExclusionQueue.Enqueue(InitialCell);

            while (! ExclusionQueue.IsEmpty())
            {
                FJCVCell* ExcludeCell;
                ExclusionQueue.Dequeue(ExcludeCell);

                ExpandCellSet.Remove(ExcludeCell);

                TArray<FJCVCell*> NeighbourCells;
                Map->GetNeighbourCells(*ExcludeCell, NeighbourCells);

                for (FJCVCell* NeighbourCell : NeighbourCells)
                {
                    if (ExpandCellSet.Contains(NeighbourCell))
                    {
                        ExclusionQueue.Enqueue(NeighbourCell);
                    }
                }
            }
        }
    }
}

void UJCVDiagramAccessor::MarkPositionsContiguous(const TArray<FVector2D>& Positions, const FJCVFeatureId& FeatureId, TArray<FJCVCell*>* VisitedCells, bool bClampPoints)
{
    check(HasValidMap())

    FJCVDiagramMap& MapRef(*Map);
    const int32 PointCount = Positions.Num();
    const int32 SiteCount = MapRef.Num();
    const int32 ReserveCount = FMath::Min(PointCount, SiteCount);

    TSet<const FJCVSite*> VisitedSiteSet;
    VisitedSiteSet.Reserve(ReserveCount);

    if (VisitedCells)
    {
        VisitedCells->Reserve(ReserveCount);
    }

    TArray<const FJCVSite*> SegmentSites;
    const FJCVSite* SiteIt = nullptr;

    const FBox2D& Bounds(MapRef.GetBounds());

    for (FVector2D Point : Positions)
    {
        // Clamp point if required
        if (bClampPoints)
        {
            Point = FMath::Clamp(Point, Bounds.Min, Bounds.Max);
        }

        if (SiteIt)
        {
            SegmentSites.Reset();
            MapRef->FindAllTo(Point, *SiteIt, SegmentSites);

            if (VisitedCells)
            {
                VisitedCells->Reserve(VisitedCells->Num()+SegmentSites.Num());
            }

            for (const FJCVSite* SegmentSite : SegmentSites)
            {
                check(SegmentSite != nullptr);
                MapRef.MarkFiltered(SegmentSite, FeatureId.Type, FeatureId.Index, VisitedSiteSet, true);

                if (VisitedCells)
                {
                    VisitedCells->Emplace(MapRef.GetCell(SegmentSite));
                }
            }

            SiteIt = SegmentSites.Last();
        }
        // Find initial site
        else
        {
            SiteIt = MapRef->Find(Point);
            MapRef.MarkFiltered(SiteIt, FeatureId.Type, FeatureId.Index, VisitedSiteSet, true);

            if (VisitedCells)
            {
                VisitedCells->Emplace(MapRef.GetCell(SiteIt));
            }
        }
    }

    if (VisitedCells)
    {
        VisitedCells->Shrink();
    }
}

void UJCVDiagramAccessor::MarkPositions(const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId, TArray<FJCVCell*>* VisitedCells, bool bClampPoints)
{
    if (Positions.Num() <= 0)
    {
        return;
    }

    check(HasValidMap());

    FJCVDiagramMap& MapRef(*Map);
    const FJCVSite* SearchSite = MapRef->FindClosest(Positions[0]);

    const FBox2D& Bounds(MapRef.GetBounds());

    for (FVector2D Point : Positions)
    {
        check(SearchSite);

        // Invalid search starting cell, abort
        if (! SearchSite)
        {
            break;
        }

        // Clamp point if required
        if (bClampPoints)
        {
            Point = FMath::Clamp(Point, Bounds.Min, Bounds.Max);
        }

        const FJCVSite* Site = MapRef->FindFrom(Point, *SearchSite);
        FJCVCell* Cell = MapRef.GetCell(Site);

        if (Site)
        {
            Cell->SetType(FeatureId.Type, FeatureId.Index);
            SearchSite = Site;

            if (VisitedCells)
            {
                VisitedCells->Emplace(Cell);
            }
        }
    }
}

//void UJCVDiagramAccessor::MarkRange(const FVector2D& StartPosition, const FVector2D& EndPosition, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraits_Deprecated FilterTraits)
//{
//    if (HasValidMap())
//    {
//        FJCVDiagramMap& MapRef(*Map);
//        const FBox2D& Bounds(GetBounds());
//        const FVector2D& v0(StartPosition);
//        const FVector2D& v1(EndPosition);
//
//        if (! Bounds.IsInside(v0) || ! Bounds.IsInside(v1))
//        {
//            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID INPUT POSITIONS"));
//            return;
//        }
//
//        const FJCVSite* s0 = MapRef->Find(v0);
//
//        if (s0)
//        {
//            TSet<const FJCVSite*> Sites;
//            Sites.Reserve(MapRef.Num()/4);
//            Sites.Emplace(s0);
//            MapRef->FindAllTo(v1, *s0, Sites);
//            for (const FJCVSite* site : Sites)
//            {
//                FJCVCell* cell = MapRef.GetCell(site);
//                check(cell);
//                if (cell && (!bUseFilter || FilterTraits.HasValidFeature(*cell)))
//                {
//                    cell->SetFeature(Value, FeatureId.Type, FeatureId.Index);
//                }
//            }
//        }
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID MAP"));
//    }
//}

//void UJCVDiagramAccessor::MarkRangeByFeature(int32 StartCellID, int32 EndCellID, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraits_Deprecated FilterTraits)
//{
//    if (HasValidMap())
//    {
//        FJCVDiagramMap& MapRef( *Map );
//
//        if (MapRef.IsValidIndex(StartCellID) && MapRef.IsValidIndex(EndCellID))
//        {
//            FVector2D v0(MapRef.GetCell(StartCellID).ToVector2D());
//            FVector2D v1(MapRef.GetCell(EndCellID).ToVector2D());
//            MarkRange(v0, v1, FeatureId, Value, bUseFilter, FilterTraits);
//        }
//        else
//        {
//            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByFeature() ABORTED, INVALID INPUT POSITIONS"));
//        }
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByFeature() ABORTED, INVALID MAP"));
//    }
//}

// FEATURE UTILITY FUNCTIONS

bool UJCVDiagramAccessor::HasFeature(FJCVFeatureId FeatureId) const
{
    return Map ? (Map->GetFeatureCellGroup(FeatureId) != nullptr) : 0;
}

int32 UJCVDiagramAccessor::GetFeatureCount() const
{
    return Map ? Map->GetFeatureCount() : 0;
}

int32 UJCVDiagramAccessor::GetFeatureGroupCount(uint8 FeatureType) const
{
    return Map ? Map->GetFeatureGroupCount(FeatureType) : 0;
}

int32 UJCVDiagramAccessor::GetLastFeatureGroupIndex(uint8 FeatureType) const
{
    return GetFeatureGroupCount(FeatureType)-1;
}

int32 UJCVDiagramAccessor::GetFeatureCellCount(FJCVFeatureId FeatureId) const
{
    if (Map)
    {
        FJCVCellGroup* FeatureCells = Map->GetFeatureCellGroup(FeatureId);
        return FeatureCells ? FeatureCells->Num() : 0;
    }
    
    return 0;
}

void UJCVDiagramAccessor::ResetFeatures(FJCVFeatureId FeatureId)
{
    if (HasValidMap())
    {
        Map->ResetFeatures(FeatureId.Type, FeatureId.Index);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ResetFeatures() ABORTED, INVALID MAP"));
    }
}

//void UJCVDiagramAccessor::ApplyValueByFeatures(FJCVCellTraits_Deprecated FeatureTraits, float Value)
//{
//    if (HasValidMap())
//    {
//        FJCVValueGenerator::ApplyValueByFeatures(*Map, FeatureTraits, Value);
//    }
//    else
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ApplyValueByFeatures() ABORTED, INVALID MAP"));
//    }
//}

void UJCVDiagramAccessor::ConvertIsolated(uint8 FeatureType0, uint8 FeatureType1, int32 FeatureIndex, bool bGroupFeatures)
{
    if (HasValidMap())
    {
        Map->ConvertIsolated(FeatureType0, FeatureType1, FeatureIndex, bGroupFeatures);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ConvertIsolated() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::ExpandFeature(FJCVFeatureId FeatureId)
{
    if (HasValidMap())
    {
        if (FeatureId.Index < 0)
        {
            Map->ExpandFeature(FeatureId.Type);
        }
        else
        {
            Map->ExpandFeature(FeatureId.Type, FeatureId.Index);
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ExpandFeature() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::ExpandFeatureFromCellGroups(const TArray<FJCVCellRef>& CellRefs, FJCVFeatureId FeatureId, int32 ExpandCount)
{
    if (HasValidMap())
    {
        TArray<FJCVCell*> Cells;
        Cells.Reserve(CellRefs.Num());

        for (const FJCVCellRef& CellRef : CellRefs)
        {
            if (Map->IsValidCell(CellRef.Data))
            {
                Cells.Emplace(&Map->GetCell(CellRef.Data->GetIndex()));
            }
        }

        FJCVFeatureUtility::ExpandFeatureFromCellGroups(*Map, Cells, FeatureId, ExpandCount);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ExpandFeature() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::PointFillIsolatedFeatures(const TArray<FJCVCellRef>& OriginCellRefs, FJCVFeatureId BoundingFeature, FJCVFeatureId TargetFeature)
{
    if (HasValidMap())
    {
        TArray<FJCVCell*> OriginCells;
        OriginCells.Reserve(OriginCellRefs.Num());

        for (int32 i=0; i<OriginCellRefs.Num(); ++i)
        {
            const FJCVCellRef& CellRef(OriginCellRefs[i]);

            if (Map->IsValidCell(CellRef.Data))
            {
                OriginCells.Emplace(&Map->GetCell(CellRef.Data->GetIndex()));
            }
        }

        FJCVFeatureUtility::PointFillIsolated(*Map, BoundingFeature, TargetFeature, OriginCells);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::PointFillIsolatedFeatures() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::PointFillSubdivideFeatures(const TArray<int32>& OriginCellIndices, int32 Seed, uint8 FeatureType, int32 SegmentCount)
{
    if (HasValidMap())
    {
        FRandomStream Rand(Seed);

        FJCVFeatureUtility::PointFillSubdivideFeatures(
            *Map,
            FeatureType,
            OriginCellIndices,
            SegmentCount,
            Rand
            );
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::PointFillSubdivideFeatures() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::GroupByFeatures()
{
    if (HasValidMap())
    {
        Map->GroupByFeatures();
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GroupByFeatures() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::ShrinkFeatures()
{
    if (HasValidMap())
    {
        Map->ShrinkGroups();
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ShrinkFeatures() ABORTED, INVALID MAP"));
    }
}

void UJCVDiagramAccessor::ScaleFeatureValuesByIndex(uint8 FeatureType, int32 IndexOffset)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ScaleFeatureValuesByIndex() ABORTED, INVALID MAP"));
        return;
    }

    FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureType);

    if (FeatureGroupPtr)
    {
        const int32 FeatureCount = GetFeatureGroupCount(FeatureType);
        const int32 LastFeatureIndex = (FeatureCount-1) + IndexOffset;
        const float FeatureCountInv = FeatureCount > 1 ? (1.f/LastFeatureIndex) : 1.f;

        TArray<int32> FeatureIndices;
        Map->GetFeatureIndices(FeatureIndices, FeatureType, -1, true);

        for (const int32 fi : FeatureIndices)
        {
            FJCVCellGroup& CellGroup(FeatureGroupPtr->CellGroups[fi]);
            const float IndexValue = fi + IndexOffset;
            const float Scale = IndexValue * FeatureCountInv;

            for (FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Cell->Value *= Scale;
            }
        }
    }
}

void UJCVDiagramAccessor::InvertFeatureValues(FJCVFeatureId FeatureId)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::InvertFeatureValues() ABORTED, INVALID MAP"));
        return;
    }

    FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureId.Type);

    if (FeatureGroupPtr)
    {
        TArray<int32> FeatureIndices;
        Map->GetFeatureIndices(FeatureIndices, FeatureId.Type, FeatureId.Index, true);

        for (const int32 fi : FeatureIndices)
        {
            FJCVCellGroup& CellGroup(FeatureGroupPtr->CellGroups[fi]);

            for (FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Cell->Value = 1.f-Cell->Value;
            }
        }
    }
}

void UJCVDiagramAccessor::ApplyCurveToFeatureValues(uint8 FeatureType, UCurveFloat* ValueCurve)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ApplyCurveToFeatureValues() ABORTED, INVALID MAP"));
        return;
    }

    if (! IsValid(ValueCurve))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ApplyCurveToFeatureValues() ABORTED, INVALID VALUE CURVE"));
        return;
    }

    FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureType);

    if (FeatureGroupPtr)
    {
        TArray<int32> FeatureIndices;
        Map->GetFeatureIndices(FeatureIndices, FeatureType, -1, true);

        for (const int32 fi : FeatureIndices)
        {
            FJCVCellGroup& CellGroup(FeatureGroupPtr->CellGroups[fi]);

            for (FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Cell->Value = ValueCurve->GetFloatValue(Cell->Value);
            }
        }
    }
}

void UJCVDiagramAccessor::MapNormalizedDistanceFromCell(FJCVCellRef OriginCellRef, FJCVFeatureId FeatureId, bool bAgainstAnyType)
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID MAP"));
        return;
    }

    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID ORIGIN CELL"));
        return;
    }
    
    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID FEATURE TYPE"));
        return;
    }

    FJCVValueGenerator::MapNormalizedDistanceFromCell(*Map, *OriginCell, FeatureId, bAgainstAnyType);
}

void UJCVDiagramAccessor::GetFeaturePoints(TArray<FVector2D>& Points, FJCVFeatureId FeatureId)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFeaturePoints() ABORTED, INVALID MAP"));
        return;
    }
    else
    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFeaturePoints() ABORTED, INVALID FEATURE"));
        return;
    }

    // Get all feature points of a feature type
    if (FeatureId.Index < 0)
    {
        const FJCVCellGroup* CellGroupPtr = Map->GetFeatureCellGroup(FeatureId);

        if (CellGroupPtr)
        {
            const FJCVCellGroup& CellGroup(*CellGroupPtr);

            Points.Reserve(CellGroup.Num());

            for (const FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Points.Emplace(Cell->ToVector2D());
            }
        }
    }
    // Get feature points of a feature cell group
    else
    {
        const FJCVFeatureGroup& FeatureGroup(*Map->GetFeatureGroup(FeatureId.Type));

        for (const FJCVCellGroup& CellGroup : FeatureGroup.CellGroups)
        {
            Points.Reserve(Points.Num() + CellGroup.Num());

            for (const FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                Points.Emplace(Cell->ToVector2DUnsafe());
            }
        }
    }
}

void UJCVDiagramAccessor::GetFeatureCells(TArray<FJCVCellRef>& CellRefs, FJCVFeatureId FeatureId)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFeatureCells() ABORTED, INVALID MAP"));
        return;
    }

    const FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureId.Type);

    if (FeatureGroupPtr)
    {
        TArray<int32> FeatureIndices;
        Map->GetFeatureIndices(FeatureIndices, FeatureId.Type, FeatureId.Index, true);

        int32 CellCount = 0;

        // Accumulate cell count
        for (const int32 fi : FeatureIndices)
        {
            CellCount += FeatureGroupPtr->GetCellCount(fi);
        }

        CellRefs.Reserve(CellCount);

        for (const int32 fi : FeatureIndices)
        {
            const FJCVCellGroup& CellGroup(FeatureGroupPtr->CellGroups[fi]);

            for (const FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                CellRefs.Emplace(Cell);
            }
        }
    }
}

// CELL QUERY FUNCTIONS

int32 UJCVDiagramAccessor::GetCellCount() const
{
    return Map ? Map->Num() : 0;
}

void UJCVDiagramAccessor::GetCellDetails(const FJCVCellRef& CellRef, FJCVCellDetailsRef& CellDetails)
{
    CellDetails.Set(CellRef);
}

void UJCVDiagramAccessor::GetCellGroupDetails(const TArray<FJCVCellRef>& CellRefs, TArray<FJCVCellDetailsRef>& CellDetails)
{
    CellDetails.SetNum(CellRefs.Num());

    for (int32 i=0; i<CellRefs.Num(); ++i)
    {
        CellDetails[i].Set(CellRefs[i].Data);
    }
}

void UJCVDiagramAccessor::GetNeighbourCells(const FJCVCellRef& CellRef, TArray<FJCVCellRef>& NeighbourCellRefs)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetNeighbourCells() ABORTED, INVALID MAP"));
        return;
    }
    else
    if (! Map->IsValidCell(CellRef.Data))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetNeighbourCells() ABORTED, INVALID ORIGIN CELL"));
        return;
    }

    FJCVDiagramMap& MapRef(*Map);

    TArray<const FJCVSite*> Neighbours;
    MapRef->GetNeighbours(*CellRef.Data->Site, Neighbours);

    NeighbourCellRefs.Reserve(Neighbours.Num());

    for (int32 i=0; i<Neighbours.Num(); ++i)
    {
        const FJCVCell* Cell(MapRef.GetCell(Neighbours[i]));
        check(Cell != nullptr);
        NeighbourCellRefs.Emplace(Cell);
    }
}

void UJCVDiagramAccessor::GetCellPositions(const TArray<FJCVCellRef>& CellRefs, TArray<FVector2D>& CellPositions)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetNeighbourCells() ABORTED, INVALID MAP"));
        return;
    }

    CellPositions.Reserve(CellRefs.Num());

    for (const FJCVCellRef& CellRef : CellRefs)
    {
        if (Map->IsValidCell(CellRef.Data))
        {
            CellPositions.Emplace(CellRef.Data->ToVector2DUnsafe());
        }
    }
}

void UJCVDiagramAccessor::GetNeighbourTypes(TArray<uint8>& Types, const FJCVCellRef& CellRef)
{
    if (HasValidMap() && CellRef.Data)
    {
        Map->GetNeighbourTypes(*CellRef.Data, Types);
    }
}

void UJCVDiagramAccessor::GetCellGroupNeighbourTypes(TArray<FJCVCellTypeGroupRef>& TypeGroups, const FJCVCellRefGroup& CellGroup)
{
    const TArray<FJCVCellRef>& Cells(CellGroup.Data);

    TypeGroups.SetNum(Cells.Num());

    if (HasValidMap())
    {
        for (int32 i=0; i<Cells.Num(); ++i)
        {
            if (const FJCVCell* Cell = Cells[i].Data)
            {
                TArray<uint8>& Types(TypeGroups[i].Data);
                Map->GetNeighbourTypes(*Cell, Types);
            }
        }
    }
}

void UJCVDiagramAccessor::GetCellRange(TArray<FJCVCellRef>& CellRefs, const FVector2D& StartPosition, const FVector2D& EndPosition)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID MAP"));
        return;
    }

    FJCVDiagramMap& MapRef(*Map);
    const FBox2D& Bounds(MapRef.GetBounds());
    const FVector2D& v0(StartPosition);
    const FVector2D& v1(EndPosition);

    if (! Bounds.IsInside(v0) || ! Bounds.IsInside(v1))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID INPUT POSITIONS"));
        return;
    }

    const FJCVSite* s0 = MapRef->Find(v0);

    if (s0)
    {
        TArray<const FJCVSite*> Sites;
        const int32 ReserveSize = MapRef.Num()/4;

        Sites.Reserve(ReserveSize);
        CellRefs.Reserve(ReserveSize);

        MapRef->FindAllTo(v1, *s0, Sites);

        for (const FJCVSite* site : Sites)
        {
            CellRefs.Emplace(MapRef.GetCell(site));
        }

        CellRefs.Shrink();
    }
}

FJCVCellRef UJCVDiagramAccessor::GetRandomCell(int32 Seed, const FJCVCellTraitsRef& Traits)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCell() ABORTED, INVALID MAP"));
        return FJCVCellRef();
    }

    if (Map->Num() > 0)
    {
        return FJCVCellRef();
    }

    FRandomStream Rand(Seed);
    const int32 MaxCellCount = Map->Num();

    if (Traits.HasValidTraits())
    {
        FJCVCell* RandomCell = nullptr;

        // Random cell search with valid traits requires max traits check iteration
        int32 TraitsCheckIt = 0;
        for (; TraitsCheckIt<JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION; ++TraitsCheckIt)
        {
            FJCVCell& Cell(Map->GetCell(Rand.RandHelper(MaxCellCount)));

            if (Traits.HasMatchingTraits(Cell))
            {
                RandomCell = &Cell;
                break;
            }
        }
#if JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION
        ensureMsgf(
            (TraitsCheckIt < JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO),
            TEXT("UJCVDiagramAccessor::GetRandomCell() JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO TRAITS CHECK HIT!")
            );
#endif

        return FJCVCellRef(RandomCell);
    }
    else
    {
        return FJCVCellRef(Map->GetCell(Rand.RandHelper(MaxCellCount)));
    }
}

FJCVCellRef UJCVDiagramAccessor::GetRandomCellByFeature(int32 Seed, const FJCVFeatureId& FeatureId, const FJCVCellTraitsRef& Traits)
{
    FJCVCellRef CellRef;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellByFeature() ABORTED, INVALID MAP"));
        return CellRef;
    }

    FRandomStream Rand(Seed);
    const FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureId.Type);

    if (FeatureGroupPtr)
    {
        const FJCVFeatureGroup& FeatureGroup(*FeatureGroupPtr);

        // Find random cells without specified feature index
        if (FeatureId.Index < 0)
        {
            const TArray<FJCVCellGroup>& CellGroups(FeatureGroup.CellGroups);
            const int32 CellGroupCount = CellGroups.Num();
            TArray<int32> NonEmptyCellGroups;

            for (int32 i=0; i<CellGroupCount; ++i)
            {
                if (FeatureGroup.GetCellCount(i) > 0)
                {
                    NonEmptyCellGroups.Emplace(i);
                }
            }

            if (NonEmptyCellGroups.Num() > 0)
            {
                if (Traits.HasValidTraits())
                {
                    // Random cell search with valid traits requires max traits check iteration
                    int32 TraitsCheckIt = 0;
                    for (; TraitsCheckIt<JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION; ++TraitsCheckIt)
                    {
                        int32 CellGroupIndex = Rand.RandHelper(NonEmptyCellGroups.Num());
                        const FJCVCellGroup& CellGroup = CellGroups[NonEmptyCellGroups[CellGroupIndex]];

                        int32 CellIndex = Rand.RandHelper(CellGroup.Num());
                        FJCVCell* Cell(CellGroup[CellIndex]);

                        if (Traits.HasMatchingTraits(*Cell))
                        {
                            CellRef = FJCVCellRef(*Cell);
                            break;
                        }
                    }
#if JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION
                    ensureMsgf(
                        (TraitsCheckIt < JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO),
                        TEXT("UJCVDiagramAccessor::GetRandomCellByFeature() JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO TRAITS CHECK HIT!")
                        );
#endif
                }
                else
                {
                    int32 CellGroupIndex = Rand.RandHelper(NonEmptyCellGroups.Num());
                    const FJCVCellGroup& CellGroup = CellGroups[NonEmptyCellGroups[CellGroupIndex]];
                    int32 CellIndex = Rand.RandHelper(CellGroup.Num());
                    CellRef = FJCVCellRef(CellGroup[CellIndex]);
                }
            }
        }
        // Find random cells with specified feature index
        else
        if (FeatureGroup.CellGroups.IsValidIndex(FeatureId.Index))
        {
            const FJCVCellGroup& CellGroup = FeatureGroup.CellGroups[FeatureId.Index];

            if (Traits.HasValidTraits())
            {
                // Random cell search with valid traits requires max traits check iteration
                int32 TraitsCheckIt = 0;
                for (; TraitsCheckIt<JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION; ++TraitsCheckIt)
                {
                    int32 CellIndex = Rand.RandHelper(CellGroup.Num());
                    FJCVCell* Cell(CellGroup[CellIndex]);

                    if (Traits.HasMatchingTraits(*Cell))
                    {
                        CellRef = FJCVCellRef(*Cell);
                        break;
                    }
                }
#if JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION
                ensureMsgf(
                    (TraitsCheckIt < JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO),
                    TEXT("UJCVDiagramAccessor::GetRandomCellByFeature() JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO TRAITS CHECK HIT!")
                    );
#endif
            }
            else
            {
                int32 CellIndex = Rand.RandHelper(CellGroup.Num());
                CellRef = FJCVCellRef(CellGroup[CellIndex]);
            }
        }
        // Invalid feature index
        else
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellsByFeature() ABORTED, INVALID FEATURE INDEX"));
            return CellRef;
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellsByFeature() ABORTED, INVALID FEATURE TYPE"));
        return CellRef;
    }

    return CellRef;
}

void UJCVDiagramAccessor::GetRandomCells(TArray<FJCVCellRef>& CellRefs, int32 Seed, const FJCVCellTraitsRef& Traits, int32 Count)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCells() ABORTED, INVALID MAP"));
        return;
    }
    if (Count <= 0)
    {
        return;
    }

    FRandomStream Rand(Seed);

    // Find random cell

    const int32 MaxCellCount = Map->Num();
    const int32 CellCount = FMath::Clamp(Count, 0, MaxCellCount);
    const int32 CopyCellThreshold = MaxCellCount / JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO;

    CellRefs.Reset(CellCount);

    if (CellCount < CopyCellThreshold)
    {
        TSet<int32> IndexSet;
        IndexSet.Reserve(CellCount);

        while (CellRefs.Num() < CellCount)
        {
            if (Traits.HasValidTraits())
            {
                // Random cell search with valid traits requires max traits check iteration
                int32 TraitsCheckIt = 0;
                for (; TraitsCheckIt<JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION; ++TraitsCheckIt)
                {
                    int32 CellIndex = Rand.RandHelper(MaxCellCount);

                    if (! IndexSet.Contains(CellIndex))
                    {
                        FJCVCell& Cell(Map->GetCell(CellIndex));

                        if (Traits.HasMatchingTraits(Cell))
                        {
                            CellRefs.Emplace(Cell);
                            IndexSet.Emplace(CellIndex);
                            break;
                        }
                    }
                }
#if JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION
                ensureMsgf(
                    (TraitsCheckIt < JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO),
                    TEXT("UJCVDiagramAccessor::GetRandomCells() JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO TRAITS CHECK HIT!")
                    );
#endif
            }
            else
            {
                int32 CellIndex = Rand.RandHelper(MaxCellCount);

                if (! IndexSet.Contains(CellIndex))
                {
                    CellRefs.Emplace(Map->GetCell(CellIndex));
                    IndexSet.Emplace(CellIndex);
                }
            }
        }
    }
    else
    {
        TArray<int32> CellIndices;
        CellIndices.Reserve(MaxCellCount);

        for (int32 i=0; i<MaxCellCount; ++i)
        {
            CellIndices.Emplace(i);
        }

        while (CellRefs.Num() < CellCount && CellIndices.Num() > 0)
        {
            int32 CellIndex = Rand.RandHelper(CellIndices.Num());
            FJCVCell& Cell(Map->GetCell(CellIndex));

            CellIndices.RemoveAtSwap(CellIndex, 1, false);

            if (Traits.HasMatchingTraits(Cell))
            {
                CellRefs.Emplace(Cell);
            }
        }
    }
}

void UJCVDiagramAccessor::GetRandomCellsByFeature(TArray<FJCVCellRef>& CellRefs, int32 Seed, const FJCVFeatureId& FeatureId, const FJCVCellTraitsRef& Traits, int32 Count)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellsByFeature() ABORTED, INVALID MAP"));
        return;
    }

    if (Count <= 0)
    {
        return;
    }

    FRandomStream Rand(Seed);
    const FJCVFeatureGroup* FeatureGroupPtr = Map->GetFeatureGroup(FeatureId.Type);

    if (FeatureGroupPtr)
    {
        const FJCVFeatureGroup& FeatureGroup(*FeatureGroupPtr);

        // Find random cells without specified feature index
        if (FeatureId.Index < 0)
        {
            const TArray<FJCVCellGroup>& CellGroups(FeatureGroup.CellGroups);
            const int32 FeatureCellCount = FeatureGroup.GetCellCount();
            const int32 CellCount = FMath::Clamp(Count, 0, FeatureCellCount);

            CellRefs.Reserve(CellCount);

            // Feature cell count equals specified output count, copy feature cells
            if (CellCount == FeatureCellCount)
            {
                for (const FJCVCellGroup& CellGroup : CellGroups)
                {
                    for (FJCVCell* Cell : CellGroup)
                    {
                        if (Traits.HasMatchingTraits(*Cell))
                        {
                            CellRefs.Emplace(Cell);
                        }
                    }
                }
            }
            // Randomly assign cell from feature cells
            else
            {
                // Generate feature cell copy. Random cell will be fetch
                // from the container to avoid duplicates.

                TArray<FJCVCell*> FeatureCells;
                FeatureCells.Reserve(FeatureCellCount);

                for (const FJCVCellGroup& CellGroup : CellGroups)
                {
                    FeatureCells.Append(CellGroup);
                }

                while (CellRefs.Num() < CellCount && FeatureCells.Num() > 0)
                {
                    int32 CellIndex = Rand.RandHelper(FeatureCells.Num());
                    FJCVCell* Cell(FeatureCells[CellIndex]);

                    FeatureCells.RemoveAtSwap(CellIndex, 1, false);

                    if (Traits.HasMatchingTraits(*Cell))
                    {
                        CellRefs.Emplace(Cell);
                    }
                }
            }
        }
        // Find random cells with specified feature index
        else
        if (FeatureGroup.CellGroups.IsValidIndex(FeatureId.Index))
        {
            const FJCVCellGroup& CellGroup(FeatureGroup.CellGroups[FeatureId.Index]);
            const int32 FeatureCellCount = CellGroup.Num();
            const int32 CellCount = FMath::Clamp(Count, 0, FeatureCellCount);

            CellRefs.Reserve(CellCount);

            // Feature cell count equals specified output count, copy feature cells
            if (CellCount == FeatureCellCount)
            {
                for (FJCVCell* Cell : CellGroup)
                {
                    if (Traits.HasMatchingTraits(*Cell))
                    {
                        CellRefs.Emplace(Cell);
                    }
                }
            }
            // Randomly assign cell from cell indices
            else
            {
                // Generate feature cell copy. Random cell will be fetch
                // from the container to avoid duplicates.

                TArray<FJCVCell*> FeatureCells = CellGroup;

                while (CellRefs.Num() < CellCount && FeatureCells.Num() > 0)
                {
                    int32 CellIndex = Rand.RandHelper(FeatureCells.Num());
                    FJCVCell* Cell(FeatureCells[CellIndex]);

                    FeatureCells.RemoveAtSwap(CellIndex, 1, false);

                    if (Traits.HasMatchingTraits(Cell))
                    {
                        CellRefs.Emplace(Cell);
                    }
                }
            }
        }
        // Invalid feature index
        else
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellsByFeature() ABORTED, INVALID FEATURE INDEX"));
            return;
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellsByFeature() ABORTED, INVALID FEATURE TYPE"));
        return;
    }
}

TArray<int32> UJCVDiagramAccessor::GetRandomCellWithinFeature(uint8 FeatureType, int32 CellCount, int32 Seed, bool bAllowBorders, int32 MinCellDistance)
{
    TArray<int32> OutIndices;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellWithinFeature() ABORTED, INVALID MAP"));
        return OutIndices;
    }

    FJCVCellGroup Cells;
    FRandomStream Rand(Seed);

    FJCVFeatureUtility::GetRandomCellWithinFeature(
        Cells,
        *Map,
        FeatureType,
        CellCount,
        Rand,
        bAllowBorders,
        MinCellDistance
        );

    OutIndices.Reserve(Cells.Num());

    for (const FJCVCell* c : Cells)
    {
        if (c)
        {
            OutIndices.Emplace(c->GetIndex());
        }
    }

    return OutIndices;
}

FJCVCellRef UJCVDiagramAccessor::GetClosestCellAt(const FVector2D& Pos)
{
    if (HasValidMap())
    {
        const FJCVSite* Site = Map->GetDiagram().FindClosest(Pos);
        return FJCVCellRef(Map->GetCell(Site));
    }
    return FJCVCellRef();
}

void UJCVDiagramAccessor::GetCellsWithinRect(TArray<FJCVCellRef>& CellRefs, const FBox2D& Rect)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellsWithinRect() ABORTED, INVALID MAP"));
        return;
    }

    FJCVDiagramMap& MapRef(*Map);

    if (! Rect.bIsValid)
    {
        return;
    }

    const FJCVSite* SearchSite = MapRef->FindClosest(Rect.Min);

    TArray<const FJCVSite*> Sites;
    MapRef->FindAllWithin(Rect, *SearchSite, Sites);

    CellRefs.Reserve(Sites.Num());

    for (const FJCVSite* Site : Sites)
    {
        CellRefs.Emplace(MapRef.GetCell(Site));
    }
}

void UJCVDiagramAccessor::GetCellWithinOriginRadius(
    TArray<FJCVCellRef>& CellRefs,
    const FJCVCellRef& OriginCellRef,
    float Radius,
    FJCVFeatureId FeatureId,
    bool bAgainstAnyType
    )
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellWithOriginRadius() ABORTED, INVALID MAP"));
        return;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellWithOriginRadius() ABORTED, INVALID ORIGIN CELL"));
        return;
    }


    FJCVDiagramMap& MapRef(*Map);
    TSet<const FJCVCell*> VisitedSet;
    TQueue<const FJCVCell*> ExpandQueue;

    CellRefs.Emplace(OriginCellRef);
    VisitedSet.Emplace(OriginCell);
    ExpandQueue.Enqueue(OriginCell);

    const FVector2D Center(OriginCell->ToVector2D());
    const float RadiusSq = Radius * Radius;

    while (! ExpandQueue.IsEmpty())
    {
        const FJCVCell* Cell;
        ExpandQueue.Dequeue(Cell);

        check(Cell);

        TArray<const FJCVSite*> Neighbours;
        MapRef->GetNeighbours(*Cell->Site, Neighbours);

        for (const FJCVSite* Site : Neighbours)
        {
            const FJCVCell* Neighbour(MapRef.GetCell(Site));

            if (! Neighbour || VisitedSet.Contains(Neighbour))
            {
                continue;
            }

            TArray<FVector2D> Points;
            MapRef->GetPoints(*Site, Points);

            for (const FVector2D& Point : Points)
            {
                if ((Point-Center).SizeSquared() < RadiusSq)
                {
                    VisitedSet.Emplace(Neighbour);
                    ExpandQueue.Enqueue(Neighbour);

                    if (bAgainstAnyType || Neighbour->IsType(FeatureId.Type, FeatureId.Index))
                    {
                        CellRefs.Emplace(Neighbour);
                    }

                    break;
                }
            }
        }
    }
}

FJCVCellRefGroup UJCVDiagramAccessor::FindBorderCells(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders, bool bAgainstAnyType)
{
    FJCVCellRefGroup CellGroup;

    if (! HasValidMap())
    {
        return CellGroup;
    }

    const uint8 ft0 = FeatureType0;
    const uint8 ft1 = FeatureType1;

    // Feature type is equal and wildcard check flag is set to false, abort
    if (! bAgainstAnyType && ft0 == ft1)
    {
        return CellGroup;
    }

    FJCVCellSet CellSet;
    Map->GetBorderCells(CellSet, ft0, ft1, bAllowBorders, bAgainstAnyType);

    CellGroup.Data.Reserve(CellSet.Num());

    for (const FJCVCell* Cell : CellSet)
    {
        CellGroup.Data.Emplace(Cell);
    }

    return CellGroup;
}

void UJCVDiagramAccessor::FindJunctionCells(TArray<FJCVCellJunctionRef>& JunctionRefs, uint8 FeatureType)
{
    if (! HasValidMap())
    {
        return;
    }

    const uint8 ft = FeatureType;

    FJCVCellSet CellSet;
    Map->GetBorderCells(CellSet, ft, -1, false, true);

    TArray<FJCVCellJunction> Junctions;

    for (FJCVCell* Cell : CellSet)
    {
        check(Cell != nullptr);

        TArray<FJCVCellJunction> CellJunctions;
        Map->GetJunctionCells(*Cell, CellJunctions);

        Junctions.Append(CellJunctions);
    }

    for (const FJCVCellJunction& Junction : Junctions)
    {
        JunctionRefs.Emplace(Junction.Point, Junction.Cells);
    }

    return;
}

void UJCVDiagramAccessor::FindEdgePoints(
    TArray<FJCVPointGroup>& PointGroups,
    uint8 FeatureType0,
    uint8 FeatureType1,
    bool bAllowBorders,
    bool bAgainstAnyType
    )
{
    if (! HasValidMap())
    {
        return;
    }

    const uint8 ft0 = FeatureType0;
    const uint8 ft1 = FeatureType1;
    const int32 FeatureGroupCount = Map->GetFeatureGroupCount(ft0);

    // Feature type is equal and wildcard check flag is set to false, abort
    if (! bAgainstAnyType && ft0 == ft1)
    {
        return;
    }

    for (int32 fi=0; fi<FeatureGroupCount; ++fi)
    {
        FJCVConstCellSet BorderCellSet;
        FJCVConstEdgeSet GraphEdgeSet;

        // Find border cells
        Map->GetBorderCells(BorderCellSet, ft0, ft1, fi, bAllowBorders, bAgainstAnyType);

        // No border cell, skip
        if (BorderCellSet.Num() == 0)
        {
            continue;
        }

        GraphEdgeSet.Reserve(BorderCellSet.Num());

        // Find border cells' graph edge set

        if (bAgainstAnyType)
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, bAllowBorders);
        }
        else
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, ft1, bAllowBorders);
        }

        // Generate edge point linked lists

        TArray<FJCVCellEdgeList> EdgeLists;
        Map->GenerateSortedBorderEdges(GraphEdgeSet, EdgeLists);

        // Gather point array from point list

        for (const FJCVCellEdgeList& EdgeList : EdgeLists)
        {
            const FJCVCellEdgeList::FPointList& Edges(EdgeList.PointList);
            const int32 pgi = PointGroups.Num();

            if (Edges.Num() > 0)
            {
                PointGroups.SetNum(pgi+1);

                TArray<FVector2D>& Points(PointGroups[pgi].Points);
                Points.Reserve(Edges.Num());

                for (const FVector2D& Pt : Edges)
                {
                    Points.Emplace(Pt);
                }
            }
        }
    }

    return;
}

bool UJCVDiagramAccessor::FindEdgePointsWithEndPoints(
    TArray<FJCVPointGroup>& PointGroups,
    TArray<FJCVCellRefGroup>& EndPointCellGroups,
    uint8 FeatureType0,
    uint8 FeatureType1,
    bool bAllowBorders,
    bool bAgainstAnyType
    )
{
    PointGroups.Empty();
    EndPointCellGroups.Empty();

    if (! HasValidMap())
    {
        return false;
    }

    const uint8 ft0 = FeatureType0;
    const uint8 ft1 = FeatureType1;
    const int32 FeatureGroupCount = Map->GetFeatureGroupCount(ft0);

    // Feature type is equal and wildcard check flag is set to false, abort
    if (! bAgainstAnyType && ft0 == ft1)
    {
        return false;
    }

    for (int32 fi=0; fi<FeatureGroupCount; ++fi)
    {
        FJCVConstCellSet BorderCellSet;
        FJCVConstEdgeSet GraphEdgeSet;

        // Find border cells
        Map->GetBorderCells(BorderCellSet, ft0, ft1, fi, bAllowBorders, bAgainstAnyType);

        // No border cell, skip
        if (BorderCellSet.Num() == 0)
        {
            continue;
        }

        GraphEdgeSet.Reserve(BorderCellSet.Num());

        // Find border cells' graph edge set

        if (bAgainstAnyType)
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, bAllowBorders);
        }
        else
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, ft1, bAllowBorders);
        }

        // Generate edge point linked lists

        TArray<FJCVCellEdgeList> EdgeLists;
        Map->GenerateSortedBorderEdges(GraphEdgeSet, EdgeLists);

        // Gather point array from point list

        for (const FJCVCellEdgeList& EdgeList : EdgeLists)
        {
            const FJCVCellEdgeList::FPointList& Edges(EdgeList.PointList);
            const FJCVCellEdgeList::FEdgePair& EdgePair(EdgeList.EdgePair);

            const int32 pgi = PointGroups.Num();

            if (Edges.Num() > 0)
            {
                // Allocate point group output

                PointGroups.SetNum(pgi+1);

                TArray<FVector2D>& Points(PointGroups[pgi].Points);
                Points.Reserve(Edges.Num());

                for (const FVector2D& Pt : Edges)
                {
                    Points.Emplace(Pt);
                }

                // Allocate edge pair group output

                EndPointCellGroups.SetNum(pgi+1);

                FJCVCellRefGroup& EndPointCellGroup(EndPointCellGroups[pgi]);

                check(Map->GetCell(EdgePair.Get<0>()) != nullptr);
                check(Map->GetCell(EdgePair.Get<1>()) != nullptr);

                EndPointCellGroup.Data.Emplace(Map->GetCell(EdgePair.Get<0>()));
                EndPointCellGroup.Data.Emplace(Map->GetCell(EdgePair.Get<1>()));
            }
        }
    }

    return true;
}

TArray<FJCVPointGroup> UJCVDiagramAccessor::GenerateOrderedFeatureBorderPoints(
    uint8 InitialFeatureType,
    TArray<uint8> AdditionalFeatures,
    bool bExpandEdges,
    bool bAllowBorders
    )
{
    TArray<FJCVPointGroup> PointGroups;

    if (! HasValidMap())
    {
        return PointGroups;
    }

    TArray<FJCVCellEdgeList> EdgeLists;

    const int32 fn = Map->GetFeatureCount();

    for (int32 ft0=InitialFeatureType; ft0<fn; ++ft0)
    {
        int32 EdgeGroupStartIndex = EdgeLists.Num();

        // Generate feature border points
        for (int32 ft1=ft0+1; ft1<fn; ++ft1)
        {
            GenerateOrderedFeatureBorderPoints(EdgeLists, ft0, ft1, bAllowBorders);
        }

        // Generate additional feature border points
        for (int32 i=0; i<AdditionalFeatures.Num(); ++i)
        {
            if (ft0 != AdditionalFeatures[i])
            {
                GenerateOrderedFeatureBorderPoints(EdgeLists, ft0, AdditionalFeatures[i], bAllowBorders);
            }
        }

        // Merge connected border edges

        int32 EdgeGroupCount = EdgeLists.Num();

        for (int32 egi0=EdgeGroupStartIndex; egi0<(EdgeGroupCount-1); ++egi0)
        {
            FJCVCellEdgeList& el0(EdgeLists[egi0]);
            FJCVCellEdgeList::FEdgePair& ep0(el0.EdgePair);
            FJCVCellEdgeList::FPointList& eg0(el0.PointList);

            check(eg0.Num() > 0);

            const FVector2D& eg0Head(eg0.GetHead()->GetValue());
            const FVector2D& eg0Tail(eg0.GetTail()->GetValue());

            int32 egi1 = egi0+1;

            while (egi1 < EdgeGroupCount)
            {
                const FJCVCellEdgeList& el1(EdgeLists[egi1]);
                const FJCVCellEdgeList::FEdgePair& ep1(el1.EdgePair);
                const FJCVCellEdgeList::FPointList& eg1(el1.PointList);

                const FVector2D& eg1Head(eg1.GetHead()->GetValue());
                const FVector2D& eg1Tail(eg1.GetTail()->GetValue());

                bool bHasConnection = false;
                bool bHasTail0Connection = false;
                bool bHasTail1Connection = false;

                if (eg0Head.Equals(eg1Head, JCV_EQUAL_THRESHOLD))
                {
                    bHasConnection = true;
                }
                else
                if (eg0Head.Equals(eg1Tail, JCV_EQUAL_THRESHOLD))
                {
                    bHasTail1Connection = true;
                    bHasConnection = true;
                }
                else
                if (eg0Tail.Equals(eg1Head, JCV_EQUAL_THRESHOLD))
                {
                    bHasTail0Connection = true;
                    bHasConnection = true;
                }
                else
                if (eg0Tail.Equals(eg1Tail, JCV_EQUAL_THRESHOLD))
                {
                    bHasTail0Connection = true;
                    bHasTail1Connection = true;
                    bHasConnection = true;
                }

                if (bHasConnection)
                {
                    // Tail0 -> (Head1/Tail1) Connection
                    if (bHasTail0Connection)
                    {
                        if (bHasTail1Connection)
                        {
                            auto* egNode1 = eg1.GetTail();
                            do
                            {
                                eg0.AddTail(egNode1->GetValue());
                            }
                            while ((egNode1 = egNode1->GetPrevNode()) != nullptr);

                            ep0.Get<1>() = ep1.Get<0>();
                        }
                        else
                        {
                            auto* egNode1 = eg1.GetHead();
                            do
                            {
                                eg0.AddTail(egNode1->GetValue());
                            }
                            while ((egNode1 = egNode1->GetNextNode()) != nullptr);

                            ep0.Get<1>() = ep1.Get<1>();
                        }
                    }
                    // Head0 -> (Head1/Tail1) Connection
                    else
                    {
                        if (bHasTail1Connection)
                        {
                            auto* egNode1 = eg1.GetTail();
                            do
                            {
                                eg0.AddHead(egNode1->GetValue());
                            }
                            while ((egNode1 = egNode1->GetPrevNode()) != nullptr);

                            ep0.Get<0>() = ep1.Get<0>();
                        }
                        else
                        {
                            auto* egNode1 = eg1.GetHead();
                            do
                            {
                                eg0.AddHead(egNode1->GetValue());
                            }
                            while ((egNode1 = egNode1->GetNextNode()) != nullptr);

                            ep0.Get<0>() = ep1.Get<1>();
                        }
                    }

                    EdgeLists.RemoveAtSwap(egi1, 1, false);
                    EdgeGroupCount = EdgeLists.Num();

                    // Continue without advancing iteration index
                    // since the current list is removed and every
                    // subsequent list index move forward by 1
                    continue;
                }

                ++egi1;
            }
        }

        // Expand edge list if required

        if (bExpandEdges)
        {
            for (int32 egi=EdgeGroupStartIndex; egi<EdgeLists.Num(); ++egi)
            {
                FJCVCellEdgeList& EdgeList(EdgeLists[egi]);
                FJCVCellEdgeList::FPointList& eg(EdgeList.PointList);

                check(eg.GetTail() != nullptr);
                check(eg.GetHead() != nullptr);

                const FVector2D& PHead(eg.GetHead()->GetValue());
                const FVector2D& PTail(eg.GetTail()->GetValue());
                const bool bIsCircular = PHead.Equals(PTail, JCV_EQUAL_THRESHOLD);

                // Only expand non-circular edge list
                if (! bIsCircular)
                {
                    const FJCVCellEdgeList::FEdgePair& EdgePair(EdgeList.EdgePair);
                    const FJCVEdge* gHead(EdgePair.Get<0>());
                    const FJCVEdge* gTail(EdgePair.Get<1>());

                    check(gHead != nullptr);
                    check(gTail != nullptr);

                    // Generate head edge expansion

                    {
                        const FJCVEdge* g = gHead;
                        const FJCVEdge* gPrev = nullptr;
                        const FJCVEdge* gNext = nullptr;

                        Map->GetAdjacentEdges(g, gPrev, gNext);

                        check(gPrev != nullptr);
                        check(gNext != nullptr);

                        const FJCVEdge* ConnectedEdge = nullptr;

                        // Find edge that is connected to the head point

                        if (Map->IsConnected(PHead, *gPrev))
                        {
                            ConnectedEdge = gPrev;
                        }
                        else
                        if (Map->IsConnected(PHead, *gNext))
                        {
                            ConnectedEdge = gNext;
                        }

                        check(ConnectedEdge != nullptr);

                        if (ConnectedEdge)
                        {
                            const FVector2D gp[2] = {
                                FJCVMathUtil::ToVector2D(ConnectedEdge->pos[0]),
                                FJCVMathUtil::ToVector2D(ConnectedEdge->pos[1])
                                };

                            // Add point that is not connected to the head edge

                            if (Map->IsConnected(gp[0], *g))
                            {
                                eg.AddHead(gp[1]);
                            }
                            else
                            if (Map->IsConnected(gp[1], *g))
                            {
                                eg.AddHead(gp[0]);
                            }
                        }
                    }

                    // Generate tail edge expansion

                    {
                        const FJCVEdge* g = gTail;
                        const FJCVEdge* gPrev = nullptr;
                        const FJCVEdge* gNext = nullptr;

                        Map->GetAdjacentEdges(g, gPrev, gNext);

                        check(gPrev != nullptr);
                        check(gNext != nullptr);

                        const FJCVEdge* ConnectedEdge = nullptr;

                        // Find edge that is connected to the head point

                        if (Map->IsConnected(PTail, *gPrev))
                        {
                            ConnectedEdge = gPrev;
                        }
                        else
                        if (Map->IsConnected(PTail, *gNext))
                        {
                            ConnectedEdge = gNext;
                        }

                        check(ConnectedEdge != nullptr);

                        if (ConnectedEdge)
                        {
                            const FVector2D gp[2] = {
                                FJCVMathUtil::ToVector2D(ConnectedEdge->pos[0]),
                                FJCVMathUtil::ToVector2D(ConnectedEdge->pos[1])
                                };

                            // Add point that is not connected to the head edge

                            if (Map->IsConnected(gp[0], *g))
                            {
                                eg.AddTail(gp[1]);
                            }
                            else
                            if (Map->IsConnected(gp[1], *g))
                            {
                                eg.AddTail(gp[0]);
                            }
                        }
                    }
                } // ! bIsCircular
            }
        } // bExpandEdges
    } // Feature Loop

    // Generate output points

    for (int32 egi=0; egi<EdgeLists.Num(); ++egi)
    {
        const FJCVCellEdgeList& EdgeList(EdgeLists[egi]);
        const FJCVCellEdgeList::FPointList& Edges(EdgeList.PointList);

        const int32 pgi = PointGroups.Num();

        if (Edges.Num() > 0)
        {
            PointGroups.SetNum(pgi+1);

            TArray<FVector2D>& Points(PointGroups[pgi].Points);
            Points.Reserve(Edges.Num());

            for (const FVector2D& Pt : Edges)
            {
                Points.Emplace(Pt);
            }
        }
    }

    return PointGroups;
}


void UJCVDiagramAccessor::GenerateOrderedFeatureBorderPoints(
    TArray<FJCVCellEdgeList>& EdgeLists,
    uint8 FeatureType0,
    uint8 FeatureType1,
    bool bAllowBorders
    ) const
{
    const uint8 ft0 = FeatureType0;
    const uint8 ft1 = FeatureType1;
    const int32 FeatureGroupCount = Map->GetFeatureGroupCount(ft0);
    const bool bAgainstAnyType = false;

    // Generate sorted border edges for the specified feature pair
    for (int32 fi=0; fi<FeatureGroupCount; ++fi)
    {
        FJCVConstCellSet BorderCellSet;
        FJCVConstEdgeSet GraphEdgeSet;

        // Find border cells
        Map->GetBorderCells(BorderCellSet, ft0, ft1, fi, bAllowBorders, bAgainstAnyType);

        // No border cell, skip
        if (BorderCellSet.Num() == 0)
        {
            continue;
        }

        GraphEdgeSet.Reserve(BorderCellSet.Num());

        // Find border cells' graph edge set

        if (bAgainstAnyType)
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, bAllowBorders);
        }
        else
        {
            Map->GetBorderEdges(BorderCellSet, GraphEdgeSet, ft1, bAllowBorders);
        }

        // Generate edge point linked lists

        Map->GenerateSortedBorderEdges(GraphEdgeSet, EdgeLists);
    }

#if 0
    // Merge connected border edges

    TSet<int32> RemovalIndexSet;
    int32 egn0 = EdgeGroups.Num()-1;
    int32 egn1 = egn0+1;

    for (int32 egi0=EdgeGroupStartIndex; egi0<egn0; ++egi0)
    {
        // The current list has already been marked as merged, skip
        if (RemovalIndexSet.Contains(egi0))
        {
            continue;
        }

        TDoubleLinkedList<FVector2D>& eg0(EdgeGroups[egi0]);

        check(eg0.Num() > 0);

        const FVector2D& eg0Head(eg0.GetHead()->GetValue());
        const FVector2D& eg0Tail(eg0.GetTail()->GetValue());

        for (int32 egi1=egi0+1; egi1<egn1; ++egi1)
        {
            // The current list has already been marked as merged, skip
            if (RemovalIndexSet.Contains(egi1))
            {
                continue;
            }

            TDoubleLinkedList<FVector2D>& eg1(EdgeGroups[egi1]);

            const FVector2D& eg1Head(eg1.GetHead()->GetValue());
            const FVector2D& eg1Tail(eg1.GetTail()->GetValue());

            bool bHasConnection = false;
            bool bHasTail0Connection = false;
            bool bHasTail1Connection = false;

            // Find Edge Group 0 connection with Edge Group 1

            if (eg0Head.Equals(eg1Head, JCV_EQUAL_THRESHOLD))
            {
                bHasConnection = true;
            }
            else
            if (eg0Head.Equals(eg1Tail, JCV_EQUAL_THRESHOLD))
            {
                bHasTail1Connection = true;
                bHasConnection = true;
            }
            else
            if (eg0Tail.Equals(eg1Head, JCV_EQUAL_THRESHOLD))
            {
                bHasTail0Connection = true;
                bHasConnection = true;
            }
            else
            if (eg0Tail.Equals(eg1Tail, JCV_EQUAL_THRESHOLD))
            {
                bHasTail0Connection = true;
                bHasTail1Connection = true;
                bHasConnection = true;
            }

            // Connection found, copy Edge Group 1 points to Edge Group 0
            // and mark Edge Group 1 as merged

            if (bHasConnection)
            {
                FJCVConstEdgeGroup& epg0(EndPointEdgeGroups[egi0]);
                FJCVConstEdgeGroup& epg1(EndPointEdgeGroups[egi1]);

                check(epg0.Num() == 2);
                check(epg1.Num() == 2);

                // Tail0 -> (Head1/Tail1) Connection
                if (bHasTail0Connection)
                {
                    if (bHasTail1Connection)
                    {
                        auto* egNode1 = eg1.GetTail();
                        do
                        {
                            eg0.AddTail(egNode1->GetValue());
                        }
                        while ((egNode1 = egNode1->GetPrevNode()) != nullptr);

                        // Transfer endpoints, End Point Tail 0 -> End Point Head 1
                        epg0[1] = epg1[0];
                    }
                    else
                    {
                        auto* egNode1 = eg1.GetHead();
                        do
                        {
                            eg0.AddTail(egNode1->GetValue());
                        }
                        while ((egNode1 = egNode1->GetNextNode()) != nullptr);

                        // Transfer endpoints, End Point Tail 0 -> End Point Tail 1
                        epg0[1] = epg1[1];
                    }
                }
                // Head0 -> (Head1/Tail1) Connection
                else
                {
                    if (bHasTail1Connection)
                    {
                        auto* egNode1 = eg1.GetTail();
                        do
                        {
                            eg0.AddHead(egNode1->GetValue());
                        }
                        while ((egNode1 = egNode1->GetPrevNode()) != nullptr);

                        // Transfer endpoints, End Point Head 0 -> End Point Head 1
                        epg0[0] = epg1[0];
                    }
                    else
                    {
                        auto* egNode1 = eg1.GetHead();
                        do
                        {
                            eg0.AddHead(egNode1->GetValue());
                        }
                        while ((egNode1 = egNode1->GetNextNode()) != nullptr);

                        // Transfer endpoints, End Point Head 0 -> End Point Tail 1
                        epg0[0] = epg1[1];
                    }
                }

                RemovalIndexSet.Emplace(egi1);
            }
        }
    }
#endif
}

FJCVCellRefGroup UJCVDiagramAccessor::ExpandCellQuery(const FJCVCellRef& CellRef, int32 ExpandCount, FJCVFeatureId FeatureId, bool bAgainstAnyType)
{
    FJCVCellRefGroup CellGroup;
    TArray<FJCVCellRef>& Cells(CellGroup.Data);

    if (! HasValidMap() || ! CellRef.Data)
    {
        return CellGroup;
    }

    FJCVDiagramMap& MapRef(*Map);
    TSet<const FJCVCell*> VisitedSet;
    TArray<const FJCVCell*> ExpandQueue0;
    TArray<const FJCVCell*> ExpandQueue1;

    Cells.Emplace(CellRef);
    VisitedSet.Emplace(CellRef.Data);
    ExpandQueue0.Emplace(CellRef.Data);

    for (int32 It=0; It<ExpandCount; ++It)
    {
        for (const FJCVCell* Cell : ExpandQueue0)
        {
            check(Cell);

            TArray<const FJCVSite*> Neighbours;
            MapRef->GetNeighbours(*Cell->Site, Neighbours);

            for (const FJCVSite* Site : Neighbours)
            {
                const FJCVCell* Neighbour(MapRef.GetCell(Site));

                if (! Neighbour || VisitedSet.Contains(Neighbour))
                {
                    continue;
                }

                VisitedSet.Emplace(Neighbour);
                ExpandQueue1.Emplace(Neighbour);

                if (bAgainstAnyType || Neighbour->IsType(FeatureId.Type, FeatureId.Index))
                {
                    Cells.Emplace(Neighbour);
                }
            }
        }

        ExpandQueue0 = MoveTemp(ExpandQueue1);
    }

    return CellGroup;
}

FJCVCellRefGroup UJCVDiagramAccessor::ExpandCellGroupQuery(const FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId, int32 ExpandCount, bool bAgainstAnyType)
{
    FJCVCellRefGroup OutGroup;
    TArray<FJCVCellRef>& Cells(OutGroup.Data);

    if (! HasValidMap() || CellGroup.Data.Num() < 0)
    {
        return OutGroup;
    }

    FJCVDiagramMap& MapRef(*Map);
    TSet<const FJCVCell*> VisitedSet;
    TArray<const FJCVCell*> ExpandQueue0;
    TArray<const FJCVCell*> ExpandQueue1;

    Cells.Reserve(CellGroup.Data.Num());
    VisitedSet.Reserve(CellGroup.Data.Num());
    ExpandQueue0.Reserve(CellGroup.Data.Num());

    for (const FJCVCellRef& CellRef : CellGroup.Data)
    {
        const FJCVCell* Cell(CellRef.Data);

        if (Cell)
        {
            Cells.Emplace(Cell);
            VisitedSet.Emplace(Cell);
            ExpandQueue0.Emplace(Cell);
        }
    }

    for (int32 It=0; It<ExpandCount; ++It)
    {
        for (const FJCVCell* Cell : ExpandQueue0)
        {
            check(Cell);

            TArray<const FJCVSite*> Neighbours;
            MapRef->GetNeighbours(*Cell->Site, Neighbours);

            for (const FJCVSite* Site : Neighbours)
            {
                const FJCVCell* Neighbour(MapRef.GetCell(Site));

                if (! Neighbour || VisitedSet.Contains(Neighbour))
                {
                    continue;
                }

                VisitedSet.Emplace(Neighbour);
                ExpandQueue1.Emplace(Neighbour);

                if (bAgainstAnyType || Neighbour->IsType(FeatureId.Type, FeatureId.Index))
                {
                    Cells.Emplace(Neighbour);
                }
            }
        }

        ExpandQueue0 = MoveTemp(ExpandQueue1);
    }

    return OutGroup;
}

void UJCVDiagramAccessor::FilterCellsByType(FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId)
{
    TArray<FJCVCellRef>& Cells(CellGroup.Data);
    int32 i = 0;

    while (i < Cells.Num())
    {
        if (Cells[i].Data && ! Cells[i].Data->IsType(FeatureId.Type, FeatureId.Index))
        {
            Cells.RemoveAtSwap(i, 1, false);
            continue;
        }

        i++;
    }
}

void UJCVDiagramAccessor::ExcludeCellsByType(FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId)
{
    TArray<FJCVCellRef>& Cells(CellGroup.Data);
    int32 i = 0;

    while (i < Cells.Num())
    {
        if (Cells[i].Data && Cells[i].Data->IsType(FeatureId.Type, FeatureId.Index))
        {
            Cells.RemoveAtSwap(i, 1, false);
            continue;
        }

        i++;
    }
}

FJCVCellRefGroup UJCVDiagramAccessor::MergeCellGroups(const TArray<FJCVCellRefGroup>& CellGroups)
{
    FJCVCellRefGroup CellGroup;
    TArray<FJCVCellRef>& Cells(CellGroup.Data);

    TSet<const FJCVCell*> CellSet;
    int32 CellCount = 0;

    for (const FJCVCellRefGroup& cg : CellGroups)
    {
        CellCount += cg.Data.Num();
    }

    CellSet.Reserve(CellCount);

    for (const FJCVCellRefGroup& cg : CellGroups)
    for (const FJCVCellRef& Cell : cg.Data)
    {
        if (Cell.Data)
        {
            CellSet.Emplace(Cell.Data);
        }
    }

    Cells.Reserve(CellSet.Num());

    for (const FJCVCell* Cell : CellSet)
    {
        Cells.Emplace(Cell);
    }

    return CellGroup;
}

TArray<FJCVCellJunctionRef> UJCVDiagramAccessor::FilterUniqueJunctions(const TArray<FJCVCellJunctionRef>& Junctions)
{
    TArray<FJCVCellJunctionRef> UniqueJunctions;
    UniqueJunctions.Reserve(Junctions.Num());

    for (const FJCVCellJunctionRef& Junction0 : Junctions)
    {
        bool bJunctionExists = false;

        for (const FJCVCellJunctionRef& Junction1 : UniqueJunctions)
        {
            if (Junction0.Point.Equals(Junction1.Point, JCV_EQUAL_THRESHOLD))
            {
                bJunctionExists = true;
                break;
            }
        }

        if (! bJunctionExists)
        {
            UniqueJunctions.Emplace(Junction0);
        }
    }

    return UniqueJunctions;
}

float UJCVDiagramAccessor::GetClosestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId)
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetClosestDistanceToFeature() ABORTED, INVALID MAP"));
        return 0.f;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetClosestDistanceToFeature() ABORTED, INVALID ORIGIN CELL"));
        return 0.f;
    }

    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID FEATURE TYPE"));
        return 0.f;
    }

    return FJCVCellUtility::GetClosestDistanceFromCell(*Map, *OriginCell, FeatureId, false);
}

float UJCVDiagramAccessor::GetFurthestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId)
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID MAP"));
        return 0.f;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID ORIGIN CELL"));
        return 0.f;
    }

    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID FEATURE TYPE"));
        return 0.f;
    }

    return FJCVCellUtility::GetFurthestDistanceFromCell(*Map, *OriginCell, FeatureId, false);
}

// CELL UTILITY FUNCTIONS

//TArray<int32> UJCVDiagramAccessor::GenerateCellGridIndices(const FJCVCellRef& Cell, const FIntPoint& GridDimension, const float BoundsExpand)
//{
//    TArray<int32> Indices;
//
//    if (! HasValidMap() || ! Cell.Data || ! Cell.Data->Site)
//    {
//        return Indices;
//    }
//
//    const FJCVSite& Site(*Cell.Data->Site);
//    FJCVDiagramMap& MapRef(*Map);
//
//    FBox2D Bounds;
//    FVector2D SiteCenter(Site.p.x, Site.p.y);
//    float SiteCenterRadiusSq;
//
//    MapRef->GetSiteBounds(Site, Bounds);
//    SiteCenterRadiusSq = MapRef->GetShortestMidPoint(Site);
//
//    if (! Bounds.bIsValid)
//    {
//        return Indices;
//    }
//
//    FVector2D VecDim(GridDimension.X, GridDimension.Y);
//
//    Bounds = Bounds.ExpandBy(BoundsExpand);
//    Bounds.Min = FMath::Clamp(Bounds.Min, FVector2D::ZeroVector, VecDim);
//    Bounds.Max = FMath::Clamp(Bounds.Max, FVector2D::ZeroVector, VecDim);
//
//    const FIntPoint IntMin(Bounds.Min.X, Bounds.Min.Y);
//    const FIntPoint IntMax(Bounds.Max.X, Bounds.Max.Y);
//    const int32 CountX = IntMax.X-IntMin.X;
//    const int32 CountY = IntMax.Y-IntMin.Y;
//    const int32 Stride = GridDimension.X;
//
//    Indices.Reserve(CountX*CountY);
//
//    for (int32 y=IntMin.Y; y<IntMax.Y; ++y)
//    for (int32 x=IntMin.X; x<IntMax.X; ++x)
//    {
//        FVector2D Pos(x, y);
//
//        if ((Pos-SiteCenter).SizeSquared() < SiteCenterRadiusSq || MapRef->IsWithin(Site, Pos))
//        {
//            Indices.Emplace(x+y*Stride);
//        }
//    }
//
//    Indices.Shrink();
//
//    return Indices;
//}

// MAP UTILITY FUNCTIONS

void UJCVDiagramAccessor::GenerateSegments(const TArray<FVector2D>& SegmentOrigins, int32 SegmentMergeCount, int32 Seed)
{
    if (HasValidMap())
    {
        FRandomStream Rand(Seed);
        FJCVFeatureUtility::GenerateSegmentExpands(*Map, SegmentOrigins, SegmentMergeCount, Rand);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateSegments() ABORTED, INVALID MAP"));
    }
}

//void UJCVDiagramAccessor::GenerateOrogeny(UJCVDiagramAccessor* PlateAccessor, int32 Seed, FJCVRadialFill FillParams, const FJCVOrogenParams& OrogenParams)
//{
//    if (! HasValidMap())
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateOrogeny() ABORTED, INVALID MAP"));
//        return;
//    }
//
//    if (! IsValid(PlateAccessor) || ! PlateAccessor->HasValidMap())
//    {
//        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateOrogeny() ABORTED, INVALID PLATE ISLAND"));
//        return;
//    }
//
//    FJCVDiagramMap& plate( PlateAccessor->GetMap() );
//    FJCVDiagramMap& landscape( *Map );
//
//    // Generates orogeny
//    FRandomStream Rand(Seed);
//    FJCVPlateGenerator::FOrogenParams orogenParams(FillParams, OrogenParams.OriginThreshold, OrogenParams.bDivergentAsConvergent);
//    FJCVPlateGenerator::GenerateOrogeny(plate, landscape, orogenParams, Rand);
//
//    // Mark features with prepared set
//    FJCVCellSet cellS;
//    cellS.Reserve(landscape.Num());
//    const float threshold = OrogenParams.AreaThreshold;
//
//    // Assign Map Feature Type
//    FJCVValueTraits_Deprecated ValueTraits(threshold, 100.f, OrogenParams.FeatureType);
//    FJCVValueGenerator::MarkFeatures(landscape, ValueTraits, cellS);
//
//    landscape.GroupByFeatures();
//}

void UJCVDiagramAccessor::GenerateDualGeometry(FJCVDualGeometry& Geometry, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID MAP"));
        return;
    }

    int32 CellCount = Map->Num();

    // Reserve container size

    TArray<FVector>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
    }

    Points.Reserve(CellCount);
    PolyIndices.Reserve(CellCount * 3 * 8);
    CellIndices.Reserve(CellCount);

    // Visit cells

    TSet<FIntPoint> VisitedPointSet;
    TMap<int32, int32> CellIndexMap;

    TFunctionRef<void(const FJCVPoint&, const FJCVCell&, const FJCVCell&, const FJCVCell&)> CallbackDual(
        [&](const FJCVPoint& Point, const FJCVCell& c0, const FJCVCell& c1, const FJCVCell& c2)
        {
            GetPointDualGeometry(
                VisitedPointSet,
                CellIndexMap,
                Points,
                PolyIndices,
                CellIndices,
                Point,
                c2,
                c1,
                c0
                );
        } );

    for (int32 i=0; i<CellCount; ++i)
    {
        Map->VisitCellDuals(Map->GetCell(i), CallbackDual);
    }

    // Shrink geometry containers

    Points.Shrink();
    PolyIndices.Shrink();
    CellIndices.Shrink();
}

void UJCVDiagramAccessor::GeneratePolyGeometry(FJCVPolyGeometry& Geometry, bool bFilterDuplicates, bool bUseCellAverageValue, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GeneratePolyGeometry() ABORTED, INVALID MAP"));
        return;
    }

    // Reserve container size

    const int32 CellCount = Map->Num();

    TArray<FVector>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);
    TArray<int32>& CellPolyCounts(Geometry.CellPolyCounts);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
        CellPolyCounts.Reset();
    }

    Points.Reserve(CellCount * 8);
    PolyIndices.Reserve(CellCount * 3 * 8);
    CellIndices.Reserve(CellCount);
    CellPolyCounts.Reserve(CellCount);

    TMap<FIntPoint, int32> PointIndexMap;
    TArray<FVector> CellPoints;

    for (int32 ci=0; ci<CellCount; ++ci)
    {
        const FJCVCell& Cell(Map->GetCell(ci));

        CellPoints.Reset();
        Map->GetCellPointValues(CellPoints, Cell);

        const int32 CellPointCount = CellPoints.Num();
        const int32 CellPointIndex = Points.Num();

        float CellValue = Cell.GetValue();
        float PointSum  = 0.f;

        CellIndices.Emplace(Cell.GetIndex());
        CellPolyCounts.Emplace(CellPointCount+1);
        Points.Emplace(Cell.ToVector2D(), CellValue);

        if (bFilterDuplicates)
        {
            for (int32 i=0; i<CellPointCount; ++i)
            {
                int32 pi0 = i;
                int32 pi1 = (i+1) % CellPointCount;

                const FVector& p0(CellPoints[pi0]);
                const FVector& p1(CellPoints[pi1]);

                FIntPoint pid0(FJCVMathUtil::ToIntPointScaled(p0.X, p0.Y));
                FIntPoint pid1(FJCVMathUtil::ToIntPointScaled(p1.X, p1.Y));

                if (! PointIndexMap.Contains(pid0))
                {
                    PointIndexMap.Emplace(pid0, Points.Num());
                    Points.Emplace(p0);
                }

                if (! PointIndexMap.Contains(pid1))
                {
                    PointIndexMap.Emplace(pid1, Points.Num());
                    Points.Emplace(p1);
                }

                PolyIndices.Emplace(CellPointIndex);
                PolyIndices.Emplace(PointIndexMap.FindChecked(pid1));
                PolyIndices.Emplace(PointIndexMap.FindChecked(pid0));

                PointSum += p0.Z;
            }
        }
        else
        {
            Points.Emplace(CellPoints[0]);
            PointSum += CellPoints[0].Z;

            for (int32 i=1; i<CellPointCount; ++i)
            {
                Points.Emplace(CellPoints[i]);

                PolyIndices.Emplace(CellPointIndex);
                PolyIndices.Emplace(Points.Num()-1);
                PolyIndices.Emplace(Points.Num()-2);

                PointSum += CellPoints[i].Z;
            }

            PolyIndices.Emplace(CellPointIndex);
            PolyIndices.Emplace(Points.Num()-CellPointCount);
            PolyIndices.Emplace(Points.Num()-1);
        }

        if (bUseCellAverageValue && CellPointCount > 0)
        {
            Points[CellPointIndex].Z = PointSum / CellPointCount;
        }
    }

    // Shrink geometry containers

    Points.Shrink();
    PolyIndices.Shrink();
    CellIndices.Shrink();
    CellPolyCounts.Shrink();
}

void UJCVDiagramAccessor::GenerateDualGeometryByFeature(FJCVDualGeometry& Geometry, FJCVFeatureId FeatureId, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID MAP"));
        return;
    }

    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID FEATURE TYPE"));
        return;
    }

    // Get feature cell group indices

    const FJCVFeatureGroup& FeatureGroup(*Map->GetFeatureGroup(FeatureId.Type));
    int32 CellCount = 0;

    TArray<int32> FeatureIndices;
    Map->GetFeatureIndices(FeatureIndices, FeatureId.Type, FeatureId.Index, true);

    for (const int32 fi : FeatureIndices)
    {
        CellCount += FeatureGroup.GetCellCount(fi);
    }

    // Reserve container size

    TArray<FVector>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
    }

    // Visit cells

    //TSet<FIntPoint> VisitedPointSet;
    TMap<int32, int32> CenterCellIndexMap;
    TMap<int32, int32> CellIndexMap;

    // Reserve containers

    Points.Reserve(CellCount);
    PolyIndices.Reserve(CellCount * 3 * 8);
    CellIndices.Reserve(CellCount);
    CenterCellIndexMap.Reserve(CellCount / 2);

    TFunctionRef<void(const FJCVPoint&, const FJCVCell&, const FJCVCell&, const FJCVCell&)> CallbackDual(
        [&](const FJCVPoint& Point,
            const FJCVCell& CenterCell,
            const FJCVCell& NeighbourCell0,
            const FJCVCell& NeighbourCell1
            )
        {
            int32 CenterCellIndex = CenterCell.GetIndex();
            int32 CenterVertIndex;

            if (CenterCellIndexMap.Contains(CenterCellIndex))
            {
                CenterVertIndex = CenterCellIndexMap.FindChecked(CenterCellIndex);
            }
            else
            {
                CenterVertIndex = Points.Num();
                CenterCellIndexMap.Emplace(CenterCellIndex, CenterVertIndex);
                Points.Emplace(CenterCell.ToVector2D(), CenterCell.Value);
            }

            int32 IndexOffset = Points.Num();

            Points.Emplace(NeighbourCell0.ToVector2D(), NeighbourCell0.Value);
            Points.Emplace(NeighbourCell1.ToVector2D(), NeighbourCell1.Value);

            PolyIndices.Emplace(CenterVertIndex);
            PolyIndices.Emplace(IndexOffset  );
            PolyIndices.Emplace(IndexOffset+1);
        } );

    for (const int32 fi : FeatureIndices)
    {
        const FJCVCellGroup& CellGroup(FeatureGroup.CellGroups[fi]);

        for (const FJCVCell* FeatureCell : CellGroup)
        {
            check(FeatureCell != nullptr);
            Map->VisitCellDuals(*FeatureCell, CallbackDual);
        }
    }

    // Shrink geometry containers

    Points.Shrink();
    PolyIndices.Shrink();
    CellIndices.Shrink();
}

void UJCVDiagramAccessor::GeneratePolyGeometryByFeature(UPARAM(ref) FJCVPolyGeometry& Geometry, FJCVFeatureId FeatureId, bool bUseCellAverageValue, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GeneratePolyGeometryByFeature() ABORTED, INVALID MAP"));
        return;
    }

    if (! Map->HasFeature(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GeneratePolyGeometryByFeature() ABORTED, INVALID FEATURE TYPE"));
        return;
    }

    // Get feature cell group indices

    const FJCVFeatureGroup& FeatureGroup(*Map->GetFeatureGroup(FeatureId.Type));
    int32 CellCount = 0;

    TArray<int32> FeatureIndices;
    Map->GetFeatureIndices(FeatureIndices, FeatureId.Type, FeatureId.Index, true);

    for (const int32 fi : FeatureIndices)
    {
        CellCount += FeatureGroup.GetCellCount(fi);
    }

    // Reserve broad approximation of point and index container size

    TArray<FVector>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);

    Points.Reserve(CellCount * 8);
    PolyIndices.Reserve(CellCount * 3 * 8);
    CellIndices.Reserve(CellCount);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
    }

    TMap<FIntPoint, int32> PointIndexMap;
    TArray<FVector> CellPoints;

    for (const int32 fi : FeatureIndices)
    {
        const FJCVCellGroup& CellGroup(FeatureGroup.CellGroups[fi]);

        for (const FJCVCell* FeatureCell : CellGroup)
        {
            check(FeatureCell != nullptr);

            CellPoints.Reset();
            Map->GetCellPointValues(CellPoints, *FeatureCell, FeatureId.Type, FeatureId.Index, true);

            const int32 CellPointCount = CellPoints.Num();
            const int32 CellPointIndex = Points.Num();

            float CellValue = FeatureCell->GetValue();
            float PointSum  = 0.f;

            CellIndices.Emplace(FeatureCell->GetIndex());
            Points.Emplace(FeatureCell->ToVector2D(), CellValue);

            for (int32 i=0; i<CellPointCount; ++i)
            {
                int32 pi0 = i;
                int32 pi1 = (i+1) % CellPointCount;

                const FVector& p0(CellPoints[pi0]);
                const FVector& p1(CellPoints[pi1]);

                FIntPoint pid0(FJCVMathUtil::ToIntPointScaled(p0.X, p0.Y));
                FIntPoint pid1(FJCVMathUtil::ToIntPointScaled(p1.X, p1.Y));

                if (! PointIndexMap.Contains(pid0))
                {
                    PointIndexMap.Emplace(pid0, Points.Num());
                    Points.Emplace(p0);
                }

                if (! PointIndexMap.Contains(pid1))
                {
                    PointIndexMap.Emplace(pid1, Points.Num());
                    Points.Emplace(p1);
                }

                PolyIndices.Emplace(CellPointIndex);
                PolyIndices.Emplace(PointIndexMap.FindChecked(pid1));
                PolyIndices.Emplace(PointIndexMap.FindChecked(pid0));

                PointSum += p0.Z;
            }

            if (bUseCellAverageValue && CellPointCount > 0)
            {
                Points[CellPointIndex].Z = PointSum / CellPointCount;
            }
        }
    }

    // Shrink geometry containers

    Points.Shrink();
    PolyIndices.Shrink();
    CellIndices.Shrink();
}

void UJCVDiagramAccessor::GetPointDualGeometry(
    TMap<int32, int32>& CellIndexMap,
    TArray<FVector>& Points,
    TArray<int32>& PolyIndices,
    TArray<int32>& CellIndices,
    const FJCVPoint& Point,
    const FJCVCell& Cell0,
    const FJCVCell& Cell1,
    const FJCVCell& Cell2
    ) const
{
    int32 i0 = Points.Num();
    int32 i1 = i0 + 1;
    int32 i2 = i0 + 2;

    Points.Emplace(Cell0.ToVector2D(), Cell0.Value);
    Points.Emplace(Cell1.ToVector2D(), Cell1.Value);
    Points.Emplace(Cell2.ToVector2D(), Cell2.Value);

    PolyIndices.Emplace(i0);
    PolyIndices.Emplace(i1);
    PolyIndices.Emplace(i2);
}

void UJCVDiagramAccessor::GetPointDualGeometry(
    TSet<FIntPoint>& VisitedPointSet,
    TMap<int32, int32>& CellIndexMap,
    TArray<FVector>& Points,
    TArray<int32>& PolyIndices,
    TArray<int32>& CellIndices,
    const FJCVPoint& Point,
    const FJCVCell& Cell0,
    const FJCVCell& Cell1,
    const FJCVCell& Cell2
    ) const
{
    FIntPoint PointId(FJCVMathUtil::ToIntPointScaled(Point));

    // Dual already visited, skip
    if (VisitedPointSet.Contains(PointId))
    {
        return;
    }

    const int32 ci0 = Cell0.GetIndex();
    const int32 ci1 = Cell1.GetIndex();
    const int32 ci2 = Cell2.GetIndex();

    if (! CellIndexMap.Contains(ci0))
    {
        CellIndexMap.Emplace(ci0, Points.Num());
        CellIndices.Emplace(ci0);
        Points.Emplace(Cell0.ToVector2D(), Cell0.Value);
    }

    if (! CellIndexMap.Contains(ci1))
    {
        CellIndexMap.Emplace(ci1, Points.Num());
        CellIndices.Emplace(ci1);
        Points.Emplace(Cell1.ToVector2D(), Cell1.Value);
    }

    if (! CellIndexMap.Contains(ci2))
    {
        CellIndexMap.Emplace(ci2, Points.Num());
        CellIndices.Emplace(ci2);
        Points.Emplace(Cell2.ToVector2D(), Cell2.Value);
    }

    PolyIndices.Emplace(CellIndexMap.FindChecked(ci2));
    PolyIndices.Emplace(CellIndexMap.FindChecked(ci1));
    PolyIndices.Emplace(CellIndexMap.FindChecked(ci0));

    VisitedPointSet.Emplace(PointId);
}
