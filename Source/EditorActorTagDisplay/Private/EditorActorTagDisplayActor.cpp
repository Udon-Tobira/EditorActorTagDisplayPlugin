#include "EditorActorTagDisplayActor.h"
#include "Components/TextRenderComponent.h"

AEditorActorTagDisplayActor::AEditorActorTagDisplayActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create text render component
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));
	RootComponent = TextRenderComponent;

	// Set as transient to prevent saving
	SetFlags(RF_Transient);
	
	// Configure for editor display
	SetActorHiddenInGame(false); // Show in PIE
	SetActorEnableCollision(false); // Disable collision
}
