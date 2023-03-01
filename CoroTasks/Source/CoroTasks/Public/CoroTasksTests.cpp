#include "CoroTasksTests.h"

#include "AsyncException.h"


bool FNetworkedTests_RunAsyncTest::Update()
{
	if (!bExecuted)
	{
		bExecuted = true;
		Test.LaunchTest(Parameters);
	}

	if (Delegate.IsBound())
		return Delegate.Execute();
	
	return false;
}

bool FAsyncAutomationTestBase::RunTest(const FString& Parameters)
{
	bIsFinished = false;
	return StartNetworkedTest(Parameters);
}

bool FAsyncAutomationTestBase::StartNetworkedTest(const FString& Parameters)
{
	FinishedDelegate = FSimpleDelegate_Bool::CreateLambda([this]
	{
		return bIsFinished;
	});

	DummyCommand = MakeShareable(new FNetworkedTests_RunAsyncTest(*this, Parameters, FinishedDelegate, false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(DummyCommand);
	return true;
}

bool FAsyncAutomationTestBase::LaunchTest(const FString& Parameters)
{
	AsyncTest(Parameters).Launch();
	return true;
}

CoroTasks::TTask<void> FAsyncAutomationTestBase::AsyncTest(const FString Parameters)
{
	SetSuccessState(true);
	bSuppressLogs = true;
	try
	{
		co_await RunTest_Async(Parameters);
	} catch (const FAsyncTestException& Exc)
	{
		bSuppressLogs = false;
		const FString& ErrorMessage = Exc.GetMessage();
		UE_LOG(LogTemp, Error, TEXT("Test failed with reason: %s"), *ErrorMessage);
		AddError(ErrorMessage);
		SetSuccessState(false);
	}
	bIsFinished = true;
}
