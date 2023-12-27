// Axel '0vercl0k' Souchet - September 12 2020
#pragma once
#include <dbgeng.h>
#include <fmt/printf.h>
#include <windows.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "constants.hpp"

using namespace std::string_view_literals;
namespace fs = std::filesystem;

#if defined(__i386__) || defined(_M_IX86)
#define SYMBOLIZER_ARCH "x86"
#elif defined(__amd64__) || defined(_M_X64)
#define SYMBOLIZER_ARCH "x64"
#elif defined(__arm64__) || defined(_M_ARM64)
#define SYMBOLIZER_ARCH "arm64"
#else
#error Platform not supported.
#endif

#ifdef _DEBUG
#define dbg(...) fmt::print("[*] " __VA_ARGS__)
#else
#define dbg(...)
#endif // _DEBUG

#define warn(...) fmt::print("[!] " __VA_ARGS__)
#define err(...) fmt::print("[-] " __VA_ARGS__)

template<typename T, auto Deleter>
using GenericHandle = std::unique_ptr<
    T,
    decltype(
        [](T* h)
        {
            if ( h )
            {
                Deleter(h);
                h = nullptr;
            }
        })>;

using UniqueHandle        = GenericHandle<void, ::CloseHandle>;
using UniqueIDebugClient8 = GenericHandle<
    IDebugClient8,
    [](IDebugClient8* cli)
    {
        cli->EndSession(DEBUG_END_ACTIVE_DETACH);
        cli->Release();
    }>;
using UniqueIDebugControl7 = GenericHandle<
    IDebugControl7,
    [](IDebugControl7* ctl)
    {
        ctl->Release();
    }>;
using UniqueIDebugSymbols5 = GenericHandle<
    IDebugSymbols5,
    [](IDebugSymbols5* sym)
    {
        sym->Release();
    }>;

namespace Symbolizer
{
//
// The trace style supported.
//

enum class TraceStyle_t : unsigned char
{
    Modoff,
    FullSymbol
};

//
// The below class is the abstraction we use to interact with the DbgEng APIs.
//

class DbgEng_t
{
    //
    // Highly inspired from:
    // C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\sdk\samples\dumpstk
    // The below is only used for debugging purposes; it allows to see the
    // messages outputed by the DbgEng APIs like you would see them in a WinDbg
    // output window.
    //

    class StdioOutputCallbacks_t final : public IDebugOutputCallbacks
    {
    public:
        // IUnknown
        STDMETHODIMP
        QueryInterface(REFIID InterfaceId, PVOID* Interface) noexcept override
        {
            *Interface = NULL;

            if ( IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
                 IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)) )
            {
                *Interface = (IDebugOutputCallbacks*)this;
                AddRef();
                return S_OK;
            }
            return E_NOINTERFACE;
        }

        STDMETHODIMP_(ULONG) AddRef() noexcept override
        {
            // This class is designed to be static so
            // there's no true refcount.
            return 1;
        }

        STDMETHODIMP_(ULONG) Release() noexcept override
        {
            // This class is designed to be static so
            // there's no true refcount.
            return 0;
        }

        STDMETHODIMP
        Output(ULONG, PCSTR Text) noexcept override
        {
            fmt::print("{}", Text);
            return S_OK;
        }
    };

    UniqueHandle DumpFileHandle_ {nullptr};

    //
    // This is the internal cache. Granted that resolving symbols is a pretty slow
    // process and the fact that traces usually contain a smaller number of
    // *unique* addresses executed, this gets us a really nice boost.
    //

    std::unordered_map<uint64_t, std::string> Cache_;

    //
    // The below are the various interfaces we need to do symbol resolution as
    // well as loading the crash-dump.
    //

    UniqueIDebugClient8 Client_ {nullptr};
    UniqueIDebugControl7 Control_ {nullptr};
    UniqueIDebugSymbols5 Symbols_ {nullptr};

#ifdef _DEBUG
    StdioOutputCallbacks_t StdioOutputCallbacks_;
#endif // _DEBUG

public:
    DbgEng_t() = default;

    //
    // Rule of three.
    //

    DbgEng_t(const DbgEng_t&) = delete;
    DbgEng_t&
    operator=(DbgEng_t&) = delete;

    //
    // Initialize the COM interfaces, load the dump files and the symbol path if provided.
    //

    bool
    Init(const fs::path& DumpPath, std::optional<fs::path> SymbolPath = std::nullopt)
    {
        //
        // Ensure that we have dbghelp.dll / dbgcore.dll / dbgeng.dll /
        // symsrv.dll in the current directory otherwise things don't work. cf
        // https://docs.microsoft.com/en-us/windows/win32/debug/using-symsrv
        // "Installation"
        //

        char ExePathBuffer[MAX_PATH];
        if ( !::GetModuleFileNameA(nullptr, &ExePathBuffer[0], sizeof(ExePathBuffer)) )
        {
            err("GetModuleFileNameA() failed\n");
            return false;
        }

        //
        // Let's check if the dlls exist in the same path as the application.
        //

        const fs::path ExePath(ExePathBuffer);
        const fs::path ParentDir(ExePath.parent_path());
        const std::array Dlls = {"dbghelp.dll"sv, "symsrv.dll"sv, "dbgeng.dll"sv, "dbgcore.dll"sv};
        const fs::path DefaultDbgDllLocation(R"(c:\program Files (x86)\windows kits\10\debuggers\)" SYMBOLIZER_ARCH);

        for ( const auto& Dll : Dlls )
        {
            if ( fs::exists(ParentDir / Dll) )
            {
                continue;
            }

            //
            // Apparently it doesn't. Be nice and try to find them by ourselves.
            //

            const fs::path DbgDllLocation(DefaultDbgDllLocation / Dll);
            if ( !fs::exists(DbgDllLocation) )
            {

                //
                // If it doesn't exist we have to exit.
                //

                err("The debugger class expects debug dlls in the directory "
                    "where the application is running from.\n");
                return false;
            }

            //
            // Sounds like we are able to fix the problem ourselves. Copy the files
            // in the directory where the application is running from and move on!
            //

            fs::copy(DbgDllLocation, ParentDir);
            dbg("Copied {} into the "
                "executable directory..\n",
                DbgDllLocation.generic_string().c_str());
        }

        //
        // Initialize the various COM interfaces that we need.
        //
        Client_ = UniqueIDebugClient8(
            []() -> IDebugClient8*
            {
                IDebugClient8* _Cli {nullptr};
                dbg("Initializing the debugger instance..\n");
                HRESULT Status = ::DebugCreate(__uuidof(IDebugClient8), (void**)&_Cli);
                if ( FAILED(Status) )
                {
                    err("DebugCreate failed with hr=0x{:08x}\n", (uint32_t)Status);
                    return nullptr;
                }

                return _Cli;
            }());
        if ( !Client_ )
        {
            return false;
        }

        Control_ = UniqueIDebugControl7(
            [this]() -> IDebugControl7*
            {
                IDebugControl7* _Ctrl {nullptr};
                HRESULT Status = Client_->QueryInterface(__uuidof(IDebugControl7), (void**)&_Ctrl);
                if ( FAILED(Status) )
                {
                    err("QueryInterface/IDebugControl7 failed with hr=0x{:08x}\n", (uint32_t)Status);
                    return nullptr;
                }
                return _Ctrl;
            }());
        if ( !Control_ )
        {
            return false;
        }

        Symbols_ = UniqueIDebugSymbols5(
            [this]() -> IDebugSymbols5*
            {
                IDebugSymbols5* _Sym {nullptr};
                HRESULT Status = Client_->QueryInterface(__uuidof(IDebugSymbols5), (void**)&_Sym);
                if ( FAILED(Status) )
                {
                    err("QueryInterface/IDebugSymbols failed with hr=0x{:08x}\n", (uint32_t)Status);
                    return nullptr;
                }
                return _Sym;
            }());
        if ( !Symbols_ )
        {
            return false;
        }

        if ( SymbolPath )
        {
            auto const& sym_path = SymbolPath.value().string();
            HRESULT Status       = Symbols_->SetSymbolPath(sym_path.c_str());
            if ( Status != S_OK )
            {
                warn("SetSymbolPath({}) failed with hr=0x{:08x}\n", sym_path, (uint32_t)Status);
            }
        }

#ifdef _DEBUG
        //
        // Turn the below on to debug issues related to dbghelp.
        //
        {
            const uint32_t SYMOPT_DEBUG = 0x80000000;
            HRESULT Status              = Symbols_->SetSymbolOptions(SYMOPT_DEBUG);
            if ( FAILED(Status) )
            {
                err("IDebugSymbols::SetSymbolOptions failed with hr=0x{:08x}\n", (uint32_t)Status);
                return false;
            }

            Client_->SetOutputCallbacks(&StdioOutputCallbacks_);
        }
#endif // _DEBUG

        //
        // We can now open the crash-dump using the dbghelp APIs.
        //

        dbg("Opening the dump file..\n");
        const std::wstring& DumpFileString = DumpPath.wstring();
        const wchar_t* DumpFileW           = DumpFileString.c_str();
        DumpFileHandle_                    = UniqueHandle(::CreateFileW(
            DumpFileW,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));
        if ( !DumpFileHandle_ || DumpFileHandle_.get() == INVALID_HANDLE_VALUE )
        {
            err("CreateFileW({}) failed with GLE=0x{:x}\n", DumpPath.string(), ::GetLastError());
            return false;
        }

        dbg("Parsing the dump file..\n");
        HRESULT Status =
            Client_->OpenDumpFileWide2(DumpFileW, (ULONG64)DumpFileHandle_.get(), IMAGE_FILE_MACHINE_AMD64);
        if ( FAILED(Status) )
        {
            err("OpenDumpFile({}) failed with hr=0x{:08x}\n", DumpPath.string(), (uint32_t)Status);
            return false;
        }

        //
        // Note that the engine doesn't completely attach to the dump file until the
        // WaitForEvent method has been called. When a dump file is created from a
        // process or kernel, information about the last event is stored in the
        // dump file. After the dump file is opened, the next time execution is
        // attempted, the engine will generate this event for the event callbacks.
        // Only then does the dump file become available in the debugging session.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-IDebugClient8-opendumpfile
        //

        Status = WaitForEvent();
        if ( FAILED(Status) )
        {
            err("WaitForEvent for OpenDumpFile failed with hr=0x{:08x}\n", (uint32_t)Status);
            return false;
        }

        return true;
    }

    //
    // This returns the symbolized version of |SymbolAddress| according to a
    // |Style|.
    //

    std::optional<std::reference_wrapper<std::string>>
    Symbolize(const uint64_t SymbolAddress, const TraceStyle_t Style)
    {
        //
        // Fast path for the addresses we have symbolized already.
        //

        if ( Cache_.contains(SymbolAddress) )
        {
            return Cache_.at(SymbolAddress);
        }

        //
        // Slow path, we need to ask dbgeng..
        //

        const auto& Res = Style == TraceStyle_t::Modoff ? SymbolizeModoff(SymbolAddress) : SymbolizeFull(SymbolAddress);

        //
        // If there has been an issue during symbolization, bail as it is not
        // expected.
        //

        if ( !Res )
        {
            return {};
        }

        //
        // Feed the result into the cache.
        //

        Cache_.emplace(SymbolAddress, *Res);

        //
        // Return the entry directly from the cache.
        //

        return Cache_.at(SymbolAddress);
    }

    //
    // Attempts to resolve a symbol name (with the notation `Mod!Symbol`) to an address
    //

    std::optional<uint64_t>
    Resolve(std::string const& SymName) const
    {
        ULONG64 Offset;
        HRESULT Status = Symbols_->GetOffsetByName(SymName.c_str(), &Offset);
        if ( FAILED(Status) )
        {
            err("GetOffsetByName('{}') failed with 0x{:08x}\n", SymName, (uint32_t)Status);
            return {};
        }
        return Offset;
    }

private:
    //
    // This returns a module+offset symbolization of |SymbolAddress|.
    //

    std::optional<std::string>
    SymbolizeModoff(const uint64_t SymbolAddress)
    {
        constexpr size_t NameSizeMax = MAX_PATH;
        char Buffer[NameSizeMax]     = {};

        //
        // module+offset style.
        //

        ULONG Index;
        ULONG64 Base;
        HRESULT Status = Symbols_->GetModuleByOffset(SymbolAddress, 0, &Index, &Base);
        if ( FAILED(Status) )
        {
            err("GetModuleByOffset failed with hr=0x{:08x}\n", (uint32_t)Status);
            return {};
        }

        ULONG NameSize;
        Status = Symbols_->GetModuleNameString(DEBUG_MODNAME_MODULE, Index, Base, &Buffer[0], NameSizeMax, &NameSize);
        if ( FAILED(Status) )
        {
            err("GetModuleNameString failed with hr=0x{:08x}\n", (uint32_t)Status);
            return {};
        }

        const uint64_t Offset = SymbolAddress - Base;
        return fmt::format("{}+0x{:x}", Buffer, Offset);
    }

    //
    // Symbolizes |SymbolAddress| with module+offset style.
    //

    std::optional<std::string>
    SymbolizeFull(const uint64_t SymbolAddress)
    {
        //
        // Full symbol style!
        //

        constexpr size_t NameSizeMax = MAX_PATH;
        char Buffer[NameSizeMax]     = {};

        uint64_t Displacement = 0;
        const HRESULT Status =
            Symbols_->GetNameByOffset(SymbolAddress, &Buffer[0], NameSizeMax, nullptr, &Displacement);
        if ( FAILED(Status) )
        {
            err("GetNameByOffset failed with hr=0x{:08x}\n", Status);
            return {};
        }

        return fmt::format("{}+0x{:x}", Buffer, Displacement);
    }

    //
    // Waits for the dbghelp machinery to signal that they are done.
    //

    HRESULT
    WaitForEvent() const noexcept
    {
        const HRESULT Status = Control_->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
        if ( FAILED(Status) )
        {
            err("Execute::WaitForEvent failed with 0x{:08x}\n", (uint32_t)Status);
        }
        return Status;
    }
};

} // namespace Symbolizer