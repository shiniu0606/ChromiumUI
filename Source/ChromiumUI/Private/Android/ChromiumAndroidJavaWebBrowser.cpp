// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumAndroidJavaWebBrowser.h"

#if USE_ANDROID_JNI

#include "Android/AndroidApplication.h"

#if UE_BUILD_SHIPPING
// always clear any exceptions in SHipping
#define CHECK_JNI_RESULT(Id) if (Id == 0) { JEnv->ExceptionClear(); }
#else
#define CHECK_JNI_RESULT(Id) \
if (Id == 0) \
{ \
	if (bIsOptional) { JEnv->ExceptionClear(); } \
	else { JEnv->ExceptionDescribe(); checkf(Id != 0, TEXT("Failed to find " #Id)); } \
}
#endif

static jfieldID FindField(JNIEnv* JEnv, jclass Class, const ANSICHAR* FieldName, const ANSICHAR* FieldType, bool bIsOptional)
{
	jfieldID Field = Class == NULL ? NULL : JEnv->GetFieldID(Class, FieldName, FieldType);
	CHECK_JNI_RESULT(Field);
	return Field;
}

FChromiumJavaAndroidWebBrowser::FChromiumJavaAndroidWebBrowser(bool swizzlePixels, bool vulkanRenderer, int32 width, int32 height,
	jlong widgetPtr, bool bEnableRemoteDebugging, bool bUseTransparency)
	: FJavaClassObject(GetClassName(), "(JIIZZZZ)V", widgetPtr, width, height,  swizzlePixels, vulkanRenderer, bEnableRemoteDebugging, bUseTransparency)
	, ReleaseMethod(GetClassMethod("release", "()V"))
	, GetVideoLastFrameDataMethod(GetClassMethod("getVideoLastFrameData", "()Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, GetVideoLastFrameMethod(GetClassMethod("getVideoLastFrame", "(I)Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, DidResolutionChangeMethod(GetClassMethod("didResolutionChange", "()Z"))
	, UpdateVideoFrameMethod(GetClassMethod("updateVideoFrame", "(I)Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, UpdateMethod(GetClassMethod("Update", "(IIII)V"))
	, ExecuteJavascriptMethod(GetClassMethod("ExecuteJavascript", "(Ljava/lang/String;)V"))
	, LoadURLMethod(GetClassMethod("LoadURL", "(Ljava/lang/String;)V"))
	, LoadStringMethod(GetClassMethod("LoadString", "(Ljava/lang/String;Ljava/lang/String;)V"))
	, StopLoadMethod(GetClassMethod("StopLoad", "()V"))
	, ReloadMethod(GetClassMethod("Reload", "()V"))
	, CloseMethod(GetClassMethod("Close", "()V"))
	, GoBackOrForwardMethod(GetClassMethod("GoBackOrForward", "(I)V"))
	, SetAndroid3DBrowserMethod(GetClassMethod("SetAndroid3DBrowser", "(Z)V"))
	, SetVisibilityMethod(GetClassMethod("SetVisibility", "(Z)V"))
{
	VideoTexture = nullptr;
	bVideoTextureValid = false;

	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();

	// get field IDs for FrameUpdateInfo class members
	FrameUpdateInfoClass = FAndroidApplication::FindJavaClassGlobalRef("com/epicgames/ue4/WebViewControl$FrameUpdateInfo");
	FrameUpdateInfo_Buffer = FindField(JEnv, FrameUpdateInfoClass, "Buffer", "Ljava/nio/Buffer;", false);
	FrameUpdateInfo_FrameReady = FindField(JEnv, FrameUpdateInfoClass, "FrameReady", "Z", false);
	FrameUpdateInfo_RegionChanged = FindField(JEnv, FrameUpdateInfoClass, "RegionChanged", "Z", false);
}

FChromiumJavaAndroidWebBrowser::~FChromiumJavaAndroidWebBrowser()
{
	if (auto Env = FAndroidApplication::GetJavaEnv())
	{
		Env->DeleteGlobalRef(FrameUpdateInfoClass);
	}
}

void FChromiumJavaAndroidWebBrowser::Release()
{
	CallMethod<void>(ReleaseMethod);
}

bool FChromiumJavaAndroidWebBrowser::GetVideoLastFrameData(void* & outPixels, int64 & outCount, bool *bRegionChanged)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	auto Result = NewScopedJavaObject(JEnv, JEnv->CallObjectMethod(Object, GetVideoLastFrameDataMethod.Method));
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		*bRegionChanged = false;
		return false;
	}

	if (!Result)
	{
		return false;
	}

	auto buffer = NewScopedJavaObject(JEnv, JEnv->GetObjectField(*Result, FrameUpdateInfo_Buffer));
	if (buffer)
	{
		bool bFrameReady = (bool)JEnv->GetBooleanField(*Result, FrameUpdateInfo_FrameReady);
		*bRegionChanged = (bool)JEnv->GetBooleanField(*Result, FrameUpdateInfo_RegionChanged);
		
		outPixels = JEnv->GetDirectBufferAddress(*buffer);
		outCount = JEnv->GetDirectBufferCapacity(*buffer);
		
		return !(nullptr == outPixels || 0 == outCount);
	}
	
	return false;
}

bool FChromiumJavaAndroidWebBrowser::DidResolutionChange()
{
	return CallMethod<bool>(DidResolutionChangeMethod);
}

bool FChromiumJavaAndroidWebBrowser::UpdateVideoFrame(int32 ExternalTextureId, bool *bRegionChanged)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	auto Result = NewScopedJavaObject(JEnv, JEnv->CallObjectMethod(Object, UpdateVideoFrameMethod.Method, ExternalTextureId));
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		*bRegionChanged = false;
		return false;
	}

	if (!Result)
	{
		*bRegionChanged = false;
		return false;
	}

	bool bFrameReady = (bool)JEnv->GetBooleanField(*Result, FrameUpdateInfo_FrameReady);
	*bRegionChanged = (bool)JEnv->GetBooleanField(*Result, FrameUpdateInfo_RegionChanged);
	
	return bFrameReady;
}

bool FChromiumJavaAndroidWebBrowser::GetVideoLastFrame(int32 destTexture)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	auto Result = NewScopedJavaObject(JEnv, JEnv->CallObjectMethod(Object, GetVideoLastFrameMethod.Method, destTexture));
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		return false;
	}

	if (!Result)
	{
		return false;
	}

	bool bFrameReady = (bool)JEnv->GetBooleanField(*Result, FrameUpdateInfo_FrameReady);
	
	return bFrameReady;
}

FName FChromiumJavaAndroidWebBrowser::GetClassName()
{
	return FName("com/epicgames/ue4/WebViewControl");
}

void FChromiumJavaAndroidWebBrowser::Update(const int posX, const int posY, const int sizeX, const int sizeY)
{
	CallMethod<void>(UpdateMethod, posX, posY, sizeX, sizeY);
}

void FChromiumJavaAndroidWebBrowser::ExecuteJavascript(const FString& Script)
{
	auto JString = FJavaClassObject::GetJString(Script);
	CallMethod<void>(ExecuteJavascriptMethod, *JString);
}

void FChromiumJavaAndroidWebBrowser::LoadURL(const FString& NewURL)
{
	auto JString = FJavaClassObject::GetJString(NewURL);
	CallMethod<void>(LoadURLMethod, *JString);
}

void FChromiumJavaAndroidWebBrowser::LoadString(const FString& Contents, const FString& BaseUrl)
{
	auto JContents = FJavaClassObject::GetJString(Contents);
	auto JUrl = FJavaClassObject::GetJString(BaseUrl);
	CallMethod<void>(LoadStringMethod, *JContents, *JUrl);
}

void FChromiumJavaAndroidWebBrowser::StopLoad()
{
	CallMethod<void>(StopLoadMethod);
}

void FChromiumJavaAndroidWebBrowser::Reload()
{
	CallMethod<void>(ReloadMethod);
}

void FChromiumJavaAndroidWebBrowser::Close()
{
	CallMethod<void>(CloseMethod);
}

void FChromiumJavaAndroidWebBrowser::GoBack()
{
	CallMethod<void>(GoBackOrForwardMethod, -1);
}

void FChromiumJavaAndroidWebBrowser::GoForward()
{
	CallMethod<void>(GoBackOrForwardMethod, 1);
}

void FChromiumJavaAndroidWebBrowser::SetAndroid3DBrowser(bool InIsAndroid3DBrowser)
{
	CallMethod<void>(SetAndroid3DBrowserMethod, InIsAndroid3DBrowser);
}

void FChromiumJavaAndroidWebBrowser::SetVisibility(bool InIsVisible)
{
	CallMethod<void>(SetVisibilityMethod, InIsVisible);
}

#endif // USE_ANDROID_JNI

