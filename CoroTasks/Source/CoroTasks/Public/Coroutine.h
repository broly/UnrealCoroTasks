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

#include "CoroSupport.h"

/**
 * Common tour to async tasks
 * You can create own async task. It is simple and very same to creation of function
 * 
 * For example:
 * >>> TTask<bool> IsFerrariCar(TSoftObjectPtr<UCar> CarAsset)
 * >>> {
 * >>>		UCar* Car = co_await CoroTasks::LoadSingleObject(CarAsset);
 * >>>		if (!Car)
 * >>>          throw FCarIsNullException("Car asset is null!");
 * >>>		co_return Car->IsA<UFerrariCar>();
 * >>> }
 *
 * You can also await your other tasks to get a result:
 * >>> TTask<FString> GetSpecialOrderMessage(TSoftObjectPtr<UCar> CarAsset)
 * >>> {
 * >>>		...
 * >>>		bool bIsFerrari = co_await IsFerrariCar(CarAsset);
 * >>>		if (bIsFerrari)
 * >>>			co_return TEXT("There is discount for this car model");
 * >>>		...
 * >>> }
 *
 * You can even throw and catch exceptions that raised in async code
 * >>> CoroTasks::TTask<TTuple<int32, FString>> GetPriceAndMessage(TSoftObjectPtr<UCar> CarAsset)
 * >>> {
 * >>>		try
 * >>>		{
 * >>>			FString SpecialOrderMessage = co_await GetSpecialOrderMessage(CarAsset);
 * >>>			int32 Price = co_await GetPriceOf(CarAsset);
 * >>>		} catch (FCarSellError& Error)
 * >>>		{
 * >>>			UE_LOG(LogCar, Fatal, TEXT("Internal sell error"), *Error.ToString());
 * >>>		}
 * >>>		co_return {Price, SpecialOrderMessage};
 * >>> }
 *
 * You can use tuple structured binding. It is looks very powerful
 * >>> CoroTasks::TTask<> BuyCar(TSoftObjectPtr<UCar> CarAsset)
 * >>> {
 * >>>		auto [Price, Message] = co_await GetPriceAndMessage(CarAsset);
 * >>>		...
 * >>> }
 *
 * To launch the in sync code, you can just call Launch method
 * >>> void LaunchCarBuy(TSoftObjectPtr<UCar> CarAsset)
 * >>> {
 * >>>		auto Task = BuyCar();
 * >>>		Task.Launch();
 * >>> }
 */
namespace CoroTasks
{

	struct FPromise_ExcContainer
	{
		FORCENOINLINE FPromise_ExcContainer()
		{
			
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
		}
		std::exception_ptr CurrentException;
	
		FORCENOINLINE void unhandled_exception()
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			CurrentException = std::current_exception();
			std::rethrow_exception(CurrentException);
		}
	
		DECLARE_DELEGATE_OneParam(FOnException, std::exception_ptr);
		mutable FOnException OnException;
	};


	/**
	 * The templated base of delegate with return value (general case)
	 * Holds delegate that will be fired when promise is finished
	 */
	template<
		typename ReturnType,
		typename TaskType
	>
	struct TPromise_DelegateContainer : FPromise_ExcContainer
	{
		DECLARE_MULTICAST_DELEGATE_OneParam(FOnDone, ReturnType);
		FOnDone OnDone;

		FORCENOINLINE void return_value(ReturnType Result)
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			if (CurrentException)
			{
				OnException.ExecuteIfBound(CurrentException);
				OnException.Unbind();
			}
			else if (OnDone.IsBound())
				OnDone.Broadcast(Result);
		}
	};

	template<typename TaskType>
	struct TPromise_DelegateContainer<void, TaskType> : FPromise_ExcContainer
	{
		DECLARE_MULTICAST_DELEGATE(FOnDone);
		FOnDone OnDone;
	
		FORCENOINLINE void return_void() const
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			if (CurrentException)
			{
				OnException.ExecuteIfBound(CurrentException);
				OnException.Unbind();
			}
			else if (OnDone.IsBound())
				OnDone.Broadcast();
		}
	};

	/**
	 * General promise
	 */
	template<typename ReturnType, typename TaskType>
	struct TPromise : TPromise_DelegateContainer<ReturnType, TaskType>
	{
		using Super = TPromise_DelegateContainer<ReturnType, TaskType>;
	
		FORCENOINLINE auto initial_suspend() noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			return std::suspend_always();
		}
		FORCENOINLINE auto final_suspend() noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			return std::suspend_never();
		}
	
		FORCENOINLINE auto get_return_object()
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			return (TaskType)TaskType::HandleType::from_promise(*this);
		}
	
		auto& GetOnDoneEvent()
		{
			return Super::OnDone;
		}
	};


	struct FTask_ExceptionInterface
	{
		void SetException(std::exception_ptr exc)
		{
			CurrentExc = exc;
		}

		FORCENOINLINE void CheckForException() const
		{
			if (CurrentExc)
				std::rethrow_exception(CurrentExc);
		}

		template<typename PromiseType, typename HandleType>
		FORCENOINLINE void SubscribeForException(PromiseType& Promise, HandleType& Continuation)
		{
			Promise.OnException.BindLambda([this, Continuation] (std::exception_ptr Exc) mutable
			{
				SetException(Exc);
				Continuation.resume();
			});
		}
	
		std::exception_ptr CurrentExc;
	};


	template<typename R>
	struct TTask_Base : FTask_ExceptionInterface
	{
		TOptional<R> Result;
	};

	template<>
	struct TTask_Base<void> : FTask_ExceptionInterface
	{
		bool bHasResult;
	};

	template<typename R = void>
	struct TTask : TTask_Base<R>
	{
		using ReturnType = R;
	
	public:
		using Super = TTask_Base<R>;
		using promise_type = TPromise<ReturnType, TTask<R>>;
		using HandleType = std::coroutine_handle<promise_type>;

		FORCENOINLINE TTask(): bLaunched(false)
		{
			
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
		}

		FORCENOINLINE TTask(HandleType InHandle = nullptr)
			: Handle(InHandle)
			, bLaunched(false)
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
		}

		auto& GetOnDone() const
		{
			return Handle.promise().GetOnDoneEvent();
		}

		FORCENOINLINE bool await_ready()
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			return false;
		}
	
		FORCENOINLINE R await_resume()
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			Super::CheckForException();
		
			if constexpr (TIsSame<R, void>::Value)
			{
				return;
			}
			else
			{
				R Value = Super::Result.GetValue();
				return Value;
			}
		}

	
		template<typename P>
		FORCENOINLINE void await_suspend(std::coroutine_handle<P> Continuation)
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			auto& Promise = Handle.promise();
		
			if constexpr (TIsSame<R, void>::Value)
			{
				Promise.OnDone.AddLambda([this, Continuation] () mutable 
				{
					TTask_Base<R>::bHasResult = true;
					Continuation.resume();
				});
			}
			else
			{
				Promise.OnDone.AddLambda([this, Continuation] (R InResult) mutable
				{
					TTask_Base<R>::Result = InResult;
					Continuation.resume();
				});	
			}

			// Super::SubscribeForException(Promise, Continuation);
		
		}

		FORCENOINLINE bool Launch()
		{
			UE_LOG(LogTemp, Log, TEXT("S %s"), *FString(ANSI_TO_TCHAR(__FUNCTION__)));
			check(Handle != nullptr);
			const bool bWasLaunched = bLaunched;
			bLaunched = true;
			if ensureMsgf(!bWasLaunched, TEXT("Task already launched"))
			{
				auto& Promise = Handle.promise();
				Super::SubscribeForException(Promise, Handle);
				Handle.resume();
			}
			return bLaunched;
		}

		FORCENOINLINE auto& operator co_await()
		{
			Launch();
			return *this;
		}
	
	protected:
		HandleType Handle;
		bool bLaunched;
	};


	struct UE_NODISCARD COROTASKS_API FFuture_Base
	{
	
	public:
		explicit FFuture_Base();
		UE_NONCOPYABLE(FFuture_Base);
	
		virtual ~FFuture_Base();

		bool ShouldResume() const;
	
		bool await_ready() { return ShouldResume(); }

		template<typename PromiseType>
		void await_suspend(std::coroutine_handle<PromiseType> Continuation)
		{
			CoroutineHandle = Continuation;
		}

		bool bResumed;

		FORCENOINLINE void Resume()
		{
			if ensureMsgf(!bResumed, TEXT("Future already resumed"))
				CoroutineHandle.resume();
			bResumed = true;
		}

		DECLARE_DELEGATE_RetVal(bool, FHasResult);
		FHasResult HasResult;
	
		void SetException(std::exception_ptr ExcPtr);

		template<typename T>
		void SetException(T&& Exception)
		{
			SetException(std::make_exception_ptr(MoveTemp(Exception)));
		}

		virtual bool ResultIsSet() { return false; }
	protected:

		void ThrowIfException() const;
	
		FDelegateHandle ExceptionDelegateHandle;
		std::exception_ptr Exception;
		std::coroutine_handle<> CoroutineHandle;

		bool bResultIsSet;
	};



	template<typename TReturnValue>
	struct TFuture_Base : FFuture_Base
	{
		explicit TFuture_Base()
			: FFuture_Base()
		{}
	
		TOptional<TReturnValue> Result;
	};

	template<>
	struct TFuture_Base<void> : FFuture_Base
	{
		explicit TFuture_Base()
			: FFuture_Base()
			, bHasResult(false)
		{
		}

		bool bHasResult;
	};


	template<typename TReturnValue>
	struct UE_NODISCARD TFuture : public TFuture_Base<TReturnValue>
	{
		using Super = TFuture_Base<TReturnValue>;
		explicit TFuture()
			: Super()
		{
		}
	
		UE_NONCOPYABLE(TFuture);

		auto await_resume()
		{		
			return GetResult();
		}

		template<typename T = TReturnValue>
		typename TEnableIf<!TIsSame<T, void>::Value, void>::Type
		SetResult(T&& InResult)
		{
			const bool bHasResult = Super::Result.IsSet(); 
			check(!bHasResult);
			if (bHasResult)
				return;
			
			SetResult_Internal<T>(Forward<T>(InResult));
			Super::Resume();
		}

		template<typename T = TReturnValue>
		typename TEnableIf<TIsSame<T, void>::Value, void>::Type
		SetResult()
		{
			const bool bHasResult = Super::bHasResult; 
			check(!bHasResult);
			if (bHasResult)
				return;
			
			SetResult_Internal();
			Super::Resume();
		}
	
	protected:
		auto GetResult()
		{
			Super::ThrowIfException();
			if constexpr (TIsSame<TReturnValue, void>::Value)
			{
				return;
			} else
			{
				check(Super::Result.IsSet());
				return Super::Result.GetValue();
			}
		}

		
		template<typename T = TReturnValue>
		typename TEnableIf<TIsSame<T, void>::Value, void>::Type
		SetResult_Internal()
		{
			Super::bHasResult = true;
		}


		template<typename T = TReturnValue>
		typename TEnableIf<!TIsSame<T, void>::Value, void>::Type
		SetResult_Internal(T&& InResult)
		{
			Super::Result.Emplace(Forward<T>(InResult));
		}
	};
}

template<typename T>
CoroTasks::TFuture<T>& operator co_await(TSharedRef<CoroTasks::TFuture<T>> InSharedRef)
{
	return InSharedRef.Get();
}


template<typename T>
auto operator co_await(const TObjectPtr<T>& ObjectPtr) ->
	decltype(ObjectPtr->operator co_await())&
{
	return ObjectPtr->operator co_await();
}

template<typename T>
auto operator co_await(const TWeakObjectPtr<T>& ObjectPtr) ->
	decltype(ObjectPtr->operator co_await())&
{
	return ObjectPtr->operator co_await();
}