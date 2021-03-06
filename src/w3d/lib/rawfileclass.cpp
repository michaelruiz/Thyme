////////////////////////////////////////////////////////////////////////////////
//                               --  THYME  --                                //
////////////////////////////////////////////////////////////////////////////////
//
//  Project Name:: Thyme
//
//          File:: RAWFILECLASS.CPP
//
//        Author:: CCHyper, OmniBlade
//
//  Contributors:: 
//
//   Description:: FileClass for reading files with raw OS API calls.
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
#include "rawfileclass.h"
#include "stringex.h"
#include <fcntl.h>
#include <cstdlib>
#include <sys/stat.h>

// Headers needed for posix open, close, read... etc.
#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <sys/utime.h>

// Make lseek 64bit on windows to match other platforms behaviour?
//#ifdef lseek
//    #undef lseek
//#endif

//#ifdef off_t
//    #undef off_t
//#endif

//#define lseek _lseeki64
//typedef __int64 off_t;
#else
#include <unistd.h>
#endif

void RawFileClass::Reset()
{
    //if file open, close it file before we can create it.
    if ( Is_Open() ) {
        Close();
    }
    
    m_handle = -1;
    
    //free the existing filename if it exists.
    if ( m_filename != nullptr && m_isAllocated ) {

        delete[] m_filename;

        m_filename = nullptr;
        m_isAllocated = false;
    }
    
    //clear the date/time.
    m_dateTime = 0;
}

bool RawFileClass::Create()
{
    //if file open, close it file before we can create it.
    if ( Is_Open() ) {
        Close();
    }

    //open the file with write access.
    if ( Open(FM_WRITE) ) {
        if ( m_biasLength != -1 ) {
            Seek(0, FS_SEEK_START);
        }
        
        Close();
        
        return true;
    }
    
    return false;
}

bool RawFileClass::Delete()
{
    //if file open, close it file before we delete it.
    if ( Is_Open() ) { 
        Close();
    }

    //is the filename valid and the file available?
    if ( m_filename == nullptr ) {
        Error(2);
    } else {
        if ( Is_Available() ) {
        
            //delete the file.
            if ( unlink(m_filename) ) {
                return true;
            } else {
                Error(errno, 0, m_filename);
                return false;
            }
        }
    }

    return false;
}

bool RawFileClass::Open(char const *filename, int rights)
{
    Set_Name(filename);
    
    return Open(rights);
}

bool RawFileClass::Open(int rights)
{
    //
    // close the file if it is already open.
    //
    RawFileClass::Close();

    //
    // make sure we have a valid filename set.
    //
    if ( m_filename == nullptr || !m_isAllocated ) {
        Error(2);
    }
    
    //
    // set the file rights.
    //
    m_rights = rights;

    switch ( rights ) {
        // this needs looking into.
        case FM_READ:
            m_handle = open(m_filename, O_RDONLY, S_IREAD);
            break;
            
        case FM_WRITE:
            m_handle = open(m_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IWRITE);
            break;
            
        case FM_READ_WRITE:
            m_handle = open(m_filename, O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
            break;
            
        default:
            break;
    }
    
    if ( m_biasStart && m_biasLength != -1 ) {
        RawFileClass::Seek(0, FS_SEEK_START);
    }
    
    return m_handle != -1;
}

bool RawFileClass::Is_Available(bool forced)
{
    //if the filename is invalid, the file is not available.
    if ( m_filename == nullptr ) {
        return false;
    }

    //if the file is already open, it is available.
    if ( Is_Open() ) {
        return true;
    }
    
    //Mode appears to indicated if we want to keep the current value of m_handle or not
    if ( !forced ) {
        if ( RawFileClass::Open(FM_READ) ) {
            RawFileClass::Close();

            return true;
        }

        return false;
    }

    int tmp_hnd = open(m_filename, O_RDONLY, S_IREAD);

    if ( tmp_hnd != -1 ) {
        //close the file handle/stream.
        if ( close(m_handle) == -1 ) {
            Error(errno, 0, m_filename);
        } else {
            return true;
        }
    }

    return false;
}

void RawFileClass::Close()
{
    //if the file is NOT open, we can return.
    if ( Is_Open() ) {
        //close the file handle/stream.
        if ( close(m_handle) == -1 )	{
            Error(errno, 0, m_filename);
        }
        
        m_handle = -1;
    }
}

int RawFileClass::Read(void *buffer, int length)
{
    bool opened = false;	// have we opened the file to allow us to read?

    if ( !Is_Open() ) {
        if ( !Open(FM_READ) ) {
            return 0;
        }

        opened = true;
    }
    
    if ( m_biasLength != -1 ) {
        int tmplen = m_biasLength - Seek(0, FS_SEEK_CURRENT);

        if ( tmplen > length ) {
            tmplen = length;
        }
        
        length = tmplen;
    }

    int totalread = 0;

    while ( length > 0 ) {
        int readlen = read(m_handle, buffer, length);
        
        if ( readlen >= 0 ) {
            length -= readlen;
            totalread += readlen;
            
            if ( !readlen ) {
                break;
            }
        } else {
            Error( errno, 0, m_filename );
            break;
        }
    }
    
    if ( opened ) {
        Close();
    }

    return totalread;
}

int RawFileClass::Seek(int offset, int whence)
{
    if ( m_biasLength != -1 ) {
        switch (whence) {
            case FS_SEEK_START:
                if ( offset > m_biasLength ) {
                    offset = m_biasStart + m_biasLength;
                }

                break;
                
            case FS_SEEK_CURRENT:
                break;
                
            case FS_SEEK_END:
                offset = m_biasLength + m_biasStart + offset;
                break;
                
            default:
                break;
        }
    
        off_t seekval = Raw_Seek(offset, whence) - m_biasStart;
        
        if ( seekval < 0 ) {
            return Raw_Seek(m_biasStart + offset, FS_SEEK_START) - m_biasStart;
        }
        
        if ( seekval > m_biasLength ) {
            return Raw_Seek(m_biasLength + m_biasStart, FS_SEEK_START) - m_biasStart;
        }
        
        return seekval;
        
    }

    return Raw_Seek(offset, whence);
}

int RawFileClass::Size()
{		
    int retval = 0;
    int size = 0;
    
    if ( m_biasLength == -1 ) {
    
        if ( Is_Open() ) {
            off_t cur = lseek(m_handle, 0, FS_SEEK_CURRENT);
            
            size = lseek(m_handle, 0, FS_SEEK_END);
            
            if ( size == -1 ) {
                Error(errno, 0, m_filename);
            }
            
            //reset our pos in the file
            lseek(m_handle, cur, FS_SEEK_START);
        } else {
            if ( Open(FM_READ) ) {
                size = Size();
                Close();
            }
        }
    
        m_biasLength = size - m_biasStart;
        retval = m_biasLength;
    
    } else {
        retval = m_biasLength;
    }
    
    return retval;
}

int RawFileClass::Write(void const *buffer, int length)
{
    int writelen = 0;		// total bytes written to file.
    bool opened = false;	// have we opened the file to allow us to write?
    
    if ( !Is_Open() ) {
        if ( !Open(FM_WRITE) ) {
            return 0;
        }

        opened = true;
    }
    
    // write the data in the buffer to the file, and
    // store the total bytes written.
    writelen = write(m_handle, buffer, length);

    if ( writelen < 0 ) {
        Error(errno, 0, m_filename);
    }
    
    if ( m_biasLength != -1 && Raw_Seek(0, FS_SEEK_CURRENT) > (int)(m_biasLength + m_biasStart) ) {
        m_biasLength = Raw_Seek(0, FS_SEEK_CURRENT) - m_biasStart;
    }
    
    // close the file if we opened it.
    if ( opened ) {
        Close();
    }
    
    return writelen;
}

char const *RawFileClass::Set_Name(char const *filename)
{
    //free the existing filename if it exists.
    if ( m_filename && m_isAllocated ) {
        delete[] m_filename; //set with nstrdup which uses new not malloc
        m_filename = nullptr;
        m_isAllocated = false;
    }
    
    //if the argument 'filename' is valid, set this as the filename.
    if ( filename != nullptr ) {
    
        //reset the file bias.
        Bias(0, -1);
        
        m_filename = nstrdup(filename);
        
        //if we could not copy the filename, return a error.
        if ( m_filename == nullptr ) {
        
            Error( 12, 0, m_filename );
            
            return 0;
        }
        
        //the filename has been set succesfully.	
        m_isAllocated = true;

        return m_filename;
    }
        
    return nullptr;  
}

time_t RawFileClass::Get_Date_Time()
{
    struct stat attrib;

    //get stats
    stat(m_filename, &attrib);
    
    return(attrib.st_mtime);
}

bool RawFileClass::Set_Date_Time(time_t datetime)
{
    struct utimbuf tstamp;
    
    //assumes datetime is time_t format
    tstamp.modtime = datetime;
    tstamp.actime = datetime;	

    if( Is_Open() && m_handle != -1) {		
        //set the file time and return if it succeeded
        return utime(m_filename, &tstamp) != 0;
    }
        
    return false;
}

void RawFileClass::Error(int error, bool can_retry, char const *filename)
{
    // Nothing in SAGE
}

void RawFileClass::Bias(int offset, int length)
{
    if ( offset ) {
        int fsize = RawFileClass::Size();
        
        m_biasLength = fsize;
        m_biasStart += offset;
    
        if ( length != -1 ) {
            if ( fsize >= length ) {
                fsize = length;
            }
                
            m_biasLength = fsize;
        }
    
        m_biasLength &= (m_biasLength >> 31) - 1;
        
        //if the file is open, reset seek position.
        if ( Is_Open() ) {
            RawFileClass::Seek(0, FS_SEEK_START);
        }	
    } else {
        m_biasStart = 0;
        m_biasLength = -1;
    }
}

int RawFileClass::Raw_Seek(int offset, int whence)
{
    //if the file is not open, raise an error.
    if ( !Is_Open() ) {
        Error( 9, 0, m_filename );
        return 0;
    }

    int retval = lseek(m_handle, offset, whence);

    //seek to the offset specified, return the position.
    //and if lseek returned with a error, raise an error too.
    if ( retval == -1 ) {
        Error(errno, 0, m_filename);
        return 0;
    }

    return retval;
}

void RawFileClass::Attach(int handle, int rights)
{
    Reset();
    
    m_handle = handle;
    m_rights = rights;
    
    m_biasStart = 0;
    m_biasLength = -1;

    m_dateTime = 0;
}

void RawFileClass::Detach()
{
    m_handle = -1;
    m_rights = FM_CLOSED;
    
    m_biasStart = 0;
    m_biasLength = -1;
    m_dateTime = 0;
}
