/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      Project independend / Standard class
  Description:  Class <SysLib> Declaration

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LIBSYS_H_
#define _LIBSYS_H_



/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LibSys                                                  */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class LibSys
{

    public:

        static  size_t  JoinStr (char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...);
        static  time_t  GetTimeStamp (char* pszBuffer_p = NULL, int iBuffSize_p = 0);
        static  size_t  FormatTimeStamp (time_t tmTimeStamp_p, char* pszBuffer_p, size_t nBuffSize_p);

};



#endif  // _LIBSYS_H_


