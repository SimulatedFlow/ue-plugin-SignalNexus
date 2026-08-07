// Copyright 2026 Simulated Flow All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "SignalNexusPayload.generated.h"

/**
 * FSignalNexusPayload — a generic, self-describing value container for a single UScriptStruct.
 *
 * It holds a *live, fully-constructed* copy of an arbitrary struct in a raw byte buffer, tagged
 * with the UScriptStruct that describes it. This lets a signal carry any C++ or Blueprint struct
 * without registering payload types up front, while staying type-safe: unpacking as the wrong
 * type fails cleanly (logged warning, no crash) instead of reinterpreting memory.
 *
 * Value semantics are correct (deep copy / proper destruction via the struct's ops), so the
 * payload can be copied, moved, queued and passed through Blueprint pins safely.
 */
USTRUCT(BlueprintType)
struct SIGNALNEXUS_API FSignalNexusPayload
{
	GENERATED_BODY()

public:
	FSignalNexusPayload() = default;
	~FSignalNexusPayload();

	FSignalNexusPayload(const FSignalNexusPayload& Other);
	FSignalNexusPayload& operator=(const FSignalNexusPayload& Other);
	FSignalNexusPayload(FSignalNexusPayload&& Other) noexcept;
	FSignalNexusPayload& operator=(FSignalNexusPayload&& Other) noexcept;

	/** Pack a typed C++ struct (must be a reflected USTRUCT with StaticStruct()). */
	template <typename T>
	void Pack(const T& InValue)
	{
		PackRaw(T::StaticStruct(), &InValue);
	}

	/**
	 * Unpack into a typed C++ struct. Returns false (and logs a warning) if the stored type does
	 * not exactly match T — the destination is left untouched. OutValue must be a valid instance.
	 */
	template <typename T>
	bool Unpack(T& OutValue) const
	{
		return UnpackRaw(T::StaticStruct(), &OutValue);
	}

	/** Pack from a reflected struct + a pointer to a live instance of it (used by Blueprint thunks). */
	void PackRaw(const UScriptStruct* InStruct, const void* InData);

	/** Copy the stored value into OutData if (and only if) InStruct matches the stored type. */
	bool UnpackRaw(const UScriptStruct* InStruct, void* OutData) const;

	/** True when this payload holds a valid, non-empty value. */
	bool IsValid() const { return StructType.IsValid() && Memory.Num() > 0; }

	/** The struct describing the stored value, or nullptr when empty. */
	const UScriptStruct* GetStructType() const { return StructType.Get(); }

	/** Human-readable type name for logging / debug UI. */
	FString GetStructName() const;

	/** Non-mutating type check against a C++ struct type. */
	template <typename T>
	bool IsA() const
	{
		return StructType.Get() == T::StaticStruct();
	}

	/** Release any stored value and reset to empty. */
	void Reset();

	/** GC hook: keep any UObject references living inside the stored struct alive. */
	void AddStructReferencedObjects(FReferenceCollector& Collector) const;

private:
	void Free();
	void CopyFrom(const FSignalNexusPayload& Other);

	/** The reflected type of the stored value (weak — UScriptStructs are effectively permanent). */
	TWeakObjectPtr<UScriptStruct> StructType;

	/** A live, constructed instance of StructType laid out in raw bytes. */
	TArray<uint8> Memory;
};

template <>
struct TStructOpsTypeTraits<FSignalNexusPayload> : public TStructOpsTypeTraitsBase2<FSignalNexusPayload>
{
	enum
	{
		WithCopy = true,
		WithDestructor = true,
		WithAddStructReferencedObjects = true,
	};
};
