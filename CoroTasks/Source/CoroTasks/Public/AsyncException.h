#pragma once

struct FAsyncException
{
public:
	FAsyncException(const FString& InMessage)
		: Message(InMessage)
	{
	}

	const FString& GetMessage() const 
	{
		return Message;
	}

protected:
	FString Message;
};

struct FAsyncTestException : FAsyncException
{
	FAsyncTestException(const FString& Message)
		: FAsyncException(Message)
	{}
};
