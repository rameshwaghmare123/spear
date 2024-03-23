//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "SpearSim/SpPlayerController.h"

#include "SpCore/Log.h"
#include "SpCore/Unreal.h"
#include "SpCore/UserInputComponent.h"

ASpPlayerController::ASpPlayerController()
{
    SP_LOG_CURRENT_FUNCTION();

    // Need to set this to true to enable moving the camera when the game is paused.
    bShouldPerformFullTickWhenPaused = true;

    // Need to set this to true so the mouse cursor doesn't disappear when hovering over the game viewport in standalone mode.
    // However, setting this to true makes rendering ever-so-slightly choppier during click-and-drag events, but we think it
    // is worth it.
    bShowMouseCursor = true;

    // UUserInputComponent
    UserInputComponent = Unreal::createComponentInsideOwnerConstructor<UUserInputComponent>(this, GetRootComponent(), "user_input");
    SP_ASSERT(UserInputComponent);

    // UserInputComponents need to be enabled explicitly.
    UserInputComponent->bHandleUserInput = true;
}

ASpPlayerController::~ASpPlayerController()
{
    SP_LOG_CURRENT_FUNCTION();

    SP_ASSERT(UserInputComponent);
    UserInputComponent = nullptr;
}

void ASpPlayerController::BeginPlay()
{
    APlayerController::BeginPlay();

    // Need to set this to true to avoid blurry visual artifacts in the editor when the game is paused.
    GetWorld()->bIsCameraMoveableWhenPaused = true;

    UserInputComponent->subscribeToUserInputs({"Escape"});
    UserInputComponent->setHandleUserInputFunc([](const std::string& key, float axis_value) -> void {
        bool force = false;
        FGenericPlatformMisc::RequestExit(force);
    });
}