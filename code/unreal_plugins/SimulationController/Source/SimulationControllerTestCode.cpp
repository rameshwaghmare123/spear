// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimulationController.h"

#include <future>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <Engine/Engine.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <GameFramework/GameModeBase.h>
#include <Kismet/GameplayStatics.h>

#include "AgentController.h"
#include "Assert/Assert.h"
#include "Box.h"
#include "Config.h"
#include "CameraAgentController.h"
#include "ImitationLearningTask.h"
#include "NullTask.h"
#include "OpenBotAgentController.h"
#include "PointGoalNavTask.h"
#include "Rpclib.h"
#include "RpcServer.h"
#include "SphereAgentController.h"
#include "Task.h"
#include "Visualizer.h"

#include <chrono>   // remove this line before PR

// Different possible frame states for thread synchronization
enum class FrameState : uint8_t
{
    Idle,
    RequestPreTick,
    ExecutingPreTick,
    ExecutingTick,
    ExecutingPostTick
};


void SimulationController::StartupModule()
{
    ASSERT(FModuleManager::Get().IsModuleLoaded(TEXT("CoreUtils")));

    std::this_thread::sleep_for(std::chrono::seconds(10));  // remove this line before PR

    post_world_initialization_delegate_handle_ = FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &SimulationController::postWorldInitializationEventHandler);

    world_cleanup_delegate_handle_ = FWorldDelegates::OnWorldCleanup.AddRaw(this, &SimulationController::worldCleanupEventHandler);

    // required for adding thread synchronization logic
    begin_frame_delegate_handle_ = FCoreDelegates::OnBeginFrame.AddRaw(this, &SimulationController::beginFrameEventHandler);
    end_frame_delegate_handle_ = FCoreDelegates::OnEndFrame.AddRaw(this, &SimulationController::endFrameEventHandler);

    // initialize frame state used for thread synchronization
    frame_state_ = FrameState::Idle;

    // config values required for rpc communication
    const auto hostname = Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "IP" });
    const auto port = Config::getValue<int>({ "SIMULATION_CONTROLLER", "PORT" });

    rpc_server_ = std::make_unique<RpcServer>(hostname, port);
    ASSERT(rpc_server_);

    bindFunctionsToRpcServer();

    rpc_server_->launchWorkerThreads(1u);
}

void SimulationController::ShutdownModule()
{
    // If this module is unloaded in the middle of simulation for some reason, raise an error because we do not support this and we want to know when this happens.
    // We expect worldCleanUpEvenHandler(...) to be called before ShutdownModule().
    ASSERT(!world_begin_play_delegate_handle_.IsValid());

    ASSERT(rpc_server_);
    rpc_server_->stop();    // stop the RPC server as we will no longer service client requests
    rpc_server_ = nullptr;

    FCoreDelegates::OnEndFrame.Remove(end_frame_delegate_handle_);
    end_frame_delegate_handle_.Reset();

    FCoreDelegates::OnBeginFrame.Remove(begin_frame_delegate_handle_);
    begin_frame_delegate_handle_.Reset();

    FWorldDelegates::OnWorldCleanup.Remove(world_cleanup_delegate_handle_);
    world_cleanup_delegate_handle_.Reset();

    FWorldDelegates::OnPostWorldInitialization.Remove(post_world_initialization_delegate_handle_);
    post_world_initialization_delegate_handle_.Reset();
}

void SimulationController::postWorldInitializationEventHandler(UWorld* world, const UWorld::InitializationValues initialization_values)
{
    ASSERT(world);

    if (world->IsGameWorld() && GEngine->GetWorldContextFromWorld(world) && world->GetName().Compare(TEXT("Entry"), ESearchCase::CaseSensitive) != 0) {

        //const auto level_name = Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_PATH" }) + "/" + Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_PREFIX" }) + Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_ID" });
        //const auto world_path_name = level_name + "." + Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_PREFIX" }) + Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_ID" });

        // if this is the first valid level being open and the current world is not the desired one, launch the desired one using OpenLevel functionality
        //if (!is_valid_level_open_once_ && TCHAR_TO_UTF8(*(world->GetPathName())) != world_path_name) {
            //UGameplayStatics::OpenLevel(world, level_name.c_str());
            //is_valid_level_open_once_ = true;
        //} else {
            // We do not support multiple concurrent game worlds. We expect worldCleanupEventHandler(...) to be called before a new world is created.
            ASSERT(!world_);

            // Cache local reference of World instance as this is required in other parts of this class.
            world_ = world;

            // required to assign an AgentController based on config param
            world_begin_play_delegate_handle_ = world_->OnWorldBeginPlay.AddRaw(this, &SimulationController::worldBeginPlayEventHandler);

            new_level_loaded_promise_.set_value();  // set this to unblock resetLevel() function
        //}
    }

    if (world->GetName().Compare(TEXT("Entry"), ESearchCase::CaseSensitive) == 0) {
        entry_level_loaded_promise_.set_value();
        //reset_level_called_future_.wait();
    }
}

void SimulationController::worldBeginPlayEventHandler()
{
    // Set few console commands for syncing Game Thread (GT) and RHI thread.
    // For more information on GTSyncType, see http://docs.unrealengine.com/en-US/SharingAndReleasing/LowLatencyFrameSyncing/index.html.
    GEngine->Exec(world_, TEXT("r.GTSyncType 1"));
    GEngine->Exec(world_, TEXT("r.OneFrameThreadLag 0"));

    // execute optional console commands from python client
    for (const auto& command : Config::getValue<std::vector<std::string>>({ "SIMULATION_CONTROLLER", "CUSTOM_UNREAL_CONSOLE_COMMANDS" })) {
        GEngine->Exec(world_, UTF8_TO_TCHAR(command.c_str()));
    }

    // set fixed simulation step time in seconds
    FApp::SetBenchmarking(true);
    FApp::SetFixedDeltaTime(Config::getValue<double>({ "SIMULATION_CONTROLLER", "SIMULATION_STEP_TIME_SECONDS" }));

    // pause gameplay
    UGameplayStatics::SetGamePaused(world_, true);

    // create AgentController
    if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "AGENT_CONTROLLER_NAME" }) == "CameraAgentController") {
        agent_controller_ = std::make_unique<CameraAgentController>(world_);
    } else if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "AGENT_CONTROLLER_NAME" }) == "OpenBotAgentController") {
        agent_controller_ = std::make_unique<OpenBotAgentController>(world_);
    } else if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "AGENT_CONTROLLER_NAME" }) == "SphereAgentController") {
        agent_controller_ = std::make_unique<SphereAgentController>(world_);
    } else {
        ASSERT(false);
    }
    ASSERT(agent_controller_);

    // create Task
    if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "TASK_NAME" }) == "ImitationLearningTask") {
        task_ = std::make_unique<ImitationLearningTask>(world_);
    } else if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "TASK_NAME" }) == "NullTask") {
        task_ = std::make_unique<NullTask>();
    } else if (Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "TASK_NAME" }) == "PointGoalNavigationTask") {
        task_ = std::make_unique<PointGoalNavTask>(world_);
    } else {
        ASSERT(false);
    }
    ASSERT(task_);

    // create Visualizer
    visualizer_ = std::make_unique<Visualizer>();
    ASSERT(visualizer_);

    // deferred initialization for AgentController, Task, and Visualizer
    agent_controller_->findObjectReferences(world_);
    task_->findObjectReferences(world_);
    visualizer_->findObjectReferences(world_);

    is_world_begin_play_executed_ = true;
}

void SimulationController::worldCleanupEventHandler(UWorld* world, bool session_ended, bool cleanup_resources)
{
    ASSERT(world);

    // We only need to perform any additional steps if the world being cleaned up is the world we cached in our world_ member variable.
    if (world == world_) {

        // worldCleanupEventHandler(...) is called for all worlds, but some local state (such as rpc_server_ and agent_controller_) is initialized only when worldBeginPlayEventHandler(...) is called for a particular world.
        // So we check if worldBeginPlayEventHandler(...) has been executed.
        if (is_world_begin_play_executed_) {

            is_world_begin_play_executed_ = false;

            ASSERT(visualizer_);
            visualizer_->cleanUpObjectReferences();
            visualizer_ = nullptr;

            ASSERT(task_);
            task_->cleanUpObjectReferences();
            task_ = nullptr;

            ASSERT(agent_controller_);
            agent_controller_->cleanUpObjectReferences();
            agent_controller_ = nullptr;
        }

        // remove event handlers bound to this world before world gets cleaned up
        world_->OnWorldBeginPlay.Remove(world_begin_play_delegate_handle_);
        world_begin_play_delegate_handle_.Reset();

        // clear local cache
        world_ = nullptr;
    }
}

void SimulationController::beginFrameEventHandler()
{
    std::cout << "beginFrameEventHandler(): beginning..." << std::endl;

    // If beginTick(...) has indicated (via RequestPreTick framestate) that we should execute a frame of work
    if (frame_state_ == FrameState::RequestPreTick) {

        std::cout << "beginFrameEventHandler(): requested preTick..." << std::endl;

        // update local state
        frame_state_ = FrameState::ExecutingPreTick;

        std::cout << "beginFrameEventHandler(): unpausing game..." << std::endl;

        // unpause the game
        UGameplayStatics::SetGamePaused(world_, false);

        std::cout << "beginFrameEventHandler(): unpaused game, waiting for tick()..." << std::endl;

        // execute all pre-tick sync work, wait here for tick(...) to reset work guard
        rpc_server_->runSync();

        std::cout << "beginFrameEventHandler(): received tick()..." << std::endl;

        // execute pre-tick work inside the task
        if (is_world_begin_play_executed_) {
            task_->beginFrame();
        }

        // update local state
        frame_state_ = FrameState::ExecutingTick;
    }
    std::cout << "beginFrameEventHandler(): ending..." << std::endl;
}

void SimulationController::endFrameEventHandler()
{
    std::cout << "endFrameEventHandler(): beginning..." << std::endl;

    // if beginFrameEventHandler(...) has indicated that we are currently executing a frame of work
    if (frame_state_ == FrameState::ExecutingTick) {

        // update local state
        frame_state_ = FrameState::ExecutingPostTick;

        // execute post-tick work inside the task
        if (is_world_begin_play_executed_) {
            task_->endFrame();
        }

        // allow tick() to finish executing
        end_frame_started_executing_promise_.set_value();

        std::cout << "endFrameEventHandler(): waiting for endtick() to reset work guard..." << std::endl;

        // execute all post-tick sync work, wait here for endTick() to reset work guard
        rpc_server_->runSync();

        std::cout << "endFrameEventHandler(): endtick() resetted work guard, now pausing game..." << std::endl;

        // pause the game
        UGameplayStatics::SetGamePaused(world_, true);

        std::cout << "endFrameEventHandler(): game is paused..." << std::endl;

        // update local state
        frame_state_ = FrameState::Idle;

        // allow endTick(...) to finish executing
        end_frame_finished_executing_promise_.set_value();
    }

    std::cout << "endFrameEventHandler(): ending..." << std::endl;
}

void SimulationController::bindFunctionsToRpcServer()
{
    rpc_server_->bindAsync("ping", []() -> std::string {
        std::cout << "SimulationController received a call to ping()..." << std::endl;
        return "SimulationController received a call to ping()...";
    });

    rpc_server_->bindAsync("waitForEntryLevelToLoad", [this]() -> void {
        std::cout << "Waiting for entry_level_to_be_loaded..." << std::endl;
        entry_level_loaded_future_.wait();
        std::cout << "entry_level loaded..." << std::endl;
    });

    rpc_server_->bindAsync("close", []() -> void {
        constexpr auto immediate_shutdown = false;
        FGenericPlatformMisc::RequestExit(immediate_shutdown);
    });

    rpc_server_->bindAsync("getEndianness", []() -> std::string {
        uint32_t dummy = 0x01020304;
        return (reinterpret_cast<const char*>(&dummy)[3] == 1) ? "little" : "big";
    });

    rpc_server_->bindAsync("beginTick", [this]() -> void {

        std::cout << "beginTick(): beginning..." << std::endl;

        ASSERT(frame_state_ == FrameState::Idle);

        // reinitialize end_frame_started_executing promise and future
        end_frame_started_executing_promise_ = std::promise<void>();
        end_frame_started_executing_future_ = end_frame_started_executing_promise_.get_future();

        // reinitialize end_frame_finished_executing promise and future
        end_frame_finished_executing_promise_ = std::promise<void>();
        end_frame_finished_executing_future_ = end_frame_finished_executing_promise_.get_future();

        // indicate that we want the game thread to execute one frame of work
        frame_state_ = FrameState::RequestPreTick;

        std::cout << "beginTick(): ending..." << std::endl;
    });

    rpc_server_->bindAsync("tick", [this]() -> void {

        std::cout << "tick(): beginning..." << std::endl;

        ASSERT((frame_state_ == FrameState::ExecutingPreTick) || (frame_state_ == FrameState::RequestPreTick));

        std::cout << "tick(): unblocking beginframe..." << std::endl;

        // indicate that we want the game thread to stop blocking in beginFrame()
        rpc_server_->unblockRunSyncWhenFinishedExecuting();

        std::cout << "tick(): waiting for endFrame to start executing..." << std::endl;

        // wait here until the game thread has started executing endFrame()
        end_frame_started_executing_future_.wait();

        std::cout << "tick(): endFrame has started executing..." << std::endl;

        ASSERT(frame_state_ == FrameState::ExecutingPostTick);

        std::cout << "tick(): ending..." << std::endl;
    });

    rpc_server_->bindAsync("endTick", [this]() -> void {

        std::cout << "endTick(): beginning..." << std::endl;

        ASSERT(frame_state_ == FrameState::ExecutingPostTick);

        std::cout << "endTick(): unblocking endFrame()..." << std::endl;

        // indicate that we want the game thread to stop blocking in endFrame()
        rpc_server_->unblockRunSyncWhenFinishedExecuting();

        std::cout << "endTick(): endFrame() unblocked, waiting for endFrme() to finish..." << std::endl;

        // wait here until the game thread has finished executing endFrame()
        end_frame_finished_executing_future_.wait();

        std::cout << "endTick(): endFrme() finished..." << std::endl;

        ASSERT(frame_state_ == FrameState::Idle);

        std::cout << "endTick(): ending..." << std::endl;
    });

    rpc_server_->bindAsync("getActionSpace", [this]() -> std::map<std::string, Box> {
        ASSERT(agent_controller_);
        return agent_controller_->getActionSpace();
    });

    rpc_server_->bindAsync("getObservationSpace", [this]() -> std::map<std::string, Box> {
        ASSERT(agent_controller_);
        return agent_controller_->getObservationSpace();
    });

    rpc_server_->bindAsync("getAgentControllerStepInfoSpace", [this]() -> std::map<std::string, Box> {
        ASSERT(agent_controller_);
        return agent_controller_->getStepInfoSpace();
    });

    rpc_server_->bindAsync("getTaskStepInfoSpace", [this]() -> std::map<std::string, Box> {
        ASSERT(task_);
        return task_->getStepInfoSpace();
    });

    rpc_server_->bindAsync("resetLevel", [this](const std::string level_id) -> void {
        const auto level_name = Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_PATH" }) + "/" + Config::getValue<std::string>({ "SIMULATION_CONTROLLER", "LEVEL_PREFIX" }) + level_id;
        UGameplayStatics::OpenLevel(world_, level_name.c_str());
        
        // every time resetLevel is called, wait till new level loads successfully before returning to client
        new_level_loaded_promise_ = std::promise<void>();
        new_level_loaded_future_ = new_level_loaded_promise_.get_future();
        new_level_loaded_future_.wait();

        //reset_level_called_promise_.set_value();
    });

    rpc_server_->bindSync("applyAction", [this](std::map<std::string, std::vector<float>> action) -> void {
        ASSERT(frame_state_ == FrameState::ExecutingPreTick);
        ASSERT(agent_controller_);
        agent_controller_->applyAction(action);
    });

    rpc_server_->bindSync("getObservation", [this]() -> std::map<std::string, std::vector<uint8_t>> {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(agent_controller_);
        return agent_controller_->getObservation();
    });

    rpc_server_->bindSync("getReward", [this]() -> float {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(task_);
        return task_->getReward();
    });

    rpc_server_->bindSync("isEpisodeDone", [this]() -> bool {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(task_);
        return task_->isEpisodeDone();
    });

    rpc_server_->bindSync("getAgentControllerStepInfo", [this]() -> std::map<std::string, std::vector<uint8_t>> {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(agent_controller_);
        return agent_controller_->getStepInfo();
    });

    rpc_server_->bindSync("getTaskStepInfo", [this]() -> std::map<std::string, std::vector<uint8_t>> {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(task_);
        return task_->getStepInfo();
    });

    rpc_server_->bindSync("resetAgentController", [this]() -> void {
        ASSERT(frame_state_ == FrameState::ExecutingPreTick);
        ASSERT(agent_controller_);
        agent_controller_->reset();
    });

    rpc_server_->bindSync("resetTask", [this]() -> void {
        ASSERT(frame_state_ == FrameState::ExecutingPreTick);
        ASSERT(task_);
        task_->reset();
    });

    rpc_server_->bindSync("isAgentControllerReady", [this]() -> bool {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(agent_controller_);
        return agent_controller_->isReady();
    });

    rpc_server_->bindSync("isTaskReady", [this]() -> bool {
        ASSERT(frame_state_ == FrameState::ExecutingPostTick);
        ASSERT(task_);
        return task_->isReady();
    });
}

IMPLEMENT_MODULE(SimulationController, SimulationController)