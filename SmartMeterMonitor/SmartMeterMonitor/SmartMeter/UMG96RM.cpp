/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Implementation for Janitza UMG96RM

  -------------------------------------------------------------------------

  Revision History:

  2026/04/24 -rs:   V1.00 Initial version (for Janitza UMG96RM)

****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cstdarg>
#include <time.h>
#include <modbus.h>
#include "UMG96RM.h"
#include "LibSys.h"
#include "Trace.h"

#ifdef _WIN32
    #pragma warning(disable : 4996)         // disable warning "This function or variable may be unsafe"
#endif





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  UMG96RM                                                 */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          C O N S T R U C T O R   /   D E S T R U C T O R                //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------

UMG96RM::UMG96RM()
{

    m_pMBInst = NULL;
    m_iModbusSlaveID = -1;

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

UMG96RM::~UMG96RM()
{

    if (m_pMBInst != NULL)
    {
        modbus_free(m_pMBInst);
        m_pMBInst = NULL;
    }

    return;

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P U B L I C    M E T H O D E N                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Setup
//---------------------------------------------------------------------------

SmartMeter::tError  UMG96RM::Setup (const char* pszRtuInterface_p, long lBaudrate_p, int iModbusSlaveID_p, const char* pszLabel_p /* = NULL */)
{

    TRACE2("UMG96RM: Create Modbus Interface ('%s', %dBd, 8N1)...\n", pszRtuInterface_p, lBaudrate_p);
    m_pMBInst = modbus_new_rtu(pszRtuInterface_p, lBaudrate_p, 'N', 8, 1);
    if (m_pMBInst == NULL)
    {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return (tError::kInitError);
    }
    TRACE1("UMG96RM: m_pMBInst = 0x%04X\n", (unsigned long)m_pMBInst);

    TRACE1("UMG96RM: Set active Modbus Slave to ID=%u\n", iModbusSlaveID_p);
    modbus_set_slave(m_pMBInst, iModbusSlaveID_p);
    m_iModbusSlaveID = iModbusSlaveID_p;

    memset(m_szLabel, 0, sizeof(m_szLabel));
    if (pszLabel_p != NULL)
    {
        strncpy(m_szLabel, pszLabel_p, sizeof(m_szLabel)-1);
        m_szLabel[sizeof(m_szLabel)-1] = '\0';
    }

    return (tError::kSuccess);

};



//---------------------------------------------------------------------------
//  ReadModbusRegs
//---------------------------------------------------------------------------

SmartMeter::tError  UMG96RM::ReadModbusRegs (void)
{

int     iNumOfRegs;
int     iRes;
tError  ErrCode;


    TRACE0("UMG96RM: Open Modbus RTU...\n");
    ErrCode = tError::kSuccess;

    if ((m_pMBInst == NULL) || (m_iModbusSlaveID < 1))
    {
        fprintf(stderr, "ERROR: Instance is not initialized\n");
        return (tError::kUninitialised);
    }

    TRACE0("UMG96RM: Connect to Modbus Interface...\n");
    iRes = modbus_connect(m_pMBInst);
    if (iRes == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        return (tError::kConnectFailed);
    }

    TRACE0("UMG96RM: Flush Modbus Interface...\n");
    iRes = modbus_flush(m_pMBInst);
    if (iRes == -1)
    {
        fprintf(stderr, "Initial flush failed: %s\n", modbus_strerror(errno));
        return (tError::kFlushFailed);
    }

    TRACE2("UMG96RM: Read RegisterBlock from Slave (RegStart=%d, RegCount=%d)...\n", MODBUS_REG_START, MODBUS_REG_COUNT);
    iNumOfRegs = modbus_read_registers(m_pMBInst, MODBUS_REG_START, MODBUS_REG_COUNT, m_aui16ModbusRegBuff);
    if (iNumOfRegs != MODBUS_REG_COUNT)
    {
        fprintf(stderr, "Failed to read: %s\n", modbus_strerror(errno));
        ErrCode = tError::kReadFailed;
    }
    TRACE1("UMG96RM: Read %d registers\n", iNumOfRegs);


    TRACE0("UMG96RM: Disconnect from Modbus Interface...\n");
    modbus_close(m_pMBInst);

    TRACE0("UMG96RM: Modbus RTU released.\n");
    return (ErrCode);

}



//---------------------------------------------------------------------------
//  DumpModbusRegs
//---------------------------------------------------------------------------

const char*  UMG96RM::DumpModbusRegs (void)
{

int  iIdx;


    memset(m_szModbusRegDump, '\0', sizeof(m_szModbusRegDump));

    for (iIdx=0; iIdx<MODBUS_REG_COUNT; iIdx++)
    {
        LibSys::JoinStr(m_szModbusRegDump, sizeof(m_szModbusRegDump), "Reg[%05d]: %5d  (%4XH)\n", (MODBUS_REG_START + iIdx), (int)m_aui16ModbusRegBuff[iIdx], (int)m_aui16ModbusRegBuff[iIdx]);
    }

    TRACE2("UMG96RM: DumpModbusRegs -> %d of %d byte buffer was used\n", strlen(m_szModbusRegDump), sizeof(m_szModbusRegDump));
    return (m_szModbusRegDump);

}



//---------------------------------------------------------------------------
//  GetMeasurements
//---------------------------------------------------------------------------

void  UMG96RM::GetMeasurements (UMG96RM::tMeasurements* pMeasurements_p)
{

    DecodeMeasurements(pMeasurements_p);
    return;

}



//---------------------------------------------------------------------------
//  GetMeasurementsAsText
//---------------------------------------------------------------------------

const char*  UMG96RM::GetMeasurementsAsText (void)
{

const char*  pszText;


    pszText = BuildTextString();
    return (pszText);

}



//---------------------------------------------------------------------------
//  GetMeasurementsAsJson
//---------------------------------------------------------------------------

const char*  UMG96RM::GetMeasurementsAsJson (time_t tmTimeStamp_p /* = 0 */, bool fAddDevInfo_p /* = false */)
{

const char*  pszJson;


    pszJson = BuildJsonString(tmTimeStamp_p, fAddDevInfo_p);
    return (pszJson);

}



//---------------------------------------------------------------------------
//  GetModbusSlaveID
//---------------------------------------------------------------------------

int  UMG96RM::GetModbusSlaveID (void)
{

    return (m_iModbusSlaveID);

}



//---------------------------------------------------------------------------
//  GetDevTypeName
//---------------------------------------------------------------------------

const char*  UMG96RM::GetDevTypeName (void)
{

    return (DEV_TYPE_NAME);

}



//---------------------------------------------------------------------------
//  GetErrorText
//---------------------------------------------------------------------------

const char*  UMG96RM::GetErrorText (SmartMeter::tError SmError_p)
{

const char*  paszErrorText;


    switch (SmError_p)
    {
        case SmartMeter::kSuccess:
        {
            paszErrorText = "No Error";
            break;
        }

        case SmartMeter::kIdleRead:
        {
            paszErrorText = "Idle Read (no Data)";
            break;
        }

        case SmartMeter::kInvalidArg:
        {
            paszErrorText = "Invalid Arguments";
            break;
        }

        case SmartMeter::kUninitialised:
        {
            paszErrorText = "Interface is not Initialized";
            break;
        }

        case SmartMeter::kInitError:
        {
            paszErrorText = "Initialization Error";
            break;
        }

        case SmartMeter::kConnectFailed:
        {
            paszErrorText = "Connection Failed";
            break;
        }

        case SmartMeter::kFlushFailed:
        {
            paszErrorText = "Flush Failed";
            break;
        }

        case SmartMeter::kReadFailed:
        {
            paszErrorText = "Modbus Read Error";
            break;
        }

        case SmartMeter::kInternalErr:
        {
            paszErrorText = "Internal Error";
            break;
        }

        default:
        {
            paszErrorText = "Unknown / Unspecified Error";
            break;
        }
    }

    return (paszErrorText);

}




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E    M E T H O D E N                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: DecodeMeasurements
//---------------------------------------------------------------------------

void  UMG96RM::DecodeMeasurements (UMG96RM::tMeasurements* pMeasurements_p)
{

    // convert measurements from Modbus Register representation into Float variables
    pMeasurements_p->m_flVoltage_L1  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[0]);             // Reg 19000/19001: Voltage L1               [V]
    pMeasurements_p->m_flVoltage_L2  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[2]);             // Reg 19002/19003: Voltage L2               [V]
    pMeasurements_p->m_flVoltage_L3  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[4]);             // Reg 19004/19005: Voltage L3               [V]
    pMeasurements_p->m_flCurrent_L1  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[12]);            // Reg 19012/19013: Current I L1             [A]
    pMeasurements_p->m_flCurrent_L2  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[14]);            // Reg 19014/19015: Current I L2             [A]
    pMeasurements_p->m_flCurrent_L3  = modbus_get_float_abcd(&m_aui16ModbusRegBuff[16]);            // Reg 19016/19017: Current I L3             [A]
    pMeasurements_p->m_flPower_L1    = modbus_get_float_abcd(&m_aui16ModbusRegBuff[20]);            // Reg 19020/19021: Real power P1 L1         [W]
    pMeasurements_p->m_flPower_L2    = modbus_get_float_abcd(&m_aui16ModbusRegBuff[22]);            // Reg 19022/19023: Real power P2 L2         [W]
    pMeasurements_p->m_flPower_L3    = modbus_get_float_abcd(&m_aui16ModbusRegBuff[24]);            // Reg 19024/19025: Real power P3 L3         [W]
    pMeasurements_p->m_flCurrent_Sum = modbus_get_float_abcd(&m_aui16ModbusRegBuff[18]);            // Reg 19018/19019: Vector sum IN=I1+I2+I3   [A]
    pMeasurements_p->m_flPower_Sum   = modbus_get_float_abcd(&m_aui16ModbusRegBuff[26]);            // Reg 19026/19027: Sum Psum3=P1+P2+P3       [W]
    pMeasurements_p->m_flFrequency   = modbus_get_float_abcd(&m_aui16ModbusRegBuff[50]);            // Reg 19050/19051: Frequency                [Hz]
    pMeasurements_p->m_flEnergy      = modbus_get_float_abcd(&m_aui16ModbusRegBuff[68]) / 1000.0f;  // Reg 19068/19069: Real energy L1..L3 [Wh -> kWh]

    return;

}



//---------------------------------------------------------------------------
//  BuildTextString
//---------------------------------------------------------------------------

const char*  UMG96RM::BuildTextString (void)
{

UMG96RM::tMeasurements  Measurements;


    memset(m_szMeasurementsText, '\0', sizeof(m_szMeasurementsText));

    DecodeMeasurements(&Measurements);

    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Voltage_L1  (Reg 19000/19001):\t%.1f [V]\n",   Measurements.m_flVoltage_L1);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Voltage_L2  (Reg 19002/19003):\t%.1f [V]\n",   Measurements.m_flVoltage_L2);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Voltage_L3  (Reg 19004/19005):\t%.1f [V]\n",   Measurements.m_flVoltage_L3);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Current_L1  (Reg 19012/19013):\t%.3f [A]\n",   Measurements.m_flCurrent_L1);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Current_L2  (Reg 19014/19015):\t%.3f [A]\n",   Measurements.m_flCurrent_L2);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Current_L3  (Reg 19016/19017):\t%.3f [A]\n",   Measurements.m_flCurrent_L3);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Power_L1    (Reg 19020/19021):\t%.3f [W]\n",   Measurements.m_flPower_L1);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Power_L2    (Reg 19022/19023):\t%.3f [W]\n",   Measurements.m_flPower_L2);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Power_L3    (Reg 19024/19025):\t%.3f [W]\n",   Measurements.m_flPower_L3);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Current_Sum (Reg 19018/19019):\t%.3f [A]\n",   Measurements.m_flCurrent_Sum);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Power_Sum   (Reg 19026/19027):\t%.3f [W]\n",   Measurements.m_flPower_Sum);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Frequency   (Reg 19050/19051):\t%.2f [Hz]\n",  Measurements.m_flFrequency);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Energy      (Reg 19068/19069):\t%.3f [kWh]\n", Measurements.m_flEnergy);

    TRACE2("UMG96RM: BuildTextString -> %d of %d byte buffer was used\n", strlen(m_szMeasurementsText), sizeof(m_szMeasurementsText));
    return (m_szMeasurementsText);

}



//---------------------------------------------------------------------------
//  BuildJsonString
//---------------------------------------------------------------------------

const char*  UMG96RM::BuildJsonString (time_t tmTimeStamp_p, bool fAddDevInfo_p)
{

UMG96RM::tMeasurements  Measurements;
char                    szTimeStamp[64];


    memset(m_szMeasurementsJson, '\0', sizeof(m_szMeasurementsJson));

    DecodeMeasurements(&Measurements);

    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "{\n");
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Type\": \"%s\",\n", DEV_TYPE_NAME);
    if (tmTimeStamp_p != 0)
    {
        LibSys::FormatTimeStamp(tmTimeStamp_p, szTimeStamp, sizeof(szTimeStamp));
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"TimeStamp\": %u,\n", (unsigned)tmTimeStamp_p);
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"TimeStampFmt\": \"%s\",\n", szTimeStamp);
    }
    if ( fAddDevInfo_p )
    {
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"ModbusID\": %d,\n", m_iModbusSlaveID);
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Label\": \"%s\",\n", m_szLabel);
    }
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Voltage_L1\": %.1f,\n",  Measurements.m_flVoltage_L1);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Voltage_L2\": %.1f,\n",  Measurements.m_flVoltage_L2);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Voltage_L3\": %.1f,\n",  Measurements.m_flVoltage_L3);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Current_L1\": %.3f,\n",  Measurements.m_flCurrent_L1);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Current_L2\": %.3f,\n",  Measurements.m_flCurrent_L2);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Current_L3\": %.3f,\n",  Measurements.m_flCurrent_L3);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Power_L1\": %.3f,\n",    Measurements.m_flPower_L1);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Power_L2\": %.3f,\n",    Measurements.m_flPower_L2);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Power_L3\": %.3f,\n",    Measurements.m_flPower_L3);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Power_Sum\": %.3f,\n",   Measurements.m_flPower_Sum);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Current_Sum\": %.3f,\n", Measurements.m_flCurrent_Sum);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Frequency\": %.2f,\n",   Measurements.m_flFrequency);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Energy\": %.3f\n",       Measurements.m_flEnergy);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "}\n");

    TRACE2("UMG96RM: BuildJsonString -> %d of %d byte buffer was used\n", strlen(m_szMeasurementsJson), sizeof(m_szMeasurementsJson));
    return (m_szMeasurementsJson);

}



//  EOF


