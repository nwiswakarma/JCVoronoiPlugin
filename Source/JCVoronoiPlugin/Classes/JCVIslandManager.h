// 

#pragma once

#include "CoreUObject.h"
#include "Containers/Map.h"
#include "Components/ActorComponent.h"
#include "SharedPointer.h"
#include "JCVTypes.h"
#include "JCVIslandManager.generated.h"

typedef TSharedPtr<class FJCVIslandManager> TPSJCVIslandManager;
typedef TWeakPtr<class FJCVIslandManager>   TPWJCVIslandManager;

USTRUCT()
struct JCVORONOIPLUGIN_API FJCVIslandManagerInstance
{
	GENERATED_BODY()

private:
    friend class FJCVIslandManager;
    static TPWJCVIslandManager _MI;
};

class FJCVIslandManager
{

    typedef const void* TInstanceID;

public:

    ~FJCVIslandManager()
    {
        InstanceMap.Empty();
    }

    FORCEINLINE static TPSJCVIslandManager MI()
    {
        return FJCVIslandManagerInstance::_MI.Pin();
    }

    FORCEINLINE static bool HasValidMI()
    {
        return FJCVIslandManagerInstance::_MI.IsValid();
    }

    static TPSJCVIslandManager CreateInstance()
    {
        if (HasValidMI())
        {
            return MI();
        }
        else
        {
            TPSJCVIslandManager mi( MakeShareable( new FJCVIslandManager() ) );
            FJCVIslandManagerInstance::_MI = mi;
            return mi;
        }
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

    FORCEINLINE bool HasInstance(TInstanceID ID, int32 Index = 0) const
    {
        return HasID(ID) ? InstanceMap.FindChecked(ID).Contains(Index) : false;
    }

    FORCEINLINE void AddInstance(TInstanceID ID, int32 Index, TPSJCVIslandContext Instance)
    {
        if (! HasID(ID))
        {
            InstanceMap.Emplace( ID, FInstanceSlot() );
        }
        InstanceMap.FindChecked(ID).Emplace(Index, Instance);
    }

    FORCEINLINE void AddInstance(TInstanceID ID, TPSJCVIslandContext Instance)
    {
        AddInstance(ID, 0, Instance);
    }

    FORCEINLINE TPSJCVIslandContext GetShared(TInstanceID ID, int32 Index)
    {
        return HasInstance(ID, Index)
            ? InstanceMap.FindChecked(ID).FindChecked(Index)
            : TPSJCVIslandContext();
    }

    FORCEINLINE FJCVIslandContext& GetRef(TInstanceID ID, int32 Index)
    {
        return *InstanceMap.FindChecked(ID).FindChecked(Index).Get();
    }

private:

    typedef TMap<int32, TPSJCVIslandContext> FInstanceSlot;
    TMap<TInstanceID, FInstanceSlot> InstanceMap;

    FJCVIslandManager()
    {
    }

};

UCLASS(BlueprintType, Blueprintable)
class JCVORONOIPLUGIN_API UJCVIslandManager : public UObject
{
	GENERATED_BODY()

	virtual void PostInitProperties() override
    {
        Super::PostInitProperties();

        // Do not create manager instance if object is a default subobject
        if (IsDefaultSubobject())
        {
            return;
        }

        if (! ManagerInstance.IsValid())
        {
            ManagerInstance = FJCVIslandManager::CreateInstance();
        }

        // Ensure the manager instance is a singleton
        check(ManagerInstance == FJCVIslandManager::MI());
    }

	virtual void BeginDestroy() override
    {
        if (ManagerInstance.IsValid())
            ManagerInstance.Reset();
        Super::BeginDestroy();
    }

private:

    TPSJCVIslandManager ManagerInstance;

};

UCLASS(BlueprintType, Blueprintable)
class JCVORONOIPLUGIN_API UJCVIslandManagerComponent : public UActorComponent
{
	GENERATED_BODY()

	virtual void BeginPlay() override
    {
        Super::BeginPlay();

        if (! ManagerInstance.IsValid())
        {
            ManagerInstance = FJCVIslandManager::CreateInstance();
        }

        // Ensure the manager instance is a singleton
        check(ManagerInstance == FJCVIslandManager::MI());
    }

	virtual void BeginDestroy() override
    {
        if (ManagerInstance.IsValid())
        {
            ManagerInstance.Reset();
        }
        Super::BeginDestroy();
    }

private:

    TPSJCVIslandManager ManagerInstance;

};

