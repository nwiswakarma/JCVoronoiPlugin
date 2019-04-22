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
#include "JCVTypes.h"
#include "JCVParameters.h"
#include "JCVDiagramAccessor.h"
#include "JCVFeatureUtility.generated.h"

class UJCVDiagramAccessor;

class FJCVFeatureUtility
{
public:

    static void PointFillVisit(FJCVDiagramMap& Map, const TArray<FJCVCell*>& OriginCells, const TFunctionRef<bool(FJCVCell&,FJCVCell&)>& VisitCallback);

    static void PointFill(FJCVDiagramMap& Map, const TArray<FJCVCell*>& OriginCells, uint8 FeatureTypeFilter = 255);

    static void PointFillIsolated(
        FJCVDiagramMap& Map,
        FJCVFeatureId BoundFeature,
        FJCVFeatureId TargetFeature,
        const TArray<FJCVCell*>& OriginCells
        );

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

    static void GetRandomCellWithinFeature(
        FJCVCellGroup& OutCells,
        const FJCVDiagramMap& Map,
        uint8 FeatureType,
        int32 CellCount,
        FRandomStream& Rand,
        bool bAllowBorder = true,
        int32 MinCellDistance = 0
        );

    static void GenerateDepthMap(FJCVDiagramMap& SrcMap, FJCVDiagramMap& DstMap, uint8 FeatureType, int32 FeatureIndex);
};

UCLASS()
class UJCVFeatureUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void GenerateDepthMap(UJCVDiagramAccessor* SrcAccessor, UJCVDiagramAccessor* DstAccessor, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static FJCVCellRef FindDepthMapCellOutsidePointRadius(UJCVDiagramAccessor* Accessor, int32 Seed, FVector2D Origin, float Radius, uint8 FeatureType, int32 FromIndex, int32 ToIndex);
};
