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

#include "JCVCellUtility.h"
#include "JCVDiagramMap.h"

float FJCVCellUtility::GetClosestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    const FJCVFeatureId& FeatureId,
    bool bAgainstAnyType
    )
{
    const uint8 FeatureType = FeatureId.Type;
    const int32 FeatureIndex = FeatureId.Index;

    check(Map.HasFeature(FeatureType));
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

float FJCVCellUtility::GetFurthestDistanceFromCellSq(
    FJCVDiagramMap& Map,
    const FJCVCell& OriginCell,
    const FJCVFeatureId& FeatureId,
    bool bAgainstAnyType
    )
{
    const uint8 FeatureType = FeatureId.Type;
    const int32 FeatureIndex = FeatureId.Index;

    check(Map.HasFeature(FeatureType));
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
