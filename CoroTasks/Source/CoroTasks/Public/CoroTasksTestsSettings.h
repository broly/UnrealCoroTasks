// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoroTasksTestsSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig)
class COROTASKS_API UCoroTasksTestsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Config, Category = "Test")
	TSoftObjectPtr<UObject> TestObjectToLoad;
};
