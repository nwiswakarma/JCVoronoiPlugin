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

#include "JCVTypes.h"
#include "JCVParameters.h"
#include "Queue.h"
#include "Set.h"
#include "Math/NumericLimits.h"

struct FJCVCellTraits
{
    uint8 TestType = 255;
    uint8 FeatureType;

    FJCVCellTraits(uint8 f)
        : FeatureType(f)
    {
    }

    FJCVCellTraits(uint8 t, uint8 f)
        : TestType(t)
        , FeatureType(f)
    {
    }

    FJCVCellTraits(const FJCVCellTraitsParams& rhs)
        : TestType(rhs.TestType)
        , FeatureType(rhs.FeatureType)
    {
    }

    FORCEINLINE virtual bool test(const FJCVCell& c) const
    {
        return c.FeatureType == TestType;
    }

    FORCEINLINE virtual bool canSeed(const FJCVCell& c) const
    {
        return c.FeatureType == EJCVCellFeature::UNDEFINED && test(c);
    }
};

struct FJCVValueTraits : public FJCVCellTraits
{
    float ValueLo;
    float ValueHi;

    FJCVValueTraits() = default;

    FJCVValueTraits(float l, float v, uint8 t)
        : FJCVCellTraits(t)
        , ValueLo(l)
        , ValueHi(v)
    {
    }

    FJCVValueTraits(const FJCVValueTraitsParams& rhs)
        : FJCVCellTraits(rhs.FeatureType)
        , ValueLo(rhs.ValueLo)
        , ValueHi(rhs.ValueHi)
    {
    }

    FORCEINLINE virtual bool test(const FJCVCell& c) const override
    {
        return c.Value > ValueLo && c.Value < ValueHi;
    }
};

class FJCVValueGenerator
{
    typedef FJCVCellTraits  FTraits;
    typedef FJCVSite        FSite;
    typedef FJCVEdge   FGraph;
    typedef FJCVCell        FCell;

    static int32 MarkFeature(FJCVDiagramMap& Map, TQueue<FCell*>& cellQ, TSet<FCell*>& ExclusionSet, FCell& c, int32 i, const FTraits& cond)
    {
        int32 count = 0;

        cellQ.Enqueue(&c);
        ExclusionSet.Emplace(&c);

        while (! cellQ.IsEmpty())
        {
            FCell* cell;
            cellQ.Dequeue(cell);

            FGraph* g = cell->GetEdge();
            check(g);

            if (cond.test(*cell))
            {
                cell->SetType(cond.FeatureType, i);
                ++count;
            }
            else
                continue;

            do
            {
                FCell* n = Map.GetCellNeighbour(g);

                if (! n) continue;
                if (ExclusionSet.Contains(n)) continue;

                if (cond.test(*n))
                {
                    ExclusionSet.Emplace(n);
                    cellQ.Enqueue(n);
                }
            }
            while ((g = g->next) != nullptr);
        }

        return count;
    }

public:

    struct FRadialFill
    {
        float Value;
        float Radius;
        float Sharpness;
        bool bRadialDegrade;
        bool bFilterBorder = true;

        FORCEINLINE FRadialFill() = default;

        FORCEINLINE FRadialFill(float v, float r, float s, bool d, bool b=true)
            : Value(v)
            , Radius(r)
            , Sharpness(s)
            , bRadialDegrade(d)
            , bFilterBorder(b)
        {
        }

        FORCEINLINE FRadialFill(const FRadialFill& rhs)
            : Value(rhs.Value)
            , Radius(rhs.Radius)
            , Sharpness(rhs.Sharpness)
            , bRadialDegrade(rhs.bRadialDegrade)
            , bFilterBorder(rhs.bFilterBorder)
        {
        }

        FORCEINLINE FRadialFill(const FJCVRadialFillParams& rhs)
            : Value(rhs.Value)
            , Radius(rhs.Radius)
            , Sharpness(rhs.Sharpness)
            , bRadialDegrade(rhs.bRadialDegrade)
            , bFilterBorder(rhs.bFilterBorder)
        {
        }

        FORCEINLINE void Set(float v, float r, float s, bool d, bool b=true)
        {
            Value = v;
            Radius = r;
            Sharpness = s;
            bRadialDegrade = d;
            bFilterBorder = b;
        }
    };

    static void AddRadialFill(FJCVDiagramMap& Map, FCell& Center, const FRadialFill& params, FRandomStream& Rand)
    {
        check(Map.IsValidIndex(Center.GetIndex()));

        float value = params.Value;
        const float radius = params.Radius;
        const float sharpness = params.Sharpness;
        const bool bRadial = params.bRadialDegrade;
        const bool bFilterBorder = params.bFilterBorder;
        const bool bUseSharpness = sharpness > 0.0001f;

        TQueue<FCell*> cellQ;
        TSet<FCell*> ExclusionSet;

        Center.Value = FMath::Min(Center.Value+value, 1.f);
        ExclusionSet.Reserve(Map.Num());
        ExclusionSet.Emplace(&Center);
        cellQ.Enqueue(&Center);

        while (! cellQ.IsEmpty() && value > .01f)
        {
            FCell* cell;
            cellQ.Dequeue(cell);

            if (bRadial)
                value = cell->Value;

            value *= radius;

            FGraph* g = cell->GetEdge();
            check(g);

            do
            {
                FCell* n = Map.GetCellNeighbour(g);

                if (! n) continue;
                if (ExclusionSet.Contains(n)) continue;

                ExclusionSet.Emplace(n);

                if (bFilterBorder && n->IsBorder())
                {
                    n->Value = 0.f;
                    continue;
                }

                cellQ.Enqueue(n);

                float mod = bUseSharpness
                    ? Rand.GetFraction() * sharpness + 1.1f - sharpness
                    : 1.f;
                float v = n->Value + value*mod;
                v = FMath::Min(v, 1.f);

                if (n->Value < v)
                    n->Value = v;
            }
            while ((g = g->next) != nullptr);
        }
    }

    FORCEINLINE static void AddRadialFill(FJCVDiagramMap& Map, FCell& Center, const FRadialFill& Params, int32 Seed)
    {
        FRandomStream rand(Seed);
        AddRadialFill(Map, Center, Params, rand);
    }

    static void MarkFeatures(FJCVDiagramMap& Map, const FTraits& Cond, FJCVCellSet& ExclusionSet)
    {
        if (Map.IsEmpty())
        {
            return;
        }

        TQueue<FCell*> cellQ;

        const int32 cellN = Map.Num();
        int32 f = 0;
        int32 n;

        do
        {
            FCell* c = nullptr;
            n = 0;

            // Find single unmarked cell
            for (int32 i=0; i<cellN; ++i)
                if (Cond.canSeed(Map.GetCell(i)))
                    c = &Map.GetCell(i);

            if (c)
                n = MarkFeature(Map, cellQ, ExclusionSet, *c, f++, Cond);
        }
        // Loop until there is no undefined cell left or no conversion made
        while (n > 0);
    }

    static void MarkFeatures(FJCVDiagramMap& Map, const FSite& Seed, const FTraits& Cond, int32 FeatureIndex, FJCVCellSet& ExclusionSet)
    {
        if (Map.IsEmpty())
        {
            return;
        }

        const FSite* s = &Seed;
        FCell* c = Map.GetCell(s);

        if (c)
        {
            TQueue<FCell*> cellQ;
            MarkFeature(Map, cellQ, ExclusionSet, *c, FeatureIndex, Cond);
        }
    }

    FORCEINLINE static void MarkFeatures(FJCVDiagramMap& Map, const FTraits& Cond)
    {
        TSet<FCell*> ExclusionSet;
        ExclusionSet.Reserve(Map.Num());
        MarkFeatures(Map, Cond, ExclusionSet);
    }

    FORCEINLINE static void MarkFeatures(FJCVDiagramMap& Map, const FSite& Seed, const FTraits& Cond, int32 FeatureIndex)
    {
        TSet<FCell*> ExclusionSet;
        ExclusionSet.Reserve(Map.Num());
        MarkFeatures(Map, Seed, Cond, FeatureIndex, ExclusionSet);
    }

    FORCEINLINE static void ApplyValueByFeatures(FJCVDiagramMap& Map, const FTraits& Cond, float Value)
    {
        for (int32 i=0; i<Map.Num(); ++i)
        {
            FCell& c( Map.GetCell(i) );
            if (Cond.test(c))
                c.Value = Value;
        }
    }

    static float GetClosestDistanceFromCellSq(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        uint8 FeatureType,
        int32 FeatureIndex = -1,
        bool bAgainstAnyType = false
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

    static float GetFurthestDistanceFromCellSq(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        uint8 FeatureType,
        int32 FeatureIndex = -1,
        bool bAgainstAnyType = false
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

    FORCEINLINE static float GetClosestDistanceFromCell(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        uint8 FeatureType,
        int32 FeatureIndex = -1,
        bool bAgainstAnyType = false
        )
    {
        return FMath::Sqrt(GetClosestDistanceFromCellSq(Map, OriginCell, FeatureType, FeatureIndex, bAgainstAnyType));
    }

    FORCEINLINE static float GetFurthestDistanceFromCell(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        uint8 FeatureType,
        int32 FeatureIndex = -1,
        bool bAgainstAnyType = false
        )
    {
        return FMath::Sqrt(GetFurthestDistanceFromCellSq(Map, OriginCell, FeatureType, FeatureIndex, bAgainstAnyType));
    }

    static void MapNormalizedDistanceFromCell(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        uint8 FeatureType,
        int32 FeatureIndex = -1,
        bool bAgainstAnyType = false
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
};
