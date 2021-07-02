#pragma once

#include <map>
#include <mutex>

#include "Utils_Win32_Closeable.h"

namespace Utils::Win32 {
	class ModuleMemoryBlocks;
	class Process : public Closeable::Handle {
		mutable std::mutex m_moduleMemoryMutex;
		mutable std::map<HMODULE, std::shared_ptr<ModuleMemoryBlocks>> m_moduleMemory;

	public:
		Process();
		Process(HANDLE hProcess, bool ownership);
		Process(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
		Process(Process&& r) noexcept;
		Process(const Process& r);
		~Process() override;

		Process& operator=(Process&& r) noexcept;
		Process& operator=(const Process& r);
		Process& operator=(std::nullptr_t) override;

		static Process& Current();

		void Detach() override;

		void Clear() override;

		enum class ModuleNameCompareMode {
			FullPath = 0,
			FileNameWithExtension = 1,
			FileNameWithoutExtension = 2,
		};
		[[nodiscard]] HMODULE AddressOf(std::filesystem::path path, ModuleNameCompareMode compareMode = ModuleNameCompareMode::FullPath, bool require = true) const;
		[[nodiscard]] std::filesystem::path PathOf(HMODULE hModule = nullptr) const;
		[[nodiscard]] bool IsCurrentProcessPseudoHandle() const;
		[[nodiscard]] bool IsProcess64Bits() const;
		DWORD GetId() const;

		int CallRemoteFunction(void* rpfn, void* rpParam, const char* pcszDescription) const;

		[[nodiscard]] std::pair<void*, void*> FindImportedFunction(HMODULE hModule, const std::filesystem::path& dllName, const char* pszFunctionName, uint32_t hintOrOrdinal = 0) const;
		[[nodiscard]] void* FindExportedFunction(HMODULE hModule, const char* pszFunctionName, USHORT ordinal = 0, bool require = true) const;
		HMODULE LoadModule(const std::filesystem::path& path) const;
		int UnloadModule(HMODULE hModule) const;

		std::vector<MEMORY_BASIC_INFORMATION> GetCommittedImageAllocation(const std::filesystem::path& path) const;
		std::vector<MEMORY_BASIC_INFORMATION> GetCommittedImageAllocation() const;

		ModuleMemoryBlocks& GetModuleMemoryBlockManager(HMODULE hModule) const;

		size_t ReadMemory(void* lpBase, size_t offset, void* buf, size_t len, bool readFull = true) const;

		template<typename T>
		size_t ReadMemory(void* lpBase, size_t offset, std::span<T> buf, bool readFull = true) const {
			return ReadMemory(lpBase, offset, buf.data(), buf.size_bytes(), readFull);
		}

		template<typename T>
		std::vector<T> ReadMemory(void* lpBase, size_t offset, size_t count, bool readFull = true) const {
			std::vector<T> buf;
			buf.resize(count);
			buf.resize(ReadMemory(lpBase, offset, std::span(buf), readFull));
			return buf;
		}

		template<typename T>
		T ReadMemory(void* lpBase, size_t offset) const {
			T buf;
			ReadMemory(lpBase, offset, std::span(&buf, &buf + 1));
			return buf;
		}

		void WriteMemory(void* lpTarget, const void* lpSource, size_t len) const;

		template<typename T>
		void WriteMemory(void* lpTarget, size_t offset, const T& data) const {
			WriteMemory(static_cast<char*>(lpTarget) + offset, &data, sizeof data);
		}

		template<typename T>
		void WriteMemory(void* lpTarget, size_t offset, const std::span<T>& data) const {
			WriteMemory(static_cast<char*>(lpTarget) + offset, data.data(), data.size_bytes());
		}

		template<typename T>
		T* VirtualAlloc(T* lpBase, size_t count, DWORD flAllocType, DWORD flProtect) const {
			return static_cast<T*>(VirtualAlloc(static_cast<void*>(lpBase), count * sizeof T, flAllocType, flProtect));
		}
		void* VirtualAlloc(void* lpBase, size_t size, DWORD flAllocType, DWORD flProtect) const;
		DWORD VirtualProtect(void* lpBase, size_t offset, size_t length, DWORD value) const;
	};

	class ModuleMemoryBlocks {
	public:
		const Process CurrentProcess;
		const HMODULE CurrentModule;
		const IMAGE_DOS_HEADER DosHeader;
		const IMAGE_NT_HEADERS NtHeaders;
		const std::vector<IMAGE_SECTION_HEADER> SectionHeaders;

	private:
		std::map<DWORD, std::vector<char>> m_readMemoryBlocks;

	public:
		ModuleMemoryBlocks(Process process, HMODULE hModule);
		~ModuleMemoryBlocks();

		bool AddressInDataDirectory(size_t rva, int directoryIndex);

	private:
		std::span<uint8_t> Read(size_t rva, size_t maxCount);

	public:
		template<typename T>
		std::span<T> ReadAligned(size_t rva, size_t maxCount = SIZE_MAX) {
			const auto res = Read(rva, maxCount / sizeof T);
			if (reinterpret_cast<size_t>(&res[0]) % sizeof T != 0)
				throw std::runtime_error("Not aligned");
			return { reinterpret_cast<T*>(&res[0]), res.size_bytes() / sizeof T };
		}
	
		template<typename T>
		std::span<T> ReadDataDirectory(int index) {
			return ReadAligned<T>(NtHeaders.OptionalHeader.DataDirectory[index].VirtualAddress);
		}
	};
}