// Copyright 2026 Simulated Flow All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SignalNexusTypes.generated.h"

/**
 * Delivery / scheduling mode for a broadcast signal.
 */
UENUM(BlueprintType)
enum class ESignalNexusPriority : uint8
{
	/** Processed synchronously on the calling thread, right now. */
	Immediate,

	/** Queued and flushed on the next world tick (via the subsystem's tickable). */
	DeferredNextFrame,

	/** Rate-limited per channel: coalesces to at most one dispatch per configured interval. */
	Throttled
};

/**
 * Result an interceptor returns for a signal it inspected.
 */
UENUM(BlueprintType)
enum class EInterceptorResult : uint8
{
	/** Let the (possibly modified) signal continue to subscribers and further interceptors. */
	Pass,

	/** Drop the signal entirely — subscribers never see it. */
	Block
};

/**
 * Opaque handle identifying a single subscription. Use it to Unsubscribe later.
 * Invalid (Id == 0) means "no subscription".
 */
USTRUCT(BlueprintType)
struct FSignalNexusHandle
{
	GENERATED_BODY()

	FSignalNexusHandle() = default;
	explicit FSignalNexusHandle(int64 InId) : Id(InId) {}

	bool IsValid() const { return Id != 0; }
	void Reset() { Id = 0; }
	int64 GetId() const { return Id; }

	bool operator==(const FSignalNexusHandle& Other) const { return Id == Other.Id; }
	bool operator!=(const FSignalNexusHandle& Other) const { return Id != Other.Id; }

	friend uint32 GetTypeHash(const FSignalNexusHandle& Handle) { return ::GetTypeHash(Handle.Id); }

private:
	UPROPERTY()
	int64 Id = 0;
};

// ---------------------------------------------------------------------------
// Ready-made payload structs. Users can broadcast any struct they like; these
// cover the most common cases so simple signals need zero boilerplate.
// ---------------------------------------------------------------------------

/** A single 32-bit integer payload. */
USTRUCT(BlueprintType)
struct FSignalIntPayload
{
	GENERATED_BODY()

	FSignalIntPayload() = default;
	explicit FSignalIntPayload(int32 InValue) : Value(InValue) {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	int32 Value = 0;
};

/** A single name payload (handy for string-ish keyed events). */
USTRUCT(BlueprintType)
struct FSignalNamePayload
{
	GENERATED_BODY()

	FSignalNamePayload() = default;
	explicit FSignalNamePayload(FName InValue) : Value(InValue) {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	FName Value;
};

/** A world-space location payload. */
USTRUCT(BlueprintType)
struct FSignalVectorPayload
{
	GENERATED_BODY()

	FSignalVectorPayload() = default;
	explicit FSignalVectorPayload(const FVector& InValue) : Value(InValue) {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	FVector Value = FVector::ZeroVector;
};

/** A small damage payload — also the canonical interceptor demo (reduce/redirect damage). */
USTRUCT(BlueprintType)
struct FSignalDamagePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	float Amount = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	FGameplayTag DamageType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SignalNexus")
	TWeakObjectPtr<UObject> Instigator;
};
