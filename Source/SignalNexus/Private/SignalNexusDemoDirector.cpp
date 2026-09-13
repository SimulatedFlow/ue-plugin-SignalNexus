// Copyright 2026 Silvan Teufel All Rights Reserved.

#include "SignalNexusDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "SignalNexusInterceptor.h"
#include "SignalNexusLog.h"
#include "SignalNexusPayload.h"
#include "SignalNexusTypes.h"

/**
 * The demo channels, declared natively.
 *
 * The first attempt put them in Plugins/SignalNexus/Config/DefaultGameplayTags.ini - which is NOT a
 * file the tag manager reads. A plugin's tags have to live under Config/Tags/ or be declared in
 * code; an ini in the wrong place fails silently, every tag comes back invalid, and the demo simply
 * does not run. Native declaration cannot fail that way.
 *
 * Namespaced under SignalNexusDemo on purpose: the obvious names for a demo of an event bus are
 * Event.Player.Spawns and friends, and "Event" is exactly the root a buyer is most likely to own
 * already.
 */
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SignalNexusDemo, "SignalNexusDemo");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SignalNexusDemoPlayer, "SignalNexusDemo.Player");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SignalNexusDemoSpawns, "SignalNexusDemo.Player.Spawns");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SignalNexusDemoTick, "SignalNexusDemo.World.Tick");

namespace SignalNexusDemoLocal
{
	constexpr float PhaseLaenge = 8.0f;
}

ASignalNexusDemoDirector::ASignalNexusDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(40.0f);
	BoardText->SetTextRenderColor(FColor::White);

	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 740.0f));

	TallyText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TallyText"));
	TallyText->SetupAttachment(Root);
	TallyText->SetHorizontalAlignment(EHTA_Center);
	TallyText->SetVerticalAlignment(EVRTA_TextBottom);
	TallyText->SetWorldSize(36.0f);
	TallyText->SetTextRenderColor(FColor(120, 200, 255));
	TallyText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	TallyText->SetRelativeLocation(FVector(0.0f, 0.0f, 650.0f));

	RailText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RailText"));
	RailText->SetupAttachment(Root);
	RailText->SetHorizontalAlignment(EHTA_Center);
	RailText->SetVerticalAlignment(EVRTA_TextTop);
	RailText->SetWorldSize(30.0f);
	RailText->SetTextRenderColor(FColor(205, 215, 230));
	RailText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));

	// Offset on X, not Y. The camera looks along +Y, so Y is DEPTH and a component moved on it ends
	// up behind the wall.
	RailText->SetRelativeLocation(FVector(-560.0f, 0.0f, 540.0f));
}

void ASignalNexusDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void ASignalNexusDemoDirector::BeginDestroy()
{
	// The router holds weak pointers to the interceptor, but the subscriptions hold lambdas that
	// capture `this`. Tearing the router down here is what keeps a deleted director from being
	// called back during editor shutdown.
	Router.Reset();
	Super::BeginDestroy();
}

void ASignalNexusDemoDirector::StartCycle()
{
	CycleTime = 0.0f;
	SeitLetztem = 0.0f;
	Gesendet = 0;
	Geblockt = 0;
	Zaehler = 0;
	bInterceptorAn = false;
	Empfangen[0] = Empfangen[1] = Empfangen[2] = 0;

	TagWurzel = TAG_SignalNexusDemo;
	TagSpieler = TAG_SignalNexusDemoPlayer;
	TagSpawns = TAG_SignalNexusDemoSpawns;
	TagTick = TAG_SignalNexusDemoTick;

	Router.Reset();
	Handles.Reset();

	if (!TagWurzel.IsValid())
	{
		UE_LOG(LogSignalNexus, Warning,
			TEXT("SignalNexus demo: the demo channels are not registered; nothing to show."));
		bAufgebaut = false;
		return;
	}

	// One subscriber per level of the tree. This is the whole point of the first phase: a broadcast
	// on the leaf has to reach all three.
	const FGameplayTag Kanaele[3] = {TagWurzel, TagSpieler, TagSpawns};
	for (int32 i = 0; i < 3; ++i)
	{
		if (!Kanaele[i].IsValid())
		{
			continue;
		}
		const int32 Index = i;
		Handles.Add(Router.Subscribe(Kanaele[i],
			[this, Index](FGameplayTag, const FSignalNexusPayload&)
			{
				++Empfangen[Index];
			}, this));
	}

	Interceptor = NewObject<USignalNexusLambdaInterceptor>(this);
	Interceptor->InterceptFn = [this](FGameplayTag, FSignalNexusPayload&)
	{
		// Blocks every second signal rather than all of them. A gate that stops everything is
		// indistinguishable from a bus that has stopped working; one that stops half of them shows
		// that the other half still arrives.
		if ((++Zaehler % 2) == 0)
		{
			++Geblockt;
			return EInterceptorResult::Block;
		}
		return EInterceptorResult::Pass;
	};

	bAufgebaut = true;
}

void ASignalNexusDemoDirector::Sende(const FGameplayTag& Kanal, ESignalNexusPriority Wie, int32 Wert)
{
	if (!Kanal.IsValid())
	{
		return;
	}
	FSignalNexusPayload Nutzlast;
	Nutzlast.Pack(FSignalIntPayload(Wert));
	Router.Broadcast(Kanal, MoveTemp(Nutzlast), Wie, ThrottleRate);
	++Gesendet;
}

void ASignalNexusDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The editor hands out one enormous delta after a recompile.
	DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);

	if (!bAufgebaut)
	{
		StartCycle();
		if (!bAufgebaut)
		{
			return;
		}
	}

	CycleTime += DeltaSeconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}
	SeitLetztem += DeltaSeconds;

	const float T = CycleTime;
	const int32 Phase = FMath::Clamp(FMath::FloorToInt(T / SignalNexusDemoLocal::PhaseLaenge), 0, 3);

	switch (Phase)
	{
	case 0:
		// Hierarchical delivery. Two a second, so the three counters are readable as they climb.
		if (SeitLetztem > 0.5f)
		{
			SeitLetztem = 0.0f;
			Sende(TagSpawns, ESignalNexusPriority::Immediate, Empfangen[2] + 1);
		}
		break;

	case 1:
		// Throttling. Thirty a second in, one per interval out.
		if (SeitLetztem > 1.0f / 30.0f)
		{
			SeitLetztem = 0.0f;
			Sende(TagTick, ESignalNexusPriority::Throttled, 0);
		}
		break;

	case 2:
		// Deferred. Five at once, then a pause - so the queue has a depth worth looking at before
		// the tick flushes it.
		if (SeitLetztem > 0.8f)
		{
			SeitLetztem = 0.0f;
			for (int32 i = 0; i < 5; ++i)
			{
				Sende(TagSpawns, ESignalNexusPriority::DeferredNextFrame, i);
			}
		}
		break;

	default:
		if (!bInterceptorAn)
		{
			bInterceptorAn = true;
			Router.RegisterInterceptor(TagSpieler, Interceptor);
		}
		if (SeitLetztem > 0.4f)
		{
			SeitLetztem = 0.0f;
			Sende(TagSpawns, ESignalNexusPriority::Immediate, 0);
		}
		break;
	}

	// The bus's own clock. Without this the deferred queue never flushes and the throttle never
	// opens - both are scheduled work, and scheduled work needs somebody to turn the handle.
	const UWorld* World = GetWorld();
	Router.Tick(World ? World->GetTimeSeconds() : CycleTime);

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(PhaseCaption(T)));
	}
	if (TallyText)
	{
		TallyText->SetText(FText::FromString(FString::Printf(
			TEXT("sent %d   -   queued %d   -   blocked %d"),
			Gesendet, Router.NumPendingDeferred(), Geblockt)));
	}
	if (RailText)
	{
		RailText->SetText(FText::FromString(FString::Printf(
			TEXT("SignalNexusDemo                 %4d\n"
			     "SignalNexusDemo.Player          %4d\n"
			     "SignalNexusDemo.Player.Spawns   %4d"),
			Empfangen[0], Empfangen[1], Empfangen[2])));
	}

	if (bDrawDemo)
	{
		DrawRails();
	}
}

FString ASignalNexusDemoDirector::PhaseCaption(float T) const
{
	const int32 Phase = FMath::Clamp(FMath::FloorToInt(T / SignalNexusDemoLocal::PhaseLaenge), 0, 3);
	switch (Phase)
	{
	case 0:  return TEXT("broadcast on the LEAF - all three levels of the tree receive it");
	case 1:  return FString::Printf(
		TEXT("throttled channel - thirty a second in, one per %.2f s out"), ThrottleRate);
	case 2:  return TEXT("deferred - the queue fills, and the tick empties it");
	default: return TEXT("an interceptor on .Player blocks every second signal");
	}
}

void ASignalNexusDemoDirector::DrawRails() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// 0.35 s, not one frame: during a screenshot run the editor renders many frames between two
	// world ticks, and anything shorter is gone by the time the picture is taken.
	constexpr float Life = 0.35f;
	constexpr float Breite = 620.0f;
	constexpr float Schritt = 110.0f;

	// The rails are scaled against the busiest one, so the picture is about the RATIO between the
	// three and not about how long the demo has been running.
	const int32 Hoechster = FMath::Max3(Empfangen[0], FMath::Max(Empfangen[1], Empfangen[2]), 1);

	const FColor Farben[3] = {FColor(120, 200, 255), FColor(150, 225, 190), FColor(240, 200, 120)};

	// Rails grow along +X. The camera looks along +Y, so a rail drawn on Y points away from it and
	// collapses into a dot.
	const FVector Ursprung = GetActorLocation() + FVector(80.0f, 0.0f, 460.0f);

	for (int32 i = 0; i < 3; ++i)
	{
		// Each level is indented, so the tree reads as a tree and not as three unrelated bars.
		const FVector Zeile = Ursprung + FVector(i * 55.0f, 0.0f, -i * Schritt);
		const float Anteil = static_cast<float>(Empfangen[i]) / Hoechster;

		DrawDebugLine(Mutable, Zeile, Zeile + FVector(Breite, 0.0f, 0.0f),
			FColor(56, 58, 66), false, Life, 0, 4.0f);
		for (int32 Ply = 0; Ply < 4; ++Ply)
		{
			const FVector Hoch(0.0f, 0.0f, Ply * 11.0f);
			DrawDebugLine(Mutable, Zeile + Hoch, Zeile + Hoch + FVector(Breite * Anteil, 0.0f, 0.0f),
				Farben[i], false, Life, 0, 9.0f);
		}

		// The connector back up to the parent level.
		if (i > 0)
		{
			const FVector Eltern = Ursprung + FVector((i - 1) * 55.0f, 0.0f, -(i - 1) * Schritt);
			DrawDebugLine(Mutable, FVector(Eltern.X, Eltern.Y, Zeile.Z), Zeile,
				FColor(110, 115, 130), false, Life, 0, 3.0f);
			DrawDebugLine(Mutable, FVector(Eltern.X, Eltern.Y, Zeile.Z), Eltern,
				FColor(110, 115, 130), false, Life, 0, 3.0f);
		}
	}

	// The queue, as a stack of blocks beside the rails - a number nobody watches, made into a shape.
	const int32 Warteschlange = FMath::Min(Router.NumPendingDeferred(), 12);
	for (int32 i = 0; i < Warteschlange; ++i)
	{
		const FVector Block = Ursprung + FVector(Breite + 160.0f, 0.0f, 30.0f + i * 26.0f);
		DrawDebugLine(Mutable, Block, Block + FVector(80.0f, 0.0f, 0.0f),
			FColor(250, 195, 90), false, Life, 0, 12.0f);
	}
#endif
}
