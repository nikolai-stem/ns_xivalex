#include "pch.h"

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <XivAlexander/XivAlexander.h>
#include <XivAlexanderCommon/Utils_Win32.h>
#include <XivAlexanderCommon/Utils_Win32_Resource.h>

#include "App_ConfigRepository.h"
#include "App_Feature_GameResourceOverrider.h"
#include "DllMain.h"
#include "resource.h"

static Utils::Win32::LoadedModule EnsureOriginalDependencyModule(const char* szDllName, std::filesystem::path originalDllPath);
static void AutoLoadAsDependencyModule();

DECLSPEC_NORETURN
static void AutoLoadAsDependencyModuleError(const std::runtime_error& e);

#if INTPTR_MAX == INT64_MAX

#include <d3d11.h>
#include <dxgi1_3.h>

HRESULT WINAPI FORWARDER_D3D11CreateDevice(
	IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	const D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	ID3D11Device** ppDevice,
	D3D_FEATURE_LEVEL* pFeatureLevel,
	ID3D11DeviceContext** ppImmediateContext
) {
	try {
		AutoLoadAsDependencyModule();

		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("d3d11.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_d3d11))
			.GetProcAddress<decltype(&D3D11CreateDevice)>("D3D11CreateDevice", true);
		return pOriginalFunction(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

HRESULT WINAPI FORWARDER_CreateDXGIFactory(
	REFIID riid,
	void** ppFactory
) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("dxgi.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_dxgi))
			.GetProcAddress<decltype(&CreateDXGIFactory)>("CreateDXGIFactory", true);
		return pOriginalFunction(riid, ppFactory);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

HRESULT WINAPI FORWARDER_CreateDXGIFactory1(
	REFIID riid,
	void** ppFactory
) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("dxgi.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_dxgi))
			.GetProcAddress<decltype(&CreateDXGIFactory1)>("CreateDXGIFactory1", true);
		return pOriginalFunction(riid, ppFactory);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

HRESULT WINAPI FORWARDER_CreateDXGIFactory2(
	UINT   Flags,
	REFIID riid,
	void** ppFactory
) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("dxgi.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_dxgi))
			.GetProcAddress<decltype(&CreateDXGIFactory2)>("CreateDXGIFactory2", true);
		return pOriginalFunction(Flags, riid, ppFactory);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

#elif INTPTR_MAX == INT32_MAX

#include <d3d9.h>

void WINAPI FORWARDER_D3DPERF_SetOptions(DWORD dwOptions) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("d3d9.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_d3d9))
			.GetProcAddress<decltype(&D3DPERF_SetOptions)>("D3DPERF_SetOptions", true);
		return pOriginalFunction(dwOptions);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

IDirect3D9* WINAPI FORWARDER_Direct3DCreate9(UINT SDKVersion) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("d3d9.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_d3d9))
			.GetProcAddress<decltype(&Direct3DCreate9)>("Direct3DCreate9", true);
		return pOriginalFunction(SDKVersion);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

#endif

HRESULT WINAPI FORWARDER_DirectInput8Create(
	HINSTANCE hinst,
	DWORD dwVersion,
	REFIID riidltf,
	LPVOID* ppvOut,
	LPUNKNOWN punkOuter
) {
	try {
		AutoLoadAsDependencyModule();
		static auto pOriginalFunction =
			EnsureOriginalDependencyModule("dinput8.dll", App::Config::Config::TranslatePath(App::Config::Acquire()->Init.ChainLoadPath_dinput8))
			.GetProcAddress<decltype(&DirectInput8Create)>("DirectInput8Create", true);
		return pOriginalFunction(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	} catch (const std::runtime_error& e) {
		AutoLoadAsDependencyModuleError(e);
	}
}

static Utils::Win32::LoadedModule EnsureOriginalDependencyModule(const char* szDllName, std::filesystem::path originalDllPath) {
	static std::mutex preventDuplicateLoad;
	std::lock_guard lock(preventDuplicateLoad);

	if (originalDllPath.empty())
		originalDllPath = Utils::Win32::GetSystem32Path() / szDllName;

	auto mod = Utils::Win32::LoadedModule(originalDllPath);
	mod.SetPinned();
	return mod;
}

void AutoLoadAsDependencyModule() {
	static std::mutex s_singleRunMutex;
	static App::Feature::GameResourceOverrider s_pinnedHashTracker;
	static bool s_loaded = false;

	if (s_loaded)
		return;

	std::lock_guard lock(s_singleRunMutex);
	if (s_loaded)
		return;

	switch (XivAlexDll::CheckPackageVersion()) {
		case XivAlexDll::CheckPackageVersionResult::OK:
			break;

		case XivAlexDll::CheckPackageVersionResult::MissingFiles:
			throw std::runtime_error("Missing files");

		case XivAlexDll::CheckPackageVersionResult::VersionMismatch:
			throw std::runtime_error("Version mismatch");
	}

	Dll::DisableUnloading(std::format("Loaded as DLL dependency in place of {}", Dll::Module().PathOf().filename()).c_str());
	InjectEntryPoint(XivAlexDll::PatchEntryPointForInjection(GetCurrentProcess()));

	s_loaded = true;
}

DECLSPEC_NORETURN
void AutoLoadAsDependencyModuleError(const std::runtime_error& e) {
	Utils::Win32::MessageBoxF(nullptr, MB_OK | MB_ICONERROR, FindStringResourceEx(Dll::Module(), IDS_APP_NAME) + 1,
		L"Failed to load: {}\n\nAll 4 files must be present, other than this file.", e.what());
	TerminateProcess(GetCurrentProcess(), -1);
}
