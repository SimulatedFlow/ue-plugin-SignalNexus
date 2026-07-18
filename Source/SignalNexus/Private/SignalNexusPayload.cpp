// Copyright 2026 Simulated Flow All Rights Reserved.

#include "SignalNexusPayload.h"
#include "SignalNexusLog.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

FSignalNexusPayload::~FSignalNexusPayload()
{
	Free();
}

FSignalNexusPayload::FSignalNexusPayload(const FSignalNexusPayload& Other)
{
	CopyFrom(Other);
}

FSignalNexusPayload& FSignalNexusPayload::operator=(const FSignalNexusPayload& Other)
{
	if (this != &Other)
	{
		Free();
		CopyFrom(Other);
	}
	return *this;
}

FSignalNexusPayload::FSignalNexusPayload(FSignalNexusPayload&& Other) noexcept
	: StructType(Other.StructType)
	, Memory(MoveTemp(Other.Memory))
{
	Other.StructType = nullptr;
}

FSignalNexusPayload& FSignalNexusPayload::operator=(FSignalNexusPayload&& Other) noexcept
{
	if (this != &Other)
	{
		Free();
		StructType = Other.StructType;
		Memory = MoveTemp(Other.Memory);
		Other.StructType = nullptr;
	}
	return *this;
}

void FSignalNexusPayload::PackRaw(const UScriptStruct* InStruct, const void* InData)
{
	Free();

	if (!InStruct || !InData)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("PackRaw called with a null struct or data pointer — payload left empty."));
		return;
	}

	StructType = const_cast<UScriptStruct*>(InStruct);

	const int32 StructureSize = InStruct->GetStructureSize();
	Memory.SetNumUninitialized(StructureSize);
	InStruct->InitializeStruct(Memory.GetData());
	InStruct->CopyScriptStruct(Memory.GetData(), InData);
}

bool FSignalNexusPayload::UnpackRaw(const UScriptStruct* InStruct, void* OutData) const
{
	const UScriptStruct* Stored = StructType.Get();

	if (!Stored || Memory.Num() == 0)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("Unpack failed: payload is empty."));
		return false;
	}

	if (!InStruct || !OutData)
	{
		UE_LOG(LogSignalNexus, Warning, TEXT("Unpack failed: null destination struct or pointer."));
		return false;
	}

	if (Stored != InStruct)
	{
		UE_LOG(LogSignalNexus, Warning,
			TEXT("Unpack type mismatch: payload holds '%s' but was unpacked as '%s'. Ignored."),
			*Stored->GetName(), *InStruct->GetName());
		return false;
	}

	Stored->CopyScriptStruct(OutData, Memory.GetData());
	return true;
}

FString FSignalNexusPayload::GetStructName() const
{
	const UScriptStruct* Stored = StructType.Get();
	return Stored ? Stored->GetName() : TEXT("<empty>");
}

void FSignalNexusPayload::Reset()
{
	Free();
}

void FSignalNexusPayload::AddStructReferencedObjects(FReferenceCollector& Collector) const
{
	const UScriptStruct* Stored = StructType.Get();
	if (Stored && Memory.Num() > 0)
	{
		Collector.AddPropertyReferencesWithStructARO(Stored, const_cast<uint8*>(Memory.GetData()));
	}
}

void FSignalNexusPayload::Free()
{
	if (Memory.Num() > 0)
	{
		if (UScriptStruct* Stored = StructType.Get())
		{
			Stored->DestroyStruct(Memory.GetData());
		}
		Memory.Reset();
	}
	StructType = nullptr;
}

void FSignalNexusPayload::CopyFrom(const FSignalNexusPayload& Other)
{
	StructType = Other.StructType;

	const UScriptStruct* Stored = Other.StructType.Get();
	if (Stored && Other.Memory.Num() > 0)
	{
		Memory.SetNumUninitialized(Stored->GetStructureSize());
		Stored->InitializeStruct(Memory.GetData());
		Stored->CopyScriptStruct(Memory.GetData(), Other.Memory.GetData());
	}
}
