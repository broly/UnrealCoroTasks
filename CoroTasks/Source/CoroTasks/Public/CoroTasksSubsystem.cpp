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

#include "CoroTasksSubsystem.h"

void UCoroTasksSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FTickerDelegate TickerDelegate = FTickerDelegate::CreateUObject(this, &ThisClass::Tick);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickerDelegate);
}

void UCoroTasksSubsystem::Deinitialize()
{
	Super::Deinitialize();
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
}

bool UCoroTasksSubsystem::Tick(float DeltaTime)
{
	if (PendingFutures.Num() > 0)
	{
		for (auto& LatentInfo : PendingFutures)
		{
			if (LatentInfo.bIsPolling && !LatentInfo.bIsFinished)
			{
				if (LatentInfo.Delegate.IsBound() && LatentInfo.Delegate.Execute())
				{
					LatentInfo.bIsFinished = true;
				}
			}
		}
		PendingFutures.RemoveAll([] (const FCoroTasksLatentActionInfo& Info)
		{
			return Info.bIsFinished;
		});
	}
	return true;
}
