//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#pragma once

#include <CoreMinimal.h>
#include <ChaosVehicleWheel.h>
#include <Components/SceneComponent.h>

#include "OpenBotWheel.generated.h"

UCLASS()
class OPENBOT_API UOpenBotWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UOpenBotWheel();
    ~UOpenBotWheel();
};
