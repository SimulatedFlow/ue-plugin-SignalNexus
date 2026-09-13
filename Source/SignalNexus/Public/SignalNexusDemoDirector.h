// Copyright 2026 Silvan Teufel All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SignalNexusRouter.h"
#include "SignalNexusDemoDirector.generated.h"

class UTextRenderComponent;
class USignalNexusLambdaInterceptor;

/**
 * Runs the shipped demo: one signal bus, three subscribers at three depths, and the four things
 * that make the bus worth having.
 *
 * A message bus is invisible by nature, so the demo counts. Three subscriber rails sit at
 * SignalNexusDemo, SignalNexusDemo.Player and SignalNexusDemo.Player.Spawns. A broadcast on the
 * leaf makes all three climb together - that is hierarchical delivery, and it is the headline.
 * Then a throttled channel takes thirty broadcasts a second and delivers four. Then a deferred
 * broadcast sits in a queue you can watch, until the tick flushes it. Then an interceptor starts
 * blocking, and the rails below it stop.
 *
 * It drives a real FSignalNexusRouter - the same UObject-free core the subsystem forwards to - so
 * the counters are the bus's own bookkeeping. It ticks in the editor viewport, because that is
 * where the store images are taken.
 */
UCLASS(meta = (DisplayName = "SignalNexus Demo Director"))
class SIGNALNEXUS_API ASignalNexusDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	ASignalNexusDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	/** One run of the script. Four phases of eight seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalNexus Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 32.0f;

	/** How often the throttled channel is allowed through, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalNexus Demo",
		meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float ThrottleRate = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalNexus Demo")
	bool bDrawDemo = true;

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SignalNexus Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** Sent, delivered, queued, blocked. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SignalNexus Demo")
	TObjectPtr<UTextRenderComponent> TallyText;

	/** The three subscriber rails, one line each. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SignalNexus Demo")
	TObjectPtr<UTextRenderComponent> RailText;

	virtual void BeginDestroy() override;

private:
	void StartCycle();
	void Sende(const FGameplayTag& Kanal, ESignalNexusPriority Wie, int32 Wert);
	FString PhaseCaption(float T) const;
	void DrawRails() const;

	/** Kept alive against the collector for as long as it is registered with the router. */
	UPROPERTY()
	TObjectPtr<USignalNexusLambdaInterceptor> Interceptor;

	/** The real routing core. Not a UObject, so the actor can simply own one. */
	FSignalNexusRouter Router;

	FGameplayTag TagWurzel;
	FGameplayTag TagSpieler;
	FGameplayTag TagSpawns;
	FGameplayTag TagTick;

	/** One counter per rail, in the order root / player / spawns. */
	int32 Empfangen[3] = {0, 0, 0};

	int32 Gesendet = 0;
	int32 Geblockt = 0;

	TArray<FSignalNexusHandle> Handles;

	float CycleTime = 0.0f;
	float SeitLetztem = 0.0f;
	bool bInterceptorAn = false;
	bool bAufgebaut = false;
	int32 Zaehler = 0;
};
