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
#include "JCVParameters.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVDiagramMapID
{
	GENERATED_BODY()

    /**
     * Island Context ID
     */
	UPROPERTY(Category = "Island ID", BlueprintReadWrite, EditAnywhere)
    int32 ContextID = -1;

    /**
     * Island ID
     */
	UPROPERTY(Category = "Island ID", BlueprintReadWrite, EditAnywhere)
    int32 IslandID = 0;
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVRadialFillParams
{
	GENERATED_BODY()

	UPROPERTY(Category="Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Value = .5f;

	UPROPERTY(Category="Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Radius = .85f;

	UPROPERTY(Category="Radial Fill", BlueprintReadWrite, EditAnywhere)
    float Sharpness = 0.f;

	UPROPERTY(Category="Radial Fill", BlueprintReadWrite, EditAnywhere)
    bool bRadialDegrade = true;

	UPROPERTY(Category="Radial Fill", BlueprintReadWrite, EditAnywhere)
    bool bFilterBorder = true;
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVOrogenParams
{
	GENERATED_BODY()

	UPROPERTY(Category = "Orogen Settings", BlueprintReadWrite, EditAnywhere)
    float OriginThreshold = .1f;

	UPROPERTY(Category = "Orogen Settings", BlueprintReadWrite, EditAnywhere)
    float AreaThreshold = .01f;

	UPROPERTY(Category = "Orogen Settings", BlueprintReadWrite, EditAnywhere)
    uint8 FeatureType = 1;

	UPROPERTY(Category = "Orogen Settings", BlueprintReadWrite, EditAnywhere)
    bool bDivergentAsConvergent = false;
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVCellTraitsParams
{
	GENERATED_BODY()

	UPROPERTY(Category = "Cell Traits", BlueprintReadWrite, EditAnywhere)
    uint8 TestType = 255;

	UPROPERTY(Category = "Cell Traits", BlueprintReadWrite, EditAnywhere)
    uint8 FeatureType;

    FJCVCellTraitsParams() = default;

    FJCVCellTraitsParams(uint8 f)
        : FeatureType(f)
    {
    }

    FJCVCellTraitsParams(uint8 t, uint8 f)
        : TestType(t)
        , FeatureType(f)
    {
    }
};

USTRUCT(BlueprintType, Blueprintable)
struct JCVORONOIPLUGIN_API FJCVValueTraitsParams : public FJCVCellTraitsParams
{
	GENERATED_BODY()

	UPROPERTY(Category = "Cell Traits", BlueprintReadWrite, EditAnywhere)
    float ValueLo;

	UPROPERTY(Category = "Cell Traits", BlueprintReadWrite, EditAnywhere)
    float ValueHi;

    FJCVValueTraitsParams() = default;

    FJCVValueTraitsParams(float l, float v, uint8 f)
        : FJCVCellTraitsParams(f)
        , ValueLo(l)
        , ValueHi(v)
    {
    }
};
