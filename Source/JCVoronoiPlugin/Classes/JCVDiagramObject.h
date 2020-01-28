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

    FORCEINLINE bool HasContext(int32 ContextId) const
    {
        return ContextMap.Contains(ContextId) && ContextMap.FindChecked(ContextId).IsValid();
    }

    FORCEINLINE bool HasMap(int32 ContextId, int32 MapID) const
    {
        return HasContext(ContextId) ? GetContext(ContextId)->HasMap(MapID) : false;
    }

    FORCEINLINE FPSJCVDiagramMapContext GetContext(int32 ContextId)
    {
        return ContextMap.FindChecked(ContextId).Get();
    }

    FORCEINLINE FPSJCVDiagramMapContext GetContext(int32 ContextId) const
    {
        return ContextMap.FindChecked(ContextId).Get();
    }

    FORCEINLINE FJCVDiagramMap& GetMap(int32 ContextId, int32 MapID)
    {
        return GetContext(ContextId)->GetMap(MapID);
    }

    FORCEINLINE FJCVDiagramMap& GetMap(int32 ContextId, int32 MapID) const
    {
        return GetContext(ContextId)->GetMap(MapID);
    }

    int32 CreateAccessor(FJCVDiagramMap& Map, int32 ContextId, int32 MapId);

public:

    virtual void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(DisplayName="Has Context"))
    bool K2_HasContext(int32 ContextId) const;

    UFUNCTION(BlueprintCallable, Category="JCV", meta=(DisplayName="Has Map"))
    bool K2_HasMap(int32 ContextId, int32 MapID) const;

    UFUNCTION(BlueprintCallable, Category="JCV")
    void ResetDiagramObject();

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateContext(int32 ContextId, FVector2D Size, TArray<FVector2D> Points);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateContextByBounds(int32 ContextId, FBox2D Bounds, TArray<FVector2D> Points);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateMap(int32 ContextId, int32 MapID);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateMapWithDefaultType(int32 ContextId, int32 MapID, uint8 FeatureType, int32 FeatureIndex);

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CopyMap(int32 ContextId, int32 SrcMapID, int32 DstMapID);

    UFUNCTION(BlueprintCallable, Category="JCV")
    UJCVDiagramAccessor* GetAccessor(int32 ContextId, int32 MapID);
};
