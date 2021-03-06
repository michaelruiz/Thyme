////////////////////////////////////////////////////////////////////////////////
//                               --  THYME --                                 //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: DLLMAIN.CPP
//
//        Author:: OmniBlade
//
//  Contributors:: CCHyper
//
//   Description:: Defines the entry point for the DLL application.
//
//       License:: Thyme is free software: you can redistribute it and/or 
//                 modify it under the terms of the GNU General Public License 
//                 as published by the Free Software Foundation, either version 
//                 2 of the License, or (at your option) any later version.
//
//                 A full copy of the GNU General Public License can be found in
//                 LICENSE
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//  Includes
////////////////////////////////////////////////////////////////////////////////
#include "hooker.h"
#include "filesystem.h"
#include "archivefile.h"
#include "archivefilesystem.h"
#include "asciistring.h"
#include "chunkio.h"
#include "copyprotect.h"
#include "commandline.h"
#include "commandlist.h"
#include "force_nocd.h"
#include "gamememory.h"
#include "gamememoryinit.h"
#include "gamedebug.h"
#include "gametext.h"
#include "ini.h"
#include "main.h"
#include "namekeygenerator.h"
#include "randomvalue.h"
#include "w3dfilesystem.h"
#include "win32gameengine.h"
#include "win32localfilesystem.h"
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

void Setup_Hooks()
{
    //
    // Hook WinMain
    //
    Hook_StdCall_Function((Make_StdCall_Ptr<int, HINSTANCE, HINSTANCE, LPSTR, int>(0x00401700)), Main_Func);

	//
	// Code that checks the launcher is running, launcher does CD check.
	//
    Hook_Function((Make_Function_Ptr<void, unsigned int, int>(0x00412420)), CopyProtect::checkForMessage);
    Hook_Function((Make_Function_Ptr<bool>(0x00412450)), CopyProtect::validate);

    //
    // Returns true for any CD checks
    //
    Hook_Function((Make_Function_Ptr<bool>(0x005F1CB0)), IsFirstCDPresent);
    
    //
    // Replace memory intialisation
    //
    Hook_Function((Make_Function_Ptr<void>(0x00414510)), Init_Memory_Manager);
    Hook_Function((Make_Function_Ptr<void>(0x004148C0)), Init_Memory_Manager_Pre_Main);


    //
    // Replace memory allocation operators
    //
    Hook_Function((Make_Function_Ptr<void *, size_t>(0x00414450)), New_New);    // operator new
    Hook_Function((Make_Function_Ptr<void *, size_t>(0x00414490)), New_New);    // operator new[]
    Hook_Function((Make_Function_Ptr<void, void *>(0x004144D0)), New_Delete);   // operator delete
    Hook_Function((Make_Function_Ptr<void, void *>(0x004144F0)), New_Delete);   // operator delete[]
    Hook_Function((Make_Function_Ptr<MemoryPool *, const char *, int>(0x00414B30)), Create_Named_Pool);
    
    //
    // Replace pool functions
    //
    Hook_Method((Make_Method_Ptr<void *, MemoryPool>(0x00413C10)), &MemoryPool::Allocate_Block);
    Hook_Method((Make_Method_Ptr<void, MemoryPool, void *>(0x00413C40)), &MemoryPool::Free_Block);

    //
    // Replace DMA functions
    //
    Hook_Method((Make_Method_Ptr<void *, DynamicMemoryAllocator, int>(0x00413EE0)), &DynamicMemoryAllocator::Allocate_Bytes_No_Zero);
    Hook_Method((Make_Method_Ptr<void *, DynamicMemoryAllocator, int>(0x00413FE0)), &DynamicMemoryAllocator::Allocate_Bytes);
    Hook_Method((Make_Method_Ptr<void, DynamicMemoryAllocator, void *>(0x00414010)), &DynamicMemoryAllocator::Free_Bytes);
    Hook_Method((Make_Method_Ptr<int, DynamicMemoryAllocator, int>(0x00414140)), &DynamicMemoryAllocator::Get_Actual_Allocation_Size);
    
    //
    // Replace MemoryPoolFactory functions
    //
    Hook_Method((Make_Method_Ptr<MemoryPool *, MemoryPoolFactory, char const *, int, int, int>(0x00414180)), static_cast<MemoryPool *(MemoryPoolFactory::*const)(char const*, int, int, int)>(&MemoryPoolFactory::Create_Memory_Pool));
    
    //
    // Replace File functions
    //
    FileSystem::Hook_Me();
    Hook_Method((Make_Method_Ptr<LocalFileSystem*, Win32GameEngine>(0x007420F0)), &Win32GameEngine::Create_Local_File_System_NV);
    Hook_Method((Make_Method_Ptr<ArchiveFileSystem*, Win32GameEngine>(0x00742150)), &Win32GameEngine::Create_Archive_File_System_NV);
    Hook_Method((Make_Method_Ptr<void, ArchiveFileSystem, AsciiString const &, AsciiString const &, AsciiString const &, std::set<AsciiString, rts::less_than_nocase<AsciiString> >&, bool>(0x0048F410)), &ArchiveFileSystem::Get_File_List_From_Dir);
    Hook_Method((Make_Method_Ptr<AsciiString, ArchiveFileSystem, AsciiString const &>(0x0048F250)), &ArchiveFileSystem::Get_Archive_Filename_For_File);
    Hook_Method((Make_Method_Ptr<bool, ArchiveFileSystem, AsciiString const &, FileInfo*>(0x0048F160)), &ArchiveFileSystem::Get_File_Info);

    //
    // Replace AsciiString
    //
    Hook_Method((Make_Method_Ptr<void, AsciiString, char const *>(0x0040D640)), static_cast<void (AsciiString::*)(char const*)>(&AsciiString::Set));
    Hook_Method((Make_Method_Ptr<void, AsciiString, int, bool, char const *, char const *>(0x00415290)), &AsciiString::Ensure_Unique_Buffer_Of_Size);
    Hook_Method((Make_Method_Ptr<void, AsciiString, char const *>(0x0040FB40)), static_cast<void (AsciiString::*)(char const*)>(&AsciiString::Concat));

    //
    // Replace INI
    //
    INI::Hook_Me();

    // Replace NameKeyGenerator
    NameKeyGenerator::Hook_Me();

    // Replace GameTextManager
    GameTextManager::Hook_Me();

    // Replace CommandList
    GameMessageList::Hook_Me();
    CommandList::Hook_Me();

    // Replace RNG
    RandomValue::Hook_Me();

    // Replace Command Line parser
    CommandLine::Hook_Me();

    // Replace W3D file system
    W3DFileSystem::Hook_Me();

    //Replace ChunkIO system
    ChunkSaveClass::Hook_Me();
    ChunkLoadClass::Hook_Me();
}

// Use DLLMain to Set up our hooks when the DLL loads. The launcher should stall
// the main thread at the entry point so hooked code called after that should
// be our code.
BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch ( ul_reason_for_call ) {

        case DLL_PROCESS_ATTACH:
			StartHooking();
            Setup_Hooks();
            break;

        case DLL_PROCESS_DETACH:
            StopHooking();
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;

    }

    return TRUE;
}
