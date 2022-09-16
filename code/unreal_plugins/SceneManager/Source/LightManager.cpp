#include "LightManager.h"

#include <Components/LightComponent.h>
#include <Components/LocalLightComponent.h>
#include <Components/PointLightComponent.h>
#include <Components/RectLightComponent.h>
#include <Components/SpotLightComponent.h>
#include <EngineUtils.h>
#include <Engine/DirectionalLight.h>
#include <Engine/PointLight.h>
#include <Engine/SpotLight.h>
#include <Engine/RectLight.h>
#include <JsonObjectConverter.h>

void LightManager::exportLights(const FString& outpath, const TArray<ALight*>& lights)
{
    FSceneLightInfo scene_light_info;
    for (auto& light : lights) {
        FLightInfo light_info;
        light_info.name = light->GetName();
        light_info.transform = light->GetActorTransform();
        light_info.lightColor = light->GetLightColor();

        if (light->IsA(ARectLight::StaticClass())) {
            URectLightComponent* light_component = Cast<URectLightComponent>(light->GetLightComponent());
            light_info.unit = (uint8)light_component->IntensityUnits;

            light_info.sourceHeight = light_component->SourceHeight;
            light_info.sourceWidth = light_component->SourceWidth;
        }
        else if (light->IsA(APointLight::StaticClass())) {
            UPointLightComponent* light_component = Cast<UPointLightComponent>(light->GetLightComponent());
            light_info.unit = (uint8)light_component->IntensityUnits;
        }
        else if (light->IsA(ASpotLight::StaticClass())) {
            USpotLightComponent* light_component = Cast<USpotLightComponent>(light->GetLightComponent());
            light_info.unit = (uint8)light_component->IntensityUnits;
            light_info.innerConeAngle = light_component->InnerConeAngle;
            light_info.outerConeAngle = light_component->OuterConeAngle;
        }

        // check if use IES
        ULightComponent* light_component = light->GetLightComponent();
        light_info.type = (uint8)light_component->GetLightType();
        light_info.bIsActive = light_component->bAffectsWorld;
        light_info.intensity = light_component->Intensity;

        if (light_component->IESTexture) {
            auto& ies_texture = light_component->IESTexture;
            light_info.iesProfile = ies_texture->GetPackage()->GetName() + "." + ies_texture->GetName();
            light_info.bUseIESBrightness = light_component->bUseIESBrightness;
            light_info.iesBrightnessScale = light_component->IESBrightnessScale;
        }

        scene_light_info.lightInfos.Add(light_info);
    }

    // save data to local file
    FString json_payload;
    FJsonObjectConverter::UStructToJsonObjectString(scene_light_info, json_payload, 0, 0, 0, nullptr, true);
    FFileHelper::SaveStringToFile(json_payload, *outpath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void LightManager::importLights(UWorld* world, const FString& outpath)
{
    FString json_payload;
    FFileHelper::LoadFileToString(json_payload, *outpath);
    FSceneLightInfo scene_light_info;
    FJsonObjectConverter::JsonObjectStringToUStruct(json_payload, &scene_light_info, 0, 0);
    importLights(world, scene_light_info.lightInfos);
}

void LightManager::importLights(UWorld* world, const TArray<FLightInfo>& light_infos)
{
    TMap<FString, ALight*> existing_lights;
    for (TActorIterator<ALight> it(world, ALight::StaticClass()); it; ++it) {
        existing_lights.Add(it->GetName(), *it);
    }

    for (auto& light_info : light_infos) {
        if (existing_lights.Contains(light_info.name)) {
            // update existing light
            ALight* light = existing_lights[light_info.name];
            updateLight(light, light_info);
            existing_lights.Remove(light_info.name);
        }
        else {
            // add new light
            addLight(world, light_info);
        }
    }
    // disable light not listed in light_info ?
    for (auto& kvp : existing_lights) {
        enableLight(kvp.Value, false);
    }
}

ALight* LightManager::addLight(UWorld* world, const FLightInfo& light_info)
{
    FActorSpawnParameters param;
    // TODO
    param.Name = FName(light_info.name);
    param.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
    // create new light based on type
    ALight* light;
    UClass* light_class;
    FName light_folder;
    switch (static_cast<ELightComponentType>(light_info.type)) {
    case LightType_Point:
        light_class = APointLight::StaticClass();
        light_folder = "lights/pointlight";
        break;
    case LightType_Spot:
        light_class = ASpotLight::StaticClass();
        light_folder = "lights/spotlight";
        break;
    case LightType_Rect:
        light_class = ARectLight::StaticClass();
        light_folder = "lights/arealight";
        break;
    case LightType_Directional:
        light_class = ADirectionalLight::StaticClass();
        light_folder = "lights/sunlight";
        break;
    default:
        // unexpected light type TODO
        return nullptr;
    }
    light = world->SpawnActor<ALight>(light_class, light_info.transform, param);
#if WITH_EDITOR
    light->SetFolderPath(light_folder);
#endif
    // add tag to identify new lights
    light->Tags.Add("light_manual_add");

    updateLight(light, light_info);
    return light;
}

void LightManager::updateLight(ALight* light, const FLightInfo& light_info)
{
    light->SetMobility(EComponentMobility::Movable);
    light->SetActorTransform(light_info.transform);

    // set light type specific parameters
    if (light->IsA(ARectLight::StaticClass())) {
        URectLightComponent* light_component = Cast<URectLightComponent>(light->GetLightComponent());
        light_component->SetIntensityUnits(static_cast<ELightUnits>(light_info.unit));
        if (light_info.sourceHeight > 0) {
            light_component->SetSourceHeight(light_info.sourceHeight);
        }
        if (light_info.sourceWidth > 0) {
            light_component->SetSourceWidth(light_info.sourceWidth);
        }
    }
    else if (light->IsA(APointLight::StaticClass())) {
        UPointLightComponent* light_component = Cast<UPointLightComponent>(light->GetLightComponent());
        light_component->SetIntensityUnits(static_cast<ELightUnits>(light_info.unit));
    }
    else if (light->IsA(ASpotLight::StaticClass())) {
        USpotLightComponent* light_component = Cast<USpotLightComponent>(light->GetLightComponent());
        light_component->SetIntensityUnits(static_cast<ELightUnits>(light_info.unit));

        light_component->InnerConeAngle = light_info.innerConeAngle;
        light_component->OuterConeAngle = light_info.outerConeAngle;
    }

    // setup IES
    ULightComponent* light_component = light->GetLightComponent();

    light_component->bAffectsWorld = light_info.bIsActive;
    light_component->SetIntensity(light_info.intensity);
    light_component->SetLightColor(light_info.lightColor);
    if (!light_info.iesProfile.IsEmpty()) {
        light_component->IESTexture = LoadObject<UTextureLightProfile>(nullptr, *light_info.iesProfile);
        light_component->bUseIESBrightness = light_info.bUseIESBrightness;
        light_component->IESBrightnessScale = light_info.iesBrightnessScale;
    }
}

void LightManager::enableLight(ALight* light, bool enable)
{
    light->GetLightComponent()->bAffectsWorld = enable;
}