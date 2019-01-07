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
#include "JCVDiagramMap.h"
#include "JCVDiagramAccessor.h"
#include "JCVDiagramObject.generated.h"

UCLASS(BlueprintType, Blueprintable)
class JCVORONOIPLUGIN_API UJCVDiagramObject : public UObject
{
	GENERATED_BODY()

    struct FContextIdentifier
    {
        FPSJCVDiagramMapContext Context;
        TMap<int32, int32> AccessorMap;

        FContextIdentifier() = default;
        FContextIdentifier(FPSJCVDiagramMapContext c) : Context(c) {};

        FORCEINLINE bool IsValid() const
        {
            return Context.IsValid();
        }

        FORCEINLINE FPSJCVDiagramMapContext Get()
        {
            return Context;
        }

        FORCEINLINE FPSJCVDiagramMapContext Get() const
        {
            return Context;
        }
    };

    TMap<int32, FContextIdentifier> ContextMap;

	UPROPERTY(Transient)
    TArray<UJCVDiagramAccessor*> Accessors;

    FORCEINLINE bool HasContext_Direct(int32 ContextID) const
    {
        return ContextMap.Contains(ContextID) && ContextMap.FindChecked(ContextID).IsValid();
    }

    FORCEINLINE bool HasMap_Direct(int32 ContextID, int32 MapID) const
    {
        return HasContext_Direct(ContextID) ? GetContext(ContextID)->HasMap(MapID) : false;
    }

    FORCEINLINE FPSJCVDiagramMapContext GetContext(int32 ContextID)
    {
        return ContextMap.FindChecked(ContextID).Get();
    }

    FORCEINLINE FPSJCVDiagramMapContext GetContext(int32 ContextID) const
    {
        return ContextMap.FindChecked(ContextID).Get();
    }

    FORCEINLINE FJCVDiagramMap& GetMap(int32 ContextID, int32 MapID)
    {
        return GetContext(ContextID)->GetMap(MapID);
    }

    FORCEINLINE FJCVDiagramMap& GetMap(int32 ContextID, int32 MapID) const
    {
        return GetContext(ContextID)->GetMap(MapID);
    }

    int32 CreateAccessor(FJCVDiagramMap& Map)
    {
        UJCVDiagramAccessor* accessor = NewObject<UJCVDiagramAccessor>(this);
        accessor->SetMap(Map);

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
            aid = Accessors.Emplace(accessor);
        }
        else
        {
            Accessors[aid] = accessor;
        }

        return aid;
    }

public:

    virtual void BeginDestroy() override
    {
        // Clear registered diagram references
        ResetDiagramObject();

        Super::BeginDestroy();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool HasContext(int32 ContextID) const
    {
        return HasContext_Direct(ContextID);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool HasMap(int32 ContextID, int32 MapID)
    {
        return HasMap_Direct(ContextID, MapID);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ResetDiagramObject()
    {
        ContextMap.Empty();
        Accessors.Empty();
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateContext(int32 ContextID, FVector2D Size, TArray<FVector2D> Points)
    {
        if (Points.Num() <= 0)
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateContext() ABORTED, UNABLE TO GENERATE ISLAND WITH EMPTY POINTS"));
            return;
        }

        if (! HasContext_Direct(ContextID))
        {
            FPSJCVDiagramMapContext Context(new FJCVDiagramMapContext(Size, Points));
            ContextMap.Emplace(ContextID, Context);
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateMap(int32 ContextID, int32 MapID)
    {
        if (HasContext_Direct(ContextID))
        {
            CreateMapWithDefaultType(ContextID, MapID, EJCVCellFeature::UNDEFINED, 0);
        }
        else
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateMap() ABORTED, INVALID ISLAND CONTEXT"));
            return;
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateMapWithDefaultType(int32 ContextID, int32 MapID, uint8 FeatureType, int32 FeatureIndex)
    {
        if (! HasContext_Direct(ContextID))
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CreateMapWithDefaultType() ABORTED, INVALID ISLAND CONTEXT"));
            return;
        }

        FPSJCVDiagramMapContext Context = GetContext(ContextID);
        FJCVDiagramMap& Map(Context->CreateMap(MapID, FeatureType, FeatureIndex, true));

        FContextIdentifier& cid(ContextMap.FindChecked(ContextID));
        int32 aid = CreateAccessor(Map);

        cid.AccessorMap.Emplace(MapID, aid);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CopyMap(int32 ContextID, int32 SrcMapID, int32 DstMapID)
    {
        if (! HasContext_Direct(ContextID))
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, INVALID ISLAND CONTEXT"));
            return;
        }

        if (! HasMap_Direct(ContextID, SrcMapID))
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, INVALID SOURCE ISLAND"));
            return;
        }

        if (SrcMapID == DstMapID)
        {
            UE_LOG(LogJCV,Warning, TEXT("UJCVDiagramObject::CopyMap() ABORTED, SOURCE AND DESTINATION ISLAND HAVE THE SAME ID"));
            return;
        }

        FPSJCVDiagramMapContext Context = GetContext(ContextID);
        FJCVDiagramMap& Map(Context->CopyMap(SrcMapID, DstMapID));

        FContextIdentifier& cid(ContextMap.FindChecked(ContextID));
        int32 aid = CreateAccessor(Map);

        cid.AccessorMap.Emplace(DstMapID, aid);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    UJCVDiagramAccessor* GetAccessor(int32 ContextID, int32 MapID)
    {
        if (HasMap_Direct(ContextID, MapID))
        {
            FContextIdentifier& cid( ContextMap.FindChecked(ContextID) );
            check(cid.AccessorMap.Contains(MapID));
            return Accessors[cid.AccessorMap.FindChecked(MapID)];
        }
        return nullptr;
    }
};
