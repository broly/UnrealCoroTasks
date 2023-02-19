# UnrealCoroTasks
Coroutine tasks to simplify your async (non-blocking) code avoiding delegates

# Asset async loading (comparison with classic approach)

Classic approach
```cpp
	void AsyncLoadAsset_Request(TSoftObjectPtr<UDataAsset> Asset)
	{
		// 1. Get Streamable Manager
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		
		// 2. Create the delegate
		const auto Delegate = FStreamableDelegate::CreateUObject(this, &ThisClass::AsyncLoadAsset_Response, Asset);

		// 3. Request async loading using this delegate
		const TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad({Asset.ToSoftObjectPath()},
			Delegate, FStreamableManager::DefaultAsyncLoadPriority);
	}

	// 4. You should create callback function (it can be done also by lambda)
	void AsyncLoadAsset_Response(TSoftObjectPtr<UDataAsset> AssetSoftPtr)
	{
		// 5. Only at this time we can get asset pointer
		UDataAsset* Asset = AssetSoftPtr.Get();

		// Asset is fully loaded
	}
```

CoroTasks approach
```cpp
	// 1. Define function with CoroTasks::TTask templated type
	CoroTasks::TTask<> TestCoro()
	{
		TSoftObjectPtr<UDataAsset> SoftAssetPtr;

		// 2. Just call CoroTasks::LoadSingleObject on your asset pointer
		UDataAsset* Asset = co_await CoroTasks::LoadSingleObject(SoftAssetPtr);

		// Asset is fully loaded
	}
```
