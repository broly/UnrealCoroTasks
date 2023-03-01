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
#include "Coroutine.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

/**
 * Tour to asset loading:
 * If you want to async load some asset you can use next functions:
 *  1. LoadSingleObject     - to load single asset
 *  2. LoadMultipleObjects  - to load array of assets
 *  3. LoadSingleClass      - to load a class
 * Use case:
 *      TSoftObjectPtr<UCar> FerrariAsset = ...;  // get soft reference from project settings for example
 *		UCar* FerrariCar = co_await CoroTasks::LoadSingleObject(FerrariAsset);
 */
namespace CoroTasks
{
	
	namespace Private
	{
		template<typename Callable>
		static void RequestAsyncLoad(const TArray<FSoftObjectPath>& ObjectPaths, Callable&& CallableObj, UObject* Context)
		{
			FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
			auto Delegate = (Context
				? FStreamableDelegate::CreateWeakLambda(Context, CallableObj)
				: FStreamableDelegate::CreateLambda(CallableObj));
			const TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(ObjectPaths, Delegate, FStreamableManager::DefaultAsyncLoadPriority);
		}
	}
	
	template<typename T>
	static TSharedRef<CoroTasks::TFuture<T*>> LoadSingleObject(const TSoftObjectPtr<T>& SoftObjectPtr, UObject* OptionalContext = nullptr)
	{
		auto Future = MakeShared<CoroTasks::TFuture<T*>>();
		auto Lambda = [SoftObjectPtr, Future = CopyTemp(Future)]
		{
			if (!SoftObjectPtr.IsNull())
				check(SoftObjectPtr.Get()->template IsA<T>());
			Future->SetResult((T*)(SoftObjectPtr.Get()));
		};
		Private::RequestAsyncLoad({SoftObjectPtr.ToSoftObjectPath()}, MoveTempIfPossible(Lambda), OptionalContext);
		return Future;
	}
	
	template<typename T>
	static TSharedRef<CoroTasks::TFuture<TArray<T*>>> LoadMultipleObjects(const TArray<TSoftObjectPtr<T>>& SoftObjects, UObject* OptionalContext = nullptr)
	{
		auto Future = MakeShared<CoroTasks::TFuture<T*>>();
		auto Lambda = [SoftObjects, Future = CopyTemp(Future)]
		{
			TArray<T*> Objects;
			Algo::Transform(SoftObjects, Objects, &TSoftObjectPtr<T>::Get);
			Future->SetResult(Objects);
		};
		TArray<FSoftObjectPath> ObjectPaths;
		Algo::Transform(SoftObjects, ObjectPaths, &TSoftObjectPtr<T>::ToSoftObjectPath);
		Private::RequestAsyncLoad(ObjectPaths, MoveTempIfPossible(Lambda), OptionalContext);
		return Future;
	}

	template<typename T>
	static TSharedRef<CoroTasks::TFuture<TSubclassOf<T>>> LoadSingleClass(const TSoftClassPtr<T>& SoftClassPtr, UObject* OptionalContext = nullptr)
	{
		auto Future = MakeShared<CoroTasks::TFuture<T*>>();
		auto Lambda = [SoftClassPtr, Future = CopyTemp(Future)]
		{
			check(SoftClassPtr.Get()->IsChildOf(T::StaticClass()));
			Future->SetResult(TSubclassOf<T>(SoftClassPtr.Get()));
		};

		Private::RequestAsyncLoad({SoftClassPtr.ToSoftObjectPath()}, MoveTempIfPossible(Lambda), OptionalContext);

		return Future;
	}

}