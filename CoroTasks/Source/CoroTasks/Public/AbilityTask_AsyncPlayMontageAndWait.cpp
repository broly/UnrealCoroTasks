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

#include "AbilityTask_AsyncPlayMontageAndWait.h"
#include "CoroTasksSubsystem.h"
#include "AbilitySystemGlobals.h"


UAbilityTask_AsyncPlayMontageAndWait::UAbilityTask_AsyncPlayMontageAndWait()
{
	bTickingTask = true;
	
}

void UAbilityTask_AsyncPlayMontageAndWait::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UAbilityTask_AsyncPlayMontageAndWait::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::Destroyed);
}

TWeakObjectPtr<UAbilityTask_AsyncPlayMontageAndWait> UAbilityTask_AsyncPlayMontageAndWait::Create(UGameplayAbility* OwningAbility,
                                                                                                  FName TaskInstanceName, UAnimMontage* MontageToPlay, float Rate, FName StartSection, bool bStopWhenAbilityEnds,
                                                                                                  float AnimRootMotionTranslationScale, float StartTimeSeconds)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);
	

	ThisClass* MyObj = NewAbilityTask<ThisClass>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->StartTimeSeconds = StartTimeSeconds;

	MyObj->OnCompleted.AddDynamic(MyObj, &ThisClass::AyncCompleted);
	MyObj->OnBlendOut.AddDynamic(MyObj, &ThisClass::AyncBlendOut);
	MyObj->OnInterrupted.AddDynamic(MyObj, &ThisClass::AyncInterrupted);
	MyObj->OnCancelled.AddDynamic(MyObj, &ThisClass::AyncCancelled);

	FTimerHandle TH;
	const float MontageDuration = MontageToPlay->GetPlayLength() * (Rate >= 0 ? Rate : 1) + 0.1;
	MyObj->GetWorld()->GetTimerManager().SetTimer(TH, MyObj, &ThisClass::AsyncTimeout, MontageDuration);

#if WITH_CPP_COROUTINES
	MyObj->bFinished = false;
#endif
	
	
	return MyObj;
}

void UAbilityTask_AsyncPlayMontageAndWait::AyncCompleted()
{
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::Completed);
}

void UAbilityTask_AsyncPlayMontageAndWait::AyncBlendOut()
{
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::BlendOut);
}

void UAbilityTask_AsyncPlayMontageAndWait::FinishWithResult_IfNothing(EPlayMontageAndWaitResult Result)
{
#if WITH_CPP_COROUTINES
	if (Future.IsSet() && !bFinished)
	{
		bFinished = true;
		(*Future)->SetResult(Result);
		Future.Reset();
	}
#endif
}

void UAbilityTask_AsyncPlayMontageAndWait::AyncInterrupted()
{
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::Interrupted);

}

void UAbilityTask_AsyncPlayMontageAndWait::AyncCancelled()
{
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::Cancelled);
}

void UAbilityTask_AsyncPlayMontageAndWait::AsyncTimeout()
{
	FinishWithResult_IfNothing(EPlayMontageAndWaitResult::Timeout);
}

#if WITH_CPP_COROUTINES
CoroTasks::TFuture<EPlayMontageAndWaitResult>& UAbilityTask_AsyncPlayMontageAndWait::operator co_await()
{
	ReadyForActivation();
	auto& LatentAction = GEngine->GetEngineSubsystem<UCoroTasksSubsystem>()->CreateLatentAction<EPlayMontageAndWaitResult>();
	auto MyFuture = StaticCastSharedRef<CoroTasks::TFuture<EPlayMontageAndWaitResult>>(LatentAction.Future);
	Future.Emplace(MyFuture);
	return MyFuture.Get();
}
#endif