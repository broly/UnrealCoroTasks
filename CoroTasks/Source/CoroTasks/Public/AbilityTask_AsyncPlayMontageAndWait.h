// Copyright (c) 2023, Artem Selivanov
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Coroutine.h"
#include "UObject/Object.h"
#include "AbilityTask_AsyncPlayMontageAndWait.generated.h"

enum class EPlayMontageAndWaitResult : uint8
{
	Completed,
	BlendOut,
	Interrupted,
	Cancelled,
	Timeout,
	Destroyed,
};

/**
 * Tour to AbilityTasks:
 * This class is very similar to it's ancestor, but with some differences
 *	1. Instead of UAbilityTask_AsyncPlayMontageAndWait::CreatePlayMontageAndWaitProxy we use
 *		UAbilityTask_AsyncPlayMontageAndWait::Create that returns templated object (may be weak) pointer
 *	2. We adds Future object to resume continuation
 *	3. We use co_await operator to add coroutine support (it automatically calls ReadyForActivation)
 *		In co_await operator we should create Future object that will be managed by UCoroTasksSubsystem
 *	
 *	Use case:
 *	>>> EPlayMontageAndWaitResult Result = co_await UAbilityTask_AsyncPlayMontageAndWait::Create(MyAbility, TEXT("MyTask"), MyMontage);
 */
UCLASS()
class COROTASKS_API UAbilityTask_AsyncPlayMontageAndWait : public UAbilityTask_PlayMontageAndWait
{
	GENERATED_BODY()

public:
	UAbilityTask_AsyncPlayMontageAndWait();
	
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	
	static TWeakObjectPtr<ThisClass> Create(UGameplayAbility* OwningAbility,
		FName TaskInstanceName, UAnimMontage* MontageToPlay, float Rate = 1.f,
		FName StartSection = NAME_None, bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f, float StartTimeSeconds = 0.f);

	
	UFUNCTION()
	void AyncCompleted();
	
	UFUNCTION()
	void AyncBlendOut();

	void FinishWithResult_IfNothing(EPlayMontageAndWaitResult Result);
	
	UFUNCTION()
	void AyncInterrupted();
	
	UFUNCTION()
	void AyncCancelled();

	UFUNCTION()
	void AsyncTimeout();

#if WITH_CPP_COROUTINES
	bool bFinished;
	
	TOptional<TSharedRef<CoroTasks::TFuture<EPlayMontageAndWaitResult>>> Future;
	
	CoroTasks::TFuture<EPlayMontageAndWaitResult>& operator co_await();
#endif
};
