#pragma once

#include "CoreMinimal.h"
#include "ScenarioDefinition.h"

class FScenarioLoader
{
    public:

    static bool LoadFromFile(const FString& FilePath,FScenarioDefinition& OutScenario,FString& OutError);
};