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

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JCVParameters.h"
#include "JCVDiagramAccessor.h"
#include "JCVFeatureUtility.generated.h"

class FJCVDiagramMap;
class UJCVDiagramAccessor;

class FJCVFeatureUtility
{
public:

    // Visit Utility

    static void PointFillVisit(
        FJCVDiagramMap& Map,
        const TArray<FJCVCell*>& OriginCells,
        const TFunctionRef<bool(FJCVCell&,FJCVCell&)>& VisitCallback
        );

    static void ExpandVisit(
        FJCVDiagramMap& Map,
        int32 ExpandCount,
        const TArray<FJCVCell*>& OriginCells,
        const TFunctionRef<bool(FJCVCell&,FJCVCell&)>& VisitCallback
        );

    static void PointFill(FJCVDiagramMap& Map, const TArray<FJCVCell*>& OriginCells, uint8 FeatureTypeFilter = 255);

    static void PointFillIsolated(
        FJCVDiagramMap& Map,
        FJCVFeatureId BoundFeature,
        FJCVFeatureId TargetFeature,
        const TArray<FJCVCell*>& OriginCells
        );

    static void ExpandFeatureFromCellGroups(
        FJCVDiagramMap& Map,
        const TArray<FJCVCell*>& OriginCells,
        const FJCVFeatureId& FeatureId,
        int32 ExpandCount
        );

    // Feature Segments

    static void GenerateSegmentExpands(
        FJCVDiagramMap& Map,
        const TArray<FVector2D>& Origins,
        int32 SegmentCount,
        FRandomStream& Rand,
        TArray<FJCVCell*>& OutOrigins,
        TArray<FJCVCell*>& OutSegments
        );

    static void GenerateSegmentExpands(
        FJCVDiagramMap& Map,
        const TArray<FVector2D>& Origins,
        int32 SegmentCount,
        FRandomStream& Rand
        )
    {
        TArray<FJCVCell*> originCells;
        TArray<FJCVCell*> segmentCells;
        GenerateSegmentExpands(Map, Origins, SegmentCount, Rand, originCells, segmentCells);
    }

    // Point fill cell the specified OriginCells cell origin within FeatureType
    // cell groups and merge them to produce SegmentCount number of new feature types.
    static void PointFillSubdivideFeatures(
        FJCVDiagramMap& Map,
        const uint8 FeatureType,
        const TArray<int32>& OriginCellIndices,
        int32 SegmentCount,
        FRandomStream& Rand
        );

    // Depth Map Utility

    static void GenerateDepthMap(FJCVDiagramMap& SrcMap, FJCVDiagramMap& DstMap, const FJCVFeatureId& FeatureId);

    // Cell Query

    static void GetRandomCellWithinFeature(
        FJCVCellGroup& OutCells,
        FJCVDiagramMap& Map,
        uint8 FeatureType,
        int32 CellCount,
        FRandomStream& Rand,
        bool bAllowBorder = true,
        int32 MinCellDistance = 0
        );

    static void GetFeatureCellGroups(
        FJCVDiagramMap& Map,
        const TArray<FJCVFeatureId> FeatureIds,
        TArray<FJCVCellGroup>& OutCellGroups
        );
};

UCLASS()
class UJCVFeatureUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(DisplayName="GenerateDepthMap"))
    static void K2_GenerateDepthMap(UJCVDiagramAccessor* SrcAccessor, UJCVDiagramAccessor* DstAccessor, FJCVFeatureId FeatureId);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //static FJCVCellRef FindDepthMapCellOutsidePointRadius(UJCVDiagramAccessor* Accessor, int32 Seed, FVector2D Origin, float Radius, uint8 FeatureType, int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void GetCellsFromFeatures(UJCVDiagramAccessor* Accessor, const TArray<FJCVFeatureId>& FeatureIds, TArray<FJCVCellRefGroup>& CellRefGroups);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void GetRandomCellsFromFeaturesByDistanceFromDepthMapEdge(
        UJCVDiagramAccessor* Accessor,
        int32 Seed,
        const TArray<FJCVFeatureId>& FeatureIds,
        TArray<FJCVCellRef>& OutCellRefs,
        TArray<float>& OutDistances,
        float FilterDistanceRatio = 1.f,
        float FilterDistanceRatioRandom = 0.f
        );

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void GetRandomCellsFromDepthFeatureRangeByDistanceFromEdge(
        UJCVDiagramAccessor* Accessor,
        int32 Seed,
        uint8 FeatureType,
        int32 FeatureIndexStart,
        int32 FeatureIndexEnd,
        TArray<FJCVCellRef>& OutCellRefs,
        TArray<float>& OutDistances,
        float FilterDistanceRatio = 1.f,
        float FilterDistanceRatioRandom = 0.f
        );
};
