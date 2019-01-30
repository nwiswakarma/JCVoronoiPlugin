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

#include "JCVValueGenerator.h"
#include "JCVDiagramAccessor.h"

int32 FJCVValueGenerator::MarkFeature(FJCVDiagramMap& Map, TQueue<FJCVCell*>& cellQ, TSet<FJCVCell*>& ExclusionSet, FJCVCell& c, int32 i, const FJCVCellTraits& cond)
{
    int32 count = 0;

    cellQ.Enqueue(&c);
    ExclusionSet.Emplace(&c);

    while (! cellQ.IsEmpty())
    {
        FJCVCell* cell;
        cellQ.Dequeue(cell);

        FJCVEdge* g = cell->GetEdge();
        check(g);

        if (cond.HasValidFeature(*cell))
        {
            cell->SetType(cond.FeatureType, i);
            ++count;
        }
        else
            continue;

        do
        {
            FJCVCell* n = Map.GetCellNeighbour(g);

            if (! n) continue;
            if (ExclusionSet.Contains(n)) continue;

            if (cond.HasValidFeature(*n))
            {
                ExclusionSet.Emplace(n);
                cellQ.Enqueue(n);
            }
        }
        while ((g = g->next) != nullptr);
    }

    return count;
}

void FJCVValueGenerator::AddRadialFill0(FJCVDiagramMap& Map, FRandomStream& Rand, FJCVCell& OriginCell, const FJCVRadialFill& FillParams)
{
    check(Map.IsValidIndex(OriginCell.GetIndex()));

    float BaseValue = FillParams.Value;
    const float Radius = FillParams.Radius;
    const float Sharpness = FillParams.Sharpness;
    const bool bRadial = FillParams.bRadialDegrade;
    const bool bFilterBorder = FillParams.bFilterBorder;
    const bool bUseSharpness = Sharpness > KINDA_SMALL_NUMBER;

    TQueue<FJCVCell*> cellQ;
    TSet<FJCVCell*> ExclusionSet;

    OriginCell.Value = FMath::Min(OriginCell.Value+BaseValue, 1.f);
    ExclusionSet.Reserve(Map.Num());
    ExclusionSet.Emplace(&OriginCell);
    cellQ.Enqueue(&OriginCell);

    while (! cellQ.IsEmpty() && BaseValue > .01f)
    {
        FJCVCell* cell;
        cellQ.Dequeue(cell);

        if (bRadial)
        {
            BaseValue = cell->Value;
        }

        BaseValue *= Radius;

        FJCVEdge* g = cell->GetEdge();
        check(g);

        do
        {
            FJCVCell* n = Map.GetCellNeighbour(g);

            // Skip invalid or already visited cells

            if (! n || ExclusionSet.Contains(n))
            {
                continue;
            }

            ExclusionSet.Emplace(n);
            cellQ.Enqueue(n);

            // Zero border cell values if required

            if (bFilterBorder && n->IsBorder())
            {
                n->Value = 0.f;
                continue;
            }

            // Calculate new cell value

            float SharpnessModifier = 1.f;

            if (bUseSharpness)
            {
                SharpnessModifier = Rand.GetFraction() * Sharpness + 1.1f - Sharpness;
            }

            float CellValue = n->Value + BaseValue * SharpnessModifier;
            CellValue = FMath::Min(CellValue, 1.f);

            if (n->Value < CellValue)
            {
                n->Value = CellValue;
            }
        }
        while ((g = g->next) != nullptr);
    }
}

void FJCVValueGenerator::AddRadialFill(FJCVDiagramMap& Map, FRandomStream& Rand, FJCVCell& OriginCell, const FJCVRadialFill& FillParams)
{
    if (FillParams.Radius < KINDA_SMALL_NUMBER)
    {
        return;
    }

    check(Map.IsValidIndex(OriginCell.GetIndex()));

    const FVector2D OriginPosition = OriginCell.ToVector2D();

    const float Radius = FillParams.Radius;
    const float RadiusSq = Radius * Radius;
    const float InvRadius = 1.f / Radius;

    const bool bFilterBorder = FillParams.bFilterBorder;

    const UCurveFloat* ValueCurve = FillParams.ValueCurve;

    float BaseValue = FillParams.Value;

    // Cell queue and visited cell set

    TQueue<FJCVCell*> cellQ;
    TSet<FJCVCell*> cellS;

    // Assign base value to origin cell

    OriginCell.Value = BaseValue;

    cellS.Reserve(Map.Num());
    cellS.Emplace(&OriginCell);
    cellQ.Enqueue(&OriginCell);

    while (! cellQ.IsEmpty())
    {
        FJCVCell* cell;
        cellQ.Dequeue(cell);

        FJCVEdge* g = cell->GetEdge();
        check(g);

        do
        {
            FJCVCell* n = Map.GetCellNeighbour(g);

            // Skip invalid or already visited cells

            if (! n || cellS.Contains(n))
            {
                continue;
            }

            cellS.Emplace(n);

            float DistToOriginSq = (n->ToVector2D()-OriginPosition).SizeSquared();

            // Skip cells outside of radius

            if (DistToOriginSq > RadiusSq)
            {
                continue;
            }

            cellQ.Enqueue(n);

            // Set border cell value to zero if filter is set

            if (bFilterBorder && n->IsBorder())
            {
                n->Value = 0.f;
                continue;
            }

            // Calculate new cell value

            float ValueRatio = (1.f - FMath::Sqrt(DistToOriginSq) * InvRadius);

            if (ValueCurve)
            {
                ValueRatio = ValueCurve->GetFloatValue(ValueRatio);
            }

            n->Value = BaseValue * ValueRatio;
        }
        while ((g = g->next) != nullptr);
    }
}

void FJCVValueGenerator::MarkFeatures(FJCVDiagramMap& Map, const FJCVCellTraits& Cond, FJCVCellSet& ExclusionSet)
{
    if (Map.IsEmpty())
    {
        return;
    }

    TQueue<FJCVCell*> cellQ;

    const int32 cellN = Map.Num();
    int32 f = 0;
    int32 n;

    do
    {
        FJCVCell* c = nullptr;
        n = 0;

        // Find single unmarked cell
        for (int32 i=0; i<cellN; ++i)
            if (Cond.HasUndefinedType(Map.GetCell(i)))
                c = &Map.GetCell(i);

        if (c)
            n = MarkFeature(Map, cellQ, ExclusionSet, *c, f++, Cond);
    }
    // Loop until there is no undefined cell left or no conversion made
    while (n > 0);
}

void FJCVValueGenerator::MarkFeatures(FJCVDiagramMap& Map, const FJCVSite& Seed, const FJCVCellTraits& Cond, int32 FeatureIndex, FJCVCellSet& ExclusionSet)
{
    if (Map.IsEmpty())
    {
        return;
    }

    const FJCVSite* s = &Seed;
    FJCVCell* c = Map.GetCell(s);

    if (c)
    {
        TQueue<FJCVCell*> cellQ;
        MarkFeature(Map, cellQ, ExclusionSet, *c, FeatureIndex, Cond);
    }
}

float FJCVValueGenerator::GetClosestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    uint8 FeatureType,
    int32 FeatureIndex,
    bool bAgainstAnyType
    )
{
    check(Map.HasFeatureType(FeatureType));
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

float FJCVValueGenerator::GetFurthestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    uint8 FeatureType,
    int32 FeatureIndex,
    bool bAgainstAnyType
    )
{
    check(Map.HasFeatureType(FeatureType));
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

void FJCVValueGenerator::MapNormalizedDistanceFromCell(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    uint8 FeatureType,
    int32 FeatureIndex,
    bool bAgainstAnyType
    )
{
    check(Map.HasFeatureType(FeatureType));
    check(Map.IsValidCell(&OriginCell));

    const FVector2D Origin = OriginCell.ToVector2D();
    const float FurthestDistanceFromCell = GetFurthestDistanceFromCell(Map, OriginCell, FeatureType, FeatureIndex, bAgainstAnyType);
    const float InvDistanceFromCell = 1.f / FMath::Max(FurthestDistanceFromCell, KINDA_SMALL_NUMBER);

    TFunctionRef<void(FJCVCell&)> CellCallback(
        [&](FJCVCell& Cell)
        {
            const FVector2D CellPoint = Cell.ToVector2D();
            const float CellDist = (CellPoint-Origin).Size();

            Cell.SetValue(CellDist * InvDistanceFromCell);
        } );

    if (bAgainstAnyType)
    {
        Map.VisitCells(CellCallback, &OriginCell);
    }
    else
    {
        Map.VisitFeatureCells(CellCallback, FeatureType, FeatureIndex);
    }
}

void UJCVValueUtilityLibrary::SetCellValues(UJCVDiagramAccessor* Accessor, float Value)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtPosition() ABORTED, INVALID ACCESSOR"));
        return;
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtPosition() ABORTED, INVALID ACCESOR MAP"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    const int32 CellCount = Map.Num();

    for (int32 i=0; i<CellCount; ++i)
    {
        Map.GetCell(i).SetValue(Value);
    }
}

void UJCVValueUtilityLibrary::AddRadialFillAtPosition(UJCVDiagramAccessor* Accessor, int32 Seed, const FVector2D& Position, FJCVRadialFill FillParams)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtPosition() ABORTED, INVALID ACCESSOR"));
        return;
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtPosition() ABORTED, INVALID ACCESOR MAP"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    FJCVCell* OriginCell = Map.GetCell(Map->Find(Position));

    if (OriginCell)
    {
        FJCVValueGenerator::AddRadialFill(Map, Seed, *OriginCell, FillParams);
    }
}

void UJCVValueUtilityLibrary::AddRadialFillAtCell(UJCVDiagramAccessor* Accessor, int32 Seed, FJCVCellRef OriginCellRef, FJCVRadialFill FillParams)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtCell() ABORTED, INVALID ACCESSOR"));
        return;
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtCell() ABORTED, INVALID ACCESOR MAP"));
        return;
    }

    if (! Accessor->IsValidCell(OriginCellRef))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillAtCell() ABORTED, INVALID ORIGIN CELL"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    FJCVCell& OriginCell(Map.GetCell(OriginCellRef.Data->GetIndex()));

    FJCVValueGenerator::AddRadialFill(Map, Seed, OriginCell, FillParams);
}

void UJCVValueUtilityLibrary::AddRadialFillByIndex(UJCVDiagramAccessor* Accessor, int32 Seed, int32 CellIndex, FJCVRadialFill FillParams)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillByIndex() ABORTED, INVALID ACCESSOR"));
        return;
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillByIndex() ABORTED, INVALID ACCESOR MAP"));
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());

    if (Map.IsValidIndex(CellIndex))
    {
        FJCVCell& OriginCell(Map.GetCell(CellIndex));
        FJCVValueGenerator::AddRadialFill(Map, Seed, OriginCell, FillParams);
    }
}

void UJCVValueUtilityLibrary::AddRadialFillNum(UJCVDiagramAccessor* Accessor, int32 Seed, int32 PointCount, FJCVRadialFill FillParams, float Padding, float ValueThreshold, int32 MaxPlacementTest)
{
    if (! IsValid(Accessor))
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillNum() ABORTED, INVALID ACCESSOR"));
        return;
    }

    if (! Accessor->HasValidMap())
    {
        UE_LOG(LogJCV,Error, TEXT("UJCVValueUtilityLibrary::AddRadialFillNum() ABORTED, INVALID ACCESOR MAP"));
        return;
    }

    if (PointCount < 1)
    {
        return;
    }

    FJCVDiagramMap& Map(Accessor->GetMap());
    FRandomStream Rand(Seed);

    const float tMin = FMath::Clamp(ValueThreshold, 0.f, 1.f);
    const float tMax = 1.f-tMin;
    const int32 CellCount = Map.Num();

    Padding = FMath::Clamp(Padding, 0.f, 1.f);

    FBox2D Bounds(Accessor->GetBounds());
    FBox2D BoundsExpand(Bounds.Min*Padding, Bounds.Max*(1.f-Padding));

    for (int32 it=0; it<PointCount; ++it)
    {
        for (int32 i=0; i<MaxPlacementTest; ++i)
        {
            int32 CellIdx = Rand.RandHelper(CellCount);
            FJCVCell& OriginCell(Map.GetCell(CellIdx));

            if (OriginCell.Value < tMin && BoundsExpand.IsInside(OriginCell.ToVector2D()))
            {
                FillParams.Value *= tMax*Rand.GetFraction();
                FillParams.Value += tMin;
                FJCVValueGenerator::AddRadialFill(Map, Rand, OriginCell, FillParams);
                break;
            }
        }
    }
}
