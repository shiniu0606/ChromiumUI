// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using Tools.DotNETCommon;

public class ChromiumUI : ModuleRules
{
	public ChromiumUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"RHI",
				"InputCore",
				"Serialization",
				"RenderCore",
				"HTTP"//, "WebBrowser"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UMG",
			}
		);

		//if (Target.Platform == UnrealTargetPlatform.Android ||
		//	Target.Platform == UnrealTargetPlatform.IOS ||
		//	Target.Platform == UnrealTargetPlatform.TVOS)
		//{
		//	// We need these on mobile for external texture support
		//	PrivateDependencyModuleNames.AddRange(
		//		new string[]
		//		{
		//			"Engine",
		//			"Launch",
		//			"WebBrowserTexture"
		//		}
		//	);

		//	// We need this one on Android for URL decoding
		//	PrivateDependencyModuleNames.Add("HTTP");
		//	CircularlyReferencedDependentModules.Add("WebBrowserTexture");
		//}

		if (Target.Type != TargetType.Program && Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Engine",
					"RenderCore"
				}
			);

		}


		if (Target.Platform == UnrealTargetPlatform.Win64)
		//|| Target.Platform == UnrealTargetPlatform.Win32
		//|| Target.Platform == UnrealTargetPlatform.Mac
		//|| Target.Platform == UnrealTargetPlatform.Linux)
		{
			//Log.TraceWarning("Log ChromiumUI Build Platform Win64");

			AddEngineThirdPartyPrivateStaticDependencies(Target,
				"ChromiumUILibrary"
				);

			if (Target.Type != TargetType.Server)
			{
				if (Target.Platform == UnrealTargetPlatform.Mac)
				{
					// Add contents of UnrealCefSubProcess.app directory as runtime dependencies
					foreach (string FilePath in Directory.EnumerateFiles(Target.RelativeEnginePath + "/Binaries/Mac/UnrealCEFSubProcess.app", "*", SearchOption.AllDirectories))
					{
						RuntimeDependencies.Add(FilePath);
					}
				}
				else if (Target.Platform == UnrealTargetPlatform.Linux)
				{
					RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/UnrealCEFSubProcess");
				}
				else
				{
					RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/UnrealCEFSubProcess.exe");
					//Log.TraceWarning("Log ChromiumUI Build Platform Win64: {0}", PluginDirectory+ "/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/UnrealCEFSubProcess.exe");
					
				}
			}
		}

		//if (Target.Platform == UnrealTargetPlatform.PS4 &&
		//	Target.bCompileAgainstEngine)
		//{
		//	PrivateDependencyModuleNames.Add("Engine");
		//}

		//if (Target.Platform == UnrealTargetPlatform.Lumin)
		//{
		//	PrecompileForTargets = ModuleRules.PrecompileTargetsType.None;
		//}
	}
}
