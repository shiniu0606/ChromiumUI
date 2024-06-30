// Copyright Epic Games, Inc. All Rights Reserved.

#include "Native/ChromiumNativeJSStructSerializerBackend.h"

#include "ChromiumNativeJSScripting.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyPortFlags.h"
#include "Templates/Casts.h"

void FChromiumNativeJSStructSerializerBackend::WriteProperty(const FStructSerializerState& State, int32 ArrayIndex)
{
	// The parent class serialzes UObjects as NULLs
	if (State.FieldType == FObjectProperty::StaticClass())
	{
		WriteUObject(State, CastFieldChecked<FObjectProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	// basic property type (json serializable)
	else
	{
		FJsonStructSerializerBackend::WriteProperty(State, ArrayIndex);
	}
}

void FChromiumNativeJSStructSerializerBackend::WriteUObject(const FStructSerializerState& State, UObject* Value)
{
	// Note this function uses WriteRawJSONValue to append non-json data to the output stream.
	FString RawValue = Scripting->ConvertObject(Value);
	if ((State.ValueProperty == nullptr) || (State.ValueProperty->ArrayDim > 1) || State.ValueProperty->GetOwner< FArrayProperty>())
	{
		GetWriter()->WriteRawJSONValue(RawValue);
	}
	else if (State.KeyProperty != nullptr)
	{
		FString KeyString;
		State.KeyProperty->ExportTextItem(KeyString, State.KeyData, nullptr, nullptr, PPF_None);
		GetWriter()->WriteRawJSONValue(KeyString, RawValue);
	}
	else
	{
		GetWriter()->WriteRawJSONValue(Scripting->GetBindingName(State.ValueProperty), RawValue);
	}
}

FChromiumNativeJSStructSerializerBackend::FChromiumNativeJSStructSerializerBackend(TSharedRef<class FChromiumNativeJSScripting> InScripting, FMemoryWriter& Writer)
	: FJsonStructSerializerBackend(Writer, EStructSerializerBackendFlags::Default)
	, Scripting(InScripting)
{
}