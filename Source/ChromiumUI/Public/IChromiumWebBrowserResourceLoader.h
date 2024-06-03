// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

typedef TMap<FString, FString> FChromiumContextRequestHeaders;
DECLARE_DELEGATE_ThreeParams(FChromiumOnBeforeContextResourceLoadDelegate, FString /*Url*/, FString /*ResourceType*/, FChromiumContextRequestHeaders& /*AdditionalHeaders*/);
