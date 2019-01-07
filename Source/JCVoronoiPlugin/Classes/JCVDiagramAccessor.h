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

#include "CoreUObject.h"
#include "CoreTypes.h"
#include "Containers/Set.h"

#include "JCVoronoiPlugin.h"
#include "JCVParameters.h"
#include "JCVDiagramMap.h"
#include "JCVDiagramAccessor.generated.h"

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVFeatureId
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Index;

    FJCVFeatureId() = default;

    FJCVFeatureId(uint8 InType, int32 InIndex)
        : Type(InType)
        , Index(InIndex)
    {
    }
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVCellTypeGroupRef
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<uint8> Data;
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVCellRef
{
	GENERATED_BODY()

    const FJCVCell* Data;

    FJCVCellRef() : Data(nullptr)
    {
    }

    FJCVCellRef(const FJCVCell* Cell) : Data(Cell)
    {
    }
};

class UJCVDiagramObject;

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVCellDetailsRef
{
	GENERATED_BODY()

    const FJCVCell* Cell;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsValid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Index;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Point;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsBorder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 FeatureType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FeatureIndex;

    FJCVCellDetailsRef() : bIsValid(false)
    {
    }

    explicit FJCVCellDetailsRef(const FJCVCellRef& InCellRef)
    {
        Set(InCellRef.Data);
    }

    explicit FJCVCellDetailsRef(const FJCVCell* InCell)
    {
        Set(InCell);
    }

    void Set(const FJCVCell* InCell)
    {
        Cell = InCell;
        bIsValid = Cell != nullptr;

        if (bIsValid)
        {
            Index        = Cell->GetIndex();
            Point        = Cell->ToVector2D();
            Value        = Cell->Value;
            bIsBorder    = Cell->bIsBorder;
            FeatureType  = Cell->FeatureType;
            FeatureIndex = Cell->FeatureIndex;
        }
    }
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVCellRefGroup
{
	GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FJCVCellRef> Data;
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVCellJunctionRef
{
	GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector2D Point;

    UPROPERTY(BlueprintReadWrite)
    TArray<FJCVCellRef> Cells;

    FJCVCellJunctionRef() = default;

    FJCVCellJunctionRef(const FVector2D& InPoint, const FJCVConstCellGroup& InCells)
        : Point(InPoint)
        , Cells(InCells)
    {
    }
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVPointGroup
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector2D> Points;
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVDualGeometry
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector2D> Points;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> PolyIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> CellIndices;

    FORCEINLINE bool HasGeometry() const
    {
        return Points.Num() >= 3 && PolyIndices.Num() >= 3;
    }
};

USTRUCT(BlueprintType)
struct JCVORONOIPLUGIN_API FJCVPolyGeometry
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> Points;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> PolyIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> CellIndices;

    FORCEINLINE bool HasGeometry() const
    {
        return Points.Num() >= 3 && PolyIndices.Num() >= 3;
    }
};

UCLASS(BlueprintType)
class JCVORONOIPLUGIN_API UJCVDiagramAccessor : public UObject
{
	GENERATED_BODY()

    FJCVDiagramMap* Map;

public:

    void SetMap(FJCVDiagramMap& AccessedMap)
    {
        Map = &AccessedMap;
    }

    FORCEINLINE bool HasValidMap() const
    {
        return Map != nullptr;
    }

    FORCEINLINE const FBox2D& GetBounds() const
    {
        check(HasValidMap());
        return Map->GetBounds();
    }

    FORCEINLINE FJCVDiagramMap& GetMap()
    {
        return *Map;
    }

    FORCEINLINE const FJCVDiagramMap& GetMap() const
    {
        return *Map;
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Has Valid Map")
    bool K2_HasValidMap() const
    {
        return HasValidMap();
    }

    UFUNCTION(BlueprintCallable, Category="JCV", DisplayName="Get Map Bounds")
    FBox2D K2_GetBounds() const
    {
        return HasValidMap() ? GetBounds() : FBox2D();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetCellIndex(const FJCVCellRef& CellRef) const
    {
        if (HasValidMap() && CellRef.Data)
        {
            return Map->GetCellIndex(CellRef.Data);
        }

        return -1;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetCellValue(int32 CellIndex) const
    {
        return (Map && Map->IsValidIndex(CellIndex))
            ? Map->GetCell(CellIndex).Value
            : -1.f;
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    FVector2D GetCellPosition(int32 CellIndex) const
    {
        return (Map && Map->IsValidIndex(CellIndex))
            ? Map->GetCell(CellIndex).ToVector2D()
            : FVector2D();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRef GetCell(int32 CellIndex)
    {
        return (Map && Map->IsValidIndex(CellIndex))
            ? FJCVCellRef(&Map->GetCell(CellIndex))
            : FJCVCellRef();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool IsValidCell(const FJCVCellRef& CellRef) const
    {
        return (GetCellIndex(CellRef) >= 0);
    }

// MARK FEATURE FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkDefaultFeatures(uint8 FeatureType);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkFeaturesByType(FJCVCellTraitsParams TypeTraits);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkFeaturesByValue(FJCVValueTraitsParams ValueTraits);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkPositions(const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId, bool bContiguous = true);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkRange(const FVector2D& StartPosition, const FVector2D& EndPosition, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MarkRangeByFeature(int32 StartCellID, int32 EndCellID, FJCVFeatureId FeatureId, float Value, bool bUseFilter, FJCVCellTraitsParams FilterCond);

// FEATURE UTILITY FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetFeatureCount() const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetFeatureGroupCount(uint8 FeatureType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ResetFeatures(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ApplyValueByFeatures(FJCVCellTraitsParams TypeTraits, float Value);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ConvertIsolated(uint8 FeatureType0, uint8 FeatureType1, int32 FeatureIndex, bool bGroupFeatures);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExpandFeature(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void PointFillSubdivideFeatures(
        const uint8 FeatureType,
        const TArray<int32>& OriginCellIndices,
        int32 SegmentCount,
        int32 Seed
        );

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GroupByFeatures();

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ShrinkFeatures();

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ScaleFeatureValuesByIndex(uint8 FeatureType, int32 IndexOffset = 0);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void InvertFeatureValues(FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ApplyCurveToFeatureValues(uint8 FeatureType, UCurveFloat* CurveScale);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void MapNormalizedDistanceFromCell(FJCVCellRef OriginCellRef, FJCVFeatureId FeatureId, bool bAgainstAnyType = false);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVPointGroup GetFeaturePoints(FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup GetFeatureCells(FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetRandomCellWithinFeature(uint8 FeatureType, int32 CellCount, int32 Seed, bool bAllowBorders = false, int32 MinCellDistance = 0) const;

// CELL QUERY FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetCellCount() const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellDetailsRef GetCellDetails(const FJCVCellRef& CellRef) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellDetailsRef> GetCellGroupDetails(const TArray<FJCVCellRef> CellRefs) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<uint8> GetNeighbourTypes(const FJCVCellRef& CellRef) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellTypeGroupRef> GetGroupNeighbourTypes(const FJCVCellRefGroup& CellGroup) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetCellRange(const FVector2D& StartPosition, const FVector2D& EndPosition);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRef GetRandomCell(FJCVFeatureId FeatureId, int32 Seed);

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GetRandomCells(int32 Count, FJCVFeatureId FeatureId, int32 Seed, int32 IterationLimit = 50);

    UFUNCTION(BlueprintCallable, Category="JCV")
    int32 GetClosestCellAt(const FVector2D& Pos) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> FilterPoints(const TArray<FVector2D>& Points, FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRef FindCell(const FVector2D& Position) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup FindCells(const TArray<FVector2D>& Positions) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellRefGroup> FindCellsWithinRects(const TArray<FBox2D>& Rects) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup FindBorderCells(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders = false, bool bAgainstAnyType = false) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellJunctionRef> FindJunctionCells(uint8 FeatureType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVPointGroup> FindEdgePoints(uint8 FeatureType0, uint8 FeatureType1, bool bAllowBorders = false, bool bAgainstAnyType = false) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool FindEdgePointsWithEndPoints(
        UPARAM(ref) TArray<FJCVPointGroup>& PointGroups,
        UPARAM(ref) TArray<FJCVCellRefGroup>& EndPointCellGroups,
        uint8 FeatureType0,
        uint8 FeatureType1,
        bool bAllowBorders = false,
        bool bAgainstAnyType = false
        ) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVPointGroup> GenerateOrderedFeatureBorderPoints(
        uint8 InitialFeatureType,
        TArray<uint8> AdditionalFeatures,
        bool bExpandEdges = false,
        bool bAllowBorders = false
        ) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup GetCellByOriginRadius(
        const FJCVCellRef& OriginCellRef,
        float Radius,
        FJCVFeatureId FeatureId,
        bool bAgainstAnyType = false
        ) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup ExpandCellQuery(const FJCVCellRef& CellRef, int32 ExpandCount, FJCVFeatureId FeatureId, bool bAgainstAnyType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup ExpandCellGroupQuery(const FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId, int32 ExpandCount, bool bAgainstAnyType) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    void FilterCellsByType(UPARAM(ref) FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ExcludeCellsByType(UPARAM(ref) FJCVCellRefGroup& CellGroup, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    FJCVCellRefGroup MergeCellGroups(const TArray<FJCVCellRefGroup>& CellGroups) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<FJCVCellJunctionRef> FilterUniqueJunctions(const TArray<FJCVCellJunctionRef>& Junctions) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetClosestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    float GetFurthestDistanceToFeature(const FJCVCellRef& OriginCellRef, FJCVFeatureId FeatureId) const;

// CELL VALUE FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillAt(const FVector2D& Position, const FJCVRadialFillParams& Params, int32 Seed);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillByIndex(int32 CellIndex, const FJCVRadialFillParams& Params, int32 Seed);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void AddRadialFillNum(int32 PointCount, const FJCVRadialFillParams& Params, int32 Seed, float Padding=0.f, float ValueThreshold=.25f, int32 MaxPlacementTest=50);

// CELL UTILITY FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    TArray<int32> GenerateCellGridIndices(const FJCVCellRef& Cell, const FIntPoint& GridDimension, const float BoundsExpand = 0.f) const;

// MAP UTILITY FUNCTIONS

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateSegments(const TArray<FVector2D>& SegmentOrigins, int32 SegmentMergeCount, int32 Seed);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateOrogeny(UJCVDiagramAccessor* PlateAccessor, int32 Seed, const FJCVRadialFillParams& ValueParams, const FJCVOrogenParams& OrogenParams);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateDepthMap(UJCVDiagramAccessor* TargetAccessor, FJCVFeatureId FeatureId);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void GenerateDualGeometry(UPARAM(ref) FJCVDualGeometry& Geometry, bool bClearContainer = true);

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
        TSet<FIntPoint>& VisitedPointSet,
        TMap<int32, int32>& CellIndexMap,
        TArray<FVector2D>& Points,
        TArray<int32>& PolyIndices,
        TArray<int32>& CellIndices,
        const FJCVPoint& Point,
        const FJCVCell& Cell0,
        const FJCVCell& Cell1,
        const FJCVCell& Cell2
        ) const;

    void MarkPositions(FJCVDiagramMap& MapRef, const TArray<FVector2D>& Positions, FJCVFeatureId FeatureId);
    void MarkPositionsContiguous(FJCVDiagramMap& MapRef, const TArray<FVector2D>& Positions, const FJCVFeatureId& FeatureId);
};
