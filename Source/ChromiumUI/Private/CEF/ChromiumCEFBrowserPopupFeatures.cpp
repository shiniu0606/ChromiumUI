// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEF/ChromiumCEFBrowserPopupFeatures.h"

#if WITH_CEF3

FChromiumCEFBrowserPopupFeatures::FChromiumCEFBrowserPopupFeatures()
	: X(0)
	, bXSet(false)
	, Y(0)
	, bYSet(false)
	, Width(0)
	, bWidthSet(false)
	, Height(0)
	, bHeightSet(false)
	, bMenuBarVisible(true)
	, bStatusBarVisible(false)
	, bToolBarVisible(true)
	, bLocationBarVisible(true)
	, bScrollbarsVisible(true)
	, bResizable(true)
	, bIsFullscreen(false)
	, bIsDialog(false)
{
}


FChromiumCEFBrowserPopupFeatures::FChromiumCEFBrowserPopupFeatures( const CefPopupFeatures& PopupFeatures )
{
	X = PopupFeatures.x;
	bXSet = PopupFeatures.xSet ? true : false;
	Y = PopupFeatures.y;
	bYSet = PopupFeatures.ySet ? true : false;
	Width = PopupFeatures.width;
	bWidthSet = PopupFeatures.widthSet ? true : false;
	Height = PopupFeatures.height;
	bHeightSet = PopupFeatures.heightSet ? true : false;
	bMenuBarVisible = PopupFeatures.menuBarVisible ? true : false;
	bStatusBarVisible = PopupFeatures.statusBarVisible ? true : false;
	bToolBarVisible = PopupFeatures.toolBarVisible ? true : false;
	bScrollbarsVisible = PopupFeatures.scrollbarsVisible ? true : false;
	
	// no longer set by the CEF API so default them here to their historic value
	bLocationBarVisible = false;
	bResizable = false;
	bIsFullscreen = false;
	bIsDialog = false;
}

FChromiumCEFBrowserPopupFeatures::~FChromiumCEFBrowserPopupFeatures()
{
}

void FChromiumCEFBrowserPopupFeatures::SetResizable(const bool bResize)
{
	bResizable = bResize;
}

int FChromiumCEFBrowserPopupFeatures::GetX() const
{
	return X;
}

bool FChromiumCEFBrowserPopupFeatures::IsXSet() const
{
	return bXSet;
}

int FChromiumCEFBrowserPopupFeatures::GetY() const
{
	return Y;
}

bool FChromiumCEFBrowserPopupFeatures::IsYSet() const
{
	return bYSet;
}

int FChromiumCEFBrowserPopupFeatures::GetWidth() const
{
	return Width;
}

bool FChromiumCEFBrowserPopupFeatures::IsWidthSet() const
{
	return bWidthSet;
}

int FChromiumCEFBrowserPopupFeatures::GetHeight() const
{
	return Height;
}

bool FChromiumCEFBrowserPopupFeatures::IsHeightSet() const
{
	return bHeightSet;
}

bool FChromiumCEFBrowserPopupFeatures::IsMenuBarVisible() const
{
	return bMenuBarVisible;
}

bool FChromiumCEFBrowserPopupFeatures::IsStatusBarVisible() const
{
	return bStatusBarVisible;
}

bool FChromiumCEFBrowserPopupFeatures::IsToolBarVisible() const
{
	return bToolBarVisible;
}

bool FChromiumCEFBrowserPopupFeatures::IsLocationBarVisible() const
{
	return bLocationBarVisible;
}

bool FChromiumCEFBrowserPopupFeatures::IsScrollbarsVisible() const
{
	return bScrollbarsVisible;
}

bool FChromiumCEFBrowserPopupFeatures::IsResizable() const
{
	return bResizable;
}

bool FChromiumCEFBrowserPopupFeatures::IsFullscreen() const
{
	return bIsFullscreen;
}

bool FChromiumCEFBrowserPopupFeatures::IsDialog() const
{
	return bIsDialog;
}

TArray<FString> FChromiumCEFBrowserPopupFeatures::GetAdditionalFeatures() const
{
	TArray<FString> Empty;
	return Empty;
}

#endif
