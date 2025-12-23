#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "EditorActorTagDisplayActor.generated.h"

/**
 * Transient actor for displaying actor tags in the editor viewport.
 * This actor is automatically spawned and destroyed by the EditorActorTagDisplayModule.
 */
UCLASS(Transient, NotPlaceable)
class EDITORACTORTAGDISPLAY_API AEditorActorTagDisplayActor : public AActor
{
	// NOLINTNEXTLINE
	GENERATED_BODY()

public:
	AEditorActorTagDisplayActor();

	/** Gets the text render component for displaying tags. */
	auto GetTextRenderComponent() const -> UTextRenderComponent* { return TextRenderComponent; }

private:
	/** Text render component for displaying actor tags. */
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> TextRenderComponent;
};
