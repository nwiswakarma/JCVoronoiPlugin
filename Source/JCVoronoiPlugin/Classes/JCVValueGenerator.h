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
    typedef FJCVGraphEdge   FGraph;
    typedef FJCVCell        FCell;

    static int32 MarkFeature(FJCVIsland& Island, TQueue<FCell*>& cellQ, TSet<FCell*>& ExclusionSet, FCell& c, int32 i, const FTraits& cond)
    {
        int32 count = 0;

        cellQ.Enqueue(&c);
        ExclusionSet.Emplace(&c);

        while (! cellQ.IsEmpty())
        {
            FCell* cell;
            cellQ.Dequeue(cell);

            FGraph* g( cell->Graph() );
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
                FCell* n( Island.Cell(g) );

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

    static void PointFill(FJCVIsland& Island, const TArray<FCell*>& seeds)
    {
        if (Island.IsEmpty() || seeds.Num() < 1)
        {
            return;
        }

        TQueue<FCell*> cellQ;
        TSet<FCell*> ExclusionSet;

        ExclusionSet.Reserve(Island.Num());

        for (FCell* c : seeds)
        {
            if (c && ! ExclusionSet.Contains(c))
            {
                cellQ.Enqueue(c);
                ExclusionSet.Emplace(c);
            }
        }

        while (! cellQ.IsEmpty())
        {
            FCell* cell;
            cellQ.Dequeue(cell);

            FGraph* g( cell->Graph() );
            check(g);

            do
            {
                FCell* n( Island.Cell(g) );

                if (! n) continue;
                if (ExclusionSet.Contains(n)) continue;

                n->SetType(*cell);
                ExclusionSet.Emplace(n);
                cellQ.Enqueue(n);
            }
            while ((g = g->next) != nullptr);
        }
    }

    static void GenerateSegmentExpands(
        FJCVIsland& Island,
        const TArray<FVector2D>& Origins,
        int32 SegmentCount,
        FRandomStream& Rand,
        TArray<FCell*>& OutOrigins,
        TArray<FCell*>& OutSegments
    ) {
        if (Island.IsEmpty())
        {
            return;
        }

        TArray<FCell*> originCells;
        const FBox2D ext( Island->GetExtents() );
        const int32 originCount( Origins.Num() );

        // Generates segmented plate origins
        for (int32 i=0; i<originCount; ++i)
        {
            FCell* c = Island.Cell(Island->Find(Origins[i]));
            if (c)
            {
                c->SetType(i, 0);
                originCells.Emplace(c);
            }
        }

        check(originCells.Num() > 0);
        OutSegments = originCells;

        // Generates segmented plate features by point fill expand the origins
        PointFill(Island, originCells);
        Island.GroupByFeatures();
        Island.ConnectFeatures();

        TQueue<uint8> plateQ;
        TSet<uint8> plateS;
        // Clamp the number of plate to be generated
        // if there is not enough plate segments
        const int32 plateN = FMath::Min(SegmentCount, originCount);

        // Generate plate origins
        for (int32 i=0; i<plateN; ++i)
        {
            FRotator randRot( FRotator(0.f,Rand.GetFraction()*360.f,0.f) );
            FVector2D randDir( randRot.Vector() );
            FVector2D randPos( ext.GetCenter() + randDir*ext.GetExtent()*2.f );
            FCell* plateCell = nullptr;
            float dist0 = TNumericLimits<float>::Max();
            for (FCell* c : originCells)
            {
                float dist1 = (randPos-c->V2D()).Size();
                if (! plateS.Contains(c->FeatureType) && dist1 < dist0)
                {
                    plateCell = c;
                    dist0 = dist1;
                }
            }
            if (plateCell)
            {
                FJCVFeatureGroup* fg = Island.GetFeatureGroup(plateCell->FeatureType);
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

            FJCVFeatureGroup* fg0 = Island.GetFeatureGroup(ft0);
            if (fg0)
            {
                bool bMerged = false;
                for (uint8 ft1 : fg0->Neighbours)
                {
                    if (plateS.Contains(ft1))
                        continue;
                    FJCVFeatureGroup* fg1 = Island.GetFeatureGroup(ft1);
                    if (fg1)
                    {
                        Island.MergeGroup(*fg0, *fg1);
                        Island.ConvertConnections(*fg0, *fg1);
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

        // Shrink unused segmented plate feature groups
        Island.ShrinkGroups();
    }

    static void GenerateSegmentExpands(
        FJCVIsland& Island,
        const TArray<FVector2D>& Origins,
        int32 SegmentCount,
        FRandomStream& Rand
    ) {
        TArray<FJCVCell*> originCells;
        TArray<FJCVCell*> segmentCells;
        GenerateSegmentExpands(Island, Origins, SegmentCount, Rand, originCells, segmentCells);
    }

    static void AddRadialFill(FJCVIsland& Island, FCell& Center, const FRadialFill& params, FRandomStream& Rand)
    {
        check(Island.IsValidIndex(Center.index()));

        float value = params.Value;
        const float radius = params.Radius;
        const float sharpness = params.Sharpness;
        const bool bRadial = params.bRadialDegrade;
        const bool bFilterBorder = params.bFilterBorder;
        const bool bUseSharpness = sharpness > 0.0001f;

        TQueue<FCell*> cellQ;
        TSet<FCell*> ExclusionSet;

        Center.Value = FMath::Min(Center.Value+value, 1.f);
        ExclusionSet.Reserve(Island.Num());
        ExclusionSet.Emplace(&Center);
        cellQ.Enqueue(&Center);

        while (! cellQ.IsEmpty() && value > .01f)
        {
            FCell* cell;
            cellQ.Dequeue(cell);

            if (bRadial)
                value = cell->Value;
            value *= radius;

            FGraph* g( cell->Graph() );
            check(g);

            do
            {
                FCell* n( Island.Cell(g) );

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

    FORCEINLINE static void AddRadialFill(FJCVIsland& Island, FCell& Center, const FRadialFill& Params, int32 Seed)
    {
        FRandomStream rand(Seed);
        AddRadialFill(Island, Center, Params, rand);
    }

    static void MarkFeatures(FJCVIsland& Island, const FTraits& Cond, FJCVCellSet& ExclusionSet)
    {
        if (Island.IsEmpty())
        {
            return;
        }

        TQueue<FCell*> cellQ;

        const int32 cellN = Island.Num();
        int32 f = 0;
        int32 n;

        do
        {
            FCell* c = nullptr;
            n = 0;

            // Find single unmarked cell
            for (int32 i=0; i<cellN; ++i)
                if (Cond.canSeed(Island[i]))
                    c = &Island[i];

            if (c)
                n = MarkFeature(Island, cellQ, ExclusionSet, *c, f++, Cond);
        }
        // Loop until there is no undefined cell left or no conversion made
        while (n > 0);
    }

    static void MarkFeatures(FJCVIsland& Island, const FSite& Seed, const FTraits& Cond, int32 FeatureIndex, FJCVCellSet& ExclusionSet)
    {
        if (Island.IsEmpty())
        {
            return;
        }

        const FSite* s( &Seed );
        FCell* c( Island.Cell(s) );

        if (c)
        {
            TQueue<FCell*> cellQ;
            MarkFeature(Island, cellQ, ExclusionSet, *c, FeatureIndex, Cond);
        }
    }

    FORCEINLINE static void MarkFeatures(FJCVIsland& Island, const FTraits& Cond)
    {
        TSet<FCell*> ExclusionSet;
        ExclusionSet.Reserve(Island.Num());
        MarkFeatures(Island, Cond, ExclusionSet);
    }

    FORCEINLINE static void MarkFeatures(FJCVIsland& Island, const FSite& Seed, const FTraits& Cond, int32 FeatureIndex)
    {
        TSet<FCell*> ExclusionSet;
        ExclusionSet.Reserve(Island.Num());
        MarkFeatures(Island, Seed, Cond, FeatureIndex, ExclusionSet);
    }

    FORCEINLINE static void ApplyValueByFeatures(FJCVIsland& Island, const FTraits& Cond, float Value)
    {
        for (int32 i=0; i<Island.Num(); ++i)
        {
            FCell& c( Island[i] );
            if (Cond.test(c))
                c.Value = Value;
        }
    }

};

