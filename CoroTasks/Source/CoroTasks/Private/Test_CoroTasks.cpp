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

#include "AsyncException.h"
#include "CoroTasksTests.h"
#include "CoroTasksTestsSettings.h"
#include "LoadAsset.h"

IMPLEMENT_ASYNC_AUTOMATION_TEST(Test_CoroTasks, "CoroTasks.TestGeneral", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

CoroTasks::TTask<void> Test_CoroTasks::RunTest_Async(const FString Parameters)
{
	const TSoftObjectPtr<UObject> SoftObjectToLoad = GetDefault<UCoroTasksTestsSettings>()->TestObjectToLoad;

	if (SoftObjectToLoad.IsNull())
		throw FAsyncTestException(TEXT("Can't find asset"));

	const UObject* Object = co_await CoroTasks::LoadSingleObject(SoftObjectToLoad);

	if (Object == nullptr)
		throw FAsyncTestException(TEXT("Something went wrong"));
	
}
