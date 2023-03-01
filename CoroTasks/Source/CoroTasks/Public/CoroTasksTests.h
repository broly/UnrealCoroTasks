#pragma once

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
	virtual CoroTasks::TTask<void> AsyncTest(const FString Parameters);
	virtual CoroTasks::TTask<void> RunTest_Async(const FString Parameters) = 0;
	
	FSimpleDelegate_Bool FinishedDelegate;

	bool bIsFinished;

	TSharedPtr<class FNetworkedTests_RunAsyncTest> DummyCommand;
};


DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FNetworkedTests_RunAsyncTest,
	FAsyncAutomationTestBase&, Test, FString, Parameters, FSimpleDelegate_Bool, Delegate, bool, bExecuted);



