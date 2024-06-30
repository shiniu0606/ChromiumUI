// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChromiumNativeJSScripting.h"
#include "Backends/JsonStructDeserializerBackend.h"
#include "Serialization/MemoryReader.h"

class FChromiumNativeJSStructDeserializerBackend
	: public FJsonStructDeserializerBackend
{
public:
	FChromiumNativeJSStructDeserializerBackend(FChromiumNativeJSScriptingRef InScripting, FMemoryReader& Reader);

	virtual bool ReadProperty( FProperty* Property, FProperty* Outer, void* Data, int32 ArrayIndex ) override;

private:
	FChromiumNativeJSScriptingRef Scripting;

};
