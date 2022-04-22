// Copyright Epic Games, Inc. All Rights Reserved.

#include "WebMenu.h"

#define LOCTEXT_NAMESPACE "WebBrowser"

/////////////////////////////////////////////////////
// UWebBrowser

UWebMenu::UWebMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Zoom = 1.0f;
}

void UWebMenu::SendJson(const FString& Json)
{
	if (WebBrowserWidget.IsValid())
	{
		const FString js = "if(window && window.Unreal && window.Unreal.receive) window.Unreal.receive(`" + Json + "`);";
		UWebMenu::ExecuteJavascript(js);
	}
}

float UWebMenu::GetZoom() const
{
	return Zoom;
}

bool UWebMenu::IsValid() const
{
	return WebBrowserWidget.IsValid();
}

void UWebMenu::ReceiveJson(FString data) const
{
	if (WebBrowserWidget.IsValid())
	{
		OnJsonReceived.Broadcast(FText::FromString(data));
	}
}

TSharedRef<SWidget> UWebMenu::RebuildWidget()
{
	if ( IsDesignTime() )
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Web Menu", "Web Menu"))
			];
	}
	else
	{
		WebBrowserWidget = SNew(SWebBrowser)
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

#undef LOCTEXT_NAMESPACE
