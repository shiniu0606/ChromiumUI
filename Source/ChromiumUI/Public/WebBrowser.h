// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/Widget.h"

#include "WebBrowser.generated.h"


/**
 * 
 */
UCLASS()
class CHROMIUMUI_API UWebBrowser : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUrlChanged, const FText&, Text);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBeforePopup, FString, URL, FString, Frame);

	/**
	 * Load the specified URL
	 *
	 * @param NewURL New URL to load
	 */
	UFUNCTION(BlueprintCallable, Category="Web Browser")
	void LoadURL(FString NewURL);

	/**
	 * Load a string as data to create a web page
	 *
	 * @param Contents String to load
	 * @param DummyURL Dummy URL for the page
	 */
	UFUNCTION(BlueprintCallable, Category="Web Browser")
	void LoadString(FString Contents, FString DummyURL);

	/**
	* Executes a JavaScript string in the context of the web page
	*
	* @param ScriptText JavaScript string to execute
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
	void ExecuteJavascript(const FString& ScriptText);
	// Call ue.interface.function(data) in the browser context.
	UFUNCTION(BlueprintCallable, Category = "Web Browser", meta = (AdvancedDisplay = "Data", AutoCreateRefTerm = "Data"))
	void CallJavascriptFunction(const FString& Function, const FString& Data);

	/**
	 * Get the current title of the web page
	 */
	UFUNCTION(BlueprintCallable, Category="Web Browser")
	FText GetTitleText() const;

	/**
	* Gets the currently loaded URL.
	*
	* @return The URL, or empty string if no document is loaded.
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
	FString GetUrl() const;

	UFUNCTION(BlueprintCallable, Category = "Web Browser")
	void Bind(const FString& Name, UObject* Object);
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
	void Unbind(const FString& Name, UObject* Object);

	// Enables input method editors for different languages.
	UFUNCTION(BlueprintCallable, Category = "Web Browser|Input")
	void EnableIME();
	// Disables input method editors for different languages.
	UFUNCTION(BlueprintCallable, Category = "Web Browser|Input")
	void DisableIME();

	// Set focus to the browser.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "MouseLockMode"), Category = "Web Browser|Helpers")
	void Focus(EMouseLockMode MouseLockMode = EMouseLockMode::LockOnCapture);
	// Set focus to the game.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "MouseCaptureMode"), Category = "Web Browser|Helpers")
	void Unfocus(EMouseCaptureMode MouseCaptureMode = EMouseCaptureMode::CapturePermanently);
	// Reset cursor to center of the viewport.
	UFUNCTION(BlueprintCallable, Category = "Web Browser|Helpers")
	void ResetMousePosition();

	// Get the width of the browser texture.
	UFUNCTION(BlueprintPure, Category = "Web Browser|Textures")
		int32 GetTextureWidth() const;
	// Get the height of the browser texture.
	UFUNCTION(BlueprintPure, Category = "Web Browser|Textures")
		int32 GetTextureHeight() const;
	// Read a pixel from the browser texture.
	UFUNCTION(BlueprintCallable, Category = "Web Browser|Textures")
		FColor ReadTexturePixel(int32 X, int32 Y);
	// Read an area of pixels from the browser texture.
	UFUNCTION(BlueprintCallable, Category = "Web Browser|Textures")
		TArray<FColor> ReadTexturePixels(int32 X, int32 Y, int32 Width, int32 Height);

	// Check if mouse transparency is enabled.
	UFUNCTION(BlueprintPure, Category = "Web Browser|Transparency")
	bool IsMouseTransparencyEnabled() const;
	// Check if virtual pointer transparency is enabled.
	UFUNCTION(BlueprintPure, Category = "Web Browser|Transparency")
	bool IsVirtualPointerTransparencyEnabled() const;

	/** Called when the Url changes. */
	UPROPERTY(BlueprintAssignable, Category = "Web Browser|Event")
	FOnUrlChanged OnUrlChanged;

	/** Called when a popup is about to spawn. */
	UPROPERTY(BlueprintAssignable, Category = "Web Browser|Event")
	FOnBeforePopup OnBeforePopup;

public:

	//~ Begin UWidget interface
	virtual void SynchronizeProperties() override;
	// End UWidget interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	/** URL that the browser will initially navigate to. The URL should include the protocol, eg http:// */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FString InitialURL;

	/** Should the browser window support transparency. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	bool bSupportsTransparency;

	UPROPERTY(EditAnywhere, Category = "Behavior", meta = (UIMin = 1, UIMax = 60))
	int32 FrameRate;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Enable Transparency"), Category = "Behavior|Mouse")
	bool bEnableMouseTransparency;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Transparency Threshold", UIMin = 0, UIMax = 1), Category = "Behavior|Mouse")
	float MouseTransparencyThreshold;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Transparency Delay", UIMin = 0, UIMax = 1), Category = "Behavior|Mouse")
	float MouseTransparencyDelay;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Enable Transparency"), Category = "Behavior|Virtual Pointer")
	bool bEnableVirtualPointerTransparency;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Transparency Threshold", UIMin = 0, UIMax = 1), Category = "Behavior|Virtual Pointer")
	float VirtualPointerTransparencyThreshold;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Custom Cursors"), Category = "Behavior|Mouse")
	bool bCustomCursors;
protected:
	TSharedPtr<class SWebBrowser> WebBrowserWidget;
protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	void HandleOnUrlChanged(const FText& Text);
	bool HandleOnBeforePopup(FString URL, FString Frame);
};
