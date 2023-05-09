//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// Copyright Epic Games, Inc. All Rights Reserved.
//

#pragma once

#include <CoreMinimal.h>
#include <Editor/UnrealEdEngine.h>

#include "SpearSimEditorUnrealEdEngine.generated.h"

class FOutputDevice;
class UWorld;

// The purpose of this class is to access Unreal console commands when the editor is running but the game is not running.
// There is no other way to do so, because UFUNCTION(Exec) methods only execute when the game is running. See the following
// links for more details:
//    https://unrealcommunity.wiki/creating-an-editor-module-x64nt5g3
//    https://michaeljcole.github.io/wiki.unrealengine.com/Create_Custom_engine_classes_for_your_game_module
//    https://forums.unrealengine.com/t/what-is-the-proper-method-for-extending-the-editor-engine/282885

// We don't typically use Unreal's built-in logging functionality, but we make an exception in this case. This class is
// only active when running the Unreal Editor, where we don't necessarily have a way of outputting to stdout. But we still
// try to write to stdout as well, because there are situations where we will see the stdout output but not the Unreal log
// output.
DECLARE_LOG_CATEGORY_EXTERN(LogSpearSimEditor, Log, All);

UCLASS()
class USpearSimEditorUnrealEdEngine : public UUnrealEdEngine
{
    GENERATED_BODY()
public:
    USpearSimEditorUnrealEdEngine();
    ~USpearSimEditorUnrealEdEngine();

    bool Exec(UWorld* world, const TCHAR* cmd, FOutputDevice& output_device) override;
};
