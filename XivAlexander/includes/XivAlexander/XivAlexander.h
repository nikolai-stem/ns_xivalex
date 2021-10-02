#pragma once

#ifndef XIVALEXANDER_DLLEXPORT

#include <minwindef.h>
#include <XivAlexanderCommon/Utils_Win32_Process.h>

#ifdef XIVALEXANDER_DLLEXPORT_SET
#define XIVALEXANDER_DLLEXPORT __declspec(dllexport)
#else
#define XIVALEXANDER_DLLEXPORT __declspec(dllimport)
#endif

namespace XivAlexDll {
	struct InjectEntryPointParameters;

	enum class LoaderAction : int {
		Interactive,
		Web,
		Ask,
		Load,
		Unload,
		Launcher,
		UpdateCheck,
		Install,
		Uninstall,
		Internal_Update_DependencyDllMode,
		Internal_Update_Step2_ReplaceFiles,
		Internal_Update_Step3_CleanupFiles,
		Internal_Inject_HookEntryPoint,
		Internal_Inject_LoadXivAlexanderImmediately,
		Internal_Inject_UnloadFromHandle,
		Count_,  // for internal use only
	};

	XIVALEXANDER_DLLEXPORT const char* LoaderActionToString(LoaderAction val);

	enum WhichLoader {
		Current,
		Opposite,
		Force32,
		Force64,
	};
	XIVALEXANDER_DLLEXPORT DWORD LaunchXivAlexLoaderWithTargetHandles(
		const std::vector<Utils::Win32::Process>& hSources,
		LoaderAction action,
		bool wait,
		const Utils::Win32::Process& waitForBeforeStarting = {},
		WhichLoader which = Current,
		const std::filesystem::path& loaderPath = {});
	XIVALEXANDER_DLLEXPORT InjectEntryPointParameters* PatchEntryPointForInjection(HANDLE hProcess);

	//
	// Everything declared below must be able to be called from CreateRemoteProcess.
	//

	class InjectOnCreateProcessAppFlags {
	public:
		enum : size_t {
			Use = 1 << 0,
			InjectAll = 1 << 1,
			InjectGameOnly = 1 << 2,
		};
	};

	extern "C" XIVALEXANDER_DLLEXPORT size_t __stdcall EnableInjectOnCreateProcess(size_t flags);

	struct InjectEntryPointParameters {
		void* EntryPoint;
		void* EntryPointOriginalBytes;
		size_t EntryPointOriginalLength;
		void* TrampolineAddress;
		bool SkipFree;
		bool LoadInstalledXivAlexDllOnly;

		struct {
			HANDLE hWorkerThread;
			HANDLE hMainThread;
		} Internal;
	};

	extern "C" XIVALEXANDER_DLLEXPORT void __stdcall InjectEntryPoint(InjectEntryPointParameters* pParam);
	extern "C" XIVALEXANDER_DLLEXPORT size_t __stdcall EnableXivAlexander(size_t bEnable);
	extern "C" XIVALEXANDER_DLLEXPORT size_t __stdcall ReloadConfiguration(void* lpReserved);
	extern "C" XIVALEXANDER_DLLEXPORT size_t __stdcall DisableAllApps(void* lpReserved);
	extern "C" XIVALEXANDER_DLLEXPORT void __stdcall CallFreeLibrary(void*);

	enum class CheckPackageVersionResult {
		OK = 0,
		MissingFiles = 1,
		VersionMismatch = 2,
	};

	XIVALEXANDER_DLLEXPORT [[nodiscard]] CheckPackageVersionResult CheckPackageVersion();

	extern "C" int XA_LoaderApp();
}

#endif
