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

#include "JCVParameters.h"
#include "JCVDiagramMap.h"

//FJCVCellTraitsRef::FJCVCellTraitsRef(const FFilterCallback& InFilterCallback)
//    : FilterCallback(InFilterCallback)
//{
//}

//bool FJCVCellTraits_Deprecated::HasValidFeature(const FJCVCell& c) const
//{
//    return c.FeatureType == TestType;
//}

//bool FJCVCellTraits_Deprecated::HasUndefinedType(const FJCVCell& c) const
//{
//    return c.FeatureType == JCV_CF_UNMARKED && HasValidFeature(c);
//}

//bool FJCVValueTraits_Deprecated::HasValidFeature(const FJCVCell& c) const
//{
//    return c.Value > ValueLo && c.Value < ValueHi;
//}

FJCVCellDetailsRef::FJCVCellDetailsRef()
    : Cell(nullptr)
    , bIsValid(false)
    , Index(-1)
    , Point(ForceInitToZero)
    , Value(0.f)
    , bIsBorder(false)
    , FeatureType(255)
    , FeatureIndex(-1)
{
}

void FJCVCellDetailsRef::Set(const FJCVCell* InCell)
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

// Traits

bool FJCVFeatureTraits::HasMatchingTraits(const FJCVCell& Cell) const
{
    return Cell.IsType(FeatureId.Type, FeatureId.Index);
}

bool FJCVValueTraits::HasMatchingTraits(const FJCVCell& Cell) const
{
    return Cell.Value >= ValueLo && Cell.Value <= ValueHi;
}

bool FJCVPointRadiusTraits::HasMatchingTraits(const FJCVCell& Cell) const
{
    return (Origin-Cell.ToVector2D()).SizeSquared() < (Radius*Radius);
}

// Traits Generator

void FJCVPointRadiusTraitsGenerator::GenerateFromMapCell(FJCVDiagramMap& Map, const FJCVCell& Cell, FPSJCVCellTraits& Traits) const
{
    FJCVPointRadiusTraits* NewTraits(new FJCVPointRadiusTraits);
    NewTraits->Origin = Cell.ToVector2D();
    NewTraits->Radius = Radius;
    Traits = MakeShareable(NewTraits);
}

void FJCVFeatureDistanceTraitsGenerator::GenerateFromMapCell(FJCVDiagramMap& Map, const FJCVCell& Cell, FPSJCVCellTraits& Traits) const
{
    FJCVPointRadiusTraits* NewTraits(new FJCVPointRadiusTraits);
    float Dist = FJCVCellUtility::GetClosestDistanceFromCell(Map, Cell, FeatureId, false);
    float DistScale = DistanceScale;
    DistScale -= DistScale * DistanceScaleRandom * RandomStream.GetFraction();
    NewTraits->Origin = Cell.ToVector2D();
    NewTraits->Radius = Dist * DistScale;
    Traits = MakeShareable(NewTraits);
}

// Traits (Blueprint)

void UJCVTraitsLibrary::K2_CreateFeatureTraits(
    FJCVCellTraitsRef& Traits,
    const FJCVCellTraitsRef& SubTraits,
    FJCVFeatureId FeatureId
    )
{
    FJCVFeatureTraits* NewTraits(new FJCVFeatureTraits);
    NewTraits->FeatureId = FeatureId;
    Traits.Traits = FPSJCVCellTraits(NewTraits);
    Traits.SubTraits = SubTraits.Traits;
}

void UJCVTraitsLibrary::K2_CreateValueTraits(
    FJCVCellTraitsRef& Traits,
    const FJCVCellTraitsRef& SubTraits,
    float ValueLo,
    float ValueHi
    )
{
    FJCVValueTraits* NewTraits(new FJCVValueTraits);
    NewTraits->ValueLo = ValueLo;
    NewTraits->ValueHi = ValueHi;
    Traits.Traits = FPSJCVCellTraits(NewTraits);
    Traits.SubTraits = SubTraits.Traits;
}

void UJCVTraitsLibrary::K2_CreatePointRadiusTraits(
    FJCVCellTraitsRef& Traits,
    const FJCVCellTraitsRef& SubTraits,
    FVector2D Origin,
    float Radius
    )
{
    FJCVPointRadiusTraits* NewTraits(new FJCVPointRadiusTraits);
    NewTraits->Origin = Origin;
    NewTraits->Radius = Radius;
    Traits.Traits = FPSJCVCellTraits(NewTraits);
    Traits.SubTraits = SubTraits.Traits;
}

void UJCVTraitsLibrary::K2_CreateFeatureDistanceTraitsGenerator(
    FJCVCellTraitsGeneratorRef& GeneratorRef,
    const FJCVCellTraitsGeneratorRef& SubGeneratorRef,
    int32 Seed,
    FJCVFeatureId FeatureId,
    float DistanceScale,
    float DistanceScaleRandom
    )
{
    FJCVFeatureDistanceTraitsGenerator* Generator(new FJCVFeatureDistanceTraitsGenerator);
    Generator->RandomStream = FRandomStream(Seed);
    Generator->FeatureId = FeatureId;
    Generator->DistanceScale = DistanceScale;
    Generator->DistanceScaleRandom = DistanceScaleRandom;
    GeneratorRef.Generator = MakeShareable(Generator);
    GeneratorRef.SubGenerator = SubGeneratorRef.Generator;
}
