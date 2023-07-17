//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "Vehicle/VehiclePawn.h"

#include <string>
#include <vector>

#include <Animation/AnimInstance.h>
#include <Camera/CameraComponent.h>
#include <Components/BoxComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include <Engine/CollisionProfile.h>
#include <UObject/ConstructorHelpers.h>

#include "CoreUtils/Assert.h"
#include "CoreUtils/Config.h"
#include "CoreUtils/Log.h"
#include "CoreUtils/Unreal.h"
#include "Vehicle/VehicleMovementComponent.h"

// Calling the AWheeledVehiclePawn constructor in this way is necessary to override the UChaosWheeledVehicleMovementComponent
// class used by AWheeledVehiclePawn. See the following link for details:
//     https://docs.unrealengine.com/5.2/en-US/API/Plugins/ChaosVehicles/AWheeledVehiclePawn
AVehiclePawn::AVehiclePawn(const FObjectInitializer& object_initializer) :
    AWheeledVehiclePawn(object_initializer.SetDefaultSubobjectClass<UVehicleMovementComponent>(AWheeledVehiclePawn::VehicleMovementComponentName))
{
    SP_LOG_CURRENT_FUNCTION();

    if (!Config::s_initialized_) {
        return;
    }

    ConstructorHelpers::FObjectFinder<USkeletalMesh> skeletal_mesh(*Unreal::toFString(Config::get<std::string>("VEHICLE.VEHICLE_PAWN.SKELETAL_MESH")));
    SP_ASSERT(skeletal_mesh.Succeeded());

    ConstructorHelpers::FClassFinder<UAnimInstance> anim_instance(*Unreal::toFString(Config::get<std::string>("VEHICLE.VEHICLE_PAWN.ANIM_INSTANCE")));
    SP_ASSERT(anim_instance.Succeeded());

    GetMesh()->SetSkeletalMesh(skeletal_mesh.Object);
    GetMesh()->SetAnimClass(anim_instance.Class);
    GetMesh()->BodyInstance.bSimulatePhysics = true;

    //GetMesh()->SetSimulatePhysics(true); // did not work
    //GetMesh()->SetNotifyRigidBodyCollision(true); // did not work

    // Setup camera
    camera_component_ = CreateDefaultSubobject<UCameraComponent>(TEXT("camera_component"));
    SP_ASSERT(camera_component_);

    FVector camera_location(
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.LOCATION_X"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.LOCATION_Y"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.LOCATION_Z"));

    FRotator camera_rotation(
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.ROTATION_PITCH"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.ROTATION_YAW"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.ROTATION_ROLL"));

    camera_component_->SetRelativeLocationAndRotation(camera_location, camera_rotation);
    camera_component_->SetupAttachment(GetMesh());
    camera_component_->bUsePawnControlRotation = false;
    camera_component_->FieldOfView = Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.FOV");
    camera_component_->AspectRatio = Config::get<float>("VEHICLE.VEHICLE_PAWN.CAMERA_COMPONENT.ASPECT_RATIO");

    // Setup IMU sensor
    imu_component_ = CreateDefaultSubobject<UBoxComponent>(TEXT("imu_component"));
    ASSERT(imu_component_);

    FVector imu_location(
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.LOCATION_X"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.LOCATION_Y"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.LOCATION_Z"));

    FRotator imu_rotation(
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.ROTATION_PITCH"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.ROTATION_YAW"),
        Config::get<float>("VEHICLE.VEHICLE_PAWN.IMU_COMPONENT.ROTATION_ROLL"));

    imu_component_->SetRelativeLocationAndRotation(imu_location, imu_rotation);
    imu_component_->SetupAttachment(GetMesh());
}

void AVehiclePawn::SetupPlayerInputComponent(UInputComponent* input_component)
{
    SP_ASSERT(input_component);
    Super::SetupPlayerInputComponent(input_component);

    input_component->BindAxisKey(EKeys::I);
    input_component->BindAxisKey(EKeys::K);
    input_component->BindAxisKey(EKeys::J);
    input_component->BindAxisKey(EKeys::L);
}

void AVehiclePawn::Tick(float delta_time)
{
    Super::Tick(delta_time);

    constexpr float epsilon = 1e-2;

    float forward = InputComponent->GetAxisKeyValue(EKeys::I);
    float reverse = InputComponent->GetAxisKeyValue(EKeys::K);
    float right = InputComponent->GetAxisKeyValue(EKeys::L);
    float left = InputComponent->GetAxisKeyValue(EKeys::J);

    float fl = 0.0;
    float fr = 0.0;
    float rl = 0.0;
    float rr = 0.0;

    if (forward > epsilon) {
        fl += forward;
        fr += forward;
        rl += forward;
        rr += forward;
    }
    else if (reverse > epsilon) {
        fl -= reverse;
        fr -= reverse;
        rl -= reverse;
        rr -= reverse;
    }
    else if (right > epsilon) {
        fl += right;
        fr -= right;
        rl += right;
        rr -= right;
    }
    else if (left > epsilon) {
        fl -= left;
        fr += left;
        rl -= left;
        rr += left;
    }

    UVehicleMovementComponent* vehicle_movement_component = dynamic_cast<UVehicleMovementComponent*>(GetVehicleMovementComponent());
    SP_ASSERT(vehicle_movement_component);

    vehicle_movement_component->SetDriveTorque(fl, 0);
    vehicle_movement_component->SetDriveTorque(fr, 1);
    vehicle_movement_component->SetDriveTorque(rl, 2);
    vehicle_movement_component->SetDriveTorque(rr, 3);
}

void AVehiclePawn::BeginPlay()
{
    SP_LOG("Begin Play");
    Super::BeginPlay();

    this->OnActorHit.AddDynamic(this, &AVehiclePawn::OnActorHitHandler);
}

void AVehiclePawn::OnActorHitHandler(AActor* self_actor, AActor* other_actor, FVector normal_impulse, const FHitResult& hit_result)
{
    SP_LOG("On Actor Hit");
}

AVehiclePawn::~AVehiclePawn()
{
    SP_LOG_CURRENT_FUNCTION();
}
