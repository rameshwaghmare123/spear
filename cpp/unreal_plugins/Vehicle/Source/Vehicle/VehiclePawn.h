//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#pragma once

#include <vector>

#include <CoreMinimal.h>
#include <WheeledVehiclePawn.h>

#include "VehiclePawn.generated.h"

class UBoxComponent;
class UCameraComponent;

UCLASS()
class VEHICLE_API AVehiclePawn : public AWheeledVehiclePawn
{
    GENERATED_BODY()
public:
    AVehiclePawn(const FObjectInitializer& object_initializer);
    ~AVehiclePawn();

    void BeginPlay();

    UFUNCTION()
    void OnActorHitHandler(AActor* self_actor, AActor* other_actor, FVector normal_impulse, const FHitResult& hit_result);

    void SetupPlayerInputComponent(UInputComponent* input_component) override;
    void Tick(float delta_time) override;

    UCameraComponent* camera_component_ = nullptr;
    UBoxComponent* imu_component_ = nullptr;
};
