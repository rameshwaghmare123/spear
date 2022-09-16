#pragma once

#include <vector>

#include <CoreMinimal.h>
#include <Engine/Light.h>

#include "LightManager.generated.h"

USTRUCT()
struct SCENEMANAGER_API FLightInfo
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY()
    FString name;
    UPROPERTY()
    uint8 type;
    UPROPERTY()
    FTransform transform;

    UPROPERTY()
    uint8 unit;
    UPROPERTY()
    bool bIsActive;
    UPROPERTY()
    float intensity = 0;

    UPROPERTY()
    FLinearColor lightColor;
    // parameter for rect light
    UPROPERTY()
    float sourceWidth = 0;
    UPROPERTY()
    float sourceHeight = 0;
    // parameter for spot light
    UPROPERTY()
    float innerConeAngle = 0;
    UPROPERTY()
    float outerConeAngle = 0;

    // parameter for IES light
    UPROPERTY()
    bool bUseIESBrightness = false;
    UPROPERTY()
    float iesBrightnessScale = 0;
    UPROPERTY()
    FString iesProfile;
};

USTRUCT()
struct SCENEMANAGER_API FSceneLightInfo
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY()
    FString mapName;

    UPROPERTY()
    TArray<FLightInfo> lightInfos;
};

class SCENEMANAGER_API LightManager
{
public:
    // export lights to outpath (temp)
    static void exportLights(const FString& outpath, const TArray<ALight*>& lights);
    // load light config from file (temp)
    static void importLights(UWorld* world, const FString& outpath);
    // load light config (temp)
    static void importLights(UWorld* world, const TArray<FLightInfo>& light_infos);
    // add lights
    static ALight* addLight(UWorld* world, const FLightInfo& light_info);
    // update existing light
    static void updateLight(ALight* light, const FLightInfo& light_info);
    // turn on/off light
    static void enableLight(ALight* light, bool enable);
};
