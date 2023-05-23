//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "OpenBot/ChaosRearWheel.h"

#include <iostream>

#include <ChaosVehicleWheel.h>

#include "CoreUtils/Config.h"

UChaosRearWheel::UChaosRearWheel()
{
    std::cout << "[SPEAR | OpenBotWheel.cpp] UChaosRearWheel::UChaosRearWheel" << std::endl;

    if (!Config::s_initialized_) {
        return;
    }

    AxleType = EAxleType::Rear;
    bAffectedByEngine   = true;
    bAffectedBySteering = true;

    MaxSteerAngle = Config::get<float>("OPENBOT.OPENBOT_WHEEL.STEER_ANGLE");
    WheelRadius   = Config::get<float>("OPENBOT.OPENBOT_WHEEL.SHAPE_RADIUS");
    WheelWidth    = Config::get<float>("OPENBOT.OPENBOT_WHEEL.SHAPE_WIDTH");

    SuspensionMaxRaise     = Config::get<float>("OPENBOT.OPENBOT_WHEEL.SUSPENSION_MAX_RAISE");
    SuspensionMaxDrop      = Config::get<float>("OPENBOT.OPENBOT_WHEEL.SUSPENSION_MAX_DROP");
    SuspensionDampingRatio = Config::get<float>("OPENBOT.OPENBOT_WHEEL.SUSPENSION_DAMPING_RATIO");
    SuspensionForceOffset  = FVector(0.0, 0.0, Config::get<float>("WHEELED_VEHICLE.VEHICLE_WHEEL.SUSPENSION_FORCE_OFFSET"));
}

UChaosRearWheel::~UChaosRearWheel()
{
    std::cout << "[SPEAR | OpenBotWheel.cpp] UChaosRearWheel::~UChaosRearWheel" << std::endl;
}
