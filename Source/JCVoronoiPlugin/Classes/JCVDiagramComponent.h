// 

#pragma once

#include "Components/ActorComponent.h"
#include "JCVIsland.h"
#include "JCVIslandManager.h"
#include "JCVDiagramAccessor.h"
#include "JCVDiagramComponent.generated.h"

UCLASS(BlueprintType, Blueprintable)
class JCVORONOIPLUGIN_API UJCVDiagramComponent : public UActorComponent
{
	GENERATED_BODY()

    struct FContextIdentifier
    {
        TPWJCVIslandContext Context;
        TMap<int32, int32> AccessorMap;

        FContextIdentifier() = default;
        FContextIdentifier(TPWJCVIslandContext c) : Context(c) {};

        FORCEINLINE bool IsValid() const
        {
            return Context.IsValid();
        }

        FORCEINLINE TPSJCVIslandContext Pin()
        {
            return Context.Pin();
        }

        FORCEINLINE TPSJCVIslandContext Pin() const
        {
            return Context.Pin();
        }
    };

    TMap<int32, FContextIdentifier> ContextMap;

	UPROPERTY(Transient)
    TArray<UJCVDiagramAccessor*> Accessors;

    FORCEINLINE bool _HasContext(int32 ContextID) const
    {
        return ContextMap.Contains(ContextID) && ContextMap.FindChecked(ContextID).IsValid();
    }

    FORCEINLINE bool _HasIsland(int32 ContextID, int32 IslandID) const
    {
        return _HasContext(ContextID) ? GetContext(ContextID)->HasIsland(IslandID) : false;
    }

    FORCEINLINE TPSJCVIslandContext GetContext(int32 ContextID)
    {
        return ContextMap.FindChecked(ContextID).Pin();
    }

    FORCEINLINE TPSJCVIslandContext GetContext(int32 ContextID) const
    {
        return ContextMap.FindChecked(ContextID).Pin();
    }

    FORCEINLINE FJCVIsland& GetIsland(int32 ContextID, int32 IslandID)
    {
        return GetContext(ContextID)->GetIsland(IslandID);
    }

    FORCEINLINE FJCVIsland& GetIsland(int32 ContextID, int32 IslandID) const
    {
        return GetContext(ContextID)->GetIsland(IslandID);
    }

    int32 CreateAccessor(FJCVIsland& Island)
    {
        UJCVDiagramAccessor* accessor = NewObject<UJCVDiagramAccessor>(this);
        accessor->SetIsland(Island);

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

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
    {
        // Make sure diagram context and its islands are destroyed
        if (FJCVIslandManager::HasValidMI())
        {
            if (FJCVIslandManager::MI()->HasID(this))
            {
                FJCVIslandManager::MI()->RemoveID(this);
            }
        }

        ContextMap.Empty();
        Accessors.Empty();

        Super::EndPlay(EndPlayReason);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool HasContext(int32 ContextID) const
    {
        return _HasContext(ContextID);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    bool HasIsland(int32 ContextID, int32 IslandID)
    {
        return _HasIsland(ContextID, IslandID);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateContext(int32 ContextID, FVector2D Size, TArray<FVector2D> Points)
    {
        if (! FJCVIslandManager::HasValidMI())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::CreateContext() ABORTED, NO VALID ISLAND MANAGER INSTANCE"));
            return;
        }

        if (Points.Num() <= 0)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::CreateContext() ABORTED, UNABLE TO GENERATE ISLAND WITH EMPTY POINTS"));
            return;
        }

        if (! _HasContext(ContextID))
        {
            TPSJCVIslandContext Context = MakeShareable(new FJCVIslandContext(Size, Points));
            ContextMap.Emplace(ContextID, Context);
            FJCVIslandManager::MI()->AddInstance(this, ContextID, Context);
        }
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    void CreateIsland(int32 ContextID, int32 IslandID)
    {
        if (! _HasContext(ContextID))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVDiagramComponent::CreateIsland() ABORTED, INVALID ISLAND CONTEXT"));
            return;
        }

        TPSJCVIslandContext context = GetContext(ContextID);
        FJCVIsland& island( context->CreateIsland(IslandID, true) );

        FContextIdentifier& cid( ContextMap.FindChecked(ContextID) );
        int32 aid = CreateAccessor(island);

        cid.AccessorMap.Emplace(IslandID, aid);
    }

    UFUNCTION(BlueprintCallable, Category="JCV")
    UJCVDiagramAccessor* GetAccessor(int32 ContextID, int32 IslandID)
    {
        if (_HasIsland(ContextID, IslandID))
        {
            FContextIdentifier& cid( ContextMap.FindChecked(ContextID) );
            check(cid.AccessorMap.Contains(IslandID));
            return Accessors[cid.AccessorMap.FindChecked(IslandID)];
        }
        return nullptr;
    }
};

