////////////////////////////////////////////////////////////////////////////////
//                               --  THYME  --                                //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: FILECLASS.H
//
//        Author:: CCHyper
//
//  Contributors:: OmniBlade
//
//   Description:: Interface for WW3D file io.
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

#ifndef _FILECLASS_H_
#define _FILECLASS_H_

#include	"always.h"

enum FileOpenType {
    FM_CLOSED     = 0,
    FM_READ       = 1,
    FM_WRITE      = 2,
    FM_READ_WRITE = FM_READ | FM_WRITE,
};

enum FileSeekType {
    FS_SEEK_START   = 0,    //Seek to position relative to the start of file.
    FS_SEEK_CURRENT = 1,    //Seek to position relative to the current offset.
    FS_SEEK_END     = 2,    //Seek to position relative to the end of file.
};

class FileClass
{
public:
    FileClass() {}
    virtual ~FileClass() {}
        
    virtual char const *File_Name() = 0;
    virtual char const *Set_Name(char const *filename) = 0;
    virtual bool Create() = 0;
    virtual bool Delete() = 0;
    virtual bool Is_Available(bool forced = false) = 0;
    virtual bool const Is_Open() = 0;
    virtual bool Open(char const *filename, int rights = FM_READ) = 0;
    virtual bool Open(int rights = FM_READ) = 0;
    virtual int Read(void *buffer, int length) = 0;
    virtual int Seek(int offset, int whence = FS_SEEK_CURRENT) = 0;
    virtual int Tell();
    virtual int Size() = 0;
    virtual int Write(void const *buffer, int size) = 0;
    virtual void Close() = 0;
    virtual time_t Get_Date_Time();
    virtual bool Set_Date_Time(time_t date_time);
    virtual int Get_File_Handle();
};

#endif // _FILECLASS_H_
