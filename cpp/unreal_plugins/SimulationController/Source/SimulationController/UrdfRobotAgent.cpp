//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "SimulationController/UrdfRobotAgent.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <Camera/CameraComponent.h>
#include <Components/SceneCaptureComponent2D.h>
#include <Engine/World.h>
#include <Math/Rotator.h>
#include <Math/Vector.h>

#include "CoreUtils/ArrayDesc.h"
#include "CoreUtils/Assert.h"
#include "CoreUtils/Config.h"
#include "CoreUtils/Log.h"
#include "CoreUtils/Std.h"
#include "CoreUtils/Unreal.h"
#include "SimulationController/CameraSensor.h"
#include "UrdfRobot/UrdfLinkComponent.h"
#include "UrdfRobot/UrdfRobotComponent.h"
#include "UrdfRobot/UrdfRobotPawn.h"

UrdfRobotAgent::UrdfRobotAgent(UWorld* world)
{
    FVector spawn_location = FVector::ZeroVector;
    FRotator spawn_rotation = FRotator::ZeroRotator;
    std::string spawn_mode = Config::get<std::string>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_MODE");
    if (spawn_mode == "specify_existing_actor") {
        AActor* spawn_actor = Unreal::findActorByName(world, Config::get<std::string>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_ACTOR_NAME"));
        SP_ASSERT(spawn_actor);
        spawn_location = spawn_actor->GetActorLocation();
        spawn_rotation = spawn_actor->GetActorRotation();
    } else if (spawn_mode == "specify_pose") {
        spawn_location = FVector(
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_LOCATION_X"),
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_LOCATION_Y"),
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_LOCATION_Z"));
        spawn_rotation = FRotator(
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_ROTATION_PITCH"),
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_ROTATION_YAW"),
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.SPAWN_ROTATION_ROLL"));
    } else {
        SP_ASSERT(false);
    }
    FActorSpawnParameters actor_spawn_params;
    actor_spawn_params.Name = Unreal::toFName(Config::get<std::string>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.URDF_ROBOT_ACTOR_NAME"));
    actor_spawn_params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    urdf_robot_pawn_ = world->SpawnActor<AUrdfRobotPawn>(spawn_location, spawn_rotation, actor_spawn_params);
    SP_ASSERT(urdf_robot_pawn_);

    // Initialize the urdf_robot_pawn
    urdf_robot_pawn_->Initialize();

    urdf_robot_pawn_->CameraComponent->FieldOfView =
        Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.FOV");
    urdf_robot_pawn_->CameraComponent->AspectRatio =
        Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.IMAGE_WIDTH") /
        Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.IMAGE_HEIGHT");

    // We don't normally cache config values in member variables, but we make an exception in this case
    // because we want ACTION_COMPONENTS and OBSERVATION_COMPONENTS to be defined in URDF_ROBOT_AGENT, but
    // we don't want to pass these arrays around every time we need to apply an action or get an observation.
    urdf_robot_pawn_->UrdfRobotComponent->setActionComponents(Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.ACTION_COMPONENTS"));
    urdf_robot_pawn_->UrdfRobotComponent->setObservationComponents(Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.OBSERVATION_COMPONENTS"));

    auto observation_components = Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.OBSERVATION_COMPONENTS");

    if (Std::contains(observation_components, "camera")) {
        camera_sensor_ = std::make_unique<CameraSensor>(
            urdf_robot_pawn_->CameraComponent,
            Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.RENDER_PASSES"),
            Config::get<unsigned int>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.IMAGE_WIDTH"),
            Config::get<unsigned int>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.IMAGE_HEIGHT"),
            Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.CAMERA.FOV"));
        SP_ASSERT(camera_sensor_);
    }
}

UrdfRobotAgent::~UrdfRobotAgent()
{
    auto observation_components = Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.OBSERVATION_COMPONENTS");

    if (Std::contains(observation_components, "camera")) {
        SP_ASSERT(camera_sensor_);
        camera_sensor_ = nullptr;
    }

    SP_ASSERT(urdf_robot_pawn_);
    urdf_robot_pawn_->Destroy();
    urdf_robot_pawn_ = nullptr;
}

void UrdfRobotAgent::findObjectReferences(UWorld* world) {}
void UrdfRobotAgent::cleanUpObjectReferences() {}

std::map<std::string, ArrayDesc> UrdfRobotAgent::getActionSpace() const
{
    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    return urdf_robot_pawn_->UrdfRobotComponent->getActionSpace();
}

std::map<std::string, ArrayDesc> UrdfRobotAgent::getObservationSpace() const
{
    std::map<std::string, ArrayDesc> observation_space;
    auto observation_components = Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.OBSERVATION_COMPONENTS");

    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    observation_space.merge(urdf_robot_pawn_->UrdfRobotComponent->getObservationSpace());

    if (Std::contains(observation_components, "camera")) {
        observation_space.merge(camera_sensor_->getObservationSpace());
    }

    return observation_space;
}

std::map<std::string, ArrayDesc> UrdfRobotAgent::getStepInfoSpace() const
{
    return {};
}

void UrdfRobotAgent::applyAction(const std::map<std::string, std::vector<uint8_t>>& action)
{
    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    urdf_robot_pawn_->UrdfRobotComponent->applyAction(action);
}

std::map<std::string, std::vector<uint8_t>> UrdfRobotAgent::getObservation() const
{
    std::map<std::string, std::vector<uint8_t>> observation;
    auto observation_components = Config::get<std::vector<std::string>>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.OBSERVATION_COMPONENTS");

    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    observation.merge(urdf_robot_pawn_->UrdfRobotComponent->getObservation());

    if (Std::contains(observation_components, "camera")) {
        observation.merge(camera_sensor_->getObservation());
    }

    return observation;
}

std::map<std::string, std::vector<uint8_t>> UrdfRobotAgent::getStepInfo() const
{
    return {};
}

void UrdfRobotAgent::reset()
{
    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    urdf_robot_pawn_->UrdfRobotComponent->reset();
}

bool UrdfRobotAgent::isReady() const
{
    SP_ASSERT(urdf_robot_pawn_->UrdfRobotComponent);
    // consider both linear and angular velocities of all components to determine if the robot is ready or not
    float sum_vel = 0.0;
    for (auto link_component : urdf_robot_pawn_->UrdfRobotComponent->LinkComponents) {
        sum_vel += link_component->GetPhysicsAngularVelocityInRadians().Size();
        sum_vel += link_component->GetPhysicsLinearVelocity().Size();
    }

    return sum_vel <= Config::get<float>("SIMULATION_CONTROLLER.URDF_ROBOT_AGENT.IS_READY_VELOCITY_THRESHOLD");
}