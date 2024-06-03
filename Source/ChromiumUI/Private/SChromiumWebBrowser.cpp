// Copyright Epic Games, Inc. All Rights Reserved.

#include "SChromiumWebBrowser.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SThrobber.h"
#include "ChromiumWebBrowserModule.h"
#include "IChromiumWebBrowserWindow.h"
#include "IChromiumWebBrowserPopupFeatures.h"
#define LOCTEXT_NAMESPACE "ChromiumWebBrowser"

SChromiumWebBrowser::SChromiumWebBrowser()
{
	bMouseTransparency = false;
	bVirtualPointerTransparency = false;

	TransparencyDelay = 0.0f;
	TransparencyThreadshold = 0.333f;

	LastMousePixel = FLinearColor::White;
	LastMouseTime = 0.0f;
}

SChromiumWebBrowser::~SChromiumWebBrowser()
{
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	for (TPair<TWeakPtr<IChromiumWebBrowserWindow>, TWeakPtr<SWindow>> Temp : BrowserWindowWidgets)
	{
		if (Temp.Key.IsValid())
		{
			TSharedPtr<IChromiumWebBrowserWindow> WebBrowserWindow = Temp.Key.Pin();
			if (WebBrowserWindow.IsValid())
				WebBrowserWindow->CloseBrowser(false);
		}

		if (Temp.Value.IsValid())
		{
			TSharedPtr<SWindow> Window = Temp.Value.Pin();
			if (Window.IsValid())
				Window->RequestDestroyWindow();
		}
	}
#endif
}

void SChromiumWebBrowser::Construct(const FArguments& InArgs, const TSharedPtr<IChromiumWebBrowserWindow>& InBrowserWindow)
{
	OnLoadCompleted = InArgs._OnLoadCompleted;
	OnLoadError = InArgs._OnLoadError;
	OnLoadStarted = InArgs._OnLoadStarted;
	OnTitleChanged = InArgs._OnTitleChanged;
	OnUrlChanged = InArgs._OnUrlChanged;
	OnBeforeNavigation = InArgs._OnBeforeNavigation;
	OnLoadUrl = InArgs._OnLoadUrl;
	OnShowDialog = InArgs._OnShowDialog;
	OnDismissAllDialogs = InArgs._OnDismissAllDialogs;
	OnBeforePopup = InArgs._OnBeforePopup;
	OnCreateWindow = InArgs._OnCreateWindow;
	OnCloseWindow = InArgs._OnCloseWindow;
	bShowInitialThrobber = InArgs._ShowInitialThrobber;
	
	bMouseTransparency = InArgs._EnableMouseTransparency;
	bVirtualPointerTransparency = InArgs._EnableVirtualPointerTransparency;

	TransparencyDelay = FMath::Max(0.0f, InArgs._TransparencyDelay);
	TransparencyThreadshold = FMath::Clamp(InArgs._TransparencyThreshold, 0.0f, 1.0f);

	
	if (InBrowserWindow)
	{
		BrowserWindow = InBrowserWindow;
	}
	else {
		FChromiumCreateBrowserWindowSettings Settings;

		Settings.BrowserFrameRate = FMath::Clamp(InArgs._BrowserFrameRate, 1, 60);
		Settings.bUseTransparency = true;
		Settings.BackgroundColor = InArgs._BackgroundColor;
		Settings.InitialURL = InArgs._InitialURL;
		Settings.ContentsToLoad = InArgs._ContentsToLoad;
		Settings.bShowErrorMessage = UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG;
		Settings.bThumbMouseButtonNavigation = false;

		IChromiumWebBrowserSingleton* Singleton = IChromiumWebBrowserModule::Get().GetSingleton();
		if (Singleton)
		{
			BrowserWindow = Singleton->CreateBrowserWindow(Settings);
		}
	}
	

	IChromiumWebBrowserSingleton* Singleton = IChromiumWebBrowserModule::Get().GetSingleton();
	if (Singleton)
	{
		Singleton->SetDevToolsShortcutEnabled(UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG);
	}

	ChildSlot
	[
		SAssignNew(BrowserView, SChromiumWebBrowserView, BrowserWindow)
		.ParentWindow(InArgs._ParentWindow)
		.InitialURL(InArgs._InitialURL)
		.ContentsToLoad(InArgs._ContentsToLoad)
		.ShowErrorMessage(InArgs._ShowErrorMessage)
		.SupportsTransparency(InArgs._SupportsTransparency)
		.SupportsThumbMouseButtonNavigation(InArgs._SupportsThumbMouseButtonNavigation)
		.BackgroundColor(InArgs._BackgroundColor)
		.PopupMenuMethod(InArgs._PopupMenuMethod)
		.ViewportSize(InArgs._ViewportSize)
		.OnLoadCompleted(OnLoadCompleted)
		.OnLoadError(OnLoadError)
		.OnLoadStarted(OnLoadStarted)
		.OnTitleChanged(OnTitleChanged)
		.OnUrlChanged(OnUrlChanged)
		.OnBeforePopup(OnBeforePopup)
		.OnCreateWindow(this, &SChromiumWebBrowser::HandleCreateWindow)
		.OnCloseWindow(this, &SChromiumWebBrowser::HandleCloseWindow)
		.OnBeforeNavigation(OnBeforeNavigation)
		.OnLoadUrl(OnLoadUrl)
		.OnShowDialog(OnShowDialog)
		.OnDismissAllDialogs(OnDismissAllDialogs)
		.Visibility(this, &SChromiumWebBrowser::GetViewportVisibility)
		.OnSuppressContextMenu(InArgs._OnSuppressContextMenu)
		.OnDragWindow(InArgs._OnDragWindow)
		.BrowserFrameRate(InArgs._BrowserFrameRate)
	];
}

bool SChromiumWebBrowser::HandleCreateWindow(const TWeakPtr<IChromiumWebBrowserWindow>& NewBrowserWindow, const TWeakPtr<IChromiumWebBrowserPopupFeatures>& PopupFeatures)
{
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	if (!PopupFeatures.IsValid())
		return false;

	TSharedPtr<IChromiumWebBrowserPopupFeatures> PopupFeaturesSP = PopupFeatures.Pin();
	if (!PopupFeaturesSP.IsValid())
		return false;

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(SharedThis(this));
	if (!ParentWindow.IsValid())
		return false;

	const int PosX = PopupFeaturesSP->IsXSet() ? PopupFeaturesSP->GetX() : 100;
	const int PosY = PopupFeaturesSP->IsYSet() ? PopupFeaturesSP->GetY() : 100;
	const FVector2D BrowserWindowPosition(PosX, PosY);

	const int Width = PopupFeaturesSP->IsWidthSet() ? PopupFeaturesSP->GetWidth() : 800;
	const int Height = PopupFeaturesSP->IsHeightSet() ? PopupFeaturesSP->GetHeight() : 600;
	const FVector2D BrowserWindowSize(Width, Height);

	const ESizingRule SizingRule = PopupFeaturesSP->IsResizable() ?
		ESizingRule::UserSized :
		ESizingRule::FixedSize;

	TSharedPtr<IChromiumWebBrowserWindow> NewBrowserWindowSP = NewBrowserWindow.Pin();
	if (!NewBrowserWindowSP.IsValid())
		return false;

	TSharedRef<SWindow> NewWindow =
		SNew(SWindow)
		.Title(FText::GetEmpty())
		.ClientSize(BrowserWindowSize)
		.ScreenPosition(BrowserWindowPosition)
		.AutoCenter(EAutoCenter::None)
		.SizingRule(SizingRule)
		.SupportsMaximize(SizingRule != ESizingRule::FixedSize)
		.SupportsMinimize(SizingRule != ESizingRule::FixedSize)
		.HasCloseButton(true)
		.CreateTitleBar(true)
		.IsInitiallyMaximized(PopupFeaturesSP->IsFullscreen())
		.LayoutBorder(FMargin(0));

	TSharedPtr<SChromiumWebBrowser> WebBrowser;
	NewWindow->SetContent(
		SNew(SBorder)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.Padding(0)
		[
			SAssignNew(WebBrowser, SChromiumWebBrowser, NewBrowserWindowSP)
			.ShowControls(false)
		.ShowAddressBar(false)
		.OnCreateWindow(this, &SChromiumWebBrowser::HandleCreateWindow)
		.OnCloseWindow(this, &SChromiumWebBrowser::HandleCloseWindow)
		]);

	{
		struct FLocal
		{
			static void RequestDestroyWindowOverride(const TSharedRef<SWindow>& Window, TWeakPtr<IChromiumWebBrowserWindow> BrowserWindowPtr)
			{
				TSharedPtr<IChromiumWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
				if (BrowserWindow.IsValid())
				{
					if (BrowserWindow->IsClosing())
						FSlateApplicationBase::Get().RequestDestroyWindow(Window);
					else
						BrowserWindow->CloseBrowser(false);
				}
			}
		};

		NewWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateStatic(&FLocal::RequestDestroyWindowOverride, TWeakPtr<IChromiumWebBrowserWindow>(NewBrowserWindow)));
	}

	FSlateApplication::Get().AddWindow(NewWindow);
	NewWindow->BringToFront();
	FSlateApplication::Get().SetKeyboardFocus(WebBrowser, EFocusCause::SetDirectly);

	BrowserWindowWidgets.Add(NewBrowserWindow, NewWindow);
	return true;
#else
	return false;
#endif
}

bool SChromiumWebBrowser::HandleCloseWindow(const TWeakPtr<IChromiumWebBrowserWindow>& BrowserWindowPtr)
{
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	if (!BrowserWindowPtr.IsValid())
		return false;

	TSharedPtr<IChromiumWebBrowserWindow> WebBrowserWindow = BrowserWindowPtr.Pin();
	if (!WebBrowserWindow.IsValid())
		return false;

	if (WebBrowserWindow->IsClosing())
	{
		const TWeakPtr<SWindow>* FoundWebBrowserWindow = BrowserWindowWidgets.Find(WebBrowserWindow);
		if (FoundWebBrowserWindow != nullptr)
		{
			TSharedPtr<SWindow> FoundWindow = FoundWebBrowserWindow->Pin();
			if (FoundWindow.IsValid())
				FoundWindow->RequestDestroyWindow();

			BrowserWindowWidgets.Remove(WebBrowserWindow);
			return true;
		}
	}
	else
		WebBrowserWindow->CloseBrowser(false);

	return false;
#else
	return false;
#endif
}

void SChromiumWebBrowser::LoadURL(FString NewURL)
{
	if (BrowserView.IsValid())
	{
		BrowserView->LoadURL(NewURL);
	}
}

void SChromiumWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if (BrowserView.IsValid())
	{
		BrowserView->LoadString(Contents, DummyURL);
	}
}

void SChromiumWebBrowser::Reload()
{
	if (BrowserView.IsValid())
	{
		BrowserView->Reload();
	}
}

void SChromiumWebBrowser::StopLoad()
{
	if (BrowserView.IsValid())
	{
		BrowserView->StopLoad();
	}
}

FText SChromiumWebBrowser::GetTitleText() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->GetTitleText();
	}
	return LOCTEXT("InvalidWindow", "Browser Window is not valid/supported");
}

FString SChromiumWebBrowser::GetUrl() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->GetUrl();
	}

	return FString();
}

FText SChromiumWebBrowser::GetAddressBarUrlText() const
{
	if(BrowserView.IsValid())
	{
		return BrowserView->GetAddressBarUrlText();
	}
	return FText::GetEmpty();
}

bool SChromiumWebBrowser::IsLoaded() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->IsLoaded();
	}

	return false;
}

bool SChromiumWebBrowser::IsLoading() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->IsLoading();
	}

	return false;
}

bool SChromiumWebBrowser::CanGoBack() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->CanGoBack();
	}
	return false;
}

void SChromiumWebBrowser::GoBack()
{
	if (BrowserView.IsValid())
	{
		BrowserView->GoBack();
	}
}

FReply SChromiumWebBrowser::OnBackClicked()
{
	GoBack();
	return FReply::Handled();
}

bool SChromiumWebBrowser::CanGoForward() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->CanGoForward();
	}
	return false;
}

void SChromiumWebBrowser::GoForward()
{
	if (BrowserView.IsValid())
	{
		BrowserView->GoForward();
	}
}

FReply SChromiumWebBrowser::OnForwardClicked()
{
	GoForward();
	return FReply::Handled();
}

FText SChromiumWebBrowser::GetReloadButtonText() const
{
	static FText ReloadText = LOCTEXT("Reload", "Reload");
	static FText StopText = LOCTEXT("StopText", "Stop");

	if (BrowserView.IsValid())
	{
		if (BrowserView->IsLoading())
		{
			return StopText;
		}
	}
	return ReloadText;
}

FReply SChromiumWebBrowser::OnReloadClicked()
{
	if (IsLoading())
	{
		StopLoad();
	}
	else
	{
		Reload();
	}
	return FReply::Handled();
}

void SChromiumWebBrowser::OnUrlTextCommitted( const FText& NewText, ETextCommit::Type CommitType )
{
	if(CommitType == ETextCommit::OnEnter)
	{
		LoadURL(NewText.ToString());
	}
}

void SChromiumWebBrowser::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	if (HasMouseTransparency() && FSlateApplication::IsInitialized())
	{
		LastMousePixel = FLinearColor::Transparent;
		LastMouseTime = LastMouseTime + InDeltaTime;

		TSharedPtr<ICursor> Mouse = FSlateApplication::Get().GetPlatformCursor();
		if (Mouse.IsValid() && Mouse->GetType() != EMouseCursor::None)
		{
			FVector2D MousePosition = Mouse->GetPosition();
			if (!MousePosition.ContainsNaN())
			{
				FVector2D LocalMouse = AllottedGeometry.AbsoluteToLocal(MousePosition);
				FVector2D LocalSize = AllottedGeometry.GetLocalSize();

				FVector2D LocalUV = LocalSize.X > 0.0f && LocalSize.Y > 0.0f ?
					FVector2D(LocalMouse.X / LocalSize.X, LocalMouse.Y / LocalSize.Y) :
					FVector2D();

				if (LocalUV.X >= 0.0f && LocalUV.X <= 1.0f && LocalUV.Y >= 0.0f && LocalUV.Y <= 1.0f)
				{
					int32 X = FMath::FloorToInt(LocalUV.X * GetTextureWidth());
					int32 Y = FMath::FloorToInt(LocalUV.Y * GetTextureHeight());

					FLinearColor Pixel = ReadTexturePixel(X, Y);
					if ((Pixel.A < TransparencyThreadshold && LastMousePixel.A >= TransparencyThreadshold)
						|| (Pixel.A >= TransparencyThreadshold && LastMousePixel.A < TransparencyThreadshold))
						LastMouseTime = 0.0f;

					LastMousePixel = Pixel;
				}
				else
					LastMousePixel = FLinearColor::White;
			}
		}
	}
	else
		LastMousePixel = FLinearColor::White;
}


EVisibility SChromiumWebBrowser::GetViewportVisibility() const
{
	if (!BrowserView.IsValid() || !BrowserView->IsInitialized())
		return EVisibility::Hidden;

	if (HasMouseTransparency() && LastMousePixel.A < TransparencyThreadshold && LastMouseTime >= TransparencyDelay)
		return EVisibility::HitTestInvisible;

	return EVisibility::Visible;
	
	/*if (!bShowInitialThrobber || BrowserView->IsInitialized())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;*/
}

EVisibility SChromiumWebBrowser::GetLoadingThrobberVisibility() const
{
	if (bShowInitialThrobber && !BrowserWindow->IsInitialized())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}


void SChromiumWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (BrowserView.IsValid())
	{
		BrowserView->ExecuteJavascript(ScriptText);
	}
}

void SChromiumWebBrowser::GetSource(TFunction<void (const FString&)> Callback) const
{
	if (BrowserView.IsValid())
	{
		BrowserView->GetSource(Callback);
	}
}

void SChromiumWebBrowser::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindUObject(Name, Object, bIsPermanent);
	}
}

void SChromiumWebBrowser::UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindUObject(Name, Object, bIsPermanent);
	}
}

void SChromiumWebBrowser::BindAdapter(const TSharedRef<IChromiumWebBrowserAdapter>& Adapter)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindAdapter(Adapter);
	}
}

void SChromiumWebBrowser::UnbindAdapter(const TSharedRef<IChromiumWebBrowserAdapter>& Adapter)
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindAdapter(Adapter);
	}
}

void SChromiumWebBrowser::BindInputMethodSystem(ITextInputMethodSystem* TextInputMethodSystem)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindInputMethodSystem(TextInputMethodSystem);
	}
}

void SChromiumWebBrowser::UnbindInputMethodSystem()
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindInputMethodSystem();
	}
}

void SChromiumWebBrowser::SetParentWindow(TSharedPtr<SWindow> Window)
{
	if (BrowserView.IsValid())
	{
		BrowserView->SetParentWindow(Window);
	}
}

bool SChromiumWebBrowser::HasMouseTransparency() const
{
	return bMouseTransparency && !bVirtualPointerTransparency;
}

bool SChromiumWebBrowser::HasVirtualPointerTransparency() const
{
	return bVirtualPointerTransparency;
}

float SChromiumWebBrowser::GetTransparencyDelay() const
{
	return TransparencyDelay;
}

float SChromiumWebBrowser::GetTransparencyThreshold() const
{
	return TransparencyThreadshold;
}
int32 SChromiumWebBrowser::GetTextureWidth() const
{
	if (!BrowserWindow.IsValid())
		return 0;
	
	FSlateShaderResource* Resource = BrowserWindow->GetTexture();
	if (!Resource)
		return 0;

	return Resource->GetWidth();
}

int32 SChromiumWebBrowser::GetTextureHeight() const
{
	if (!BrowserWindow.IsValid())
		return 0;

	FSlateShaderResource* Resource = BrowserWindow->GetTexture();
	if (!Resource)
		return 0;

	return Resource->GetHeight();
}

FColor SChromiumWebBrowser::ReadTexturePixel(int32 X, int32 Y) const
{
	if (X < 0 || X >= GetTextureWidth())
		return FColor::Transparent;
	if (Y < 0 || Y >= GetTextureHeight())
		return FColor::Transparent;

	TArray<FColor> Pixels = ReadTexturePixels(X, Y, 1, 1);
	if (Pixels.Num() > 0)
		return Pixels[0];

	return FColor::Transparent;
}

TArray<FColor> SChromiumWebBrowser::ReadTexturePixels(int32 X, int32 Y, int32 Width, int32 Height) const
{
	TArray<FColor> OutPixels;
	if (!BrowserWindow.IsValid())
		return OutPixels;

	FSlateShaderResource* Resource = BrowserWindow->GetTexture();
	if (!Resource || Resource->GetType() != ESlateShaderResource::NativeTexture)
		return OutPixels;

	FTexture2DRHIRef TextureRHI;
	TextureRHI = ((TSlateTexture<FTexture2DRHIRef>*)Resource)->GetTypedResource();

	struct FReadSurfaceContext
	{
		FTexture2DRHIRef Texture;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	int32 ResourceWidth = (int32)Resource->GetWidth();
	int32 ResourceHeight = (int32)Resource->GetHeight();

	X = FMath::Clamp(X, 0, ResourceWidth - 1);
	Y = FMath::Clamp(Y, 0, ResourceHeight - 1);

	Width = FMath::Clamp(Width, 1, ResourceWidth);
	Width = Width - FMath::Max(X + Width - ResourceWidth, 0);

	Height = FMath::Clamp(Height, 1, ResourceHeight);
	Height = Height - FMath::Max(Y + Height - ResourceHeight, 0);

	FReadSurfaceContext Context =
	{
		TextureRHI,
		&OutPixels,
		FIntRect(X, Y, X + Width, Y + Height),
		FReadSurfaceDataFlags()
	};

	ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)(
		[Context](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.ReadSurfaceData(
				Context.Texture,
				Context.Rect,
				*Context.OutData,
				Context.Flags
			);
		});
	FlushRenderingCommands();

	return OutPixels;
}



#undef LOCTEXT_NAMESPACE
