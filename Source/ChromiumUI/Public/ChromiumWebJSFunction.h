// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"
#include "ChromiumWebJSFunction.generated.h"

class FChromiumWebJSScripting;

struct CHROMIUMUI_API FChromiumWebJSParam
{

	struct IStructWrapper
	{
		virtual ~IStructWrapper() {};
		virtual UStruct* GetTypeInfo() = 0;
		virtual const void* GetData() = 0;
		virtual IStructWrapper* Clone() = 0;
	};

	template <typename T> struct FStructWrapper
		: public IStructWrapper
	{
		T StructValue;
		FStructWrapper(const T& InValue)
			: StructValue(InValue)
		{}
		virtual ~FStructWrapper()
		{}
		virtual UStruct* GetTypeInfo() override
		{
			return T::StaticStruct();
		}
		virtual const void* GetData() override
		{
			return &StructValue;
		}
		virtual IStructWrapper* Clone() override
		{
			return new FStructWrapper<T>(StructValue);
		}
	};

	FChromiumWebJSParam() : Tag(PTYPE_NULL) {}
	FChromiumWebJSParam(bool Value) : Tag(PTYPE_BOOL), BoolValue(Value) {}
	FChromiumWebJSParam(int8 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FChromiumWebJSParam(int16 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FChromiumWebJSParam(int32 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FChromiumWebJSParam(uint8 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FChromiumWebJSParam(uint16 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FChromiumWebJSParam(uint32 Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FChromiumWebJSParam(int64 Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FChromiumWebJSParam(uint64 Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FChromiumWebJSParam(double Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FChromiumWebJSParam(float Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FChromiumWebJSParam(const FString& Value) : Tag(PTYPE_STRING), StringValue(new FString(Value)) {}
	FChromiumWebJSParam(const FText& Value) : Tag(PTYPE_STRING), StringValue(new FString(Value.ToString())) {}
	FChromiumWebJSParam(const FName& Value) : Tag(PTYPE_STRING), StringValue(new FString(Value.ToString())) {}
	FChromiumWebJSParam(const TCHAR* Value) : Tag(PTYPE_STRING), StringValue(new FString(Value)) {}
	FChromiumWebJSParam(UObject* Value) : Tag(PTYPE_OBJECT), ObjectValue(Value) {}
	template <typename T> FChromiumWebJSParam(const T& Value,
		typename TEnableIf<!TIsPointer<T>::Value, UStruct>::Type* InTypeInfo=T::StaticStruct())
		: Tag(PTYPE_STRUCT)
		, StructValue(new FStructWrapper<T>(Value))
	{}
	template <typename T> FChromiumWebJSParam(const TArray<T>& Value)
		: Tag(PTYPE_ARRAY)
	{
		ArrayValue = new TArray<FChromiumWebJSParam>();
		ArrayValue->Reserve(Value.Num());
		for(T Item : Value)
		{
			ArrayValue->Add(FChromiumWebJSParam(Item));
		}
	}
	template <typename T> FChromiumWebJSParam(const TMap<FString, T>& Value)
		: Tag(PTYPE_MAP)
	{
		MapValue = new TMap<FString, FChromiumWebJSParam>();
		MapValue->Reserve(Value.Num());
		for(const auto& Pair : Value)
		{
			MapValue->Add(Pair.Key, FChromiumWebJSParam(Pair.Value));
		}
	}
	template <typename K, typename T> FChromiumWebJSParam(const TMap<K, T>& Value)
		: Tag(PTYPE_MAP)
	{
		MapValue = new TMap<FString, FChromiumWebJSParam>();
		MapValue->Reserve(Value.Num());
		for(const auto& Pair : Value)
		{
			MapValue->Add(Pair.Key.ToString(), FChromiumWebJSParam(Pair.Value));
		}
	}
	FChromiumWebJSParam(const FChromiumWebJSParam& Other);
	FChromiumWebJSParam(FChromiumWebJSParam&& Other);
	~FChromiumWebJSParam();

	enum { PTYPE_NULL, PTYPE_BOOL, PTYPE_INT, PTYPE_DOUBLE, PTYPE_STRING, PTYPE_OBJECT, PTYPE_STRUCT, PTYPE_ARRAY, PTYPE_MAP } Tag;
	union
	{
		bool BoolValue;
		double DoubleValue;
		int32 IntValue;
		UObject* ObjectValue;
		const FString* StringValue;
		IStructWrapper* StructValue;
		TArray<FChromiumWebJSParam>* ArrayValue;
		TMap<FString, FChromiumWebJSParam>* MapValue;
	};

};

class FChromiumWebJSScripting;

/** Base class for JS callback objects. */
USTRUCT()
struct CHROMIUMUI_API FChromiumWebJSCallbackBase
{
	GENERATED_USTRUCT_BODY()
	FChromiumWebJSCallbackBase()
	{}

	bool IsValid() const
	{
		return ScriptingPtr.IsValid();
	}


protected:
	FChromiumWebJSCallbackBase(TSharedPtr<FChromiumWebJSScripting> InScripting, const FGuid& InCallbackId)
		: ScriptingPtr(InScripting)
		, CallbackId(InCallbackId)
	{}

	void Invoke(int32 ArgCount, FChromiumWebJSParam Arguments[], bool bIsError = false) const;

private:

	TWeakPtr<FChromiumWebJSScripting> ScriptingPtr;
	FGuid CallbackId;
};


/**
 * Representation of a remote JS function.
 * FWebJSFunction objects represent a JS function and allow calling them from native code.
 * FWebJSFunction objects can also be added to delegates and events using the Bind/AddLambda method.
 */
USTRUCT()
struct CHROMIUMUI_API FChromiumWebJSFunction
	: public FChromiumWebJSCallbackBase
{
	GENERATED_USTRUCT_BODY()

	FChromiumWebJSFunction()
		: FChromiumWebJSCallbackBase()
	{}

	FChromiumWebJSFunction(TSharedPtr<FChromiumWebJSScripting> InScripting, const FGuid& InFunctionId)
		: FChromiumWebJSCallbackBase(InScripting, InFunctionId)
	{}

	template<typename ...ArgTypes> void operator()(ArgTypes... Args) const
	{
		FChromiumWebJSParam ArgArray[sizeof...(Args)] = {FChromiumWebJSParam(Args)...};
		Invoke(sizeof...(Args), ArgArray);
	}
};

/** 
 *  Representation of a remote JS async response object.
 *  UFUNCTIONs taking a FChromiumWebJSResponse will get it passed in automatically when called from a web browser.
 *  Pass a result or error back by invoking Success or Failure on the object.
 *  UFunctions accepting a FChromiumWebJSResponse should have a void return type, as any value returned from the function will be ignored.
 *  Calling the response methods does not have to happen before returning from the function, which means you can use this to implement asynchronous functionality.
 *
 *  Note that the remote object will become invalid as soon as a result has been delivered, so you can only call either Success or Failure once.
 */
USTRUCT()
struct CHROMIUMUI_API FChromiumWebJSResponse
	: public FChromiumWebJSCallbackBase
{
	GENERATED_USTRUCT_BODY()

	FChromiumWebJSResponse()
		: FChromiumWebJSCallbackBase()
	{}

	FChromiumWebJSResponse(TSharedPtr<FChromiumWebJSScripting> InScripting, const FGuid& InCallbackId)
		: FChromiumWebJSCallbackBase(InScripting, InCallbackId)
	{}

	/**
	 * Indicate successful completion without a return value.
	 * The remote Promise's then() handler will be executed without arguments.
	 */
	void Success() const
	{
		Invoke(0, nullptr, false);
	}

	/**
	 * Indicate successful completion passing a return value back.
	 * The remote Promise's then() handler will be executed with the value passed as its single argument.
	 */
	template<typename T>
	void Success(T Arg) const
	{
		FChromiumWebJSParam ArgArray[1] = {FChromiumWebJSParam(Arg)};
		Invoke(1, ArgArray, false);
	}

	/**
	 * Indicate failed completion, passing an error message back to JS.
	 * The remote Promise's catch() handler will be executed with the value passed as the error reason.
	 */
	template<typename T>
	void Failure(T Arg) const
	{
		FChromiumWebJSParam ArgArray[1] = {FChromiumWebJSParam(Arg)};
		Invoke(1, ArgArray, true);
	}


};
