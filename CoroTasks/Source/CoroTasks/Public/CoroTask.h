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
		FPromise_ExcContainer()
		{
			
		}
		virtual ~FPromise_ExcContainer()
		{
			
		}
		std::exception_ptr CurrentException;
	
		void unhandled_exception()
		{
			CurrentException = std::current_exception();
			std::rethrow_exception(CurrentException);
		}

		void ExecuteException_IfPending()
		{
			if (CurrentException)
			{
				OnException.ExecuteIfBound(CurrentException);
				OnException.Unbind();
			}
		}
	
		DECLARE_DELEGATE_OneParam(FOnException, std::exception_ptr);
		mutable FOnException OnException;
	};

	
	template<
		typename ReturnType,
		typename TaskType
	>
	struct TPromise_Base : public FPromise_ExcContainer
	{
		using Super = FPromise_ExcContainer;

		std::suspend_always initial_suspend() noexcept
		{
			return {};
		}
		std::suspend_never final_suspend() noexcept
		{
			return {};
		}
	};

	/**
	 * The templated base of delegate with return value (general case)
	 * Holds delegate that will be fired when promise is finished
	 */
	template<
		typename ReturnType,
		typename TaskType
	>
	struct TPromise_Return : TPromise_Base<ReturnType, TaskType>
	{
		using Super = TPromise_Base<ReturnType, TaskType>;

		TFunction<void(ReturnType&&)> OnReturn;

		void return_value(ReturnType&& Result)
		{
			Super::ExecuteException_IfPending();
			if (OnReturn != nullptr)
				OnReturn(Forward<ReturnType>(Result));
		}
		
	};

	template<typename TaskType>
	struct TPromise_Return<void, TaskType> : TPromise_Base<void, TaskType>
	{
		using Super = TPromise_Base<void, TaskType>;
		
		TFunction<void()> OnReturn;
	
		void return_void()
		{
			Super::ExecuteException_IfPending();
			
			if (OnReturn != nullptr)
				OnReturn();
		}

	};

	/**
	 * General promise
	 */
	template<typename ReturnType, typename TaskType>
	struct TPromise : TPromise_Return<ReturnType, TaskType>
	{
		using Super = TPromise_Return<ReturnType, TaskType>;
		
		TaskType get_return_object()
		{
			return (TaskType)(TaskType::HandleType::from_promise(*this));
		}
		
		auto& GetOnDoneEvent()
		{
			return Super::OnDone;
		}

	};


	struct FTask_ExceptionInterface
	{
		virtual ~FTask_ExceptionInterface()
		{
			
		}
		void SetException(std::exception_ptr exc)
		{
			CurrentExc = exc;
		}

		void CheckForException() const
		{
			if (CurrentExc)
				std::rethrow_exception(CurrentExc);
		}

		template<typename PromiseType, typename HandleType>
		void SubscribeForException(PromiseType& Promise, HandleType& Continuation)
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
		TOptional<R> ReturnValue = {};

		void SetResult(R&& Result)
		{
			ReturnValue.Emplace(Forward<R>(Result));
		}
		
		bool HasResult() const
		{
			return ReturnValue.IsSet();
		}

		R GetResult() const
		{
			return ReturnValue.GetValue();
		}
	};

	template<>
	struct TTask_Base<void> : FTask_ExceptionInterface
	{
		bool bHasResult = false;

		void SetResult()
		{
			bHasResult = true;
		}
		
		bool HasResult() const
		{
			return bHasResult;
		}

		void GetResult() const
		{
			check(bHasResult);
		}
	};

	template<typename R = void>
	struct TTask : TTask_Base<R>
	{
		using ReturnType = R;
	
	public:
		using Super = TTask_Base<R>;
		using promise_type = TPromise<ReturnType, TTask<R>>;
		using HandleType = std::coroutine_handle<promise_type>;

		virtual ~TTask() override
		{
			auto& Promise = Handle.promise();
			Promise.OnReturn.Reset();
			SetContinuation(nullptr);
		}

		TTask(HandleType InHandle = nullptr)
			: CurrentContinuation(nullptr)
			, Handle(InHandle)
			, bLaunched(false)
		{
			auto& Promise = Handle.promise();
			Promise.OnReturn = [this] <typename... Types>(Types&&... Args) mutable 
			{
				Super::SetResult(Forward<Types>(Args)...);
				ResumeIfNeeded();
			};
		}

		auto& GetOnDone() const
		{
			return Handle.promise().GetOnDoneEvent();
		}

		bool await_ready()
		{
			return Super::HasResult();
		}
	
		R await_resume()
		{
			Super::CheckForException();
			return Super::GetResult();
		}

	
		template<typename P>
		void await_suspend(std::coroutine_handle<P> Continuation)
		{
			SetContinuation(Continuation);
		}

		bool Launch()
		{
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

		auto& operator co_await()
		{
			Handle.resume();
			return *this;
		}
		
		void SetContinuation(std::coroutine_handle<> InContinuation)
		{
			CurrentContinuation = InContinuation;
		}

		std::coroutine_handle<> CurrentContinuation;

		void ResumeIfNeeded()
		{
			if (CurrentContinuation)
			{
				CurrentContinuation.resume();
				CurrentContinuation = nullptr;
			}
		}
	
	protected:
		HandleType Handle;
		bool bLaunched;
	};

}