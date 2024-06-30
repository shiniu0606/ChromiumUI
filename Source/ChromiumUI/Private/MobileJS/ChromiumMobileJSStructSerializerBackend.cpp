// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumMobileJSStructSerializerBackend.h"

#if PLATFORM_ANDROID || PLATFORM_IOS

#include "ChromiumMobileJSScripting.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyPortFlags.h"
#include "Templates/Casts.h"

void FChromiumMobileJSStructSerializerBackend::WriteProperty(const FStructSerializerState& State, int32 ArrayIndex)
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

void FChromiumMobileJSStructSerializerBackend::WriteUObject(const FStructSerializerState& State, UObject* Value)
{
	// Note this function uses WriteRawJSONValue to append non-json data to the output stream.
	FString RawValue = Scripting->ConvertObject(Value);
	if ((State.ValueProperty == nullptr) || (State.ValueProperty->ArrayDim > 1) || (State.ValueProperty->GetOwner<FArrayProperty>() != nullptr))
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

FString FChromiumMobileJSStructSerializerBackend::ToString()
{
	ReturnBuffer.Add(0);
	ReturnBuffer.Add(0); // Add two as we're dealing with UTF-16, so 2 bytes
	return UTF16_TO_TCHAR((UTF16CHAR*)ReturnBuffer.GetData());
}

FChromiumMobileJSStructSerializerBackend::FChromiumMobileJSStructSerializerBackend(TSharedRef<class FChromiumMobileJSScripting> InScripting)
	: FJsonStructSerializerBackend(Writer, EStructSerializerBackendFlags::Legacy)
	, Scripting(InScripting)
	, ReturnBuffer()
	, Writer(ReturnBuffer)
{
}

#endif // PLATFORM_ANDROID || PLATFORM_IOS