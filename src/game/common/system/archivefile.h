////////////////////////////////////////////////////////////////////////////////
//                               --  THYME  --                                //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: ARCHIVEFILE.H
//
//        Author:: OmniBlade
//
//  Contributors:: 
//
//   Description:: Base class for archive file handling.
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
#ifdef _MSC_VER
#pragma once
#endif // _MSC_VER

#ifndef _ARCHIVEFILE_H_
#define _ARCHIVEFILE_H_

#include "asciistring.h"
#include "file.h"
#include "rtsutils.h"
#include <map>
#include <set>

struct FileInfo;
class File;

struct ArchivedFileInfo
{
    AsciiString FileName;
    AsciiString ArchiveName;
    int Position;
    int Size;
};

struct DetailedArchiveDirectoryInfo
{
    AsciiString Name;
    mutable std::map<AsciiString, DetailedArchiveDirectoryInfo> Directories;    // Mutable to use operator[] in const functions
    std::map<AsciiString, ArchivedFileInfo> Files;
};

class ArchiveFile
{
public:
    ArchiveFile();
    virtual ~ArchiveFile() {}

    virtual bool Get_File_Info(AsciiString const &name, FileInfo *info) = 0;
    virtual File *Open_File(char const *filename, int mode) = 0;
    virtual void Close_All_Files() = 0;
    virtual AsciiString Get_Name() = 0;
    virtual AsciiString Get_Path() = 0;
    virtual void Set_Search_Priority(int priority) = 0;
    virtual void Close() = 0;

    ArchivedFileInfo *Get_Archived_File_Info(AsciiString const &filename);
    void Add_File(AsciiString const &filename, ArchivedFileInfo const *info);
    void Attach_File(File *file);
    void Get_File_List_From_Dir(AsciiString const &subdir, AsciiString const &dirpath, AsciiString const &filter, std::set<AsciiString, rts::less_than_nocase<AsciiString> > &filelist, bool search_subdir) const;

protected:
    void Get_File_List_From_Dir(DetailedArchiveDirectoryInfo const *dir_info, AsciiString const &dirpath, AsciiString const &filter, std::set<AsciiString, rts::less_than_nocase<AsciiString> > &filelist, bool search_subdir) const;

    File *BackingFile;
    DetailedArchiveDirectoryInfo ArchiveInfo;
};

bool Search_String_Matches(AsciiString string, AsciiString search);

#endif // _ARCHIVEFILE_H_
