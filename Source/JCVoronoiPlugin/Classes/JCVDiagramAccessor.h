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

#include "JCVoronoiPlugin.h"
#include "JCVParameters.h"
#include "JCVDiagramTypes.h"
#include "JCVDiagramAccessor.generated.h"

#define JCV_RANDOM_CELL_COPY_THRESHOLD_RATIO 4
#define JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION 50

#ifndef JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION
#define JCV_RANDOM_CELL_TRAITS_CHECK_MAX_ITERATION_DETECTION 1
#endif

class FJCVDiagramMap;
struct FJCVCellEdgeList;

UCLASS(BlueprintType)
class JCVORONOIPLUGIN_API UJCVDiagramAccessor : public UObject
{
	GENERATED_BODY()

    friend class UJCVDiagramObject;

    FJCVDiagramMap* Map;
    int32 ContextId;
    int32 MapId;

    void SetMap(FJCVDiagramMap& AccessedMap, int32 InContextId, int32 InMapId);

public:

    FORCEINLINE bool HasValidMap() const
    {
        return Map != nullptr;
    }

    FORCEINLINE FJCVDiagramMap& GetMap()
    {
        return *Map;
    }

    FORCEINLINE const FJCVDiagramMap& GetMap() const
    {
        return *Map;
    }

    FORCEINLINE int32 GetContextId() const
    {
        return ContextId;
    }

    FORCEINLINE int32 GetMapId() const
    {
        return MapId;
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Has Valid Map")
    bool K2_HasValidMap() const
    {
        return HasValidMap();
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Get Context Id")
    int32 K2_GetContextId() const
    {
        return GetContextId();
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Get Map Id")
    int32 K2_GetMapId() const
    {
        return GetMapId();
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Get Context Map Id")
    void K2_GetContextMapId(int32& OutContextId, int32& OutMapId)
    {
        OutContextId = ContextId;
        OutMapId = MapId;
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Get Map Bounds")
    FBox2D K2_GetBounds() const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetCellIndex(const FJCVCellRef& CellRef) const;

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(DisplayName="GetCell"))
    FJCVCellRef GetCellRef(int32 CellIndex);

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool IsValidCell(const FJCVCellRef& CellRef) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetCellCount() const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellDetails(const FJCVCellRef& CellRef, FJCVCellDetailsRef& CellDetails);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellBounds(const FJCVCellRef& CellRef, FBox2D& CellBounds);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellGroupDetails(const TArray<FJCVCellRef>& CellRefs, TArray<FJCVCellDetailsRef>& CellDetails);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetNeighbourCells(const FJCVCellRef& CellRef, TArray<FJCVCellRef>& NeighbourCellRefs);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellPositions(const TArray<FJCVCellRef>& CellRefs, TArray<FVector2D>& CellPositions);

// CELL QUERY FUNCTIONS

    // Neighbour Type Query

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetNeighbourTypes(TArray<uint8>& Types, const FJCVCellRef& CellRef);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellGroupNeighbourTypes(TArray<FJCVCellTypeGroupRef>& TypeGroups, const FJCVCellRefGroup& CellGroup);

    // Range Query

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellRange(TArray<FJCVCellRef>& CellRefs, const FVector2D& StartPosition, const FVector2D& EndPosition);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRef GetClosestCellAt(const FVector2D& Pos);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellsWithinRect(TArray<FJCVCellRef>& CellRefs, const FBox2D& Rect);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetCellWithinOriginRadius(
        TArray<FJCVCellRef>& CellRefs,
        const FJCVCellRef& OriginCellRef,
        float Radius,
        FJCVFeatureId FeatureId,
        bool bAgainstAnyType = false
        );

    // Expand / Filter Query

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup ExpandCellQuery(const FJCVCellRef& CellRef, int32 ExpandCount, FJCVFeatureId FeatureId, bool bAgainstAnyType);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup ExpandCellGroupQuery(const FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId, int32 ExpandCount, bool bAgainstAnyType);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void FilterCellsByType(UPARAM(ref) FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExcludeCellsByType(UPARAM(ref) FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup MergeCellGroups(const TArray<FJCVCellRefGroup>& CellGroups);

    // Query Information

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetClosestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetFurthestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId);

    // Random Search

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(AutoCreateRefTerm="Traits"))
    FJCVCellRef GetRandomCell(int32 Seed, const FJCVCellTraitsRef& Traits);

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(AutoCreateRefTerm="Traits,FeatureId"))
    FJCVCellRef GetRandomCellByFeature(int32 Seed, const FJCVFeatureId& FeatureId, const FJCVCellTraitsRef& Traits);

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(AutoCreateRefTerm="Traits"))
    void GetRandomCells(TArray<FJCVCellRef>& CellRefs, int32 Seed, const FJCVCellTraitsRef& Traits, int32 Count = 1);

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(AutoCreateRefTerm="Traits,FeatureId"))
    void GetRandomCellsByFeature(TArray<FJCVCellRef>& CellRefs, int32 Seed, const FJCVFeatureId& FeatureId, const FJCVCellTraitsRef& Traits, int32 Count = 1);

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetRandomCellWithinFeature(uint8 FeatureType, int32 CellCount, int32 Seed, bool bAllowBorders = false, int32 MinCellDistance = 0);

    // Border & Junction Search

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup FindBorderCells(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders = false, bool bAgainstAnyType = false);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void FindJunctionCells(TArray<FJCVCellJunctionRef>& JunctionRefs, uint8 FeatureType);

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellJunctionRef> FilterUniqueJunctions(const TArray<FJCVCellJunctionRef>& Junctions);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void FindEdgePoints(
        TArray<FJCVPointGroup>& PointGroups,
        uint8 FeatureType0,
        uint8 FeatureType1,
        bool bAllowBorders = false,
        bool bAgainstAnyType = false
        );

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool FindEdgePointsWithEndPoints(
        TArray<FJCVPointGroup>& PointGroups,
        TArray<FJCVCellRefGroup>& EndPointCellGroups,
        uint8 FeatureType0,
        uint8 FeatureType1,
        bool bAllowBorders = false,
        bool bAgainstAnyType = false
        );

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVPointGroup> GenerateOrderedFeatureBorderPoints(
        uint8 InitialFeatureType,
        TArray<uint8> AdditionalFeatures,
        bool bExpandEdges = false,
        bool bAllowBorders = false
        );

// CELL VALUE FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ScaleFeatureValuesByIndex(uint8 FeatureType, int32 IndexOffset = 0);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void InvertFeatureValues(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ApplyCurveToFeatureValues(uint8 FeatureType, UCurveFloat* CurveScale);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void ApplyValueByFeatures(FJCVCellTraits_Deprecated FeatureTraits, float Value);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MapNormalizedDistanceFromCell(FJCVCellRef OriginCellRef, FJCVFeatureId FeatureId, bool bAgainstAnyType = false);

// CELL UTILITY FUNCTIONS

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //TArray<int32> GenerateCellGridIndices(const FJCVCellRef& Cell, const FIntPoint& GridDimension, const float BoundsExpand = 0.f);

// MARK FEATURE FUNCTIONS

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void MarkUnmarkedFeatures(uint8 FeatureType);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void MarkFeaturesByType(FJCVCellTraits_Deprecated FeatureTraits);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void MarkFeaturesByValue(FJCVValueTraits_Deprecated ValueTraits);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void MarkRange(const FVector2D& StartPosition, const FVector2D& EndPosition, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraits_Deprecated FilterTraits);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void MarkRangeByFeature(int32 StartCellID, int32 EndCellID, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraits_Deprecated FilterTraits);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkPositions(
        TArray<FJCVCellRef>& VisitedCellRefs,
        const TArray<FVector2D>& Positions,
        FJCVFeatureId FeatureId,
        bool bContiguous = true,
        bool bClampPoints = true,
        bool bExtractVisitedCells = false
        );

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkPoly(const TArray<FVector2D>& Points, FJCVFeatureId FeatureMarkId, bool bClampPoints = true);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkIsolatedFeatures(const TArray<FJCVCellRef>& BoundingCellRefs, FJCVFeatureId FeatureMarkId);

// FEATURE UTILITY FUNCTIONS

    // Feature Information

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool HasFeature(FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetFeatureCount() const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetFeatureGroupCount(uint8 FeatureType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetLastFeatureGroupIndex(uint8 FeatureType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetFeatureCellCount(FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetFeaturePoints(TArray<FVector2D>& Points, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GetFeatureCells(TArray<FJCVCellRef>& CellRefs, FJCVFeatureId FeatureId);

    // Feature Modifications

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ResetFeatures(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ConvertIsolated(uint8 FeatureType0, uint8 FeatureType1, int32 FeatureIndex, bool bGroupFeatures);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExpandFeature(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExpandFeatureFromCellGroups(const TArray<FJCVCellRef>& CellRefs, FJCVFeatureId FeatureId, int32 ExpandCount = 1);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void PointFillIsolatedFeatures(const TArray<FJCVCellRef>& OriginCellRefs, FJCVFeatureId BoundingFeature, FJCVFeatureId TargetFeature);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void PointFillSubdivideFeatures(const TArray<int32>& OriginCellIndices, int32 Seed, uint8 FeatureType, int32 SegmentCount);

    // Feature Group

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GroupByFeatures();

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ShrinkFeatures();

// MAP UTILITY FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateSegments(const TArray<FVector2D>& SegmentOrigins, int32 SegmentMergeCount, int32 Seed);

    //UFUNCTION(BlueprintCallable, Category="JCV")
    //void GenerateOrogeny(UJCVDiagramAccessor* PlateAccessor, int32 Seed, FJCVRadialFill FillParams, const FJCVOrogenParams& OrogenParams);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateDualGeometry(UPARAM(ref) FJCVDualGeometry& Geometry, bool bClearContainer = true);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GeneratePolyGeometry(UPARAM(ref) FJCVPolyGeometry& Geometry, bool bFilterDuplicates = true, bool bUseCellAverageValue = false, bool bClearContainer = true);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateDualGeometryByFeature(UPARAM(ref) FJCVDualGeometry& Geometry, FJCVFeatureId FeatureId, bool bClearContainer = true);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GeneratePolyGeometryByFeature(UPARAM(ref) FJCVPolyGeometry& Geometry, FJCVFeatureId FeatureId, bool bUseCellAverageValue, bool bClearContainer = true);

private:

    void GenerateOrderedFeatureBorderPoints(
        TArray<FJCVCellEdgeList>& EdgeLists,
        uint8 FeatureType0,
        uint8 FeatureType1,
        bool bAllowBorders
        ) const;

    void GetPointDualGeometry(
        TMap<int32, int32>& CellIndexMap,
        TArray<FVector>& Points,
        TArray<int32>& PolyIndices,
        TArray<int32>& CellIndices,
        const FJCVPoint& Point,
        const FJCVCell& Cell0,
        const FJCVCell& Cell1,
        const FJCVCell& Cell2
        ) const;

    void GetPointDualGeometry(
        TSet<FIntPoint>& VisitedPointSet,
        TMap<int32, int32>& CellIndexMap,
        TArray<FVector>& Points,
        TArray<int32>& PolyIndices,
        TArray<int32>& CellIndices,
        const FJCVPoint& Point,
        const FJCVCell& Cell0,
        const FJCVCell& Cell1,
        const FJCVCell& Cell2
        ) const;

    void MarkPositions(const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId, TArray<FJCVCell*>* VisitedCells = nullptr, bool bClampPoints = true);
    void MarkPositionsContiguous(const TArray<FVector2D>& Positions, const FJCVFeatureId& FeatureId, TArray<FJCVCell*>* VisitedCells = nullptr, bool bClampPoints = true);
    void MarkPoly(const TArray<FVector2D>& Points, const FJCVFeatureId& FeatureId, TArray<FJCVCell*>* VisitedCells = nullptr);
    void MarkIsolatedFeatures(const TArray<FJCVCell*>& BoundingCells, const FJCVFeatureId& FeatureMarkId);

    void GetCellsFromRefs(const TArray<FJCVCellRef>& CellRefs, TArray<FJCVCell*>& Cells) const;
};
