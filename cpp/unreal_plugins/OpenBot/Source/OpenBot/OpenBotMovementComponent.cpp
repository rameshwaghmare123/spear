//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "OpenBot/OpenBotMovementComponent.h"

#include <iostream>

#include "CoreUtils/Config.h"
#include "OpenBot/ChaosFrontWheel.h"
#include "OpenBot/ChaosRearWheel.h"

UOpenBotMovementComponent::UOpenBotMovementComponent()
{
    std::cout << "[SPEAR | OpenBotComponent.cpp] UOpenBotMovementComponent::UOpenBotMovementComponent" << std::endl;

    WheelSetups.SetNum(4);

    UClass* front_wheel_class = UChaosFrontWheel::StaticClass();
    UClass* rear_wheel_class  = UChaosRearWheel::StaticClass();

    WheelSetups[0].WheelClass = front_wheel_class;
    WheelSetups[0].BoneName = FName("FL");
    WheelSetups[0].AdditionalOffset = FVector::ZeroVector; // Offset the wheel from the bone's location

    WheelSetups[1].WheelClass = front_wheel_class;
    WheelSetups[1].BoneName = FName("FR");
    WheelSetups[1].AdditionalOffset = FVector::ZeroVector; // Offset the wheel from the bone's location

    WheelSetups[2].WheelClass = rear_wheel_class;
    WheelSetups[2].BoneName = FName("RL");
    WheelSetups[2].AdditionalOffset = FVector::ZeroVector; // Offset the wheel from the bone's location

    WheelSetups[3].WheelClass = rear_wheel_class;
    WheelSetups[3].BoneName = FName("RR");
    WheelSetups[3].AdditionalOffset = FVector::ZeroVector; // Offset the wheel from the bone's location

    InertiaTensorScale = FVector::OneVector;
    Mass               = Config::get<float>("OPENBOT.OPENBOT_PAWN.VEHICLE_COMPONENT.MASS");
    DragCoefficient    = Config::get<float>("OPENBOT.OPENBOT_PAWN.VEHICLE_COMPONENT.DRAG_COEFFICIENT");
    ChassisWidth       = Config::get<float>("OPENBOT.OPENBOT_PAWN.VEHICLE_COMPONENT.CHASSIS_WIDTH");
    ChassisHeight      = Config::get<float>("OPENBOT.OPENBOT_PAWN.VEHICLE_COMPONENT.CHASSIS_HEIGHT");
    
    ConstructorHelpers::FObjectFinder<UCurveFloat> torque_curve_asset(TEXT("/Script/Engine.CurveFloat'/OpenBot/TorqueCurve.TorqueCurve'"));
    ASSERT(torque_curve_asset.Succeeded());

    EngineSetup.MaxRPM    = Config::get<float>("OPENBOT.OPENBOT_PAWN.VEHICLE_COMPONENT.MOTOR_MAX_RPM");
    EngineSetup.MaxTorque = Config::get<float>("OPENBOT.OPENBOT_PAWN.MOTOR_TORQUE_MAX");
    //EngineSetup.TorqueCurve.ExternalCurve = torque_curve_asset.Object;

    //DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
    //SteeringSetup.SteeringType = ESteeringType::SingleAngle;
}

UOpenBotMovementComponent::~UOpenBotMovementComponent()
{
    std::cout << "[SPEAR | OpenBotComponent.cpp] UOpenBotMovementComponent::~UOpenBotMovementComponent" << std::endl;
}

Eigen::Vector4f UOpenBotMovementComponent::getWheelRotationSpeeds() const
{
    Eigen::Vector4f wheel_rotation_speeds;
    wheel_rotation_speeds(0) = VehicleSimulationPT->PVehicle->GetWheel(0).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(1) = VehicleSimulationPT->PVehicle->GetWheel(1).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(2) = VehicleSimulationPT->PVehicle->GetWheel(2).GetAngularVelocity(); // Expressed in [RPM]
    wheel_rotation_speeds(3) = VehicleSimulationPT->PVehicle->GetWheel(3).GetAngularVelocity(); // Expressed in [RPM]
    return wheel_rotation_speeds;
}

void UOpenBotMovementComponent::printDebugValues()
{
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(0).GetAngularVelocity() = %f"), VehicleSimulationPT->PVehicle->GetWheel(0).GetAngularVelocity());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(1).GetAngularVelocity() = %f"), VehicleSimulationPT->PVehicle->GetWheel(1).GetAngularVelocity());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(2).GetAngularVelocity() = %f"), VehicleSimulationPT->PVehicle->GetWheel(2).GetAngularVelocity());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(3).GetAngularVelocity() = %f"), VehicleSimulationPT->PVehicle->GetWheel(3).GetAngularVelocity());

    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(0).GetDriveTorque() = %f"), VehicleSimulationPT->PVehicle->GetWheel(0).GetDriveTorque());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(1).GetDriveTorque() = %f"), VehicleSimulationPT->PVehicle->GetWheel(1).GetDriveTorque());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(2).GetDriveTorque() = %f"), VehicleSimulationPT->PVehicle->GetWheel(2).GetDriveTorque());
    UE_LOG(LogTemp, Warning, TEXT("UOpenBotMovementComponent.cpp::printDebugValues(), VehicleSimulationPT->PVehicle->GetWheel(3).GetDriveTorque() = %f"), VehicleSimulationPT->PVehicle->GetWheel(3).GetDriveTorque());
}