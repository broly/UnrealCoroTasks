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

namespace CoroTasks
{
	struct UE_NODISCARD COROTASKS_API FFuture_Base
	{
	
	public:
		explicit FFuture_Base();
	
		virtual ~FFuture_Base();

		bool ShouldResume() const;
	
		bool await_ready()
		{
			return bResultIsSet;
		}

		template<typename PromiseType>
		void await_suspend(std::coroutine_handle<PromiseType> Continuation)
		{
			bWasSuspended = true;
			CoroutineHandle = Continuation;
		}

		bool bResumed;

		void Resume()
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
		bool bWasSuspended;
	};



	template<typename TReturnValue>
	struct TFuture_Base : FFuture_Base
	{
		using Super = FFuture_Base;
		
		explicit TFuture_Base()
			: FFuture_Base()
		{}
	
		TOptional<TReturnValue> Result;
	};

	template<>
	struct TFuture_Base<void> : FFuture_Base
	{
		using Super = FFuture_Base;
		
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
			if (Super::Super::bWasSuspended)
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
			
			if (Super::Super::bWasSuspended)
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
			Super::bResultIsSet = true;
			Super::bHasResult = true;
		}


		template<typename T = TReturnValue>
		typename TEnableIf<!TIsSame<T, void>::Value, void>::Type
		SetResult_Internal(T&& InResult)
		{
			Super::bResultIsSet = true;
			Super::Result.Emplace(Forward<T>(InResult));
		}
	};
}