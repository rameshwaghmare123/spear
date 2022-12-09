//
// Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//

#include "CoreUtils.h"
#include "Config.h"

void CoreUtils::StartupModule()
{
    Config::initialize();
}

void CoreUtils::ShutdownModule()
{
    Config::terminate();
}

IMPLEMENT_MODULE(CoreUtils, CoreUtils)
