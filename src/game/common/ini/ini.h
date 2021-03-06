////////////////////////////////////////////////////////////////////////////////
//                               --  THYME  --                                //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: INI.H
//
//        Author:: OmniBlade
//
//  Contributors:: 
//
//   Description:: Parser for SAGE engine configuration files.
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

#ifndef _INI_H_
#define _INI_H_

#include "asciistring.h"
#include "gamedebug.h"
#include "hookcrt.h"
#include "hooker.h"

class File;
class Xfer;
class INI;

#define SXfer (Make_Global<Xfer*>(0x00A2A6B8))

extern const float _SECONDS_PER_LOGICFRAME_REAL_74;
extern const float _ANGLE_MULTIPLIER;

enum INILoadType
{
    INI_LOAD_INVALID          = 0,
    INI_LOAD_OVERWRITE        = 1,
    INI_LOAD_CREATE_OVERRIDES = 2,
};

// Function pointer type for the field parser functions
typedef void(*inifieldparse_t)(INI *, void *, void *, void const *);
typedef void(*iniblockparse_t)(INI *);

struct LookupListRec
{
    char const *name;
    int value;
};

struct BlockParse
{
    char const *token;
    iniblockparse_t parse_func;
};

struct FieldParse
{
    char const *token;
    inifieldparse_t parse_func;
    void const *user_data;
    int offset;
};

struct MultiIniFieldParse
{
    MultiIniFieldParse() : count(0) { memset(extra_offsets, 0, sizeof(extra_offsets)); }

    void Add(FieldParse *field_parse, unsigned int extra_offset)
    {
        ASSERT_THROW(count < MAX_MULTI_FIELDS, 0xDEAD0001);

        field_parsers[count] = field_parse;
        extra_offsets[count] = extra_offset;
        ++count;
    }

    enum { MAX_MULTI_FIELDS = 16 };

    FieldParse *field_parsers[MAX_MULTI_FIELDS];
    unsigned int extra_offsets[MAX_MULTI_FIELDS];
    int count;
};

class INI
{
friend void Setup_Hooks(); //Enables hooking functions we have as private
public:
    enum
    {
        MAX_LINE_LENGTH = 1028,
        MAX_BUFFER_SIZE = 8192,
    };

    INI();
    ~INI();

    void Load(AsciiString filename, INILoadType type, Xfer *xfer);
    void Load_Directory(AsciiString dir, bool search_subdirs, INILoadType type, Xfer *xfer);

    void Init_From_INI(void *what, FieldParse *parse_table);
    void Init_From_INI_Multi(void *what, MultiIniFieldParse const &parse_table_list);
    void Init_From_INI_Multi_Proc(void *what, void (*proc)(MultiIniFieldParse *));

    char *Get_Next_Token_Or_Null(char const *seps = nullptr);
    char *Get_Next_Token(char const *seps = nullptr);
    char *Get_Next_Sub_Token(char const *expected);
    AsciiString Get_Next_Ascii_String();
    AsciiString Get_Filename() { return m_fileName; }

    // Scan functions
    static int Scan_Science(char const *token);
    static float Scan_PercentToReal(char const *token);
    static float Scan_Real(char const *token);
    static uint32_t Scan_UnsignedInt(char const *token);
    static int32_t Scan_Int(char const *token);
    static bool Scan_Bool(char const *token);
    static int Scan_IndexList(char const *token, char const* const* list);
    static int Scan_LookupList(char const *token, LookupListRec const *list);

    // Field parsing functions
    static void Parse_Bool(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Byte(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Int(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Unsigned(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Positive_None_Zero_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Percent_To_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Angle_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Angular_Velocity_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_AsciiString(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_AsciiString_Vector_Append(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_RGB_Color(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_RGBA_Color_Int(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Color_Int(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Coord2D(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Coord3D(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Index_List(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Duration_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Duration_Int(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Velocity_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Acceleration_Real(INI *ini, void *formal, void *store, void const *user_data);
    static void Parse_Bit_In_Int32(INI *ini, void *formal, void *store, void const *user_data);
        
    // Block parsing functions


    // Hooking function
    static void Hook_Me();

private:
    void Read_Line();
    void Prep_File(AsciiString filename, INILoadType type);
    void Unprep_File();

    File *m_backingFile;
    char m_buffer[MAX_BUFFER_SIZE];
    int m_bufferReadPos;
    int m_bufferData;
    AsciiString m_fileName;
    INILoadType m_loadType;
    int m_lineNumber;
    char m_currentBlock[MAX_LINE_LENGTH];
    char m_blockEnd;
    char const *m_seps;
    char const *m_sepsPercent;
    char const *m_sepsColon;
    char const *m_sepsQuote;
    char const *m_endToken;
    bool m_endOfFile;

    //static Xfer *SXfer;
};

inline void INI::Hook_Me()
{
    Hook_Method((Make_Method_Ptr<char *, INI, char const*>(0x0041D6E0)), &INI::Get_Next_Token);
    Hook_Method((Make_Method_Ptr<char *, INI, char const*>(0x0041D720)), &INI::Get_Next_Token_Or_Null);
    Hook_Method((Make_Method_Ptr<char *, INI, char const*>(0x0041D950)), &INI::Get_Next_Sub_Token);
    Hook_Method((Make_Method_Ptr<void, INI, AsciiString, INILoadType>(0x0041A4B0)), &INI::Prep_File);
    Hook_Method((Make_Method_Ptr<void, INI, void *, MultiIniFieldParse const &>(0x0041D460)), &INI::Init_From_INI_Multi);
    Hook_Method((Make_Method_Ptr<void, INI, AsciiString, INILoadType, Xfer*>(0x0041A5C0)), &INI::Load);
    
    // Field parsing functions
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041ADA0)), &INI::Parse_Bool);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041A980)), &INI::Parse_Byte);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AAB0)), &INI::Parse_Int);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AB20)), &INI::Parse_Unsigned);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AB90)), &INI::Parse_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AC00)), &INI::Parse_Positive_None_Zero_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041BA50)), &INI::Parse_Percent_To_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041ACA0)), &INI::Parse_Angle_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AD20)), &INI::Parse_Angular_Velocity_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041AF20)), &INI::Parse_AsciiString);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041B1B0)), &INI::Parse_AsciiString_Vector_Append);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041BDD0)), &INI::Parse_RGB_Color);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041C100)), &INI::Parse_Color_Int);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041C530)), &INI::Parse_Coord2D);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041C2C0)), &INI::Parse_Coord3D);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041D140)), &INI::Parse_Index_List);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041DB50)), &INI::Parse_Duration_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041DBD0)), &INI::Parse_Duration_Int);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041DD10)), &INI::Parse_Velocity_Real);
    Hook_Function((Make_Function_Ptr<void, INI*, void*, void*, void const*>(0x0041DD90)), &INI::Parse_Acceleration_Real);

}

// Functions for inlining, neater than including in class declaration
inline char *INI::Get_Next_Token_Or_Null(char const *seps)
{
    return crt_strtok(0, seps != nullptr ? seps : m_seps);
}

inline char *INI::Get_Next_Token(char const *seps)
{
    char *ret = crt_strtok(0, seps != nullptr ? seps : m_seps);
    ASSERT_THROW_PRINT(ret != nullptr, 0xDEAD0006, "Expected further tokens\n");

    return ret;
}

inline char *INI::Get_Next_Sub_Token(char const *expected)
{
    ASSERT_PRINT(strcasecmp(Get_Next_Token(m_sepsColon), expected) == 0, "Did not get expected token\n" )
    return Get_Next_Token(m_sepsColon);
}

inline AsciiString INI::Get_Next_Ascii_String()
{
    static char _buffer[1024];
    AsciiString next;

    char *token = Get_Next_Token_Or_Null();

    if ( token != nullptr ) {
        if ( *token == '"' ) {
            if ( strlen(token) > 1 ) {
                strcpy(_buffer, token + 1);
            }

            char *ntoken = Get_Next_Token(m_sepsQuote);

            if ( strlen(ntoken) > 1 && ntoken[1] != '\t' ) {
                strcat(_buffer, " ");
            }

            strcat(_buffer, ntoken);
            next = _buffer;
        } else {
            next = token;
        }
    }

    return next;
}

#endif // _INI_H_
