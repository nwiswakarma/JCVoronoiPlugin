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

#include "JCVDiagramObject.h"
#include "JCVDiagramAccessor.h"

void UJCVDiagramObject::BeginDestroy()
{
    // Clear registered diagram references
    ResetDiagramObject();

    Super::BeginDestroy();
}

bool UJCVDiagramObject::K2_HasContext(int32 ContextId) const
{
    return HasContext(ContextId);
}

bool UJCVDiagramObject::K2_HasMap(int32 ContextId, int32 MapId) const
{
    return HasMap(ContextId, MapId);
}

void UJCVDiagramObject::ResetDiagramObject()
{
    ContextMap.Empty();
    Accessors.Empty();
}

void UJCVDiagramObject::CreateContext(int32 ContextId, FVector2D Size, TArray<FVector2D> Points)
{
    if (Points.Num() <= 0)
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateContext() ABORTED, UNABLE TO GENERATE ISLAND WITH EMPTY POINTS"));
        return;
    }

    if (! HasContext(ContextId))
    {
        FPSJCVDiagramMapContext Context(new FJCVDiagramMapContext(Size, Points));
        ContextMap.Emplace(ContextId, Context);
    }
}

void UJCVDiagramObject::CreateMap(int32 ContextId, int32 MapId)
{
    if (HasContext(ContextId))
    {
        CreateMapWithDefaultType(ContextId, MapId, JCV_CF_UNMARKED, 0);
    }
    else
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateMap() ABORTED, INVALID ISLAND CONTEXT"));
        return;
    }
}

void UJCVDiagramObject::CreateMapWithDefaultType(int32 ContextId, int32 MapId, uint8 FeatureType, int32 FeatureIndex)
{
    if (! HasContext(ContextId))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateMapWithDefaultType() ABORTED, INVALID ISLAND CONTEXT"));
        return;
    }

    FPSJCVDiagramMapContext Context = GetContext(ContextId);
    FJCVDiagramMap& Map(Context->CreateMap(MapId, FeatureType, FeatureIndex, true));

    FContextIdentifier& cid(ContextMap.FindChecked(ContextId));
    int32 aid = CreateAccessor(Map, ContextId, MapId);

    cid.AccessorMap.Emplace(MapId, aid);
}

void UJCVDiagramObject::CopyMap(int32 ContextId, int32 SrcMapId, int32 DstMapId)
{
    if (! HasContext(ContextId))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, INVALID ISLAND CONTEXT"));
        return;
    }

    if (! HasMap(ContextId, SrcMapId))
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, INVALID SOURCE ISLAND"));
        return;
    }

    if (SrcMapId == DstMapId)
    {
        UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, SOURCE AND DESTINATION ISLAND HAVE THE SAME ID"));
        return;
    }

    FPSJCVDiagramMapContext Context = GetContext(ContextId);
    FJCVDiagramMap& Map(Context->CopyMap(SrcMapId, DstMapId));

    FContextIdentifier& cid(ContextMap.FindChecked(ContextId));
    int32 aid = CreateAccessor(Map, ContextId, DstMapId);

    cid.AccessorMap.Emplace(DstMapId, aid);
}

UJCVDiagramAccessor* UJCVDiagramObject::GetAccessor(int32 ContextId, int32 MapId)
{
    if (HasMap(ContextId, MapId))
    {
        FContextIdentifier& cid( ContextMap.FindChecked(ContextId) );
        check(cid.AccessorMap.Contains(MapId));
        return Accessors[cid.AccessorMap.FindChecked(MapId)];
    }
    return nullptr;
}

int32 UJCVDiagramObject::CreateAccessor(FJCVDiagramMap& Map, int32 ContextId, int32 MapId)
{
    UJCVDiagramAccessor* Accessor = NewObject<UJCVDiagramAccessor>(this);
    Accessor->SetMap(Map, ContextId, MapId);

    int32 aid = -1;

    for (int32 i=0; i<Accessors.Num(); ++i)
    {
        if (! Accessors[i])
        {
            aid = i;
            break;
        }
    }

    if (aid < 0)
    {
        aid = Accessors.Emplace(Accessor);
    }
    else
    {
        Accessors[aid] = Accessor;
    }

    return aid;
}
