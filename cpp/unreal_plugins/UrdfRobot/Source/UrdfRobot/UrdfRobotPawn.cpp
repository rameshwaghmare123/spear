//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "UrdfRobot/UrdfRobotPawn.h"

#include <map>
#include <string>
#include <vector>

#include <Camera/CameraComponent.h>
#include <Components/InputComponent.h>
#include <CoreMinimal.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerInput.h>
#include <Math/Rotator.h>
#include <Math/Vector.h>

#include "CoreUtils/Assert.h"
#include "CoreUtils/Config.h"
#include "CoreUtils/Log.h"
#include "CoreUtils/PlayerInputComponent.h"
#include "CoreUtils/Std.h"
#include "CoreUtils/Unreal.h"
// -----------------------------------------------------
#include "UrdfRobot/UrdfLinkComponent.h"
// -----------------------------------------------------

#include "UrdfRobot/UrdfParser.h"
#include "UrdfRobot/UrdfRobotComponent.h"


const auto DEFAULT_URDF_FILE = std::filesystem::path() / ".." / ".." / ".." / "python" / "spear" / "urdf" / "pendulum.urdf";


AUrdfRobotPawn::AUrdfRobotPawn()
{
    SP_LOG_CURRENT_FUNCTION();
}

AUrdfRobotPawn::~AUrdfRobotPawn()
{
    SP_LOG_CURRENT_FUNCTION();

    // Pawns don't need to be cleaned up explicitly.

    UrdfFile = Unreal::toFString("");
    UrdfRobotComponent = nullptr;
    CameraComponent = nullptr;
}

void AUrdfRobotPawn::Initialize()
{
    // UUrdfRobotComponent
    std::filesystem::path urdf_file;
    if (Config::s_initialized_) {
        urdf_file =
            std::filesystem::path() /
            Config::get<std::string>("URDF_ROBOT.URDF_ROBOT_PAWN.URDF_DIR") /
            Config::get<std::string>("URDF_ROBOT.URDF_ROBOT_PAWN.URDF_FILE");
    } else if (std::filesystem::exists(Unreal::toStdString(UrdfFile))) {
        urdf_file = Unreal::toStdString(UrdfFile);
    } else if (WITH_EDITOR) {
        urdf_file = std::filesystem::canonical(Unreal::toStdString(FPaths::ProjectDir()) / DEFAULT_URDF_FILE);
    }
    SP_ASSERT(std::filesystem::exists(urdf_file));

    UrdfRobotDesc robot_desc = UrdfParser::parse(urdf_file.string());
    SP_ASSERT(!Std::containsSubstring(robot_desc.name_, "."));
    UrdfRobotComponent = NewObject<UUrdfRobotComponent>(this, Unreal::toFName(robot_desc.name_));
    SP_ASSERT(UrdfRobotComponent);
    UrdfRobotComponent->initialize(&robot_desc);
    UrdfRobotComponent->RegisterComponent();

    SetRootComponent(UrdfRobotComponent);

// -----------------------------------------------------
    // Assign custom stencil depth value to just the link components for demo purpose
    for (auto& link_component : UrdfRobotComponent->LinkComponents) {
        for (auto& sm_component : link_component->StaticMeshComponents) {
            sm_component->SetRenderCustomDepth(true);
            sm_component->SetCustomDepthStencilValue(250);
        }
    }
// -----------------------------------------------------

    // UCameraComponent
    FVector camera_location;
    FRotator camera_rotation;
    float field_of_view;
    float aspect_ratio;
    if (Config::s_initialized_) {
        camera_location = {
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.LOCATION_X"),
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.LOCATION_Y"),
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.LOCATION_Z")};
        camera_rotation = {
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.ROTATION_PITCH"),
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.ROTATION_YAW"),
            Config::get<double>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.ROTATION_ROLL")};
        field_of_view = Config::get<float>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.FOV");
        aspect_ratio = Config::get<float>("URDF_ROBOT.URDF_ROBOT_PAWN.CAMERA_COMPONENT.ASPECT_RATIO");
    } else {
// -----------------------------------------------------
        camera_location = FVector(-70, 70, 140);// a bit further thirdperson view // FVector(-45, 0, 142); //a close up third-personview
        camera_rotation = FRotator(-20, 330, 0);// a bit further thirdperson view // FRotator(-30, 0, 0);
        //camera_location = FVector::ZeroVector;
        //camera_rotation = FRotator::ZeroRotator;
// -----------------------------------------------------
        field_of_view = 90.0;
    }

    CameraComponent = NewObject<UCameraComponent>(this, Unreal::toFName("camera_component"));
    SP_ASSERT(CameraComponent);
    CameraComponent->SetRelativeLocationAndRotation(camera_location, camera_rotation);
    CameraComponent->SetupAttachment(UrdfRobotComponent);
    CameraComponent->bUsePawnControlRotation = false;
    CameraComponent->FieldOfView = field_of_view;
    //CameraComponent->AspectRatio = aspect_ratio;
    CameraComponent->RegisterComponent();
}

void AUrdfRobotPawn::SetupPlayerInputComponent(UInputComponent* input_component)
{
    APawn::SetupPlayerInputComponent(input_component);

    SP_ASSERT(input_component);

    UPlayerInput* player_input = GetWorld()->GetFirstPlayerController()->PlayerInput;
    SP_ASSERT(player_input);
    player_input->AddAxisMapping(FInputAxisKeyMapping(Unreal::toFName("Exit"), FKey(Unreal::toFName("Escape")), 1.0f));
    input_component->BindAxis(Unreal::toFName("Exit"));

    // Forward input_component to all of our UPlayerInputComponents so they can add their own input bindings.

    // TODO (MR): move this functionality into a findComponents(...) function in Unreal.h
    std::vector<AActor*> actors = Unreal::findActors(GetWorld());
    for (auto actor : actors) {
        TArray<UActorComponent*> components;
        actor->GetComponents(UPlayerInputComponent::StaticClass(), components);
        for (auto component : components) {
            auto player_input_component = dynamic_cast<UPlayerInputComponent*>(component);
            SP_ASSERT(player_input_component);
            player_input_component->input_component_ = input_component;
        }
    }

    // bind a UFUNCTION to a keypress
    player_input->AddActionMapping(FInputActionKeyMapping(Unreal::toFName("ChangeCameraComponent"), FKey("Gamepad_LeftThumbstick")));
    input_component->BindAction("ChangeCameraComponent", IE_Pressed, this, &AUrdfRobotPawn::ChangeCameraComponent);
}

void AUrdfRobotPawn::ChangeCameraComponent()
{
    SP_LOG_CURRENT_FUNCTION();

    static int camera_index = 0;

    TArray<USceneComponent*> scene_components = UrdfRobotComponent->GetAttachChildren();

    for (int i = 0; i < scene_components.Num(); i++) {
        UCameraComponent* camera_component = dynamic_cast<UCameraComponent*>(scene_components[i]);
        if (i == camera_index) {
            if (camera_component) {
                camera_component->Activate(true);
            }
        } else {
            if (camera_component) {
                camera_component->Deactivate();
            }
        }
    }

    camera_index++;
    camera_index %= scene_components.Num();
}
