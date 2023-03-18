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

#include "CoroTask.h"
#include "Coroutine.h"
#include "Misc/AutomationTest.h"


#define IMPLEMENT_ASYNC_TEST_PRIVATE( TClass, TBaseClass, PrettyName, TFlags, FileName, LineNumber, ... ) \
	class TClass : public TBaseClass \
	{ \
	public: \
		TClass( const FString& InName ) \
		:TBaseClass( InName, true ) { \
			static_assert((TFlags)&EAutomationTestFlags::ApplicationContextMask, "AutomationTest has no application flag.  It shouldn't run.  See AutomationTest.h."); \
			static_assert(	(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::SmokeFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::EngineFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::ProductFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::PerfFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::StressFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::NegativeFilter), \
							"All AutomationTests must have exactly 1 filter type specified.  See AutomationTest.h."); \
		} \
		virtual uint32 GetTestFlags() const override { return ((TFlags) & ~(EAutomationTestFlags::SmokeFilter)); } \
		virtual bool IsStressTest() const { return true; } \
		virtual uint32 GetRequiredDeviceNum() const override { return 1; } \
		virtual FString GetTestSourceFileName() const override { return FileName; } \
		virtual int32 GetTestSourceFileLine() const override { return LineNumber; } \
	protected: \
		virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const override \
		{ \
			OutBeautifiedNames.Add(PrettyName); \
			OutTestCommands.Add(FString()); \
		} \
		virtual CoroTasks::TTask<void> RunTest_Async(const FString Parameters) override; \
		virtual FString GetBeautifiedTestName() const override { return PrettyName; } \
	};


#define IMPLEMENT_ASYNC_AUTOMATION_TEST( TClass, PrettyName, TFlags, ... ) \
	IMPLEMENT_ASYNC_TEST_PRIVATE(TClass, FAsyncAutomationTestBase, PrettyName, TFlags, __FILE__, __LINE__, ##__VA_ARGS__) \
	namespace\
	{\
		TClass TClass##AutomationTestInstance( TEXT(#TClass), ##__VA_ARGS__ );\
	}

DECLARE_DELEGATE_RetVal(bool, FSimpleDelegate_Bool);

class COROTASKS_API FAsyncAutomationTestBase : public FAutomationTestBase
{
public:
	FAsyncAutomationTestBase(const FString& InName, bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
		, bIsFinished(false)
	{
		bSuppressLogs = true;
	}

	virtual bool RunTest(const FString& Parameters) override;

	virtual bool StartNetworkedTest(const FString& Parameters);
	virtual bool LaunchTest(const FString& Parameters);
	virtual CoroTasks::TTask<bool> AsyncTest(const FString Parameters);
	virtual CoroTasks::TTask<void> RunTest_Async(const FString Parameters) = 0;
	
	FSimpleDelegate_Bool FinishedDelegate;

	bool bIsFinished;

	TSharedPtr<class FNetworkedTests_RunAsyncTest> DummyCommand;
};


DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FNetworkedTests_RunAsyncTest,
	FAsyncAutomationTestBase&, Test, FString, Parameters, FSimpleDelegate_Bool, Delegate, bool, bExecuted);



