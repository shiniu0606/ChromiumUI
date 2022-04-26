// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "./WebBrowser.h"

#include "WebMenu.generated.h"


/**
 * 
 */
UCLASS()
class CHROMIUMUI_API UWebMenu : public UWebBrowser
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJsonReceived, FString, Json);

	/**
	* Send an event to web browser with Json
	*
	* @param Json Json string to execute
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
		void SendJson(const FString& Json);

	/**
	* Get resolution from webbrowser
	*
	* @param Json Json string to execute
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
		float GetZoom() const;

	/**
	* Check if web Browser is valid
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
		bool IsValid() const;

	/**
	* Called by web Browser to send json to Unreal
	*/
	UFUNCTION(BlueprintCallable, Category = "Web Browser")
		void ReceiveJson(FString json) const;


	/** Called when json received from client. */
	UPROPERTY(BlueprintAssignable, Category = "Web Browser|Event")
		FOnJsonReceived OnJsonReceivedd;

	/** Set zoom of page */
	UPROPERTY(BlueprintReadWrite, Category = "Web Browser|Appearance")
		float Zoom;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() final;
};
