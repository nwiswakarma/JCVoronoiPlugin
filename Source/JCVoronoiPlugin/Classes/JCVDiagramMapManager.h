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
#include "Containers/Map.h"
#include "Components/ActorComponent.h"
#include "SharedPointer.h"
#include "JCVoronoiPlugin.h"
#include "JCVTypes.h"

typedef TSharedPtr<class FJCVDiagramMapManager> FPSJCVDiagramMapManager;
typedef TWeakPtr<class FJCVDiagramMapManager>   FPWJCVDiagramMapManager;

class FJCVDiagramMapManager
{
    typedef const void* TInstanceID;

public:

    FJCVDiagramMapManager() = default;

    ~FJCVDiagramMapManager()
    {
        UE_LOG(LogTemp,Warning, TEXT("~FJCVDiagramMapManager()"));
        InstanceMap.Empty();
    }

    FORCEINLINE bool HasID(TInstanceID ID) const
    {
        return ID != nullptr && InstanceMap.Contains(ID);
    }

    FORCEINLINE void RemoveID(TInstanceID ID)
    {
        if (HasID(ID))
        {
            InstanceMap.FindChecked(ID).Empty();
            InstanceMap.Remove(ID);
        }
    }

    FORCEINLINE bool HasContext(TInstanceID ID, int32 Index = 0) const
    {
        return HasID(ID) ? InstanceMap.FindChecked(ID).Contains(Index) : false;
    }

    FORCEINLINE void AddContext(TInstanceID ID, int32 Index, FPSJCVDiagramMapContext Instance)
    {
        if (! HasID(ID))
        {
            InstanceMap.Emplace( ID, FInstanceSlot() );
        }
        InstanceMap.FindChecked(ID).Emplace(Index, Instance);
    }

    FORCEINLINE void AddContext(TInstanceID ID, FPSJCVDiagramMapContext Instance)
    {
        AddContext(ID, 0, Instance);
    }

    FORCEINLINE FPSJCVDiagramMapContext GetContextShared(TInstanceID ID, int32 Index)
    {
        return HasContext(ID, Index)
            ? InstanceMap.FindChecked(ID).FindChecked(Index)
            : FPSJCVDiagramMapContext();
    }

    FORCEINLINE FJCVDiagramMapContext& GetContextRef(TInstanceID ID, int32 Index)
    {
        return *InstanceMap.FindChecked(ID).FindChecked(Index).Get();
    }

private:

    typedef TMap<int32, FPSJCVDiagramMapContext> FInstanceSlot;
    TMap<TInstanceID, FInstanceSlot> InstanceMap;
};
