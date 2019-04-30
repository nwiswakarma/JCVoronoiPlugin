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
#include "JCVValueGenerator.generated.h"

class FJCVDiagramMap;
class UJCVDiagramAccessor;

class FJCVValueGenerator
{
    //static int32 MarkFeature(FJCVDiagramMap& Map, TQueue<FJCVCell*>& cellQ, TSet<FJCVCell*>& ExclusionSet, FJCVCell& c, int32 i, const FJCVCellTraits_Deprecated& cond);

public:

    static void AddRadialFill0(FJCVDiagramMap& Map, FRandomStream& Rand, FJCVCell& OriginCell, const FJCVRadialFill& FillParams);
    static void AddRadialFill(FJCVDiagramMap& Map, FRandomStream& Rand, FJCVCell& OriginCell, const FJCVRadialFill& FillParams);

    FORCEINLINE static void AddRadialFill(FJCVDiagramMap& Map, int32 Seed, FJCVCell& OriginCell, const FJCVRadialFill& FillParams)
    {
        FRandomStream Rand(Seed);
        AddRadialFill(Map, Rand, OriginCell, FillParams);
    }

    //static void MarkFeatures(FJCVDiagramMap& Map, const FJCVCellTraits_Deprecated& Cond, FJCVCellSet& ExclusionSet);

    //static void MarkFeatures(FJCVDiagramMap& Map, const FJCVSite& Seed, const FJCVCellTraits_Deprecated& Cond, int32 FeatureIndex, FJCVCellSet& ExclusionSet);

    //FORCEINLINE static void MarkFeatures(FJCVDiagramMap& Map, const FJCVCellTraits_Deprecated& Cond)
    //{
    //    TSet<FJCVCell*> ExclusionSet;
    //    ExclusionSet.Reserve(Map.Num());
    //    MarkFeatures(Map, Cond, ExclusionSet);
    //}

    //FORCEINLINE static void MarkFeatures(FJCVDiagramMap& Map, const FJCVSite& Seed, const FJCVCellTraits_Deprecated& Cond, int32 FeatureIndex)
    //{
    //    TSet<FJCVCell*> ExclusionSet;
    //    ExclusionSet.Reserve(Map.Num());
    //    MarkFeatures(Map, Seed, Cond, FeatureIndex, ExclusionSet);
    //}

    //FORCEINLINE static void ApplyValueByFeatures(FJCVDiagramMap& Map, const FJCVCellTraits_Deprecated& Cond, float Value)
    //{
    //    for (int32 i=0; i<Map.Num(); ++i)
    //    {
    //        FJCVCell& c( Map.GetCell(i) );
    //        if (Cond.HasValidFeature(c))
    //            c.Value = Value;
    //    }
    //}

    static void MapNormalizedDistanceFromCell(
        FJCVDiagramMap& Map,
        const FJCVCell& OriginCell,
        const FJCVFeatureId& FeatureId,
        bool bAgainstAnyType = false
        );
};

UCLASS()
class UJCVValueUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void SetCellValues(UJCVDiagramAccessor* Accessor, float Value);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void AddRadialFillAtPosition(UJCVDiagramAccessor* Accessor, int32 Seed, const FVector2D& Position, FJCVRadialFill FillParams);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void AddRadialFillAtCell(UJCVDiagramAccessor* Accessor, int32 Seed, FJCVCellRef OriginCellRef, FJCVRadialFill FillParams);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void AddRadialFillByIndex(UJCVDiagramAccessor* Accessor, int32 Seed, int32 CellIndex, FJCVRadialFill FillParams);

    UFUNCTION(BlueprintCallable, Category="JCV")
    static void AddRadialFillNum(UJCVDiagramAccessor* Accessor, int32 Seed, int32 PointCount, FJCVRadialFill FillParams, float Padding = 0.f, float ValueThreshold = .25f, int32 MaxPlacementTest = 50);
};
