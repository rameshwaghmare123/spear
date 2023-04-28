//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#pragma once

#include <Eigen/Dense>

#include <CoreMinimal.h>
#include <ChaosWheeledVehicleMovementComponent.h>

#include "OpenBotComponent.generated.h"

UCLASS()
class OPENBOT_API UOpenBotComponent : public UChaosWheeledVehicleMovementComponent
{
    GENERATED_BODY()
public:
    UOpenBotComponent();
    ~UOpenBotComponent();
    
    // Provides access to the wheels rotation speed in rad/s
    Eigen::Vector4f getWheelRotationSpeeds() const;
};
