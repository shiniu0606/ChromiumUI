﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumWebBrowser.h"
#include "SChromiumWebBrowser.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Async/TaskGraphInterfaces.h"
#include "UObject/ConstructorHelpers.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"

#include "Engine/World.h"

#if WITH_EDITOR
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialFunction.h"
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistryModule.h"
#include "PackageHelperFunctions.h"
#endif

#define LOCTEXT_NAMESPACE "ChromiumWebBrowser"

/////////////////////////////////////////////////////
// UChromiumWebBrowser

UChromiumWebBrowser::UChromiumWebBrowser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = true;

	Visibility = ESlateVisibility::SelfHitTestInvisible;
	FrameRate = 60;

	bEnableMouseTransparency = false;
	MouseTransparencyThreshold = 0.333f;
	MouseTransparencyDelay = 0.1f;

	bEnableVirtualPointerTransparency = false;
	VirtualPointerTransparencyThreshold = 0.333f;

	bCustomCursors = false;
}

void UChromiumWebBrowser::LoadURL(FString NewURL)
{
	if ( WebBrowserWidget.IsValid() )
	{
		return WebBrowserWidget->LoadURL(NewURL);
	}
}

void UChromiumWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if ( WebBrowserWidget.IsValid() )
	{
		return WebBrowserWidget->LoadString(Contents, DummyURL);
	}
}

void UChromiumWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->ExecuteJavascript(ScriptText);
	}
}

void UChromiumWebBrowser::CallJavascriptFunction(const FString& Function, const FString& Data)
{
	if (Function == "broadcast")
		return;

	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->ExecuteJavascript(FString::Printf(TEXT("ue.interface[%s](%s)"),
			*Function,
			*Data));
	}
}

FText UChromiumWebBrowser::GetTitleText() const
{
	if ( WebBrowserWidget.IsValid() )
	{
		return WebBrowserWidget->GetTitleText();
	}

	return FText::GetEmpty();
}

FString UChromiumWebBrowser::GetUrl() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetUrl();
	}

	return FString();
}

void UChromiumWebBrowser::Bind(const FString& Name, UObject* Object)
{
	if (!Object)
		return;

	if (Name.ToLower() == "interface")
		return;

#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		WebBrowserWidget->BindUObject(Name, Object);
#endif
}

void UChromiumWebBrowser::Unbind(const FString& Name, UObject* Object)
{
	if (!Object)
		return;

	if (Name.ToLower() == "interface")
		return;

#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		WebBrowserWidget->UnbindUObject(Name, Object);
#endif
}

void UChromiumWebBrowser::EnableIME()
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		WebBrowserWidget->BindInputMethodSystem(FSlateApplication::Get().GetTextInputMethodSystem());
#endif
}

void UChromiumWebBrowser::DisableIME()
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		WebBrowserWidget->UnbindInputMethodSystem();
#endif
}

void FindChildWidgetsOfType(const FString& Type, TSharedRef<SWidget> Widget, TArray<TSharedRef<SWidget>>& Array)
{
	FChildren* Children = Widget->GetChildren();
	if (!Children)
		return;

	for (int32 i = 0; i < Children->Num(); i++)
	{
		TSharedRef<SWidget> Child = Children->GetChildAt(i);
		if (Type.IsEmpty() || Child->GetTypeAsString() == Type)
			Array.Add(Child);

		FindChildWidgetsOfType(Type, Child, Array);
	}
}

void UChromiumWebBrowser::Focus(EMouseLockMode MouseLockMode)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

#if !UE_SERVER
	UWorld* World = GetWorld();
	if (!World)
		return;

	UGameViewportClient* GameViewport = World->GetGameViewport();
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameViewport)
	{
		TSharedPtr<SWidget> BrowserWidget = WebBrowserWidget;
		if (BrowserWidget.IsValid())
		{
			TSharedPtr<SViewport> ViewportWidget = GameViewport->GetGameViewportWidget();
			if (GameInstance && ViewportWidget.IsValid())
			{
				TSharedRef<SWidget>   BrowserWidgetRef = BrowserWidget.ToSharedRef();
				TSharedRef<SViewport> ViewportWidgetRef = ViewportWidget.ToSharedRef();

				TArray<TSharedRef<SWidget>> Children;
				FindChildWidgetsOfType("SViewport", BrowserWidgetRef, Children);
				if (Children.Num() == 1)
				{
					BrowserWidgetRef = Children[0];
					BrowserWidget = BrowserWidgetRef;
				}

				bool bLockMouseToViewport = MouseLockMode == EMouseLockMode::LockAlways
					|| (MouseLockMode == EMouseLockMode::LockInFullscreen && GameViewport->IsExclusiveFullscreenViewport());

				for (int32 i = 0; i < GameInstance->GetNumLocalPlayers(); i++)
				{
					ULocalPlayer* LocalPlayer = GameInstance->GetLocalPlayerByIndex(i);
					if (!LocalPlayer)
						continue;

					FReply& SlateOperations = LocalPlayer->GetSlateOperations();
					SlateOperations.SetUserFocus(BrowserWidgetRef);

					if (bLockMouseToViewport)
						SlateOperations.LockMouseToWidget(ViewportWidgetRef);
					else
						SlateOperations.ReleaseMouseLock();

					SlateOperations.ReleaseMouseCapture();
				}
			}

			FSlateApplication::Get().SetAllUserFocus(BrowserWidget, EFocusCause::SetDirectly);
			FSlateApplication::Get().SetKeyboardFocus(BrowserWidget, EFocusCause::SetDirectly);
		}

		GameViewport->SetMouseLockMode(MouseLockMode);
		GameViewport->SetIgnoreInput(true);
		GameViewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
	}
#endif
}

void UChromiumWebBrowser::Unfocus(EMouseCaptureMode MouseCaptureMode)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

#if !UE_SERVER
	UWorld* World = GetWorld();
	if (!World)
		return;

	UGameViewportClient* GameViewport = World->GetGameViewport();
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameViewport)
	{
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		FSlateApplication::Get().SetAllUserFocusToGameViewport();

		TSharedPtr<SViewport> ViewportWidget = GameViewport->GetGameViewportWidget();
		if (GameInstance && ViewportWidget.IsValid())
		{
			TSharedRef<SViewport> ViewportWidgetRef = ViewportWidget.ToSharedRef();
			for (int32 i = 0; i < GameInstance->GetNumLocalPlayers(); i++)
			{
				ULocalPlayer* LocalPlayer = GameInstance->GetLocalPlayerByIndex(i);
				if (!LocalPlayer)
					continue;

				FReply& SlateOperations = LocalPlayer->GetSlateOperations();
				SlateOperations.UseHighPrecisionMouseMovement(ViewportWidgetRef);
				SlateOperations.SetUserFocus(ViewportWidgetRef);
				SlateOperations.LockMouseToWidget(ViewportWidgetRef);
			}
		}

		GameViewport->SetMouseLockMode(EMouseLockMode::LockOnCapture);
		GameViewport->SetIgnoreInput(false);
		GameViewport->SetMouseCaptureMode(MouseCaptureMode);
	}
#endif
}

void UChromiumWebBrowser::ResetMousePosition()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UGameViewportClient* GameViewport = World->GetGameViewport();
	if (GameViewport && GameViewport->Viewport)
	{
		FIntPoint SizeXY = GameViewport->Viewport->GetSizeXY();
		GameViewport->Viewport->SetMouse(SizeXY.X / 2, SizeXY.Y / 2);
	}
}

int32 UChromiumWebBrowser::GetTextureWidth() const
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->GetTextureWidth();
#endif
	return 0;
}

int32 UChromiumWebBrowser::GetTextureHeight() const
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->GetTextureHeight();
#endif
	return 0;
}

FColor UChromiumWebBrowser::ReadTexturePixel(int32 X, int32 Y)
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->ReadTexturePixel(X, Y);
#endif
	return FColor::Transparent;
}

TArray<FColor> UChromiumWebBrowser::ReadTexturePixels(int32 X, int32 Y, int32 Width, int32 Height)
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->ReadTexturePixels(X, Y, Width, Height);
#endif
	return TArray<FColor>();
}

bool UChromiumWebBrowser::IsMouseTransparencyEnabled() const
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->HasMouseTransparency();
#endif
	return false;
}

bool UChromiumWebBrowser::IsVirtualPointerTransparencyEnabled() const
{
#if !UE_SERVER
	if (WebBrowserWidget.IsValid())
		return WebBrowserWidget->HasVirtualPointerTransparency();
#endif
	return false;
}

void UChromiumWebBrowser::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	WebBrowserWidget.Reset();
}

TSharedRef<SWidget> UChromiumWebBrowser::RebuildWidget()
{
	if ( IsDesignTime() )
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Web Browser", "Web Browser"))
			];
	}
	else
	{
		WebBrowserWidget = SNew(SChromiumWebBrowser)
			.InitialURL(InitialURL)
			.ShowControls(false)
			.SupportsTransparency(bSupportsTransparency)
			.EnableMouseTransparency(bEnableMouseTransparency)
			.TransparencyDelay(MouseTransparencyDelay)
			.TransparencyThreshold(MouseTransparencyThreshold)
			.EnableVirtualPointerTransparency(bEnableVirtualPointerTransparency)
			.OnUrlChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnUrlChanged))
			.OnBeforePopup(BIND_UOBJECT_DELEGATE(FOnBeforePopupDelegate, HandleOnBeforePopup));

#if WITH_CEF3
		WebBrowserWidget->BindUObject("interface", this);
#endif

		return WebBrowserWidget.ToSharedRef();
	}
}

void UChromiumWebBrowser::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if ( WebBrowserWidget.IsValid() )
	{

	}
}

void UChromiumWebBrowser::HandleOnUrlChanged(const FText& InText)
{
	OnUrlChanged.Broadcast(InText);
}

bool UChromiumWebBrowser::HandleOnBeforePopup(FString URL, FString Frame)
{
	if (OnBeforePopup.IsBound())
	{
		if (IsInGameThread())
		{
			OnBeforePopup.Broadcast(URL, Frame);
		}
		else
		{
			// Retry on the GameThread.
			TWeakObjectPtr<UChromiumWebBrowser> WeakThis = this;
			FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, URL, Frame]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleOnBeforePopup(URL, Frame);
				}
			}, TStatId(), nullptr, ENamedThreads::GameThread);
		}

		return true;
	}

	return false;
}

#if WITH_EDITOR

const FText UChromiumWebBrowser::GetPaletteCategory()
{
	return LOCTEXT("Experimental", "Experimental");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
