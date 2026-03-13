/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      Project independend / Standard class
  Description:  Class <SysLib> Implementation

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <cstdarg>
#include <time.h>
#include "LibSys.h"

#ifdef _WIN32
    #pragma warning(disable : 4996)         // disable warning "This function or variable may be unsafe"
#endif





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LibSys                                                  */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P U B L I C    M E T H O D E N                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: JoinStr
//---------------------------------------------------------------------------

size_t  LibSys::JoinStr (char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...)
{

va_list  pArgList;
size_t   nUsedBuffLen;
size_t   nFreeBuffLen;


    if ((pszLogBuff_p == NULL) || (nLogBuffSize_p == 0) || (pszFmt_p == NULL))
    {
        return (0);
    }

    nUsedBuffLen = strlen(pszLogBuff_p);
    nFreeBuffLen = nLogBuffSize_p - nUsedBuffLen;
    if (nFreeBuffLen <= 0)
    {
        return (0);
    }

    va_start(pArgList, pszFmt_p);
    vsnprintf(&pszLogBuff_p[nUsedBuffLen], nFreeBuffLen, pszFmt_p, pArgList);
    va_end(pArgList);

    return (strlen(pszLogBuff_p) - nUsedBuffLen);

}



//---------------------------------------------------------------------------
//  Get current Date/Time as String as well as Seconds since 01.01.1970
//---------------------------------------------------------------------------

time_t  LibSys::GetTimeStamp (char* pszBuffer_p /* = NULL */, int iBuffSize_p /* = 0 */)
{

time_t  tmTimeStamp;


    tmTimeStamp = time(NULL);               // Seconds since 01.01.1970

    if ((pszBuffer_p != NULL) && (iBuffSize_p > 0))
    {
        FormatTimeStamp(tmTimeStamp, pszBuffer_p, iBuffSize_p);
    }

    return (tmTimeStamp);

}



//---------------------------------------------------------------------------
//  Format TimeStamp as Date/Time String
//---------------------------------------------------------------------------

size_t  LibSys::FormatTimeStamp (time_t tmTimeStamp_p, char* pszBuffer_p, size_t nBuffSize_p)
{

struct tm*  LocTime;
size_t      nStrLen;


    LocTime = localtime(&tmTimeStamp_p);

    snprintf(pszBuffer_p, nBuffSize_p, "%04d/%02d/%02d - %02d:%02d:%02d",
             LocTime->tm_year + 1900, LocTime->tm_mon + 1, LocTime->tm_mday,
             LocTime->tm_hour, LocTime->tm_min, LocTime->tm_sec);

    nStrLen = strlen(pszBuffer_p);

    return (nStrLen);

}



//  EOF


