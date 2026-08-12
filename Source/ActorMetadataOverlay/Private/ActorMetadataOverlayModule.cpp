// Copyright (c) 2026 metyatech. All rights reserved.

#include "ActorMetadataOverlayModule.h"

#include "ActorMetadataOverlayController.h"

void FActorMetadataOverlayModule::StartupModule()
{
    Controller = MakeUnique<FActorMetadataOverlayController>();
    Controller->Initialize();
}

void FActorMetadataOverlayModule::ShutdownModule()
{
    if (Controller.IsValid())
    {
        Controller->Shutdown();
        Controller.Reset();
    }
}

IMPLEMENT_MODULE(FActorMetadataOverlayModule, ActorMetadataOverlay)
