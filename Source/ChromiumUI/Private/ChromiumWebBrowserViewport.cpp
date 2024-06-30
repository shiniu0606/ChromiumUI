// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumWebBrowserViewport.h"

#include "Textures/SlateShaderResource.h"
#include "Widgets/SWidget.h"
#include "IChromiumWebBrowserWindow.h"
#include "Layout/WidgetPath.h"

#if WITH_CEF3
#include "CEF/ChromiumCEFWebBrowserWindow.h"
#endif

FIntPoint FChromiumWebBrowserViewport::GetSize() const
{
	return (WebBrowserWindow->GetTexture(bIsPopup) != nullptr)
		? FIntPoint(WebBrowserWindow->GetTexture(bIsPopup)->GetWidth(), WebBrowserWindow->GetTexture(bIsPopup)->GetHeight())
		: FIntPoint();
}

FSlateShaderResource* FChromiumWebBrowserViewport::GetViewportRenderTargetTexture() const
{
	return WebBrowserWindow->GetTexture(bIsPopup);
}

void FChromiumWebBrowserViewport::Tick( const FGeometry& AllottedGeometry, double InCurrentTime, float DeltaTime )
{
	if (!bIsPopup)
	{
		const float DPI = (WebBrowserWindow->GetParentWindow().IsValid() ? WebBrowserWindow->GetParentWindow()->GetNativeWindow()->GetDPIScaleFactor() : 1.0f);
		const float DPIScale = AllottedGeometry.Scale / DPI;
		FVector2D AbsoluteSize = AllottedGeometry.GetLocalSize() * DPIScale;
		WebBrowserWindow->SetViewportSize(AbsoluteSize.IntPoint(), AllottedGeometry.GetAbsolutePosition().IntPoint());

#if WITH_CEF3
		// Forward the AllottedGeometry to the WebBrowserWindow so the IME implementation can use it
		TSharedPtr<FChromiumCEFWebBrowserWindow> CefWebBrowserWindow = StaticCastSharedPtr<FChromiumCEFWebBrowserWindow>(WebBrowserWindow);
		CefWebBrowserWindow->UpdateCachedGeometry(AllottedGeometry);
#endif
	}
}

bool FChromiumWebBrowserViewport::RequiresVsync() const
{
	return false;
}

FCursorReply FChromiumWebBrowserViewport::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent )
{
	return WebBrowserWindow->OnCursorQuery(MyGeometry, CursorEvent);
}

FReply FChromiumWebBrowserViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Capture mouse on left button down so that you can drag out of the viewport
	FReply Reply = WebBrowserWindow->OnMouseButtonDown(MyGeometry, MouseEvent, bIsPopup);
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FWidgetPath* Path = MouseEvent.GetEventPath();
		if (Path->IsValid())
		{
			TSharedRef<SWidget> TopWidget = Path->Widgets.Last().Widget;
			return Reply.CaptureMouse(TopWidget);
		}
	}
	return Reply;
}

FReply FChromiumWebBrowserViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Release mouse capture when left button released
	FReply Reply = WebBrowserWindow->OnMouseButtonUp(MyGeometry, MouseEvent, bIsPopup);
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return Reply.ReleaseMouseCapture();
	}
	return Reply;
}

void FChromiumWebBrowserViewport::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
}

void FChromiumWebBrowserViewport::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	WebBrowserWindow->OnMouseLeave(MouseEvent);
}

FReply FChromiumWebBrowserViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WebBrowserWindow->OnMouseMove(MyGeometry, MouseEvent, bIsPopup);
}

FReply FChromiumWebBrowserViewport::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WebBrowserWindow->OnMouseWheel(MyGeometry, MouseEvent, bIsPopup);
}

FReply FChromiumWebBrowserViewport::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = WebBrowserWindow->OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent, bIsPopup);
	return Reply;
}

FReply FChromiumWebBrowserViewport::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return WebBrowserWindow->OnKeyDown(InKeyEvent) ? FReply::Handled() : FReply::Unhandled();
}

FReply FChromiumWebBrowserViewport::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return WebBrowserWindow->OnKeyUp(InKeyEvent) ? FReply::Handled() : FReply::Unhandled();
}

FReply FChromiumWebBrowserViewport::OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent )
{
	return WebBrowserWindow->OnKeyChar(InCharacterEvent) ? FReply::Handled() : FReply::Unhandled();
}

FReply FChromiumWebBrowserViewport::OnFocusReceived(const FFocusEvent& InFocusEvent)
{
	WebBrowserWindow->OnFocus(true, bIsPopup);
	return FReply::Handled();
}

void FChromiumWebBrowserViewport::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	WebBrowserWindow->OnFocus(false, bIsPopup);
}
