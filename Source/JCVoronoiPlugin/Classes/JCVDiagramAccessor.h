// 

#pragma once

#include "CoreUObject.h"
#include "CoreTypes.h"
#include "Containers/Set.h"
#include "JCVParameters.h"
#include "JCVIsland.h"
#include "JCVValueGenerator.h"
#include "JCVPlateGenerator.h"
#include "JCVDiagramAccessor.generated.h"

UCLASS(BlueprintType)
class JCVORONOIPLUGIN_API UJCVDiagramAccessor : public UObject
{
	GENERATED_BODY()

    FJCVIsland* Island;

public:

    void SetIsland(FJCVIsland& AccessedIsland)
    {
        Island = &AccessedIsland;
    }

    FORCEINLINE FJCVIsland& GetIsland()
    {
        return *Island;
    }

    FORCEINLINE const FJCVIsland& GetIsland() const
    {
        return *Island;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool IsValid() const
    {
        return Island != nullptr;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetCellValue(int32 CellIndex) const
    {
        return (Island && Island->IsValidIndex(CellIndex))
            ? Island->Cell(CellIndex).Value
            : -1.f;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    FVector2D GetCellPosition(int32 CellIndex) const
    {
        return (Island && Island->IsValidIndex(CellIndex))
            ? Island->Cell(CellIndex).V2D()
            : FVector2D();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetCellRange(const FVector2D& StartPosition, const FVector2D& EndPosition)
    {
        TArray<int32> indices;

        if (! Island)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID ISLAND"));
            return indices;
        }

        FJCVIsland& isle( *Island );
        const FBox2D& area( isle->GetExtents() );
        const FVector2D& v0( StartPosition );
        const FVector2D& v1( EndPosition );

        if (! area.IsInside(v0) || ! area.IsInside(v1))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::GetCellRange() ABORTED, INVALID INPUT POSITIONS"));
            return indices;
        }

        const FJCVSite* s0 = isle->Find(v0);

        if (s0)
        {
            TArray<const FJCVSite*> sites;
            const int32 reserveSize = isle.Num()/4;

            sites.Reserve(reserveSize);
            indices.Reserve(reserveSize);

            isle->FindAllTo(v1, *s0, sites);

            for (const FJCVSite* site : sites)
            {
                indices.Emplace( isle.Cell(site)->index() );
            }

            indices.Shrink();
        }

        return MoveTemp( indices );
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetRandomCells(int32 Count, int32 FeatureType, int32 FeatureIndex, int32 Seed, int32 IterationLimit = 50)
    {
        if (! Island)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::GetRandomCells() ABORTED, INVALID ISLAND"));
            return TArray<int32>();
        }
        if (Count <= 0)
        {
            return TArray<int32>();
        }

        FJCVIsland& isle( *Island );
        FRandomStream rand(Seed);
        TSet<int32> indexSet;

        const int32 iterN = IterationLimit;

        if (FeatureType < 0)
        {
            int32 cellN = FMath::Clamp(Count, 0, isle.Num());
            indexSet.Reserve(cellN);

            for (int32 i=0; i<iterN && indexSet.Num()<cellN; ++i)
            {
                indexSet.Emplace(rand.RandHelper(cellN));
            }
        }
        else
        {
            FJCVFeatureGroup* pfg = isle.GetFeatureGroup((uint8) FeatureType);
            if (pfg)
            {
                FJCVFeatureGroup& fg( *pfg );

                if (fg.CellGroups.IsValidIndex(FeatureIndex))
                {
                    FJCVCellGroup& cg( fg.CellGroups[FeatureIndex] );
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
                        indexSet.Emplace(cg[rand.RandHelper(cellN)]->index());
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
                            srcIndices.Emplace(c->index());

                    const int32 outN = FMath::Clamp(Count, 0, cellN);

                    // Feature group does not contain any cell, abort
                    if (outN < 1)
                    {
                        return TArray<int32>();
                    }

                    indexSet.Reserve(outN);

                    for (int32 i=0; i<iterN && indexSet.Num()<outN; ++i)
                    {
                        indexSet.Emplace(srcIndices[rand.RandHelper(cellN)]);
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::GetRandomCells() ABORTED, INVALID FEATURE GROUP"));
            }
        }

        return MoveTemp( indexSet.Array() );
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetClosestCellAt(const FVector2D& Pos) const
    {
        if (Island)
        {
            const FJCVSite* site = Island->GetDiagram().FindClosest(Pos);
            return site ? site->index : -1;
        }
        return -1;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ResetFeatures(uint8 FeatureType, int32 FeatureIndex)
    {
        if (Island)
        {
            Island->ResetFeatures(FeatureType, FeatureIndex);
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByType() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkFeaturesByType(FJCVCellTraitsParams TypeTraits)
    {
        if (Island)
        {
            FJCVValueGenerator::MarkFeatures(*Island, FJCVCellTraits(TypeTraits));
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByType() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkFeaturesByValue(FJCVValueTraitsParams ValueTraits)
    {
        if (Island)
        {
            FJCVValueGenerator::MarkFeatures(*Island, FJCVValueTraits(ValueTraits));
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByValue() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkPositions(const TArray<FVector2D>& Positions, uint8 FeatureType, int32 FeatureIndex)
    {
        if (! Island)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkPositions() ABORTED, INVALID ISLAND"));
            return;
        }

        FJCVIsland& isle( *Island );

        const int32 posN = Positions.Num();
        const int32 siteN = isle.Num();
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
                isle->FindAllTo(v, *s, siteQ);
                for (const FJCVSite* s1 : siteQ)
                    isle.MarkFiltered(s1, FeatureType, FeatureIndex, siteS, true);
                s = siteQ.Last();
            }
            else
            {
                s = isle->Find(v);
                isle.MarkFiltered(s, FeatureType, FeatureIndex, siteS, true);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkRange(const FVector2D& StartPosition, const FVector2D& EndPosition, uint8 FeatureType, int32 FeatureIndex, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond)
    {
        if (Island)
        {
            FJCVIsland& isle( *Island );
            const FBox2D& area( isle->GetExtents() );
            const FVector2D& v0( StartPosition );
            const FVector2D& v1( EndPosition );

            if (! area.IsInside(v0) || ! area.IsInside(v1))
            {
                UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID INPUT POSITIONS"));
                return;
            }

            const FJCVSite* s0 = isle->Find(v0);
            const FJCVCellTraits cond( FilterCond );

            if (s0)
            {
                TSet<const FJCVSite*> sites;
                sites.Reserve(isle.Num()/4);
                sites.Emplace(s0);
                isle->FindAllTo(v1, *s0, sites);
                for (const FJCVSite* site : sites)
                {
                    FJCVCell* cell = isle.Cell(site);
                    check(cell);
                    if (cell && (!bUseFilter || cond.test(*cell)))
                    {
                        cell->SetFeature(Value, FeatureType, FeatureIndex);
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkRange() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkRangeByID(int32 StartCellID, int32 EndCellID, uint8 FeatureType, int32 FeatureIndex, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond)
    {
        if (Island)
        {
            FJCVIsland& isle( *Island );

            if (isle.IsValidIndex(StartCellID) && isle.IsValidIndex(EndCellID))
            {
                FVector2D v0( isle[StartCellID].V2D() );
                FVector2D v1( isle[EndCellID].V2D() );
                MarkRange(v0, v1, FeatureType, FeatureIndex, Value, bUseFilter, FilterCond);
            }
            else
            {
                UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByID() ABORTED, INVALID INPUT POSITIONS"));
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkRangeByID() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ApplyValueByFeatures(FJCVCellTraitsParams TypeTraits, float Value)
    {
        if (Island)
        {
            FJCVValueGenerator::ApplyValueByFeatures(*Island, FJCVCellTraits(TypeTraits), Value);
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::MarkFeaturesByValue() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ConvertIsolated(uint8 FeatureType0, uint8 FeatureType1, int32 FeatureIndex, bool bGroupFeatures)
    {
        if (Island)
        {
            Island->ConvertIsolated(FeatureType0, FeatureType1, FeatureIndex, bGroupFeatures);
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::ConvertIsolated() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExpandFeature(uint8 FeatureType, int32 FeatureIndex)
    {
        if (Island)
        {
            if (FeatureIndex < 0)
            {
                Island->ExpandFeature(FeatureType);
            }
            else
            {
                Island->ExpandFeature(FeatureType, FeatureIndex);
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::ExpandFeature() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GroupByFeatures()
    {
        if (Island)
        {
            Island->GroupByFeatures();
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramAccessor::GroupByFeatures() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillAt(const FVector2D& Position, const FJCVRadialFillParams& Params, int32 Seed)
    {
        if (Island)
        {
            FJCVIsland& isle( *Island );
            FJCVCell* cell( isle.Cell(isle->Find(Position)) );
            if (cell)
            {
                FJCVValueGenerator::FRadialFill fillParams(Params);
                FJCVValueGenerator::AddRadialFill(isle, *cell, fillParams, Seed);
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::AddRadialFill() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillByIndex(int32 CellIndex, const FJCVRadialFillParams& Params, int32 Seed)
    {
        if (Island)
        {
            FJCVIsland& isle( *Island );
            if (isle.IsValidIndex(CellIndex))
            {
                FJCVCell& cell( isle.Cell(CellIndex) );
                FJCVValueGenerator::FRadialFill fillParams(Params);
                FJCVValueGenerator::AddRadialFill(isle, cell, fillParams, Seed);
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::AddRadialFill() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillNum(int32 PointCount, const FJCVRadialFillParams& Params, int32 Seed, float Padding=0.f, float ValueThreshold=.25f, int32 MaxPlacementTest=50)
    {
        if (Island)
        {
            if (PointCount <= 0)
            {
                return;
            }

            FJCVIsland& isle( *Island );
            FRandomStream rand(Seed);

            const float tMin = FMath::Clamp(ValueThreshold, 0.f, 1.f);
            const float tMax = 1.f-tMin;
            const float pad = FMath::Clamp(Padding, 0.f, 1.f);
            const int32 cellN = isle.Num();

            FBox2D extents( isle->GetExtents() );
            FBox2D area(extents.Min*pad, extents.Max*(1.f-pad));

            for (int32 it=0; it<PointCount; ++it)
            {
                for (int32 i=0; i<MaxPlacementTest; ++i)
                {
                    int32 cellIdx = rand.RandHelper(cellN);
                    FJCVCell& cell( isle[cellIdx] );

                    if (cell.Value < tMin && area.IsInside(cell.V2D()))
                    {
                        FJCVValueGenerator::FRadialFill fillParams(Params);
                        fillParams.Value *= rand.GetFraction()*tMax;
                        fillParams.Value += tMin;
                        FJCVValueGenerator::AddRadialFill(isle, cell, fillParams, rand);
                        break;
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::AddRadialFill() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateSegments(const TArray<FVector2D>& SegmentOrigins, int32 SegmentMergeCount, int32 Seed)
    {
        if (Island)
        {
            FRandomStream rand(Seed);
            FJCVValueGenerator::GenerateSegmentExpands(*Island, SegmentOrigins, SegmentMergeCount, rand);
        }
        else
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::GenerateSegments() ABORTED, INVALID ISLAND"));
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateOrogeny(UJCVDiagramAccessor* PlateAccessor, int32 Seed, const FJCVRadialFillParams& ValueParams, const FJCVOrogenParams& OrogenParams)
    {
        if (! Island)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::GenerateOrogeny() ABORTED, INVALID ISLAND"));
            return;
        }

        if (! PlateAccessor || ! PlateAccessor->IsValid())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::GenerateOrogeny() ABORTED, INVALID PLATE ISLAND"));
            return;
        }

        FJCVIsland& plate( PlateAccessor->GetIsland() );
        FJCVIsland& landscape( *Island );

        // Generates orogeny
        FRandomStream rand(Seed);
        FJCVValueGenerator::FRadialFill vp(ValueParams);
        FJCVPlateGenerator::FOrogenParams orogenParams(vp, OrogenParams.OriginThreshold, OrogenParams.bDivergentAsConvergent);
        FJCVPlateGenerator::GenerateOrogeny(plate, landscape, orogenParams, rand);

        // Mark features with prepared set
        FJCVCellSet cellS;
        cellS.Reserve(landscape.Num());
        const float threshold = OrogenParams.AreaThreshold;

        // Assign Island Feature Type
        FJCVValueTraits IslandCond(threshold, 100.f, OrogenParams.FeatureType);
        FJCVValueGenerator::MarkFeatures(landscape, IslandCond, cellS);

        landscape.GroupByFeatures();
    }
};

