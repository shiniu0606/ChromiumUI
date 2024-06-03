// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID  || PLATFORM_IOS

#include "ChromiumMobileJSScripting.h"
#include "Backends/JsonStructDeserializerBackend.h"
#include "Serialization/MemoryReader.h"

class FChromiumMobileJSStructDeserializerBackend
	: public FJsonStructDeserializerBackend
{
public:
	FChromiumMobileJSStructDeserializerBackend(FChromiumMobileJSScriptingRef InScripting, const FString& JsonString);

	virtual bool ReadProperty( FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex ) override;

private:
	FChromiumMobileJSScriptingRef Scripting;
	TArray<uint8> JsonData;
	FMemoryReader Reader;
};

#endif // USE_ANDROID_JNI || PLATFORM_IOS