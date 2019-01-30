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
#include "JCVDiagramMap.h"
#include "JCVParameters.generated.h"

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

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVRadialFill
{
	GENERATED_BODY()

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Value = .5f;

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Radius = .85f;

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Sharpness = 0.f;

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    UCurveFloat* ValueCurve = nullptr;

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    bool bRadialDegrade = true;

	UPROPERTY(Category = "JCV|Radial Fill", BlueprintReadWrite, EditAnywhere)
    bool bFilterBorder = true;
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVCellTraits
{
	GENERATED_BODY()

	UPROPERTY(Category = "JCV|Cell Traits", BlueprintReadWrite, EditAnywhere)
    uint8 TestType = 255;

	UPROPERTY(Category = "JCV|Cell Traits", BlueprintReadWrite, EditAnywhere)
    uint8 FeatureType;

    FJCVCellTraits() = default;

    FJCVCellTraits(uint8 f)
        : FeatureType(f)
    {
    }

    FJCVCellTraits(uint8 t, uint8 f)
        : TestType(t)
        , FeatureType(f)
    {
    }

    FORCEINLINE virtual bool HasValidFeature(const FJCVCell& c) const
    {
        return c.FeatureType == TestType;
    }

    FORCEINLINE virtual bool HasUndefinedType(const FJCVCell& c) const
    {
        return c.FeatureType == EJCVCellFeature::UNDEFINED && HasValidFeature(c);
    }
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVValueTraits : public FJCVCellTraits
{
	GENERATED_BODY()

	UPROPERTY(Category = "JCV|Cell Traits", BlueprintReadWrite, EditAnywhere)
    float ValueLo;

	UPROPERTY(Category = "JCV|Cell Traits", BlueprintReadWrite, EditAnywhere)
    float ValueHi;

    FJCVValueTraits() = default;

    FJCVValueTraits(float l, float v, uint8 f)
        : FJCVCellTraits(f)
        , ValueLo(l)
        , ValueHi(v)
    {
    }

    FORCEINLINE virtual bool HasValidFeature(const FJCVCell& c) const override
    {
        return c.Value > ValueLo && c.Value < ValueHi;
    }
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVOrogenParams
{
	GENERATED_BODY()

	UPROPERTY(Category = "JCV|Orogen Settings", BlueprintReadWrite, EditAnywhere)
    float OriginThreshold = .1f;

	UPROPERTY(Category = "JCV|Orogen Settings", BlueprintReadWrite, EditAnywhere)
    float AreaThreshold = .01f;

	UPROPERTY(Category = "JCV|Orogen Settings", BlueprintReadWrite, EditAnywhere)
    uint8 FeatureType = 1;

	UPROPERTY(Category = "JCV|Orogen Settings", BlueprintReadWrite, EditAnywhere)
    bool bDivergentAsConvergent = false;
};
