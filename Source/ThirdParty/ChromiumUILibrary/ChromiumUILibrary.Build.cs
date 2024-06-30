// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;
using System.Collections.Generic;
using Tools.DotNETCommon;


public class ChromiumUILibrary : ModuleRules
{
	public ChromiumUILibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		/** Mark the current version of the library */
		//string CEFVersion = "3.3071.1611.g4a19305";
		string CEFVersion = "84.1.6+gc551bc2+chromium-84.0.4147.38";
		string CEFPlatform = "";

		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			CEFPlatform = "windows64";
		}
		//else if (Target.Platform == UnrealTargetPlatform.Win32)
		//{
		//	CEFPlatform = "windows32";
		//}
		//else if (Target.Platform == UnrealTargetPlatform.Mac)
		//{
		//	CEFPlatform = "macosx64";
		//}
		//else if (Target.Platform == UnrealTargetPlatform.Linux)
		//{
		//	CEFVersion = "3.2623.1395.g3034273";
		//	CEFPlatform = "linux64";
		//}

		if (CEFPlatform.Length > 0 && Target.bCompileCEF3)
		{
			string PlatformPath = Path.Combine(ModuleDirectory, "cef_binary_" + CEFVersion + "_" + CEFPlatform);

			PublicSystemIncludePaths.Add(PlatformPath);
			var ModuleBinaryFiles = Directory.GetFiles(Path.Combine(ModuleDirectory, "Binaries"), "*", SearchOption.AllDirectories);
			foreach (var ModuleBinaryFile in ModuleBinaryFiles)
			{
				string DependencyName = ModuleBinaryFile.Substring(Path.Combine(ModuleDirectory, "Binaries").Length);
				string DependencyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/ThirdParty/CEF3/")) + DependencyName;
				Directory.CreateDirectory(Path.GetDirectoryName(DependencyPath));
				if (!File.Exists(DependencyPath))
				{
					File.Copy(ModuleBinaryFile, DependencyPath, false);
				}
			}
			string LibraryPath = Path.Combine(PlatformPath, "Release");
			string RuntimePath = Path.Combine(ModuleDirectory, "../../../Binaries/ThirdParty/CEF3/", Target.Platform.ToString());

			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, "libcef.lib"));

				// There are different versions of the C++ wrapper lib depending on the version of VS we're using
				string VSVersionFolderName = "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName();

				//Log.TraceWarning("Log VSVersionFolderName: {0}", VSVersionFolderName);

				string WrapperLibraryPath = Path.Combine(PlatformPath, VSVersionFolderName, "libcef_dll");

				if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
				{
					WrapperLibraryPath += "\\Debug";
				}
				else
				{
					WrapperLibraryPath += "\\Release";
				}

				PublicAdditionalLibraries.Add(Path.Combine(WrapperLibraryPath, "libcef_dll_wrapper.lib"));

				//Log.TraceWarning("Log libcef_dll_wrapper.lib Path: {0}", Path.Combine(WrapperLibraryPath, "libcef_dll_wrapper.lib"));

				List<string> Dlls = new List<string>();

				Dlls.Add("chrome_elf.dll");
				Dlls.Add("d3dcompiler_43.dll");
				Dlls.Add("d3dcompiler_47.dll");
				Dlls.Add("libcef.dll");
				Dlls.Add("libEGL.dll");
				Dlls.Add("libGLESv2.dll");
				//Dlls.Add("pepflashplayer64_29_0_0_171.dll");
				PublicDelayLoadDLLs.AddRange(Dlls);

				// Add the runtime dlls to the build receipt
				foreach (string Dll in Dlls)
				{
					RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/" + Dll);
				}
				// We also need the icu translations table required by CEF
				RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/icudtl.dat");

				// Add the V8 binary data files as well
				RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/v8_context_snapshot.bin");
				RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/snapshot_blob.bin");

				// And the entire Resources folder. Enumerate the entire directory instead of mentioning each file manually here.
				foreach (string FileName in Directory.EnumerateFiles(Path.Combine(RuntimePath, "Resources"), "*", SearchOption.AllDirectories))
				{
					RuntimeDependencies.Add(FileName);
				}
			}
			// TODO: Ensure these are filled out correctly when adding other platforms
			//else if (Target.Platform == UnrealTargetPlatform.Mac)
			//{
			//	string WrapperPath = LibraryPath + "/libcef_dll_wrapper.a";
			//	string FrameworkPath = Target.UEThirdPartyBinariesDirectory + "CEF3/Mac/Chromium Embedded Framework.framework";

			//	PublicAdditionalLibraries.Add(WrapperPath);
			//	PublicFrameworks.Add(FrameworkPath);

			//	if (Directory.Exists(LibraryPath + "/locale"))
			//	{
			//		var LocaleFolders = Directory.GetFileSystemEntries(LibraryPath + "/locale", "*.lproj");
			//		foreach (var FolderName in LocaleFolders)
			//		{
			//			AdditionalBundleResources.Add(new BundleResource(FolderName, bShouldLog: false));
			//		}
			//	}

			//	// Add contents of framework directory as runtime dependencies
			//	foreach (string FilePath in Directory.EnumerateFiles(FrameworkPath, "*", SearchOption.AllDirectories))
			//	{
			//		RuntimeDependencies.Add(FilePath);
			//	}
			//}
			//else if (Target.Platform == UnrealTargetPlatform.Linux)
			//{
			//	// link against runtime library since this produces correct RPATH
			//	string RuntimeLibCEFPath = Path.Combine(RuntimePath, "libcef.so");
			//	PublicAdditionalLibraries.Add(RuntimeLibCEFPath);

			//	string Configuration = "build_release";
			//	string WrapperLibraryPath = Path.Combine(PlatformPath, Configuration, "libcef_dll");

			//	PublicAdditionalLibraries.Add(Path.Combine(WrapperLibraryPath, "libcef_dll_wrapper.a"));

			//	PrivateRuntimeLibraryPaths.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString());

			//	RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/libcef.so");
			//	RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/icudtl.dat");
			//	RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/natives_blob.bin");
			//	RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/snapshot_blob.bin");

			//	// And the entire Resources folder. Enunerate the entire directory instead of mentioning each file manually here.
			//	foreach (string FileName in Directory.EnumerateFiles(Path.Combine(RuntimePath, "Resources"), "*", SearchOption.AllDirectories))
			//	{
			//		RuntimeDependencies.Add(FileName);
			//	}
			//}
		}
	}
}
