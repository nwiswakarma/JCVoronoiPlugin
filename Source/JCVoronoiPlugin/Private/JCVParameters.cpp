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

FJCVCellTraits::FJCVCellTraits(const FFilterCallback& InFilterCallback)
    : FilterCallback(InFilterCallback)
{
}

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

void UJCVTraitsLibrary::K2_CreateFeatureTraits(
    FJCVCellTraits& Traits,
    const FJCVCellTraits& SubTraits,
    const FJCVFeatureId& FeatureId,
    bool bInvertResult
    )
{
    CreateFeatureTraits(
        Traits,
        FeatureId,
        bInvertResult,
        SubTraits.GetCallbackRef()
        );
}

void UJCVTraitsLibrary::K2_CreateValueTraits(
    FJCVCellTraits& Traits,
    const FJCVCellTraits& SubTraits,
    float ValueLo,
    float ValueHi,
    bool bInvertResult
    )
{
    CreateValueTraits(
        Traits,
        ValueLo,
        ValueHi,
        bInvertResult,
        SubTraits.GetCallbackRef()
        );
}

void UJCVTraitsLibrary::CreateFeatureTraits(
    FJCVCellTraits& Traits,
    FJCVFeatureId FeatureId,
    bool bInvertResult,
    const FJCVCellTraits::FFilterCallback* SubCallbackRef
    )
{
    bool bIsSubCallbackValid = SubCallbackRef && !!(*SubCallbackRef);
    if (bIsSubCallbackValid)
    {
        FJCVCellTraits::FFilterCallback SubCallback(*SubCallbackRef);
        Traits.FilterCallback =
            [SubCallback,FeatureId,bInvertResult](const FJCVCell& Cell)
            {
                bool bResult;
                bResult = Cell.IsType(FeatureId.Type, FeatureId.Index);
                bResult = bResult && SubCallback(Cell);
                return bInvertResult ? !bResult : bResult;
            };
    }
    else
    {
        Traits.FilterCallback =
            [FeatureId,bInvertResult](const FJCVCell& Cell)
            {
                bool bResult = Cell.IsType(FeatureId.Type, FeatureId.Index);
                return bInvertResult ? !bResult : bResult;
            };
    }
}

void UJCVTraitsLibrary::CreateValueTraits(
    FJCVCellTraits& Traits,
    float ValueLo,
    float ValueHi,
    bool bInvertResult,
    const FJCVCellTraits::FFilterCallback* SubCallbackRef
    )
{
    bool bIsSubCallbackValid = SubCallbackRef && !!(*SubCallbackRef);
    if (bIsSubCallbackValid)
    {
        FJCVCellTraits::FFilterCallback SubCallback(*SubCallbackRef);
        Traits.FilterCallback =
            [SubCallback,ValueLo,ValueHi,bInvertResult](const FJCVCell& Cell)
            {
                bool bResult;
                bResult = Cell.Value >= ValueLo && Cell.Value <= ValueHi;
                bResult = bResult && SubCallback(Cell);
                return bInvertResult ? !bResult : bResult;
            };
    }
    else
    {
        Traits.FilterCallback =
            [ValueLo,ValueHi,bInvertResult](const FJCVCell& Cell)
            {
                bool bResult = Cell.Value >= ValueLo && Cell.Value <= ValueHi;
                return bInvertResult ? !bResult : bResult;
            };
    }
}
