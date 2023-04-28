
//  Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "OpenBot/OpenBotComponent.h"

#include <iostream>

#include "CoreUtils/Config.h"

UOpenBotComponent::UOpenBotComponent()
{
    std::cout << "[SPEAR | OpenBotComponent.cpp] UOpenBotComponent::UOpenBotComponent" << std::endl;
}

UOpenBotComponent::~UOpenBotComponent()
{
    std::cout << "[SPEAR | OpenBotComponent.cpp] UOpenBotComponent::~UOpenBotComponent" << std::endl;
}

Eigen::Vector4f UOpenBotComponent::getWheelRotationSpeeds() const
{
    Eigen::Vector4f wheel_rotation_speeds;
    wheel_rotation_speeds(0) = VehicleSimulationPT->PVehicle->GetWheel(0).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(1) = VehicleSimulationPT->PVehicle->GetWheel(1).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(2) = VehicleSimulationPT->PVehicle->GetWheel(2).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(3) = VehicleSimulationPT->PVehicle->GetWheel(3).GetAngularVelocity(); // Expressed in [RPM]
    return wheel_rotation_speeds;                                                               // Expressed in [RPM]
}
