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
#include "Coroutine.h"
#include "UObject/Object.h"
#include "CoroTasksSubsystem.generated.h"



DECLARE_DELEGATE_RetVal(bool, FLatentPollingDelegate);

struct FCoroTasksLatentActionInfo
{
	FCoroTasksLatentActionInfo(TSharedRef<CoroTasks::FFuture_Base> InFuture, int32 InId, bool InIsPolling = true)
		: Future(InFuture)
		, Id(InId)
		, bIsPolling(InIsPolling)
		, bIsFinished(false)
	{}
	TSharedRef<CoroTasks::FFuture_Base> Future;
	FLatentPollingDelegate Delegate;
	int32 Id;
	bool bIsPolling;
	bool bIsFinished;
};

/**
 * 
 */
UCLASS()
class COROTASKS_API UCoroTasksSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	
	// BEGIN UEngineSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// END UEngineSubsystem

	template<typename ResultType, typename Callable>
	TSharedRef<CoroTasks::TFuture<ResultType>> CreateLatentPollingAction(Callable&& InCallable, UObject* Object = nullptr)
	{
		auto& LatentInfo = CreateLatentAction<ResultType>();
		LatentInfo.bIsPolling = true;
		LatentInfo.Delegate.BindWeakLambda(Object ? Object : this, InCallable);
		return StaticCastSharedRef<CoroTasks::TFuture<ResultType>>(LatentInfo.Future);
	}

	template<typename ResultType>
	FCoroTasksLatentActionInfo& CreateLatentAction(UObject* Object = nullptr)
	{
		auto Future = MakeShared<CoroTasks::TFuture<ResultType>>();
		const FCoroTasksLatentActionInfo Info(Future, IdCounter++);
		PendingFutures.Add(Info);
		return PendingFutures.Last();
	}

private:
	bool Tick(float DeltaTime);

	FTSTicker::FDelegateHandle TickerHandle;

	TArray<FCoroTasksLatentActionInfo> PendingFutures;

	int32 IdCounter;
	
};
