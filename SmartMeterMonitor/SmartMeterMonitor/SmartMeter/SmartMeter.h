/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Declarations for 'SmartMeter' BaseClass

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _SMARTMETER_H_
#define _SMARTMETER_H_



/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          Virtual CLASS  SmartMeter                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class SmartMeter
{

    //-----------------------------------------------------------------------
    //  Definitions
    //-----------------------------------------------------------------------

    public:

        typedef enum
        {
            kSuccess        = 0,
            kIdleRead       = 1,
            kInvalidArg     = 2,
            kUninitialised  = 3,
            kInitError      = 4,
            kConnectFailed  = 5,
            kFlushFailed    = 6,
            kReadFailed     = 7,

            kInternalErr    = 255

        } tError;



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

//      virtual  SmartMeter() { };
        virtual  ~SmartMeter() { };

        virtual  SmartMeter::tError  Setup (const char* pszRtuInterface_p, long lBaudrate_p, int iModbusSlaveID_p, const char* pszLabel_p = NULL) = 0;
        virtual  SmartMeter::tError  ReadModbusRegs (void) = 0;
        virtual  const char*         DumpModbusRegs (void) = 0;

        virtual  const char*         GetMeasurementsAsText (void) = 0;
        virtual  const char*         GetMeasurementsAsJson (time_t tmTimeStamp_p = 0, bool fAddDevInfo_p = false) = 0;

        virtual  int                 GetModbusSlaveID (void) = 0;
        virtual  const char*         GetDevTypeName (void) = 0;
        virtual  const char*         GetErrorText (SmartMeter::tError SmError_p) = 0;

};



#endif  // _SMARTMETER_H_


