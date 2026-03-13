/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Declarations for Eastron SDM630

  -------------------------------------------------------------------------

  When implementing this class, top priority was given to robust and
  reliable 24/7 continuous operation. For this reason, types such as
  std::string, which work with dynamic memory management at runtime,
  were not used. Static buffers are used instead.

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _SDM630_H_
#define _SDM630_H_

#include "SmartMeter.h"



/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  SDM630                                                  */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class SDM630 : public SmartMeter
{

    //-----------------------------------------------------------------------
    //  Definitions
    //-----------------------------------------------------------------------

    public:

        typedef struct
        {
            float           m_flVoltage_L1;                 // Reg  0/1:  Voltage L1                 [V]
            float           m_flVoltage_L2;                 // Reg  2/3:  Voltage L2                 [V]
            float           m_flVoltage_L3;                 // Reg  4/5:  Voltage L3                 [V]
            float           m_flCurrent_L1;                 // Reg  6/7:  Current L1                 [A]
            float           m_flCurrent_L2;                 // Reg  8/9:  Current L2                 [A]
            float           m_flCurrent_L3;                 // Reg 10/11: Current L3                 [A]
            float           m_flPower_L1;                   // Reg 12/13: Power L1                   [W]
            float           m_flPower_L2;                   // Reg 14/15: Power L2                   [W]
            float           m_flPower_L3;                   // Reg 16/17: Power L3                   [W]
            float           m_flCurrent_Sum;                // Reg 48/49: Current L3                 [A]
            float           m_flPower_Sum;                  // Reg 52/53: Total system power         [W]
            float           m_flFrequency;                  // Reg 70/71: Frequency                  [Hz]
            float           m_flEnergy;                     // Reg 72/73: Total import active energy [kWh]

        } tMeasurements;


    private:

        const char*         DEV_TYPE_NAME       = "SDM630";
        static  const int   MODBUS_REG_START    =  0;
        static  const int   MODBUS_REG_COUNT    = 74;



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        SDM630();
        ~SDM630();

        SmartMeter::tError  Setup (const char* pszRtuInterface_p, long lBaudrate_p, int iModbusSlaveID_p, const char* pszLabel_p = NULL);
        SmartMeter::tError  ReadModbusRegs (void);
        const char*         DumpModbusRegs (void);

        void                GetMeasurements (SDM630::tMeasurements* pMeasurements_p);
        const char*         GetMeasurementsAsText (void);
        const char*         GetMeasurementsAsJson (time_t tmTimeStamp_p = 0, bool fAddDevInfo_p = false);

        int                 GetModbusSlaveID (void);
        const char*         GetDevTypeName (void);
        const char*         GetErrorText (SmartMeter::tError Sdm630Err_p);


    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        modbus_t*           m_pMBInst                               = NULL;
        int                 m_iModbusSlaveID                        = -1;
        uint16_t            m_aui16ModbusRegBuff[MODBUS_REG_COUNT]  = { 0 };
        char                m_szLabel[128]                          = { 0 };

        char                m_szModbusRegDump[4096]                 = { 0 };
        char                m_szMeasurementsText[1024]              = { 0 };
        char                m_szMeasurementsJson[1024]              = { 0 };



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        void                DecodeMeasurements (SDM630::tMeasurements* pMeasurements_p);
        const char*         BuildTextString (void);
        const char*         BuildJsonString (time_t tmTimeStamp_p, bool fAddDevInfo_p);

};



#endif  // _SDM630_H_


