////////////////////////////////////////////////////////////////////////////////
//                               --  THYME  --                                //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: CHUNKIO.H
//
//        Author:: OmniBlade
//
//  Contributors:: 
//
//   Description:: Classes for reading and writing the binary chunk format used
//                 in WW3D and SAGE games.
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
#include    "chunkio.h"
#include    "fileclass.h"
#include    "gamedebug.h"

////////////////////////////////////////////////////////////////////////////////
// TODO 
//
// These classes currently assume little endian arch and so do no byte swapping
// to host order.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Class Constructor.
/// \param
///     file - Pointer to a FileClass instance to use to write the chunk.
/// \see 
///     FileClass
////////////////////////////////////////////////////////////////////////////////
ChunkSaveClass::ChunkSaveClass(FileClass *file) :
    m_file(file),
    m_stackIndex(0),
    m_inMicroChunk(false),
    m_microChunkPos(0),
    m_microChunkHeader()
{
    //
    // Original code does this, seems pointless when it memsets it to 0 again
    // just after anyway though.
    //
    //for ( int i = 0; i < ARRAY_SIZE(m_headerStack); ++i ) {
    //    m_headerStack[i].Set_Type(0);
    //    m_headerStack[i].Set_Size(0);
    //}

    memset(m_positionStack, 0, sizeof(m_positionStack));
    memset(m_headerStack, 0, sizeof(m_headerStack));
}

////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Starts a new chunk in the current file.
/// \param
///     id - ID number for the chunk type to be written.
/// \return
///     Bool indicating if the chunk header was written successfully or not.
////////////////////////////////////////////////////////////////////////////////
bool ChunkSaveClass::Begin_Chunk(unsigned int id)
{
    ChunkHeader chunkh;

    //
    // Set a flag on the type to flag as nested?
    //
    if ( m_stackIndex > 0 ) {
        m_headerStack[m_stackIndex - 1].Set_Sub_Chunk_Flag(0);
    }

    chunkh.Set_Type(id);
    chunkh.Set_Size(0);

    m_positionStack[m_stackIndex] = m_file->Seek(0);
    m_headerStack[m_stackIndex] = chunkh;
    ++m_stackIndex;

    return m_file->Write(&chunkh, sizeof(chunkh)) == sizeof(chunkh);
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Marks the end of the current working chunk.
/// \return
///     Bool indicating if the end of the chunk was successfully written.
////////////////////////////////////////////////////////////////////////////////
bool ChunkSaveClass::End_Chunk()
{
    int startpos = m_file->Seek(0);
    --m_stackIndex;

    ChunkHeader chunkh = m_headerStack[m_stackIndex];
    m_file->Seek(m_positionStack[m_stackIndex], FS_SEEK_START);

    if ( m_file->Write(&chunkh, sizeof(chunkh)) == sizeof(chunkh) ) {
        if ( m_stackIndex > 0 ) {
            m_headerStack[m_stackIndex - 1].Add_Size(chunkh.Get_Size() + sizeof(chunkh));
        }

        m_file->Seek(startpos, FS_SEEK_START);

        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Starts a new micro chunk in the current file.
/// \param
///     id - ID number for the chunk type to be written.
/// \return
///     Bool indicating if the chunk header was written successfully or not.
////////////////////////////////////////////////////////////////////////////////
bool ChunkSaveClass::Begin_Micro_Chunk(unsigned int id)
{
    DEBUG_ASSERT_PRINT(id < MAX_STACK_DEPTH, "id is outside the stack range.\n");
    DEBUG_ASSERT_PRINT(!m_inMicroChunk, "Already in a micro chunk.\n");

    m_microChunkHeader.Set_Type(id);
    m_microChunkHeader.Set_Size(0);
    m_microChunkPos = m_file->Seek(0);

    if ( Write(&m_microChunkHeader, sizeof(m_microChunkHeader)) == sizeof(m_microChunkHeader) ) {
        m_inMicroChunk = true;

        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Marks the end of the current working micro chunk.
/// \return
///     Bool indicating if the end of the chunk was successfully written.
////////////////////////////////////////////////////////////////////////////////
bool ChunkSaveClass::End_Micro_Chunk()
{
    DEBUG_ASSERT_PRINT(m_inMicroChunk, "Not in a micro chunk.\n");

    int current_pos = m_file->Seek(0);
    m_file->Seek(m_microChunkPos, FS_SEEK_START);

    if ( Write(&m_microChunkHeader, sizeof(m_microChunkHeader)) == sizeof(m_microChunkHeader) ) {
        m_file->Seek(current_pos, FS_SEEK_START);
        m_inMicroChunk = false;

        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Write arbitrary data to a chunk.
/// \param
///     buf - Pointer to the source of the data.
/// \param
///     bytes - Number of bytes to write from the source buffer.
/// \return
///     Number of bytes written.
////////////////////////////////////////////////////////////////////////////////
// Inline here should just inline the call in the other write functions.
inline unsigned int ChunkSaveClass::Write(void const *buf, unsigned int bytes)
{
    DEBUG_ASSERT_PRINT(m_headerStack[m_stackIndex - 1].Get_Sub_Chunk_Flag() == 0, "Subchunk flag is set.\n");
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index below 1.\n");

    if ( m_file->Write(buf, bytes) != bytes ) {
        return 0;
    }

    m_headerStack[m_stackIndex - 1].Add_Size(bytes);

    if ( m_inMicroChunk ) {
        m_microChunkHeader.Add_Size(bytes);
    }

    return bytes;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Write a 2D Vector to the chunk.
/// \param
///     vect - Vector to write to the chunk.
/// \return
///     Number of bytes written.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkSaveClass::Write(IOVector2Struct const &vect)
{
    return Write(&vect, sizeof(vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Write a 3D Vector to the chunk.
/// \param
///     vect - Vector to write to the chunk.
/// \return
///     Number of bytes written.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkSaveClass::Write(IOVector3Struct const &vect)
{
    return Write(&vect, sizeof(vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Write a 4D Vector to the chunk.
/// \param
///     vect - Vector to write to the chunk.
/// \return
///     Number of bytes written.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkSaveClass::Write(IOVector4Struct const &vect)
{
    return Write(&vect, sizeof(vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Write a Quaternion to the chunk.
/// \param
///     quat - Quaternion to write to the chunk.
/// \return
///     Number of bytes written.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkSaveClass::Write(IOQuaternionStruct const &quat)
{
    return Write(&quat, sizeof(quat));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Class Constructor.
/// \param
///     file - Pointer to a FileClass instance to use to read the chunk.
/// \see 
///     FileClass
////////////////////////////////////////////////////////////////////////////////
ChunkLoadClass::ChunkLoadClass(FileClass *file) :
    m_file(file),
    m_stackIndex(0),
    m_inMicroChunk(false),
    m_microChunkPos(0),
    m_microChunkHeader()
{
    //
    // Original code does this, seems pointless when it memsets it to 0 again
    // just after anyway though.
    //
    //for ( int i = 0; i < ARRAY_SIZE(m_headerStack); ++i ) {
    //    m_headerStack[i].Set_Type(0);
    //    m_headerStack[i].Set_Size(0);
    //}

    memset(m_positionStack, 0, sizeof(m_positionStack));
    memset(m_headerStack, 0, sizeof(m_headerStack));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Opens the chunk at the current file position.
/// \return
///     Bool indicating if a chunk was opened.
////////////////////////////////////////////////////////////////////////////////
bool ChunkLoadClass::Open_Chunk()
{
    DEBUG_ASSERT_PRINT(!m_inMicroChunk, "Currently in a micro chunk.\n");
    DEBUG_ASSERT_PRINT(m_stackIndex < MAX_STACK_DEPTH - 1, "Stack is too full.\n");

    if ( m_stackIndex > 0 && m_positionStack[m_stackIndex - 1] == m_headerStack[m_stackIndex - 1].Get_Size() ) {
        return false;
    }

    if ( m_file->Read(&m_headerStack[m_stackIndex], sizeof(m_headerStack[0])) == sizeof(m_headerStack[0]) ) {
        m_positionStack[m_stackIndex++] = 0;

        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Close the current chunk.
/// \return
///     Bool indicating if the chunk was closed.
////////////////////////////////////////////////////////////////////////////////
bool ChunkLoadClass::Close_Chunk()
{
    DEBUG_ASSERT_PRINT(!m_inMicroChunk, "Currently in a micro chunk.\n");
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index less than 1.\n");

    int chunksize = m_headerStack[m_stackIndex - 1].Get_Size();
    int position = m_positionStack[m_stackIndex - 1];

    if ( position < chunksize ) {
        m_file->Seek(chunksize - position);
    }

    --m_stackIndex;

    if ( m_stackIndex > 0 ) {
        m_positionStack[m_stackIndex - 1] += chunksize + sizeof(*m_headerStack);
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Gets the ID of the current chunk.
/// \return
///     The chunk ID.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Cur_Chunk_ID()
{
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index less than 1.\n");

    return m_headerStack[m_stackIndex - 1].Get_Type();
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Get the size of the current chunk.
/// \return
///     Size of the current chunk in bytes.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Cur_Chunk_Length()
{
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index less than 1.\n");

    return m_headerStack[m_stackIndex - 1].Get_Size();
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Returns if the current chunk itself contains chunks.
/// \return
///     Int containing value of the flag indicating if it contains other chunks. 
////////////////////////////////////////////////////////////////////////////////
int ChunkLoadClass::Contains_Chunks()
{
    return m_headerStack[m_stackIndex - 1].Get_Sub_Chunk_Flag();
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Opens a micro chunk at the current file position.
/// \return
///     Bool indicating if a chunk was opened. 
////////////////////////////////////////////////////////////////////////////////
bool ChunkLoadClass::Open_Micro_Chunk()
{
    DEBUG_ASSERT_PRINT(!m_inMicroChunk, "Already in a micro chunk.\n");

    if ( Read(&m_microChunkHeader, sizeof(m_microChunkHeader)) == sizeof(m_microChunkHeader) ) {
        m_microChunkPos = 0;
        m_inMicroChunk = true;

        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Closes the current micro chunk.
/// \return
///     Bool indicating if the chunk was closed.
////////////////////////////////////////////////////////////////////////////////
bool ChunkLoadClass::Close_Micro_Chunk()
{
    DEBUG_ASSERT_PRINT(m_inMicroChunk, "Not in a micro chunk.\n");

    m_inMicroChunk = false;

    if ( m_microChunkPos < m_microChunkHeader.Get_Size() ) {
        m_file->Seek(m_microChunkHeader.Get_Size() - m_microChunkPos);

        if ( m_stackIndex > 0 ) {
            m_positionStack[m_stackIndex - 1] += m_microChunkHeader.Get_Size() - m_microChunkPos;
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Get the ID of the current micro chunk.
/// \return
///     ID of the micro chunk.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Cur_Micro_Chunk_ID()
{
    DEBUG_ASSERT_PRINT(m_inMicroChunk, "Not in a micro chunk.\n");

    return m_microChunkHeader.Get_Type();
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Get the length of the current micro chunk.
/// \return
///     Length of the micro chunk in bytes.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Cur_Micro_Chunk_Length()
{
    DEBUG_ASSERT_PRINT(m_inMicroChunk, "Not in a micro chunk.\n");

    return m_microChunkHeader.Get_Size();
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Seeks to a position within the file.
/// \param
///     pos - Position in the file to seek to.
/// \return
///     New position in the file.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Seek(unsigned int bytes)
{
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index less than 1.\n");
    DEBUG_ASSERT_PRINT(m_file->Is_Open(), "File is not open for seeking.\n");

    if ( bytes + m_positionStack[m_stackIndex - 1] > m_headerStack[m_stackIndex - 1].Get_Size() ) {
        return 0;
    }

    if ( m_inMicroChunk && bytes + m_microChunkPos > m_microChunkHeader.Get_Size() ) {
        return 0;
    }

    int current = m_file->Tell();

    if ( m_file->Seek(bytes) - current == bytes ) {
        m_positionStack[m_stackIndex - 1] += bytes;

        if ( m_inMicroChunk ) {
            m_microChunkPos += bytes;
        }

        return bytes;
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Reads an arbitrary amount of data from the file.
/// \param
///     buf - Buffer to read data into.
/// \param
///     bytes - Number of bytes to read from the file into the buffer.
/// \return
///     Number of bytes actually read.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Read(void *buf, unsigned int bytes)
{
    DEBUG_ASSERT_PRINT(m_stackIndex > 0, "Stack index less than 1.\n");
    DEBUG_ASSERT_PRINT(m_file->Is_Open(), "File is not open for reading.\n");

    if ( bytes + m_positionStack[m_stackIndex - 1] > m_headerStack[m_stackIndex - 1].Get_Size() ) {
        return 0;
    }

    if ( m_inMicroChunk && bytes + m_microChunkPos > m_microChunkHeader.Get_Size() ) {
        return 0;
    }

    if ( m_file->Read(buf, bytes) != bytes ) {
        return 0;
    }

    m_positionStack[m_stackIndex - 1] += bytes;

    if ( m_inMicroChunk ) {
        m_microChunkPos += bytes;
    }

    return bytes;
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Reads a 2D vector from the file.
/// \param
///     vect - Pointer to a vector that data will be loaded into.
/// \return
///     Number of bytes read.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Read(IOVector2Struct *vect)
{
    // Bug in original code has them read sizeof(vect) which is the pointer size
    // not the size of the struct. This applies to the next 3 functions as well.
    return Read(vect, sizeof(*vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Reads a 3D vector from the file.
/// \param
///     vect - Pointer to a vector that data will be loaded into.
/// \return
///     Number of bytes read.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Read(IOVector3Struct *vect)
{
    return Read(vect, sizeof(*vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Reads a 4D vector from the file.
/// \param
///     vect - Pointer to a vector that data will be loaded into.
/// \return
///     Number of bytes read.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Read(IOVector4Struct *vect)
{
    return Read(vect, sizeof(*vect));
}


////////////////////////////////////////////////////////////////////////////////
/// \brief
///     Reads a quaternion from the file.
/// \param
///     quat - Pointer to a quaternion that data will be loaded into.
/// \return
///     Number of bytes read.
////////////////////////////////////////////////////////////////////////////////
unsigned int ChunkLoadClass::Read(IOQuaternionStruct *quat)
{
    return Read(quat, sizeof(*quat));
}
