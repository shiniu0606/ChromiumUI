// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_CEF3

#include "ChromiumWebBrowserSingleton.h"
#include "UObject/UnrealType.h"
#include "IStructDeserializerBackend.h"
#include "ChromiumCEFJSScripting.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#endif
#pragma push_macro("OVERRIDE")
#undef OVERRIDE // cef headers provide their own OVERRIDE macro
THIRD_PARTY_INCLUDES_START
#if PLATFORM_APPLE
PRAGMA_DISABLE_DEPRECATION_WARNINGS
#endif
#include "include/cef_values.h"
#if PLATFORM_APPLE
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
THIRD_PARTY_INCLUDES_END
#pragma pop_macro("OVERRIDE")
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#endif

class FChromiumCEFJSScripting;
class IStructDeserializerBackend;
enum class EStructDeserializerBackendTokens;
class FProperty;
class UStruct;

#if WITH_CEF3

class IChromiumCefContainerWalker
	: public TSharedFromThis<IChromiumCefContainerWalker>
{
public:
	IChromiumCefContainerWalker(TSharedPtr<IChromiumCefContainerWalker> InParent)
		: Parent(InParent)
	{}
	virtual ~IChromiumCefContainerWalker() {}

	virtual TSharedPtr<IChromiumCefContainerWalker> GetNextToken( EStructDeserializerBackendTokens& OutToken, FString& PropertyName ) = 0;
	virtual bool ReadProperty(TSharedPtr<FChromiumCEFJSScripting> Scripting, FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex) = 0;

	TSharedPtr<IChromiumCefContainerWalker> Parent;
};

class FChromiumCefListValueWalker
	: public IChromiumCefContainerWalker
{
public:
	FChromiumCefListValueWalker(TSharedPtr<IChromiumCefContainerWalker> InParent, CefRefPtr<CefListValue> InList)
		: IChromiumCefContainerWalker(InParent)
		, List(InList)
		, Index(-2)
	{}

	virtual TSharedPtr<IChromiumCefContainerWalker> GetNextToken( EStructDeserializerBackendTokens& OutToken, FString& PropertyName ) override;
	virtual bool ReadProperty(TSharedPtr<FChromiumCEFJSScripting> Scripting, FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex ) override;

	CefRefPtr<CefListValue> List;
	size_t Index;
};

class FChromiumCefDictionaryValueWalker
	: public IChromiumCefContainerWalker
{
public:
	FChromiumCefDictionaryValueWalker(TSharedPtr<IChromiumCefContainerWalker> InParent, CefRefPtr<CefDictionaryValue> InDictionary)
		: IChromiumCefContainerWalker(InParent)
		, Dictionary(InDictionary)
		, Index(-2)
	{
		Dictionary->GetKeys(Keys);
	}

	virtual TSharedPtr<IChromiumCefContainerWalker> GetNextToken( EStructDeserializerBackendTokens& OutToken, FString& PropertyName ) override;
	virtual bool ReadProperty(TSharedPtr<FChromiumCEFJSScripting> Scripting, FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex ) override;

private:
	CefRefPtr<CefDictionaryValue> Dictionary;
	size_t Index;
	CefDictionaryValue::KeyList Keys;
};

/**
 * Implements a writer for UStruct serialization using CefDictionary.
 */
class FChromiumCEFJSStructDeserializerBackend
	: public IStructDeserializerBackend
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param Archive The archive to deserialize from.
	 */
	FChromiumCEFJSStructDeserializerBackend(TSharedPtr<FChromiumCEFJSScripting> InScripting, CefRefPtr<CefDictionaryValue> InDictionary)
		: Scripting(InScripting)
		, Walker(new FChromiumCefDictionaryValueWalker(nullptr, InDictionary))
		, CurrentPropertyName()
	{ }

public:

	// IStructDeserializerBackend interface

	virtual const FString& GetCurrentPropertyName() const override;
	virtual FString GetDebugString() const override;
	virtual const FString& GetLastErrorMessage() const override;
	virtual bool GetNextToken( EStructDeserializerBackendTokens& OutToken ) override;
	virtual bool ReadProperty( FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex ) override;
	virtual void SkipArray() override;
	virtual void SkipStructure() override;

private:
	TSharedPtr<FChromiumCEFJSScripting> Scripting;
	/** Holds the source CEF dictionary containing a serialized verion of the structure. */
	TSharedPtr<IChromiumCefContainerWalker> Walker;
	FString CurrentPropertyName;
};

#endif
