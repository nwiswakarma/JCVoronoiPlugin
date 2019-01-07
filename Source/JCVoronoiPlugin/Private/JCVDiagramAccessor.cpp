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
#include "JCVFeatureUtility.h"
#include "JCVValueGenerator.h"
#include "JCVPlateGenerator.h"

// MARK FEATURE FUNCTIONS

void UJCVDiagramAccessor::MarkDefaultFeatures(uint8 FeatureType)
{
    if (HasValidMap())
    {
        FJCVCellTraits CellTraits(EJCVCellFeature::UNDEFINED, FeatureType);
        FJCVValueGenerator::MarkFeatures(*Map, CellTraits);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkDefaultFeatures() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::MarkFeaturesByType(FJCVCellTraitsParams TypeTraits)
{
    if (HasValidMap())
    {
        FJCVValueGenerator::MarkFeatures(*Map, FJCVCellTraits(TypeTraits));
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByType() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::MarkFeaturesByValue(FJCVValueTraitsParams ValueTraits)
{
    if (HasValidMap())
    {
        FJCVValueGenerator::MarkFeatures(*Map, FJCVValueTraits(ValueTraits));
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByValue() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::MarkPositions(const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId, bool bContiguous)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkPositions() ABORTED, INVALID ISLAND"));
        return;
    }

    if (Positions.Num() <= 0)
    {
        return;
    }

    if (bContiguous)
    {
        MarkPositionsContiguous(*Map, Positions, FeatureId);
    }
    else
    {
        MarkPositions(*Map, Positions, FeatureId);
    }
}

void UJCVDiagramAccessor::MarkPositionsContiguous(FJCVDiagramMap& MapRef, const TArray<FVector2D>& Positions, const FJCVFeatureId& FeatureId)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkPositions() ABORTED, INVALID ISLAND"));
        return;
    }

    const int32 posN = Positions.Num();
    const int32 siteN = MapRef.Num();
    const int32 resvN = posN<siteN ? posN : siteN;

    TArray<const FJCVSite*> siteQ;
    TSet<const FJCVSite*> siteS;
    siteS.Reserve(resvN);

    const FJCVSite* s = nullptr;
    for (const FVector2D& v : Positions)
    {
        if (s)
        {
            siteQ.Reset();
            MapRef->FindAllTo(v, *s, siteQ);
            for (const FJCVSite* s1 : siteQ)
                MapRef.MarkFiltered(s1, FeatureId.Type, FeatureId.Index, siteS, true);
            s = siteQ.Last();
        }
        else
        {
            s = MapRef->Find(v);
            MapRef.MarkFiltered(s, FeatureId.Type, FeatureId.Index, siteS, true);
        }
    }
}

void UJCVDiagramAccessor::MarkPositions(FJCVDiagramMap& MapRef, const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId)
{
    if (Positions.Num() <= 0)
    {
        return;
    }

    const FJCVSite* SearchSite = MapRef->FindClosest(Positions[0]);

    for (const FVector2D& Point : Positions)
    {
        check(SearchSite);

        // Invalid search starting cell, abort
        if (! SearchSite)
        {
            break;
        }

        const FJCVSite* Site = MapRef->FindFrom(Point, *SearchSite);
        FJCVCell* Cell = MapRef.GetCell(Site);

        if (Site)
        {
            Cell->SetType(FeatureId.Type, FeatureId.Index);
            SearchSite = Site;
        }
    }
}

void UJCVDiagramAccessor::MarkRange(const FVector2D& StartPosition, const FVector2D& EndPosition, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond)
{
    if (HasValidMap())
    {
        FJCVDiagramMap& MapRef(*Map);
        const FBox2D& Bounds(GetBounds());
        const FVector2D& v0(StartPosition);
        const FVector2D& v1(EndPosition);

        if (! Bounds.IsInside(v0) || ! Bounds.IsInside(v1))
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID INPUT POSITIONS"));
            return;
        }

        const FJCVSite* s0 = MapRef->Find(v0);
        const FJCVCellTraits cond( FilterCond );

        if (s0)
        {
            TSet<const FJCVSite*> Sites;
            Sites.Reserve(MapRef.Num()/4);
            Sites.Emplace(s0);
            MapRef->FindAllTo(v1, *s0, Sites);
            for (const FJCVSite* site : Sites)
            {
                FJCVCell* cell = MapRef.GetCell(site);
                check(cell);
                if (cell && (!bUseFilter || cond.test(*cell)))
                {
                    cell->SetFeature(Value, FeatureId.Type, FeatureId.Index);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::MarkRangeByFeature(int32 StartCellID, int32 EndCellID, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond)
{
    if (HasValidMap())
    {
        FJCVDiagramMap& MapRef( *Map );

        if (MapRef.IsValidIndex(StartCellID) && MapRef.IsValidIndex(EndCellID))
        {
            FVector2D v0(MapRef.GetCell(StartCellID).ToVector2D());
            FVector2D v1(MapRef.GetCell(EndCellID).ToVector2D());
            MarkRange(v0, v1, FeatureId, Value, bUseFilter, FilterCond);
        }
        else
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByFeature() ABORTED, INVALID INPUT POSITIONS"));
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByFeature() ABORTED, INVALID ISLAND"));
    }
}

// FEATURE UTILITY FUNCTIONS

int32 UJCVDiagramAccessor::GetFeatureCount() const
{
    return Map ? Map->GetFeatureCount() : 0;
}

int32 UJCVDiagramAccessor::GetFeatureGroupCount(uint8 FeatureType) const
{
    return Map ? Map->GetFeatureGroupCount(FeatureType) : 0;
}

void UJCVDiagramAccessor::ResetFeatures(FJCVFeatureId FeatureId)
{
    if (HasValidMap())
    {
        Map->ResetFeatures(FeatureId.Type, FeatureId.Index);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByType() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::ApplyValueByFeatures(FJCVCellTraitsParams TypeTraits, float Value)
{
    if (HasValidMap())
    {
        FJCVValueGenerator::ApplyValueByFeatures(*Map, FJCVCellTraits(TypeTraits), Value);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByValue() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::ConvertIsolated(uint8 FeatureType0, uint8 FeatureType1, int32 FeatureIndex, bool bGroupFeatures)
{
    if (HasValidMap())
    {
        Map->ConvertIsolated(FeatureType0, FeatureType1, FeatureIndex, bGroupFeatures);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ConvertIsolated() ABORTED, INVALID ISLAND"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ExpandFeature() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::PointFillSubdivideFeatures(
    const uint8 FeatureType,
    const TArray<int32>& OriginCellIndices,
    int32 SegmentCount,
    int32 Seed
    )
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::PointFillSubdivideFeatures() ABORTED, INVALID ISLAND"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GroupByFeatures() ABORTED, INVALID ISLAND"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::ShrinkFeatures() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::ScaleFeatureValuesByIndex(uint8 FeatureType, int32 IndexOffset)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::InvertFeatureValues() ABORTED, INVALID ISLAND"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::InvertFeatureValues() ABORTED, INVALID ISLAND"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::InvertFeatureValues() ABORTED, INVALID ISLAND"));
    }

    if (! IsValid(ValueCurve))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::InvertFeatureValues() ABORTED, INVALID VALUE CURVE"));
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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID ISLAND"));
        return;
    }

    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID ORIGIN CELL"));
        return;
    }
    
    if (! Map->HasFeatureType(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::MapNormalizedDistanceFromCell() ABORTED, INVALID FEATURE TYPE"));
        return;
    }

    FJCVValueGenerator::MapNormalizedDistanceFromCell(*Map, *OriginCell, FeatureId.Type, FeatureId.Index, bAgainstAnyType);
}

FJCVPointGroup UJCVDiagramAccessor::GetFeaturePoints(FJCVFeatureId FeatureId) const
{
    FJCVPointGroup PointGroup;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFeaturePoints() ABORTED, INVALID ISLAND"));
        return PointGroup;
    }

    const FJCVCellGroup* CellGroupPtr = Map->GetCellsByFeature(FeatureId.Type, FeatureId.Index);

    if (CellGroupPtr)
    {
        TArray<FVector2D>& Points(PointGroup.Points);
        const FJCVCellGroup& CellGroup(*CellGroupPtr);

        Points.Reserve(CellGroup.Num());

        for (const FJCVCell* Cell : CellGroup)
        {
            if (Cell)
            {
                Points.Emplace(Cell->ToVector2D());
            }
        }
    }

    return PointGroup;
}

FJCVCellRefGroup UJCVDiagramAccessor::GetFeatureCells(FJCVFeatureId FeatureId) const
{
    FJCVCellRefGroup OutCellGroup;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFeatureCells() ABORTED, INVALID ISLAND"));
        return OutCellGroup;
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

        OutCellGroup.Data.Reserve(CellCount);

        for (const int32 fi : FeatureIndices)
        {
            const FJCVCellGroup& CellGroup(FeatureGroupPtr->CellGroups[fi]);

            for (const FJCVCell* Cell : CellGroup)
            {
                check(Cell != nullptr);
                OutCellGroup.Data.Emplace(Cell);
            }
        }
    }

    return OutCellGroup;
}

TArray<int32> UJCVDiagramAccessor::GetRandomCellWithinFeature(uint8 FeatureType, int32 CellCount, int32 Seed, bool bAllowBorders, int32 MinCellDistance) const
{
    TArray<int32> OutIndices;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCellWithinFeature() ABORTED, INVALID ISLAND"));
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

// CELL QUERY FUNCTIONS

int32 UJCVDiagramAccessor::GetCellCount() const
{
    return Map ? Map->Num() : 0;
}

FJCVCellDetailsRef UJCVDiagramAccessor::GetCellDetails(const FJCVCellRef& CellRef) const
{
    return FJCVCellDetailsRef(CellRef);
}

TArray<FJCVCellDetailsRef> UJCVDiagramAccessor::GetCellGroupDetails(const TArray<FJCVCellRef> CellRefs) const
{
    TArray<FJCVCellDetailsRef> Details;
    Details.SetNum(CellRefs.Num());

    for (int32 i=0; i<CellRefs.Num(); ++i)
    {
        Details[i].Set(CellRefs[i].Data);
    }

    return Details;
}

TArray<uint8> UJCVDiagramAccessor::GetNeighbourTypes(const FJCVCellRef& CellRef) const
{
    TArray<uint8> Types;

    if (HasValidMap() && CellRef.Data)
    {
        Map->GetNeighbourTypes(*CellRef.Data, Types);
    }

    return Types;
}

TArray<FJCVCellTypeGroupRef> UJCVDiagramAccessor::GetGroupNeighbourTypes(const FJCVCellRefGroup& CellGroup) const
{
    const TArray<FJCVCellRef>& Cells(CellGroup.Data);

    TArray<FJCVCellTypeGroupRef> TypeGroup;
    TypeGroup.SetNum(Cells.Num());

    if (HasValidMap())
    {
        for (int32 i=0; i<Cells.Num(); ++i)
        {
            if (const FJCVCell* Cell = Cells[i].Data)
            {
                TArray<uint8>& Types(TypeGroup[i].Data);
                Map->GetNeighbourTypes(*Cell, Types);
            }
        }
    }

    return TypeGroup;
}

TArray<int32> UJCVDiagramAccessor::GetCellRange(const FVector2D& StartPosition, const FVector2D& EndPosition)
{
    TArray<int32> OutIndices;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID ISLAND"));
        return OutIndices;
    }

    FJCVDiagramMap& MapRef(*Map);
    const FBox2D& Bounds(GetBounds());
    const FVector2D& v0(StartPosition);
    const FVector2D& v1(EndPosition);

    if (! Bounds.IsInside(v0) || ! Bounds.IsInside(v1))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID INPUT POSITIONS"));
        return OutIndices;
    }

    const FJCVSite* s0 = MapRef->Find(v0);

    if (s0)
    {
        TArray<const FJCVSite*> Sites;
        const int32 ReserveSize = MapRef.Num()/4;

        Sites.Reserve(ReserveSize);
        OutIndices.Reserve(ReserveSize);

        MapRef->FindAllTo(v1, *s0, Sites);

        for (const FJCVSite* site : Sites)
        {
            OutIndices.Emplace( MapRef.GetCell(site)->GetIndex() );
        }

        OutIndices.Shrink();
    }

    return MoveTemp(OutIndices);
}

FJCVCellRef UJCVDiagramAccessor::GetRandomCell(FJCVFeatureId FeatureId, int32 Seed)
{
    TArray<int32> CellIndices;
    FJCVCellRef CellRef;

    GetRandomCells(1, FeatureId, Seed);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID ISLAND"));
        return CellRef;
    }

    const FJCVCellGroup* CellGroupPtr = Map->GetCellsByFeature(FeatureId.Type, FeatureId.Index);

    if (CellGroupPtr)
    {
        const FJCVCellGroup& CellGroup(*CellGroupPtr);
        FRandomStream Rand(Seed);

        CellRef = FJCVCellRef(CellGroup[Rand.RandHelper(CellGroup.Num())]);
    }

    return CellRef;
}

TArray<int32> UJCVDiagramAccessor::GetRandomCells(int32 Count, FJCVFeatureId FeatureId, int32 Seed, int32 IterationLimit)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCells() ABORTED, INVALID ISLAND"));
        return TArray<int32>();
    }
    if (Count <= 0)
    {
        return TArray<int32>();
    }

    FJCVDiagramMap& isle( *Map );
    FRandomStream Rand(Seed);
    TSet<int32> indexSet;

    const int32 iterN = IterationLimit;

    if (FeatureId.Type < 0)
    {
        int32 cellN = FMath::Clamp(Count, 0, isle.Num());
        indexSet.Reserve(cellN);

        for (int32 i=0; i<iterN && indexSet.Num()<cellN; ++i)
        {
            indexSet.Emplace(Rand.RandHelper(cellN));
        }
    }
    else
    {
        FJCVFeatureGroup* pfg = isle.GetFeatureGroup((uint8) FeatureId.Type);
        if (pfg)
        {
            FJCVFeatureGroup& fg( *pfg );

            if (fg.CellGroups.IsValidIndex(FeatureId.Index))
            {
                FJCVCellGroup& cg( fg.CellGroups[FeatureId.Index] );
                const int32 cellN = cg.Num();
                const int32 outN = FMath::Clamp(Count, 0, cellN);

                // Feature group does not contain any cell, abort
                if (outN < 1)
                {
                    return TArray<int32>();
                }

                indexSet.Reserve(outN);

                for (int32 i=0; i<iterN && indexSet.Num()<outN; ++i)
                {
                    indexSet.Emplace(cg[Rand.RandHelper(cellN)]->GetIndex());
                }
            }
            else
            {
                TArray<FJCVCellGroup>& CellGroups( fg.CellGroups );
                TArray<int32> srcIndices;
                const int32 cellN = fg.GetCellCount();

                srcIndices.Reserve(cellN);

                for (const FJCVCellGroup& cg : CellGroups)
                    for (const FJCVCell* c : cg)
                        srcIndices.Emplace(c->GetIndex());

                const int32 outN = FMath::Clamp(Count, 0, cellN);

                // Feature group does not contain any cell, abort
                if (outN < 1)
                {
                    return TArray<int32>();
                }

                indexSet.Reserve(outN);

                for (int32 i=0; i<iterN && indexSet.Num()<outN; ++i)
                {
                    indexSet.Emplace(srcIndices[Rand.RandHelper(cellN)]);
                }
            }
        }
        else
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetRandomCells() ABORTED, INVALID FEATURE GROUP"));
        }
    }

    return MoveTemp( indexSet.Array() );
}

int32 UJCVDiagramAccessor::GetClosestCellAt(const FVector2D& Pos) const
{
    if (HasValidMap())
    {
        const FJCVSite* site = Map->GetDiagram().FindClosest(Pos);
        return site ? site->index : -1;
    }
    return -1;
}

TArray<int32> UJCVDiagramAccessor::FilterPoints(const TArray<FVector2D>& Points, FJCVFeatureId FeatureId) const
{
    TArray<int32> OutPointIndices;

    if (! Map || Points.Num() == 0)
    {
        return OutPointIndices;
    }

    const FJCVDiagramContext& Diagram( Map->GetDiagram() );

    const int32 PointNum = Points.Num();
    const FJCVSite* SearchSite = Diagram.FindClosest(Points[0]);

    // No initial search site found, diagram might be empty. Abort
    if (! SearchSite)
    {
        return OutPointIndices;
    }

    // Reserve result space
    OutPointIndices.Reserve(PointNum);

    for (int32 i=0; i<PointNum; ++i)
    {
        check(SearchSite);

        const FVector2D& Point(Points[i]);
        const FJCVSite* Site = Diagram.FindFrom(Point, *SearchSite);
        const FJCVCell* Cell = Map->GetCell(Site);

        if (Site)
        {
            if (Cell->IsType(FeatureId.Type, FeatureId.Index))
            {
                OutPointIndices.Emplace(i);
            }

            SearchSite = Site;
        }
    }

    // Shrink reserved space
    OutPointIndices.Shrink();

    return MoveTemp(OutPointIndices);
}

FJCVCellRef UJCVDiagramAccessor::FindCell(const FVector2D& Position) const
{
    return Map
        ? FJCVCellRef(Map->GetCell((*Map)->FindClosest(Position)))
        : FJCVCellRef();
}

FJCVCellRefGroup UJCVDiagramAccessor::FindCells(const TArray<FVector2D>& Positions) const
{
    FJCVCellRefGroup CellGroup;

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::FindCell() ABORTED, INVALID ISLAND"));
        return CellGroup;
    }

    if (Positions.Num() <= 0)
    {
        return CellGroup;
    }

    const FJCVDiagramMap& MapRef(*Map);
    const FJCVSite* SearchSite = MapRef->FindClosest(Positions[0]);

    CellGroup.Data.Reserve(Positions.Num());

    for (const FVector2D& Point : Positions)
    {
        check(SearchSite);

        // Invalid search starting cell, abort
        if (! SearchSite)
        {
            break;
        }

        const FJCVSite* Site = MapRef->FindFrom(Point, *SearchSite);

        if (Site)
        {
            CellGroup.Data.Emplace(MapRef.GetCell(Site));
            SearchSite = Site;
        }
    }

    return CellGroup;
}

TArray<FJCVCellRefGroup> UJCVDiagramAccessor::FindCellsWithinRects(const TArray<FBox2D>& Rects) const
{
    TArray<FJCVCellRefGroup> CellGroups;
    CellGroups.SetNum(Rects.Num());

    if (! Map || Rects.Num() <= 0)
    {
        return CellGroups;
    }

    FJCVDiagramMap& MapRef(*Map);
    const FJCVSite* SearchSite = nullptr;

    for (int32 i=0; i<Rects.Num(); ++i)
    {
        const FBox2D& Rect(Rects[i]);
        FJCVCellRefGroup& CellGroup(CellGroups[i]);

        if (! Rect.bIsValid)
        {
            continue;
        }

        if (! SearchSite)
        {
            SearchSite = MapRef->FindClosest(Rect.Min);
        }
        else
        {
            SearchSite = MapRef->FindFrom(Rect.Min, *SearchSite);
        }

        TArray<const FJCVSite*> Sites;
        MapRef->FindAllWithin(Rect, *SearchSite, Sites);

        CellGroup.Data.Reserve(Sites.Num());

        for (const FJCVSite* Site : Sites)
        {
            CellGroup.Data.Emplace(MapRef.GetCell(Site));
        }
    }

    return CellGroups;
}

FJCVCellRefGroup UJCVDiagramAccessor::FindBorderCells(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders, bool bAgainstAnyType) const
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

TArray<FJCVCellJunctionRef> UJCVDiagramAccessor::FindJunctionCells(uint8 FeatureType) const
{
    TArray<FJCVCellJunctionRef> JunctionRefs;

    if (! HasValidMap())
    {
        return JunctionRefs;
    }

    const uint8 ft = FeatureType;

    FJCVCellSet CellSet;
    Map->GetBorderCells(CellSet, ft, -1, false, true);

    TArray<FJCVCellJunction> Junctions;

    for (const FJCVCell* Cell : CellSet)
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

    return JunctionRefs;
}

TArray<FJCVPointGroup> UJCVDiagramAccessor::FindEdgePoints(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders, bool bAgainstAnyType) const
{
    TArray<FJCVPointGroup> PointGroups;

    if (! HasValidMap())
    {
        return PointGroups;
    }

    const uint8 ft0 = FeatureType0;
    const uint8 ft1 = FeatureType1;
    const int32 FeatureGroupCount = Map->GetFeatureGroupCount(ft0);

    // Feature type is equal and wildcard check flag is set to false, abort
    if (! bAgainstAnyType && ft0 == ft1)
    {
        return PointGroups;
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

    return PointGroups;
}

bool UJCVDiagramAccessor::FindEdgePointsWithEndPoints(
    TArray<FJCVPointGroup>& PointGroups,
    TArray<FJCVCellRefGroup>& EndPointCellGroups,
    uint8 FeatureType0,
    uint8 FeatureType1,
    bool bAllowBorders,
    bool bAgainstAnyType
    ) const
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
    ) const
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

FJCVCellRefGroup UJCVDiagramAccessor::GetCellByOriginRadius(
    const FJCVCellRef& OriginCellRef,
    float Radius,
    FJCVFeatureId FeatureId,
    bool bAgainstAnyType
    ) const
{
    FJCVCellRefGroup OutCellGroup;
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellByOriginRadius() ABORTED, INVALID ISLAND"));
        return OutCellGroup;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetCellByOriginRadius() ABORTED, INVALID ORIGIN CELL"));
        return OutCellGroup;
    }

    TArray<FJCVCellRef>& OutCells(OutCellGroup.Data);

    FJCVDiagramMap& MapRef(*Map);
    TSet<const FJCVCell*> VisitedSet;
    TQueue<const FJCVCell*> ExpandQueue;

    OutCells.Emplace(OriginCellRef);
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
                        OutCells.Emplace(Neighbour);
                    }

                    break;
                }
            }
        }
    }

    return OutCellGroup;
}

FJCVCellRefGroup UJCVDiagramAccessor::ExpandCellQuery(const FJCVCellRef& CellRef, int32 ExpandCount, FJCVFeatureId FeatureId, bool bAgainstAnyType) const
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

FJCVCellRefGroup UJCVDiagramAccessor::ExpandCellGroupQuery(const FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId, int32 ExpandCount, bool bAgainstAnyType) const
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

FJCVCellRefGroup UJCVDiagramAccessor::MergeCellGroups(const TArray<FJCVCellRefGroup>& CellGroups) const
{
    FJCVCellRefGroup CellGroup;
    TArray<FJCVCellRef>& Cells(CellGroup.Data);

    TSet<const FJCVCell*> CellSet;
    int32 CellCount = 0;

    for (const FJCVCellRefGroup& CellGroup : CellGroups)
    {
        CellCount += CellGroup.Data.Num();
    }

    CellSet.Reserve(CellCount);

    for (const FJCVCellRefGroup& CellGroup : CellGroups)
    for (const FJCVCellRef& Cell : CellGroup.Data)
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

TArray<FJCVCellJunctionRef> UJCVDiagramAccessor::FilterUniqueJunctions(const TArray<FJCVCellJunctionRef>& Junctions) const
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

float UJCVDiagramAccessor::GetClosestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId) const
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetClosestDistanceToFeature() ABORTED, INVALID ISLAND"));
        return 0.f;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetClosestDistanceToFeature() ABORTED, INVALID ORIGIN CELL"));
        return 0.f;
    }

    return FJCVValueGenerator::GetClosestDistanceFromCell(*Map, *OriginCell, FeatureId.Type, FeatureId.Index, false);
}

float UJCVDiagramAccessor::GetFurthestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId) const
{
    const FJCVCell* OriginCell(OriginCellRef.Data);

    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID ISLAND"));
        return 0.f;
    }
    
    if (! Map->IsValidCell(OriginCell))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID ORIGIN CELL"));
        return 0.f;
    }
    
    if (! Map->HasFeatureType(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GetFurthestDistanceToFeature() ABORTED, INVALID FEATURE TYPE"));
        return 0.f;
    }

    return FJCVValueGenerator::GetFurthestDistanceFromCell(*Map, *OriginCell, FeatureId.Type, FeatureId.Index, false);
}

// CELL VALUE FUNCTIONS

void UJCVDiagramAccessor::AddRadialFillAt(const FVector2D& Position, const FJCVRadialFillParams& Params, int32 Seed)
{
    if (HasValidMap())
    {
        FJCVDiagramMap& isle( *Map );
        FJCVCell* cell( isle.GetCell(isle->Find(Position)) );
        if (cell)
        {
            FJCVValueGenerator::FRadialFill fillParams(Params);
            FJCVValueGenerator::AddRadialFill(isle, *cell, fillParams, Seed);
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::AddRadialFill() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::AddRadialFillByIndex(int32 CellIndex, const FJCVRadialFillParams& Params, int32 Seed)
{
    if (HasValidMap())
    {
        FJCVDiagramMap& isle( *Map );
        if (isle.IsValidIndex(CellIndex))
        {
            FJCVCell& cell( isle.GetCell(CellIndex) );
            FJCVValueGenerator::FRadialFill fillParams(Params);
            FJCVValueGenerator::AddRadialFill(isle, cell, fillParams, Seed);
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::AddRadialFill() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::AddRadialFillNum(int32 PointCount, const FJCVRadialFillParams& Params, int32 Seed, float Padding, float ValueThreshold, int32 MaxPlacementTest)
{
    if (HasValidMap())
    {
        if (PointCount <= 0)
        {
            return;
        }

        FJCVDiagramMap& MapRef( *Map );
        FRandomStream Rand(Seed);

        const float tMin = FMath::Clamp(ValueThreshold, 0.f, 1.f);
        const float tMax = 1.f-tMin;
        const float pad = FMath::Clamp(Padding, 0.f, 1.f);
        const int32 cellN = MapRef.Num();

        FBox2D Bounds(GetBounds());
        FBox2D BoundsExpand(Bounds.Min*pad, Bounds.Max*(1.f-pad));

        for (int32 it=0; it<PointCount; ++it)
        {
            for (int32 i=0; i<MaxPlacementTest; ++i)
            {
                int32 cellIdx = Rand.RandHelper(cellN);
                FJCVCell& cell(MapRef.GetCell(cellIdx));

                if (cell.Value < tMin && BoundsExpand.IsInside(cell.ToVector2D()))
                {
                    FJCVValueGenerator::FRadialFill fillParams(Params);
                    fillParams.Value *= Rand.GetFraction()*tMax;
                    fillParams.Value += tMin;
                    FJCVValueGenerator::AddRadialFill(MapRef, cell, fillParams, Rand);
                    break;
                }
            }
        }
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::AddRadialFill() ABORTED, INVALID ISLAND"));
    }
}

// CELL UTILITY FUNCTIONS

TArray<int32> UJCVDiagramAccessor::GenerateCellGridIndices(const FJCVCellRef& Cell, const FIntPoint& GridDimension, const float BoundsExpand) const
{
    TArray<int32> Indices;

    if (! HasValidMap() || ! Cell.Data || ! Cell.Data->Site)
    {
        return Indices;
    }

    const FJCVSite& Site(*Cell.Data->Site);
    FJCVDiagramMap& MapRef(*Map);

    FBox2D Bounds;
    FVector2D SiteCenter(Site.p.x, Site.p.y);
    float SiteCenterRadiusSq;

    MapRef->GetSiteBounds(Site, Bounds);
    SiteCenterRadiusSq = MapRef->GetShortestMidPoint(Site);

    if (! Bounds.bIsValid)
    {
        return Indices;
    }

    FVector2D VecDim(GridDimension.X, GridDimension.Y);

    Bounds = Bounds.ExpandBy(BoundsExpand);
    Bounds.Min = FMath::Clamp(Bounds.Min, FVector2D::ZeroVector, VecDim);
    Bounds.Max = FMath::Clamp(Bounds.Max, FVector2D::ZeroVector, VecDim);

    const FIntPoint IntMin(Bounds.Min.X, Bounds.Min.Y);
    const FIntPoint IntMax(Bounds.Max.X, Bounds.Max.Y);
    const int32 CountX = IntMax.X-IntMin.X;
    const int32 CountY = IntMax.Y-IntMin.Y;
    const int32 Stride = GridDimension.X;

    Indices.Reserve(CountX*CountY);

    for (int32 y=IntMin.Y; y<IntMax.Y; ++y)
    for (int32 x=IntMin.X; x<IntMax.X; ++x)
    {
        FVector2D Pos(x, y);

        if ((Pos-SiteCenter).SizeSquared() < SiteCenterRadiusSq || MapRef->IsWithin(Site, Pos))
        {
            Indices.Emplace(x+y*Stride);
        }
    }

    Indices.Shrink();

    return Indices;
}

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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateSegments() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::GenerateOrogeny(UJCVDiagramAccessor* PlateAccessor, int32 Seed, const FJCVRadialFillParams& ValueParams, const FJCVOrogenParams& OrogenParams)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateOrogeny() ABORTED, INVALID ISLAND"));
        return;
    }

    if (! IsValid(PlateAccessor) || ! PlateAccessor->HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateOrogeny() ABORTED, INVALID PLATE ISLAND"));
        return;
    }

    FJCVDiagramMap& plate( PlateAccessor->GetMap() );
    FJCVDiagramMap& landscape( *Map );

    // Generates orogeny
    FRandomStream Rand(Seed);
    FJCVValueGenerator::FRadialFill vp(ValueParams);
    FJCVPlateGenerator::FOrogenParams orogenParams(vp, OrogenParams.OriginThreshold, OrogenParams.bDivergentAsConvergent);
    FJCVPlateGenerator::GenerateOrogeny(plate, landscape, orogenParams, Rand);

    // Mark features with prepared set
    FJCVCellSet cellS;
    cellS.Reserve(landscape.Num());
    const float threshold = OrogenParams.AreaThreshold;

    // Assign Map Feature Type
    FJCVValueTraits MapCond(threshold, 100.f, OrogenParams.FeatureType);
    FJCVValueGenerator::MarkFeatures(landscape, MapCond, cellS);

    landscape.GroupByFeatures();
}

void UJCVDiagramAccessor::GenerateDepthMap(UJCVDiagramAccessor* TargetAccessor, FJCVFeatureId FeatureId)
{
    if (HasValidMap() && IsValid(TargetAccessor) && TargetAccessor->HasValidMap())
    {
        FJCVDiagramMap& SrcMap(*Map);
        FJCVDiagramMap& DstMap(TargetAccessor->GetMap());

        // Make sure source and target map have the same cell count
        if (SrcMap.Num() != DstMap.Num())
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDepthMap() ABORTED, SOURCE AND TARGET MAP HAVE DIFFERENT CELL COUNT"));
            return;
        }

        FJCVFeatureUtility::GenerateDepthMap(SrcMap, FeatureId.Type, FeatureId.Index, DstMap);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDepthMap() ABORTED, INVALID ISLAND"));
    }
}

void UJCVDiagramAccessor::GenerateDualGeometry(FJCVDualGeometry& Geometry, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID ISLAND"));
        return;
    }

    int32 CellCount = Map->Num();

    // Reserve broad approximation of point and index container size

    TArray<FVector2D>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);

    Points.Reserve(CellCount);
    PolyIndices.Reserve(CellCount * 3 * 8);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
    }

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

void UJCVDiagramAccessor::GenerateDualGeometryByFeature(FJCVDualGeometry& Geometry, FJCVFeatureId FeatureId, bool bClearContainer)
{
    if (! HasValidMap())
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GenerateDualGeometry() ABORTED, INVALID ISLAND"));
        return;
    }

    if (! Map->HasFeatureType(FeatureId.Type))
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

    // Reserve broad approximation of point and index container size

    TArray<FVector2D>& Points(Geometry.Points);
    TArray<int32>& PolyIndices(Geometry.PolyIndices);
    TArray<int32>& CellIndices(Geometry.CellIndices);

    Points.Reserve(CellCount);
    PolyIndices.Reserve(CellCount * 3 * 8);

    if (bClearContainer)
    {
        Points.Reset();
        PolyIndices.Reset();
        CellIndices.Reset();
    }

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
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GeneratePolyGeometry() ABORTED, INVALID ISLAND"));
        return;
    }

    if (! Map->HasFeatureType(FeatureId.Type))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramAccessor::GeneratePolyGeometry() ABORTED, INVALID FEATURE TYPE"));
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
    TSet<FIntPoint>& VisitedPointSet,
    TMap<int32, int32>& CellIndexMap,
    TArray<FVector2D>& Points,
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
        Points.Emplace(Cell0.ToVector2D());
    }

    if (! CellIndexMap.Contains(ci1))
    {
        CellIndexMap.Emplace(ci1, Points.Num());
        CellIndices.Emplace(ci1);
        Points.Emplace(Cell1.ToVector2D());
    }

    if (! CellIndexMap.Contains(ci2))
    {
        CellIndexMap.Emplace(ci2, Points.Num());
        CellIndices.Emplace(ci2);
        Points.Emplace(Cell2.ToVector2D());
    }

    PolyIndices.Emplace(CellIndexMap.FindChecked(ci0));
    PolyIndices.Emplace(CellIndexMap.FindChecked(ci1));
    PolyIndices.Emplace(CellIndexMap.FindChecked(ci2));

    VisitedPointSet.Emplace(PointId);
}
