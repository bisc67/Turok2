//==============================================================================
//
//  e_cdfs.hpp
//
//==============================================================================

#ifndef E_CDFS_HPP
#define E_CDFS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "x_files.hpp"

//==============================================================================
//  Initialization and shut down.
//==============================================================================

xbool   cdfs_Init           ( const char* pFilesystemFile );
void    cdfs_Kill           ( void );

//==============================================================================
//  Logging
//==============================================================================

void    cdfs_WriteLog       ( const char* pPathName );
void    cdfs_ClearLog       ( void );

//==============================================================================

#endif //PS2_CDFS_HPP