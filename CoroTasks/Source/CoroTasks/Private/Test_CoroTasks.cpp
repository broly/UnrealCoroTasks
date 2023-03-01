#include "AsyncException.h"
#include "CoroTasksTests.h"
#include "CoroTasksTestsSettings.h"
#include "LoadAsset.h"

IMPLEMENT_ASYNC_AUTOMATION_TEST(Test_CoroTasks, "CoroTasks.Test", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

CoroTasks::TTask<void> Test_CoroTasks::RunTest_Async(const FString Parameters)
{
	const TSoftObjectPtr<UObject> SoftObjectToLoad = GetDefault<UCoroTasksTestsSettings>()->TestObjectToLoad;

	if (SoftObjectToLoad.IsNull())
		throw FAsyncTestException(TEXT("Can't find asset"));

	const UObject* Object = co_await CoroTasks::LoadSingleObject(SoftObjectToLoad);

	if (Object == nullptr)
		throw FAsyncTestException(TEXT("Something went wrong"));
	
}
