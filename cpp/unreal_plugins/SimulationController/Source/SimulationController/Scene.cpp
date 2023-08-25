//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "SimulationController/Scene.h"

#include <map>
#include <string>
#include <vector>

#include <Components/SceneComponent.h>  // USceneComponent
#include <Components/StaticMeshComponent.h> // UStaticMeshComponent
#include <GameFramework/Actor.h>    //AActor
#include <PhysicsEngine/PhysicsConstraintComponent.h>   //UPhysicsConstraintComponent

#include "CoreUtils/Assert.h"
#include "CoreUtils/Unreal.h"

void Scene::findObjectReferences(UWorld* world)
{
    SP_ASSERT(world);
    world_ = world;

    // find all AActors in the world
    actors_name_ref_map_ = Unreal::findActorsByTagAllAsMap(world_, {});
    SP_ASSERT(!actors_name_ref_map_.empty());

    // find all USceneComponents in the world
    for (auto& element : actors_name_ref_map_) {
        TArray<USceneComponent*> scene_components;
        element.second->GetComponents<USceneComponent*>(scene_components);
        for (auto& scene_component : scene_components) {
            std::string name = element.first + "." + Unreal::toStdString(scene_component->GetName());
            SP_ASSERT(!Std::containsKey(scene_components_name_ref_map_, name)); // There shouldn't be two actor.component with the same name
            scene_components_name_ref_map_[name] = scene_component;
        }
    }
}

void Scene::cleanUpObjectReferences()
{
    scene_components_name_ref_map_.clear();
    actors_name_ref_map_.clear();
    world_ = nullptr;
}

std::vector<std::string> Scene::getAllActorNames()
{
    std::vector<std::string> actor_names;
    for(auto& element: actors_name_ref_map_) {
        actor_names.emplace_back(element.first);
    }
    return actor_names;
}

std::map<std::string, std::vector<std::string>> Scene::getStaticMeshComponentsForActors(std::vector<std::string> actors)
{
    std::map<std::string, std::vector<std::string>> static_mesh_components;
    for (auto& element : actors_name_ref_map_) {

        TArray<UStaticMeshComponent*> sm_comps;
        element.second->GetComponents<UStaticMeshComponent*>(sm_comps);
        for (auto& sm_comp : sm_comps) {
            std::string name = Unreal::toStdString(sm_comp->GetName());
            static_mesh_components[element.first].emplace_back(name);
        }
    }
    return static_mesh_components;
}

std::map<std::string, std::vector<std::string>> Scene::getPhysicsConstraintComponentsForActors(std::vector<std::string> actors)
{
    std::map<std::string, std::vector<std::string>> physics_constraint_components;
    for (auto& element : actors_name_ref_map_) {

        TArray<UPhysicsConstraintComponent*> pc_comps;
        element.second->GetComponents<UPhysicsConstraintComponent*>(pc_comps);
        for (auto& pc_comp : pc_comps) {
            std::string name = Unreal::toStdString(pc_comp->GetName());
            physics_constraint_components[element.first].emplace_back(name);
        }
    }
    return physics_constraint_components;
}

std::map<std::string, std::vector<uint8_t>> Scene::getAllActorLocations()
{
    std::map<std::string, std::vector<uint8_t>> actor_locations;
    for (auto& element : actors_name_ref_map_) {
        FVector location = element.second->GetActorLocation();
        actor_locations[element.first] = Std::reinterpretAs<uint8_t>(std::vector<double>{location.X, location.Y, location.Z});
    }
    return actor_locations;
}

std::map<std::string, std::vector<uint8_t>> Scene::getAllActorRotations()
{
    std::map<std::string, std::vector<uint8_t>> actor_rotations;
    for (auto& element : actors_name_ref_map_) {
        FQuat rotation = element.second->GetActorQuat();
        actor_rotations[element.first] = Std::reinterpretAs<uint8_t>(std::vector<double>{rotation.X, rotation.Y, rotation.Z, rotation.W});
    }
    return actor_rotations;
}

std::vector<uint8_t> Scene::getActorLocations(std::vector<std::string> actor_names)
{
    std::vector<uint8_t> actor_locations;
    for(auto& actor_name: actor_names){
        SP_ASSERT(actors_name_ref_map_.count(actor_name));
        FVector location = actors_name_ref_map_.at(actor_name)->GetActorLocation();
        std::vector<uint8_t> actor_location = Std::reinterpretAs<uint8_t>(std::vector<double>{location.X, location.Y, location.Z});
        actor_locations.insert(actor_locations.end(), actor_location.begin(), actor_location.end());
    }
    return actor_locations;
}

std::vector<uint8_t> Scene::getActorRotations(std::vector<std::string> actor_names)
{
    std::vector<uint8_t> object_rotations;
    for(auto& actor_name: actor_names){
        SP_ASSERT(actors_name_ref_map_.count(actor_name));
        FQuat rotation = actors_name_ref_map_.at(actor_name)->GetActorQuat();
        std::vector<uint8_t> object_rotation = Std::reinterpretAs<uint8_t>(std::vector<double>{rotation.X, rotation.Y, rotation.Z, rotation.W});
        object_rotations.insert(object_rotations.end(), object_rotation.begin(), object_rotation.end());
    }
    return object_rotations;
}

std::vector<bool> Scene::isUsingAbsoluteLocation(std::vector<std::string> component_names) {
    
    std::vector<bool> absolute_locations;

    for (auto& component_name : component_names) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_name));
        bool is_using_absolute = scene_components_name_ref_map_.at(component_name)->IsUsingAbsoluteLocation();
        absolute_locations.emplace_back(is_using_absolute);
    }
    return absolute_locations;
}

std::vector<bool> Scene::isUsingAbsoluteRotation(std::vector<std::string> component_names) {
    std::vector<bool> absolute_locations;

    for (auto& component_name : component_names) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_name));
        bool is_using_absolute = scene_components_name_ref_map_.at(component_name)->IsUsingAbsoluteRotation();
        absolute_locations.emplace_back(is_using_absolute);
    }
    return absolute_locations;
}

std::vector<bool> Scene::isUsingAbsoluteScale(std::vector<std::string> component_names) {
    std::vector<bool> absolute_locations;

    for (auto& component_name : component_names) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_name));
        bool is_using_absolute = scene_components_name_ref_map_.at(component_name)->IsUsingAbsoluteScale();
        absolute_locations.emplace_back(is_using_absolute);
    }
    return absolute_locations;
}

void Scene::SetAbolute(std::vector<std::string> component_names, std::vector<bool> blocations, std::vector<bool> brotations, std::vector<bool> bscales) {

    SP_ASSERT(component_names.size() == blocations.size() == brotations.size() == bscales.size()); //assert all array sizes are equal

    for (int i = 0; i < component_names.size(); i++) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_names.at(i)));
        scene_components_name_ref_map_.at(component_names.at(i))->SetAbsolute(blocations.at(i), brotations.at(i), bscales.at(i));
    }
}

void Scene::setActorLocations(std::map<std::string, std::vector<uint8_t>> actor_locations)
{
    for (auto& actor_location : actor_locations) {
        SP_ASSERT(actors_name_ref_map_.count(actor_location.first));

        std::vector<double> location = Std::reinterpretAs<double>(actor_location.second);
        SP_ASSERT(location.size() % 3 == 0);

        FVector location_fvector = { location.at(0), location.at(1), location.at(2) };
        bool sweep = false;
        FHitResult* hit_result = nullptr;
        bool success = actors_name_ref_map_.at(actor_location.first)->SetActorLocation(location_fvector, sweep, hit_result, ETeleportType::TeleportPhysics);
        SP_ASSERT(success);
    }
}

void Scene::setActorRotations(std::map<std::string, std::vector<uint8_t>> actor_rotations)
{
    for (auto& actor_rotation : actor_rotations) {
        SP_ASSERT(actors_name_ref_map_.count(actor_rotation.first));

        std::vector<double> rotation = Std::reinterpretAs<double>(actor_rotation.second);
        SP_ASSERT(rotation.size() % 4 == 0);

        FQuat rotation_fquat = { rotation.at(0), rotation.at(1), rotation.at(2), rotation.at(3) };
        bool success = actors_name_ref_map_.at(actor_rotation.first)->SetActorRotation(rotation_fquat, ETeleportType::TeleportPhysics);
        SP_ASSERT(success);
    }
}

void Scene::setComponentWorldLocations(std::map<std::string, std::vector<uint8_t>> component_locations)
{
    for (auto& component_location : component_locations) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_location.first));

        std::vector<double> location = Std::reinterpretAs<double>(component_location.second);
        SP_ASSERT(location.size() % 3 == 0);

        FVector location_fvector = { location.at(0), location.at(1), location.at(2) };
        bool sweep = false;
        FHitResult* hit_result = nullptr;
        scene_components_name_ref_map_.at(component_location.first)->SetWorldLocation(location_fvector, sweep, hit_result, ETeleportType::TeleportPhysics);
    }
}

void Scene::setComponentWorldRotations(std::map<std::string, std::vector<uint8_t>> component_rotations)
{
    for (auto& component_rotation : component_rotations) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_rotation.first));

        std::vector<double> rotation = Std::reinterpretAs<double>(component_rotation.second);
        SP_ASSERT(rotation.size() % 4 == 0);

        FQuat rotation_fquat = { rotation.at(0), rotation.at(1), rotation.at(2), rotation.at(3) };
        bool sweep = false;
        FHitResult* hit_result = nullptr;
        scene_components_name_ref_map_.at(component_rotation.first)->SetWorldRotation(rotation_fquat, sweep, hit_result, ETeleportType::TeleportPhysics);
    }
}

void Scene::setComponentRelativeLocations(std::map<std::string, std::vector<uint8_t>> component_locations)
{
    for (auto& component_location : component_locations) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_location.first));

        std::vector<double> location = Std::reinterpretAs<double>(component_location.second);
        SP_ASSERT(location.size() % 3 == 0);

        FVector location_fvector = { location.at(0), location.at(1), location.at(2) };
        bool sweep = false;
        FHitResult* hit_result = nullptr;
        scene_components_name_ref_map_.at(component_location.first)->SetRelativeLocation(location_fvector, sweep, hit_result, ETeleportType::TeleportPhysics);
    }
}

void Scene::setComponentRelativeRotations(std::map<std::string, std::vector<uint8_t>> component_rotations)
{
    for (auto& component_rotation : component_rotations) {
        SP_ASSERT(scene_components_name_ref_map_.count(component_rotation.first));

        std::vector<double> rotation = Std::reinterpretAs<double>(component_rotation.second);
        SP_ASSERT(rotation.size() % 4 == 0);

        FQuat rotation_fquat = { rotation.at(0), rotation.at(1), rotation.at(2), rotation.at(3) };
        bool sweep = false;
        FHitResult* hit_result = nullptr;
        scene_components_name_ref_map_.at(component_rotation.first)->SetRelativeRotation(rotation_fquat, sweep, hit_result, ETeleportType::TeleportPhysics);
    }
}