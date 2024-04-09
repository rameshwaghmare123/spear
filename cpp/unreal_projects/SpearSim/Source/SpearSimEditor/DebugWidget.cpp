//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "SpearSimEditor/DebugWidget.h"

#include <Components/StaticMeshComponent.h>
#include <Engine/StaticMeshActor.h>
#include <Engine/World.h> // FActorSpawnParameters
#include <GameFramework/Actor.h>
#include <Math/Rotator.h>
#include <Math/Transform.h>
#include <Math/Vector.h>
#include <PhysicsEngine/BodyInstance.h>
#include <UObject/Object.h>

#include "SpCore/Assert.h"
#include "SpCore/Log.h"
#include "SpCore/SpCoreActor.h"
#include "SpCore/Std.h"
#include "SpCore/Unreal.h"
#include "SpCore/UnrealObj.h"
#include "SpCore/UnrealClassRegistrar.h"

ADebugWidget::ADebugWidget()
{
    SP_LOG_CURRENT_FUNCTION();
}

ADebugWidget::~ADebugWidget()
{
    SP_LOG_CURRENT_FUNCTION();
}

void ADebugWidget::LoadConfig()
{
    AActor::LoadConfig();
}

void ADebugWidget::SaveConfig()
{
    AActor::SaveConfig();
}

void ADebugWidget::PrintDebugString()
{
    SP_LOG("DebugString: ", Unreal::toStdString(DebugString));
}

void ADebugWidget::GetAndSetObjectProperties()
{
    UWorld* world = GetWorld();
    SP_ASSERT(world);

    std::map<std::string, AStaticMeshActor*> static_mesh_actors = Unreal::findActorsByNameAsMap<AStaticMeshActor>(world, {"Debug/SM_Prop_04", "null"});
    SP_ASSERT(Std::containsKey(static_mesh_actors, "Debug/SM_Prop_04"));
    SP_ASSERT(Std::containsKey(static_mesh_actors, "null"));
    SP_ASSERT(static_mesh_actors.at("Debug/SM_Prop_04"));
    SP_ASSERT(!static_mesh_actors.at("null"));

    AStaticMeshActor* static_mesh_actor = Unreal::findActorByName<AStaticMeshActor>(world, "Debug/SM_Prop_04");
    SP_ASSERT(static_mesh_actor);

    // Get actor from registrar
    AActor* static_mesh_actor_from_registrar = UnrealClassRegistrar::findActorByName("AStaticMeshActor", world, "Debug/SM_Prop_04");
    SP_ASSERT(static_mesh_actor_from_registrar);

    // Get and set object properties from UObject*
    SP_LOG(Unreal::getObjectPropertiesAsString(static_mesh_actor));

    Unreal::setObjectPropertiesFromString(static_mesh_actor, "{\"bHidden\": true }"); // partial updates are allowed
    SP_LOG(Unreal::getObjectPropertiesAsString(static_mesh_actor));

    Unreal::setObjectPropertiesFromString(static_mesh_actor, "{\"bHidden\": false }");
    SP_LOG(Unreal::getObjectPropertiesAsString(static_mesh_actor));

    // Find properties by fully qualified name (pointers like RootComponent are handled correctly)
    Unreal::PropertyDesc root_component_property_desc = Unreal::findPropertyByName(static_mesh_actor, "RootComponent");
    Unreal::PropertyDesc relative_location_property_desc = Unreal::findPropertyByName(static_mesh_actor, "RootComponent.RelativeLocation");

    // Get property value from PropertyDesc
    SP_LOG(Std::toStringFromPtr(static_mesh_actor->GetStaticMeshComponent()));
    SP_LOG(Unreal::getPropertyValueAsString(root_component_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_property_desc));

    // Pointer properties can be set by converting the desired pointer value to a string
    Unreal::setPropertyValueFromString(root_component_property_desc, Std::toStringFromPtr(static_mesh_actor->GetStaticMeshComponent()));
    SP_LOG(Unreal::getPropertyValueAsString(root_component_property_desc));

    UStaticMeshComponent* static_mesh_component = Unreal::getComponentByType<UStaticMeshComponent>(static_mesh_actor);
    SP_ASSERT(static_mesh_component);

    // Get component from registrar
    UActorComponent* static_mesh_component_from_registrar = UnrealClassRegistrar::getComponentByType("UStaticMeshComponent", static_mesh_actor);
    SP_ASSERT(static_mesh_component_from_registrar);

    Unreal::PropertyDesc relative_location_property_desc_  = Unreal::findPropertyByName(static_mesh_component, "RelativeLocation");
    Unreal::PropertyDesc relative_location_x_property_desc = Unreal::findPropertyByName(static_mesh_component, "RelativeLocation.X");
    Unreal::PropertyDesc relative_location_y_property_desc = Unreal::findPropertyByName(static_mesh_component, "RelativeLocation.Y");
    Unreal::PropertyDesc relative_location_z_property_desc = Unreal::findPropertyByName(static_mesh_component, "RelativeLocation.Z");
    Unreal::PropertyDesc body_instance_property_desc       = Unreal::findPropertyByName(static_mesh_component, "BodyInstance");
    Unreal::PropertyDesc com_nudge_property_desc           = Unreal::findPropertyByName(static_mesh_component, "BodyInstance.COMNudge");
    Unreal::PropertyDesc com_nudge_x_property_desc         = Unreal::findPropertyByName(static_mesh_component, "BodyInstance.COMNudge.X");
    Unreal::PropertyDesc com_nudge_y_property_desc         = Unreal::findPropertyByName(static_mesh_component, "BodyInstance.COMNudge.Y");
    Unreal::PropertyDesc com_nudge_z_property_desc         = Unreal::findPropertyByName(static_mesh_component, "BodyInstance.COMNudge.Z");
    Unreal::PropertyDesc com_nudge_property_desc_          = Unreal::findPropertyByName(static_mesh_component, "bodyinstance.comnudge"); // not case-sensitive
    Unreal::PropertyDesc simulate_physics_property_desc    = Unreal::findPropertyByName(static_mesh_component, "BodyInstance.bSimulatePhysics");
    Unreal::PropertyDesc component_velocity_property_desc  = Unreal::findPropertyByName(static_mesh_component, "ComponentVelocity"); // defined in base class

    // Get property value from PropertyDesc
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_property_desc_));
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_x_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_y_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_z_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(body_instance_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(com_nudge_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(com_nudge_x_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(com_nudge_y_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(com_nudge_z_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(com_nudge_property_desc_));
    SP_LOG(Unreal::getPropertyValueAsString(simulate_physics_property_desc));
    SP_LOG(Unreal::getPropertyValueAsString(component_velocity_property_desc));

    void* value_ptr = nullptr;
    UStruct* ustruct = nullptr;

    // Get property values from void* and UStruct*
    value_ptr = &(static_mesh_component->BodyInstance);
    ustruct = UnrealClassRegistrar::getStaticStruct<FBodyInstance>();
    SP_LOG(Unreal::getObjectPropertiesAsString(value_ptr, ustruct));
    SP_LOG();

    // Get property values from void* and UStruct*
    value_ptr = relative_location_property_desc.value_ptr_;
    ustruct = UnrealClassRegistrar::getStaticStruct<FVector>();
    SP_LOG(Unreal::getObjectPropertiesAsString(value_ptr, ustruct));
    SP_LOG();

    static int i = 0;
    std::string str;
    FVector vec(1.23, 4.56, 7.89);

    // Set property value from void* and UStruct*
    str = Std::toString("{", "\"x\": ", 12.3*i, ", \"y\": ", 45.6*i, "}");
    value_ptr = &vec;
    ustruct = UnrealClassRegistrar::getStaticStruct<FVector>();
    SP_LOG(Unreal::getObjectPropertiesAsString(value_ptr, ustruct));
    Unreal::setObjectPropertiesFromString(value_ptr, ustruct, str);
    SP_LOG(Unreal::getObjectPropertiesAsString(value_ptr, ustruct));
    SP_LOG();

    // Set property value from PropertyDesc
    str = Std::toString("{", "\"x\": ", 1.1*i, ", \"y\": ", 2.2*i, ", \"z\": ", 3.3*i, "}");
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_property_desc));
    Unreal::setPropertyValueFromString(relative_location_property_desc, str);
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_property_desc));
    SP_LOG();

    // Set property value from PropertyDesc
    str = "1.2345";
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_z_property_desc));
    Unreal::setPropertyValueFromString(relative_location_z_property_desc, str);
    SP_LOG(Unreal::getPropertyValueAsString(relative_location_z_property_desc));
    SP_LOG();

    ArrayOfInts.Add(10);
    ArrayOfInts.Add(20);
    ArrayOfInts.Add(30);
    ArrayOfVectors.Add(FVector(1.0f, 2.0f, 3.0f));
    ArrayOfVectors.Add(FVector(4.0f, 5.0f, 6.0f));
    ArrayOfStrings.Add(Unreal::toFString("Hello"));
    SP_LOG(Unreal::getObjectPropertiesAsString(this));

    // Arrays can be indexed when searching for properties
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "ArrayOfInts[1]")));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "ArrayOfVectors[1]")));

    Unreal::setPropertyValueFromString(Unreal::findPropertyByName(this, "PrimaryActorTick.TickGroup"), "\"TG_PostPhysics\"");
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "PrimaryActorTick.TickGroup")));
    Unreal::setPropertyValueFromString(Unreal::findPropertyByName(this, "PrimaryActorTick.TickGroup"), "\"TG_PrePhysics\"");
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "PrimaryActorTick.TickGroup")));

    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "ArrayOfInts")));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "ArrayOfVectors")));

    str = Std::toString("{", "\"x\": ", 12.3 * i, ", \"y\": ", 45.6 * i, ", \"z\": ", 78.9 * i, "}");
    Unreal::setPropertyValueFromString(Unreal::findPropertyByName(this, "ArrayOfVectors"), "[ " + str + ", " + str + ", " + str + "]");
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "ArrayOfVectors")));

    MapFromIntToInt.Add(1, 2);
    MapFromIntToInt.Add(3, 4);
    MapFromIntToInt.Add(5, 6);
    SP_LOG(Unreal::getObjectPropertiesAsString(this));

    MapFromStringToVector.Add(Unreal::toFString("Hello"), 1.0*vec);
    MapFromStringToVector.Add(Unreal::toFString("World"), 2.0*vec);
    SP_LOG(Unreal::getObjectPropertiesAsString(this));

    // Maps can also be indexed when searching for properties
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "MapFromIntToInt")));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "MapFromStringToVector")));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "MapFromIntToInt[3]")));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "MapFromStringToVector[\"World\"]")));

    SetOfStrings.Add("Hello");
    SetOfStrings.Add("1");
    SetOfStrings.Add("World");
    SetOfStrings.Add("2");
    SP_LOG(Unreal::getObjectPropertiesAsString(this));
    SP_LOG(Unreal::getPropertyValueAsString(Unreal::findPropertyByName(this, "SetOfStrings")));

    //
    // We need to do this do see visual updates in the editor. But this interface is not ideal because
    // it requires passing in a position and rotation delta, and it doesn't take the UPROPERTIES we set
    // previously into account. Therefore, we must update the position of objects by calling UFUNCTIONS
    // rather than setting UPROPERTIES.
    // 
    // static_mesh_component->MoveComponentImpl(FVector(5.0, 5.0, 5.0), FQuat::Identity, false);
    //

    i++;
}

void ADebugWidget::CallFunctions()
{
    static int i = 0;

    UFunction* ufunction = nullptr;
    std::map<std::string, std::string> args;
    std::map<std::string, std::string> return_values;
    std::string vec_str = Std::toString("{", "\"x\": ", 1.1*i, ", \"y\": ", 2.2*i, ", \"z\": ", 3.3*i, "}");

    args = {{"arg_0", "\"Hello World\""}, {"arg_1", "true"}, {"arg_2", "12345"}, {"arg_3", vec_str}};
    ufunction = Unreal::findFunctionByName(this->GetClass(), "GetString");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(this, ufunction, args);
    SP_LOG(return_values.at("ReturnValue"));

    args = {{"arg_0", "\"Hello World\""}, {"arg_1", "true"}, {"arg_2", "12345"}, {"arg_3", vec_str}};
    ufunction = Unreal::findFunctionByName(this->GetClass(), "GetVector");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(this, ufunction, args);
    SP_LOG(return_values.at("arg_3")); // arg_3 is modified by GetVector(...)
    SP_LOG(return_values.at("ReturnValue"));

    // Pointers can be passed into functions by converting them to strings, and static functions can be
    // called by passing in the class' default object when calling callFunction(...).
    args = {{"world_context_object", Std::toStringFromPtr(GetWorld())}, {"arg_0", "\"Hello World\""}, {"arg_1", "true"}};
    ufunction = Unreal::findFunctionByName(this->GetClass(), "GetWorldContextObject");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(this->GetClass()->GetDefaultObject(), ufunction, args);
    SP_LOG(return_values.at("ReturnValue"));

    UWorld* world = GetWorld();
    SP_ASSERT(world);

    AStaticMeshActor* static_mesh_actor = Unreal::findActorByName<AStaticMeshActor>(world, "Debug/SM_Prop_04");
    SP_ASSERT(static_mesh_actor);

    UStaticMeshComponent* static_mesh_component = Unreal::getComponentByType<UStaticMeshComponent>(static_mesh_actor);
    SP_ASSERT(static_mesh_component);

    //
    // Since partial updates are allowed throughout our setObjectPropertiesFromString(...) and
    // setPropertyValueFromString(...) interfaces, we follow the same convention in our callFunction(...)
    // interface. For each object that gets passed to a given target function, we attempt to initialize
    // it as follows. First, we set its entire memory region to 0, then we (partially) update it
    // according to its corresponding string in the args std::map provided by the caller. This approach
    // enables a caller to explicitly initialize a subset of data members when passing an object to a
    // target function, but always allows the caller to fully initialize the object if necessary. We also
    // allow entire arguments to be omitted from args, in which case the default-initialized (as
    // described above) object will be passed to the target function without further modification.
    // 
    // The signature for the target function below is as follows,
    //
    //     void SceneComponent::K2_AddRelativeLocation(
    //         FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    // 
    // where SweepHitResult is an "out" parameter used to return additional data to the caller. Since
    // SweepHitResult will be filled in by the target function anyway, we simply omit it from args.
    // Regardless of whether or not it is included in args, SweepHitResult (and all other arguments), are
    // always included in the values returned by callFunction(...). So the caller can always access any
    // additional data that was filled in by the target function, as well as the target function's formal
    // return value.
    //

    args = {{"DeltaLocation", vec_str}, {"bSweep", "false"}, {"bTeleport", "false"}};
    ufunction = Unreal::findFunctionByName(static_mesh_component->GetClass(), "K2_AddRelativeLocation");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(static_mesh_component, ufunction, args);
    SP_LOG(return_values.at("SweepHitResult"));

    UObject* uobject = Unreal::findActorByName(world, "SpCore/SpCoreActor");
    SP_ASSERT(uobject);
    ufunction = Unreal::findFunctionByName(uobject->GetClass(), "GetActorHitEventDescs");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(uobject, ufunction, {});
    SP_LOG(return_values.at("ReturnValue"));

    args = {{"DeltaLocation", vec_str}, {"bSweep", "false"}, {"bTeleport", "false"}};
    ufunction = Unreal::findFunctionByName(static_mesh_component->GetClass(), "K2_AddRelativeLocation");
    SP_ASSERT(ufunction);
    return_values = Unreal::callFunction(static_mesh_component, ufunction, args);
    SP_LOG(return_values.at("SweepHitResult"));

    i++;
}

void ADebugWidget::CreateObjects()
{
    static int i = 0;

    UClass* uclass = UnrealClassRegistrar::getStaticClass("UGameplayStatics");
    SP_ASSERT(uclass);

    std::string vec_str = Std::toString("{", "\"x\": ", 1.1*i, ", \"y\": ", 2.2*i, ", \"z\": ", 3.3*i, "}");

    UnrealObj<FVector> location("location");
    UnrealObj<FRotator> rotation("rotation");

    // get object properties as strings for all objects in the input vector
    std::map<std::string, std::string> strings = UnrealObjUtils::getObjectPropertiesAsStrings({location.getPtr(), rotation.getPtr()});
    for (auto& [name, property_string] : strings) {
        SP_LOG(name);
        SP_LOG(property_string);
    }

    // set object properties from a map of strings
    UnrealObjUtils::setObjectPropertiesFromStrings({location.getPtr(), rotation.getPtr()}, {{"location", vec_str}, {"rotation", "{}"}});

    // verify objects have been updated
    strings = UnrealObjUtils::getObjectPropertiesAsStrings({location.getPtr(), rotation.getPtr()});
    for (auto& [name, property_string] : strings) {
        SP_LOG(name);
        SP_LOG(property_string);
    }

    FActorSpawnParameters spawn_parameters;
    AActor* actor = UnrealClassRegistrar::spawnActor("AStaticMeshActor", GetWorld(), location.getObj(), rotation.getObj(), spawn_parameters);
    SP_ASSERT(actor);

    i++;
}

void ADebugWidget::SubscribeToActorHitEvents()
{
    AStaticMeshActor* static_mesh_actor = Unreal::findActorByName<AStaticMeshActor>(GetWorld(), "Debug/SM_Prop_04");
    SP_ASSERT(static_mesh_actor);

    ASpCoreActor* sp_core_actor = Unreal::findActorByName<ASpCoreActor>(GetWorld(), "SpCore/SpCoreActor");
    SP_ASSERT(sp_core_actor);

    UFunction* ufunction = Unreal::findFunctionByName(sp_core_actor->GetClass(), "SubscribeToActorHitEvents");
    Unreal::callFunction(sp_core_actor, ufunction, {{"actor", Std::toStringFromPtr(static_mesh_actor)}});
}

FString ADebugWidget::GetString(FString arg_0, bool arg_1, int arg_2, FVector arg_3)
{
    SP_LOG_CURRENT_FUNCTION();
    return FString("GetString return value.");
}

FVector ADebugWidget::GetVector(FString arg_0, bool arg_1, int arg_2, FVector& arg_3)
{
    SP_LOG_CURRENT_FUNCTION();
    arg_3 = FVector(1.11, 2.22, 3.33);
    return FVector(9.87, 6.54, 3.21);
}

UObject* ADebugWidget::GetWorldContextObject(const UObject* world_context_object, FString arg_0, bool arg_1)
{
    SP_LOG_CURRENT_FUNCTION();
    return const_cast<UObject*>(world_context_object);
}
