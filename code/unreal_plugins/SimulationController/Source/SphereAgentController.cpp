#include "SphereAgentController.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <Components/SceneCaptureComponent2D.h>
#include <Components/StaticMeshComponent.h>
#include <Engine/TextureRenderTarget2D.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <GameFramework/Actor.h>
#include <UObject/UObjectGlobals.h>

#include "Assert.h"
#include "Box.h"
#include "Config.h"
#include "Serialize.h"
#include "TickEvent.h"

SphereAgentController::SphereAgentController(UWorld* world)
{
    for (TActorIterator<AActor> actor_itr(world, AActor::StaticClass()); actor_itr; ++actor_itr) {
        std::string actor_name = TCHAR_TO_UTF8(*(*actor_itr)->GetName());
        if (actor_name == Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "AGENT_ACTOR_NAME"})) { 
            ASSERT(!agent_actor_);
            agent_actor_ = *actor_itr;
        } else if (actor_name == Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "GOAL_ACTOR_NAME"})) {
            ASSERT(!goal_actor_);
            goal_actor_ = *actor_itr;
        }
    }
    ASSERT(agent_actor_);
    ASSERT(goal_actor_);

    // setup observation camera
    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {

        for (TActorIterator<AActor> actor_itr(world, AActor::StaticClass()); actor_itr; ++actor_itr) {
            std::string actor_name = TCHAR_TO_UTF8(*(*actor_itr)->GetName());
            if (actor_name == Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OBSERVATION_CAMERA_ACTOR_NAME"})) {
                ASSERT(!observation_camera_actor_);
                observation_camera_actor_ = *actor_itr;
                break;
            }
        }
        ASSERT(observation_camera_actor_);

        new_object_parent_actor_ = world->SpawnActor<AActor>();
        ASSERT(new_object_parent_actor_);
        
        // create SceneCaptureComponent2D and TextureRenderTarget2D
        scene_capture_component_ = NewObject<USceneCaptureComponent2D>(new_object_parent_actor_, TEXT("SceneCaptureComponent2D"));
        ASSERT(scene_capture_component_);

        scene_capture_component_->AttachToComponent(observation_camera_actor_->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        scene_capture_component_->SetVisibility(true);
        scene_capture_component_->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
        scene_capture_component_->FOVAngle = 60.f;
        scene_capture_component_->ShowFlags.SetTemporalAA(false);

        texture_render_target_ = NewObject<UTextureRenderTarget2D>(new_object_parent_actor_, TEXT("TextureRenderTarget2D"));
        ASSERT(texture_render_target_);

        // texture_render_target_->bHDR_DEPRECATED = false;
        texture_render_target_->InitCustomFormat(
            Config::getValue<unsigned long>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_HEIGHT"}),
            Config::getValue<unsigned long>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_WIDTH"}),
            PF_B8G8R8A8,
            true); // PF_B8G8R8A8 disables HDR;
        texture_render_target_->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        texture_render_target_->bGPUSharedFlag = true; // demand buffer on GPU - might improve performance?
        texture_render_target_->TargetGamma = 1;
        texture_render_target_->SRGB = false; // false for pixels to be stored in linear space
        texture_render_target_->bAutoGenerateMips = false;
        texture_render_target_->UpdateResourceImmediate(true);

        scene_capture_component_->TextureTarget = texture_render_target_;
        scene_capture_component_->RegisterComponent();

        // assign observation camera to post physics tick group
        post_physics_pre_render_tick_event_ = NewObject<UTickEvent>(new_object_parent_actor_, TEXT("PostPhysicsPreRenderTickEvent"));
        ASSERT(post_physics_pre_render_tick_event_);
        post_physics_pre_render_tick_event_->RegisterComponent();
        post_physics_pre_render_tick_event_->initialize(ETickingGroup::TG_PostPhysics);
        post_physics_pre_render_tick_event_handle_ = post_physics_pre_render_tick_event_->delegate_.AddRaw(this, &SphereAgentController::postPhysicsPreRenderTickEventHandler);
    }

    sphere_static_mesh_component_ = Cast<UStaticMeshComponent>(agent_actor_->GetRootComponent());
    ASSERT(sphere_static_mesh_component_);

    goal_static_mesh_component_ = Cast<UStaticMeshComponent>(goal_actor_->GetRootComponent());
    ASSERT(goal_static_mesh_component_);

    // need to set this to apply forces or move objects
    sphere_static_mesh_component_->SetMobility(EComponentMobility::Type::Movable);
    goal_static_mesh_component_->SetMobility(EComponentMobility::Type::Movable);

    // set physics state
    sphere_static_mesh_component_->BodyInstance.SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
    sphere_static_mesh_component_->SetSimulatePhysics(true);
    sphere_static_mesh_component_->SetAngularDamping(Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "SPHERE", "ANGULAR_DAMPING"}));
    sphere_static_mesh_component_->SetLinearDamping(Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "SPHERE", "LINEAR_DAMPING"}));
    sphere_static_mesh_component_->BodyInstance.MaxAngularVelocity = FMath::RadiansToDegrees(Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "SPHERE", "MAX_ANGULAR_VELOCITY"}));
    sphere_static_mesh_component_->BodyInstance.MassScale = FMath::RadiansToDegrees(Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "SPHERE", "MASS_SCALE"}));
    sphere_static_mesh_component_->SetNotifyRigidBodyCollision(true);
}

SphereAgentController::~SphereAgentController()
{
    ASSERT(goal_static_mesh_component_);
    goal_static_mesh_component_ = nullptr;

    ASSERT(sphere_static_mesh_component_);   
    sphere_static_mesh_component_ = nullptr;

    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {
        ASSERT(post_physics_pre_render_tick_event_);
        post_physics_pre_render_tick_event_->delegate_.Remove(post_physics_pre_render_tick_event_handle_);
        post_physics_pre_render_tick_event_handle_.Reset();
        post_physics_pre_render_tick_event_->DestroyComponent();
        post_physics_pre_render_tick_event_ = nullptr;
        
        ASSERT(texture_render_target_);
        texture_render_target_->MarkPendingKill();
        texture_render_target_ = nullptr;

        ASSERT(scene_capture_component_);
        scene_capture_component_->DestroyComponent();
        scene_capture_component_ = nullptr;

        ASSERT(new_object_parent_actor_);
        new_object_parent_actor_->Destroy();
        new_object_parent_actor_ = nullptr;

        ASSERT(observation_camera_actor_);
        observation_camera_actor_ = nullptr;
    }
    
    ASSERT(agent_actor_);
    agent_actor_ = nullptr;

    ASSERT(goal_actor_);
    goal_actor_ = nullptr;
}

std::map<std::string, Box> SphereAgentController::getActionSpace() const
{
    std::map<std::string, Box> action_space;
    Box box;
    
    box.low = -1.f;
    box.high = 1.f;
    box.shape = {2};
    box.dtype = DataType::Float32;
    action_space["apply_force"] = std::move(box);

    return action_space;
}

std::map<std::string, Box> SphereAgentController::getObservationSpace() const
{
    std::map<std::string, Box> observation_space;
    Box box;
    
    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {
        box.low = std::numeric_limits<float>::lowest();
        box.high = std::numeric_limits<float>::max();
        box.shape = {5};
        box.dtype = DataType::Float32;
        observation_space["physical_observation"] = std::move(box);

        box.low = 0;
        box.high = 255;
        box.shape = {Config::getValue<unsigned long>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_HEIGHT"}),
                     Config::getValue<unsigned long>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_WIDTH"}),
                     3};
        box.dtype = DataType::UInteger8;
        observation_space["visual_observation"] = std::move(box);
    } else if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "physical") {
        box.low = std::numeric_limits<float>::lowest();
        box.high = std::numeric_limits<float>::max();
        box.shape = {4};
        box.dtype = DataType::Float32;
        observation_space["physical_observation"] = std::move(box);
    } else {
        ASSERT(false);
    }

    return observation_space;
}

void SphereAgentController::applyAction(const std::map<std::string, std::vector<float>>& action)
{
    ASSERT(action.count("apply_force"));
    ASSERT(isfinite(action.at("apply_force").at(0)));
    ASSERT(isfinite(action.at("apply_force").at(1)));

    // @TODO: This can be checked in python?
    ASSERT(action.at("apply_force").at(0) >= getActionSpace()["apply_force"].low && action.at("apply_force").at(0) <= getActionSpace()["apply_force"].high, "%f", action.at("apply_force").at(0));
    ASSERT(action.at("apply_force").at(1) >= getActionSpace()["apply_force"].low && action.at("apply_force").at(1) <= getActionSpace()["apply_force"].high, "%f", action.at("apply_force").at(1));

    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {
        // Get yaw from the observation camera, apply force to the sphere in that direction
        FVector force = observation_camera_actor_->GetActorRotation().RotateVector(FVector(action.at("apply_force").at(0), 0.0f, 0.0f)) * Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "ACTION_APPLY_FORCE_SCALE"});

        ASSERT(isfinite(force.X));
        ASSERT(isfinite(force.Y));
        ASSERT(isfinite(force.Z));

        sphere_static_mesh_component_->AddForce(force);

        // Set observation camera yaw by adding to the current observation camera yaw
        FRotator rotation = observation_camera_actor_->GetActorRotation().Add(0.0f, action.at("apply_force").at(1) * Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "ACTION_ROTATE_OBSERVATION_CAMERA_SCALE"}), 0.0f);

        ASSERT(isfinite(rotation.Pitch));
        ASSERT(isfinite(rotation.Yaw));
        ASSERT(isfinite(rotation.Roll));
        ASSERT(rotation.Pitch >= -360.0 && rotation.Pitch <= 360.0, "%f", rotation.Pitch);
        ASSERT(rotation.Yaw   >= -360.0 && rotation.Yaw   <= 360.0, "%f", rotation.Yaw);
        ASSERT(rotation.Roll  >= -360.0 && rotation.Roll  <= 360.0, "%f", rotation.Roll);

        observation_camera_actor_->SetActorRotation(rotation);
    } else if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "physical") {
        FVector force = FVector(action.at("apply_force").at(0), action.at("apply_force").at(1), 0.0f) * Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "PHYSICAL_MODE", "ACTION_APPLY_FORCE_SCALE"});

        ASSERT(isfinite(force.X));
        ASSERT(isfinite(force.Y));
        ASSERT(isfinite(force.Z));

        sphere_static_mesh_component_->AddForce(force);
    } else {
        ASSERT(false);
    }
}

std::map<std::string, std::vector<uint8_t>> SphereAgentController::getObservation() const
{
    std::map<std::string, std::vector<uint8_t>> observation;

    // get game state
    const FVector sphere_to_goal = goal_actor_->GetActorLocation() - agent_actor_->GetActorLocation();
    const FVector linear_velocity = sphere_static_mesh_component_->GetPhysicsLinearVelocity();

    // get observations
    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {
        const float observation_camera_yaw = observation_camera_actor_->GetActorRotation().Yaw;
        observation["physical_observation"] = Serialize::toUint8(std::vector<float>{
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OFFSET_TO_GOAL_SCALE"}) * sphere_to_goal.X,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OFFSET_TO_GOAL_SCALE"}) * sphere_to_goal.Y,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "LINEAR_VELOCITY_SCALE"}) * linear_velocity.X,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "LINEAR_VELOCITY_SCALE"}) * linear_velocity.Y,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "YAW_SCALE"}) * observation_camera_yaw});

        ASSERT(IsInGameThread());

        FTextureRenderTargetResource* target_resource = scene_capture_component_->TextureTarget->GameThread_GetRenderTargetResource();
        ASSERT(target_resource);

        TArray<FColor> pixels;

        struct FReadSurfaceContext
        {
            FRenderTarget* src_render_target_;
            TArray<FColor>& out_data_;
            FIntRect rect_;
            FReadSurfaceDataFlags flags_;
        };

        FReadSurfaceContext context = {target_resource, pixels, FIntRect(0, 0, target_resource->GetSizeXY().X, target_resource->GetSizeXY().Y), FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)};

        // Required for uint8 read mode
        context.flags_.SetLinearToGamma(false);

        ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)([context](FRHICommandListImmediate& RHICmdList) {
            RHICmdList.ReadSurfaceData(context.src_render_target_->GetRenderTargetTexture(), context.rect_, context.out_data_, context.flags_);
        });

        FRenderCommandFence ReadPixelFence;
        ReadPixelFence.BeginFence(true);
        ReadPixelFence.Wait(true);


        std::vector<uint8_t> image(Config::getValue<int>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_HEIGHT"}) *
                                   Config::getValue<int>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "IMAGE_WIDTH"}) *
                                   3);

        for (uint32 i = 0; i < static_cast<uint32>(pixels.Num()); ++i) {
            image.at(3 * i + 0) = pixels[i].R;
            image.at(3 * i + 1) = pixels[i].G;
            image.at(3 * i + 2) = pixels[i].B;
        }

        observation["visual_observation"] = std::move(image);
    } else if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "physical") {
        observation["physical_observation"] = Serialize::toUint8(std::vector<float>{
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "PHYSICAL_MODE", "OFFSET_TO_GOAL_SCALE"}) * sphere_to_goal.X,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "PHYSICAL_MODE", "OFFSET_TO_GOAL_SCALE"}) * sphere_to_goal.Y,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "PHYSICAL_MODE", "LINEAR_VELOCITY_SCALE"}) * linear_velocity.X,
            Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "PHYSICAL_MODE", "LINEAR_VELOCITY_SCALE"}) * linear_velocity.Y});
    } else {
        ASSERT(false);
    }

    return observation;
}

void SphereAgentController::reset()
{
    sphere_static_mesh_component_->SetPhysicsLinearVelocity(FVector(0), false);
    sphere_static_mesh_component_->SetPhysicsAngularVelocityInRadians(FVector(0), false);
    sphere_static_mesh_component_->GetBodyInstance()->ClearTorques();
    sphere_static_mesh_component_->GetBodyInstance()->ClearForces();
}

bool SphereAgentController::isReady() const
{
    return true;
}

void SphereAgentController::postPhysicsPreRenderTickEventHandler(float delta_time, enum ELevelTick level_tick)
{    
    if (Config::getValue<std::string>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "OBSERVATION_MODE"}) == "mixed") {
        if (Config::getValue<bool>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "SET_OBSERVATION_CAMERA_POSE_EGOCENTRIC"})) {
            const FVector observation_camera_pose(
                agent_actor_->GetActorLocation() +
                FVector(Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OBSERVATION_CAMERA_POSITION_OFFSET_X"}),
                        Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OBSERVATION_CAMERA_POSITION_OFFSET_Y"}),
                        Config::getValue<float>({"SIMULATION_CONTROLLER", "SPHERE_AGENT_CONTROLLER", "MIXED_MODE", "OBSERVATION_CAMERA_POSITION_OFFSET_Z"})));
            observation_camera_actor_->SetActorLocation(observation_camera_pose);
        }
    }
}