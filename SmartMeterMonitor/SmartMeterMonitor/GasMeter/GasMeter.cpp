/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Extension for S0 based GasMeter)
  Description:  Implementation for GasMeter

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cstdarg>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include "GasMeter.h"
#include "GMSensIf.h"
#include "LibSys.h"
#include "Trace.h"

#ifdef _WIN32
    #pragma warning(disable : 4996)         // disable warning "This function or variable may be unsafe"
#endif





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  GasMeter                                                */
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

GasMeter::GasMeter()
{

    m_iSensorGpio = -1;

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

GasMeter::~GasMeter()
{

    Close();

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

GasMeter::tError  GasMeter::Setup (int iSensorGpio_p, int iElasticity_p, int iCounterDelta_p, float flVolumeFactor_p, float flEnergyFactor_p, const char* pszRetainFile_p, const char* pszLabel_p /* = NULL */)
{

float   flMeterValue;
int     iRes;
tError  ErrCode;


    TRACE5("GasMeter: Setup (SensorGpio=%d, Elasticity=%d, CounterDelta=%d, VolumeFactor=%f, EnergyFactor=%f,\n", iSensorGpio_p, iElasticity_p, iCounterDelta_p, flVolumeFactor_p, flEnergyFactor_p);
    TRACE2("                 RetainFile='%s', Label='%s')...\n", (pszRetainFile_p ? pszRetainFile_p : "NULL"), (pszLabel_p ? pszLabel_p : "NULL"));

    m_ulSensorCounterValue = 0;
    m_iSensorGpio = iSensorGpio_p;
    m_iElasticity = iElasticity_p;
    m_iCounterDelta = iCounterDelta_p;
    m_flVolumeFactor = flVolumeFactor_p;
    m_flEnergyFactor = flEnergyFactor_p;
    m_iPowerCalcCycleCounter = 0;
    m_iPowerRangeCycles = 0;
    m_tmPrevReadTimeStamp = 0;
    m_ulPrevReadCounterValue = 0;
    m_flSavedMeterValue = 0.0;
    m_fForceRecalc = false;

    memset(m_szRetainFile, 0, sizeof(m_szRetainFile));
    if (pszRetainFile_p != NULL)
    {
        strncpy(m_szRetainFile, pszRetainFile_p, sizeof(m_szRetainFile)-1);
        m_szRetainFile[sizeof(m_szRetainFile)-1] = '\0';
    }

    memset(m_szLabel, 0, sizeof(m_szLabel));
    if (pszLabel_p != NULL)
    {
        strncpy(m_szLabel, pszLabel_p, sizeof(m_szLabel)-1);
        m_szLabel[sizeof(m_szLabel)-1] = '\0';
    }

    // check if SensorGpio specifies a valid GPIO Port Number
    if (iSensorGpio_p < 0)
    {
        fprintf(stderr, "ERROR: Invalid SensorGpio for GasMeter (%d)\n", iSensorGpio_p);
        return (GasMeter::kInvalidArg);
    }

    // if there is a RetainFile specified than read initial MeterValue from it
    flMeterValue = 0.0;
    if (strlen(m_szRetainFile) > 0)
    {
        TRACE1("GasMeter: Read initial MeterValue from RetainFile '%s'...\n", m_szRetainFile);
        ErrCode = ReadRetainMeterValue(m_szRetainFile, &flMeterValue);
        if (ErrCode != GasMeter::kSuccess)
        {
            return (ErrCode);
        }
        m_flSavedMeterValue = flMeterValue;
        TRACE1("GasMeter: Initial MeterValue: '%.3f'\n", flMeterValue);
    }

    // set initial CounterValue
    // (scale the value to an integer type, taking m_flVolumeFactor into account, so that the
    // counter value can be increased by a simple increment with each counting pulse)
    m_ulSensorCounterValue = (long)(flMeterValue / m_flVolumeFactor);
    TRACE1("GasMeter: Initial SensorCounterValue: %ld\n", m_ulSensorCounterValue.load());

    // set initial Measurement Data Record
    m_Measurements.m_ulCounterValue = m_ulSensorCounterValue;               // Counting unit pulses
    m_Measurements.m_flMeterValue   = flMeterValue;                         // MeterValue [m3]
    m_Measurements.m_flPower        = 0.0;                                  // Power [W]
    m_Measurements.m_flEnergy       = flMeterValue * m_flEnergyFactor;      // Energy [kWh]

    // set Workspace Variables to startup values
    m_ulPrevReadCounterValue = m_Measurements.m_ulCounterValue;
    m_tmPrevReadTimeStamp = LibSys::GetTimeStamp();
    m_fForceRecalc = true;

    // setup GasMeter Sensor Interface
    iRes = SensorInterfaceSetup(iSensorGpio_p, &m_ulSensorCounterValue);
    if (iRes != 0)
    {
        fprintf(stderr, "ERROR: Open GasMeter Sensor Interface failed. (%d)\n", iRes);
        return (GasMeter::kSensIfSupError);
    }

    return (tError::kSuccess);

};



//---------------------------------------------------------------------------
//  Close
//---------------------------------------------------------------------------

GasMeter::tError  GasMeter::Close (void)
{

int  iRes;


    TRACE0("GasMeter: Close...\n");

    // setup GasMeter Sensor Interface
    iRes = SensorInterfaceClose();
    if (iRes != 0)
    {
        fprintf(stderr, "ERROR: Close GasMeter Sensor Interface failed. (%d)\n", iRes);
        return (GasMeter::kSensIfSupError);
    }

    return (tError::kSuccess);

}
    
    

//---------------------------------------------------------------------------
//  ReadCounter
//---------------------------------------------------------------------------

GasMeter::tError  GasMeter::ReadCounter (void)
{

static  const  float  SECONDS_PER_HOUR = 3600;

unsigned long  ulReadCounterValue;
time_t         tmReadTimeStamp;
unsigned long  ulReadCounterDiff;
time_t         tmMessurePeriodTimeSpan;
float          flMeterValue;                            // MeterValue [m3]
float          flPower;                                 // Power [W]
float          flEnergy;                                // Energy [kWh]
int            iRes;
tError         ErrCode;


    TRACE0("GasMeter: ReadCounter...\n");
    ErrCode = tError::kSuccess;

    // call GasMeter Sensor Interface
    iRes = SensorInterfaceReadCounter();
    if (iRes != 0)
    {
        fprintf(stderr, "ERROR: ReadCounter from GasMeter Sensor Interface failed. (%d)\n", iRes);
        return (GasMeter::kSensIfRdError);
    }

    //-------------------------------------------------------------------
    // Fetch free running SensorCounterValue to save Workspace Variable
    ulReadCounterValue = m_ulSensorCounterValue.load();
    tmReadTimeStamp = LibSys::GetTimeStamp();
    TRACE2("GasMeter: CurrReadCounterValue = %lu   (-> TimeStamp=%lu)\n", ulReadCounterValue, (unsigned long)tmReadTimeStamp);
    //-------------------------------------------------------------------

    // calculate basic Measurement Values
    flMeterValue = (float)ulReadCounterValue * m_flVolumeFactor;
    flEnergy = (float)ulReadCounterValue * m_flEnergyFactor;

    // set Measurement Data Record
    m_Measurements.m_ulCounterValue = ulReadCounterValue;               // Counting unit pulses
    m_Measurements.m_flMeterValue   = flMeterValue;                     // MeterValue [m3]
    m_Measurements.m_flPower        = -1;                               // Power [W]  (-> will be evaluated/calculated subsequently)
    m_Measurements.m_flEnergy       = flEnergy;                         // Energy [kWh]

    //-------------------------------------------------------------------
    // Recalculation of 'Power' if:
    // (1) CounterValue has increased by at least 'CounterDelta' since last calculation
    //     - OR -
    // (2) Value 'm_iPowerCalcCycleCounter' has reached/exceeded the value 'm_iElasticity'
    //     - OR -
    // (3) Flag 'm_fForceRecalc' is set
    //-------------------------------------------------------------------
    m_iPowerCalcCycleCounter++;

    // Has the CounterValue been increased by at least the amount of 'CounterDelta' since last sending?
    if ( (ulReadCounterValue >= (m_ulPrevReadCounterValue + m_iCounterDelta)) ||
         (m_iPowerCalcCycleCounter >= m_iElasticity) ||
         ( m_fForceRecalc ) )
    {
        TRACE0("GasMeter: Recalculation of Measurement Value 'Power'\n");
        TRACE1("GasMeter:   Condition 'CounterDelta'    -> %s\n", (ulReadCounterValue >= (m_ulPrevReadCounterValue + m_iCounterDelta)) ? "true" : "false");
        TRACE1("GasMeter:   Condition 'Elasticity'      -> %s\n", (m_iPowerCalcCycleCounter >= m_iElasticity) ? "true" : "false");
        TRACE1("GasMeter:   Condition 'ForceRecalc'     -> %s\n", (m_fForceRecalc) ? "true" : "false");

        // determine difference of CounterValue and TimeSpan since the last calculation of Power Value
        ulReadCounterDiff = ulReadCounterValue - m_ulPrevReadCounterValue;
        tmMessurePeriodTimeSpan = tmReadTimeStamp - m_tmPrevReadTimeStamp;
        TRACE2("GasMeter: ulReadCounterDiff = %lu   (-> MessurePeriodTimeSpan=%lu)\n", ulReadCounterDiff, (unsigned long)tmMessurePeriodTimeSpan);

        // calculation of the Power Value, taking into account of the elapsed measurement period
        if (tmMessurePeriodTimeSpan > 0)    // check 'MessurePeriodTimeSpan' to prevent division by zero when there is no time difference
        {
            flPower = (ulReadCounterDiff * m_flEnergyFactor) * (SECONDS_PER_HOUR / (float)tmMessurePeriodTimeSpan) * 1000;
        }
        else
        {
            flPower = 0.0;
        }
        m_Measurements.m_flPower = flPower;                                 // Power [W]

        // save processed CounterValue and its TimeStamp (needed for next Period of Power calclation)
        m_ulPrevReadCounterValue = ulReadCounterValue;
        m_tmPrevReadTimeStamp = tmReadTimeStamp;

        // save IdleCycles for embedding in JSON Record an reset IdleCycleCounter
        m_iPowerRangeCycles = m_iPowerCalcCycleCounter;
        m_iPowerCalcCycleCounter = 0;
    }
    else
    {
        TRACE2("GasMeter: CounterValue unchanged -> Idle Cycle (%d of max. %d)\n", m_iPowerCalcCycleCounter, m_iElasticity);
        m_iPowerRangeCycles = m_iPowerCalcCycleCounter;
    }

    TRACE1("GasMeter: CounterValue = %lu [Imp]\n",  m_Measurements.m_ulCounterValue);
    TRACE1("GasMeter: MeterValue   = %.3f [m3]\n",  m_Measurements.m_flMeterValue);
    TRACE1("GasMeter: Power        = %.3f [W]\n",   m_Measurements.m_flPower);
    TRACE1("GasMeter: Energy       = %.3f [kWh]\n", m_Measurements.m_flEnergy);

    // if there is a RetainFile specified than write current MeterValue to it (if MeterValue has increased by at least 1 m3)
    if (strlen(m_szRetainFile) > 0)
    {
        TRACE0("GasMeter: Check if current MeterValue has increased by at least 1 m3...\n");
        flMeterValue = (float)m_Measurements.m_ulCounterValue * m_flVolumeFactor;
        if (flMeterValue >= (m_flSavedMeterValue + SAVE_DELTA_VALUE))   // minimum increase of MeterValue before rewriting ReatinFile?
        {
            TRACE2("GasMeter: Write current MeterValue %f to RetainFile '%s'...\n", flMeterValue, m_szRetainFile);
            ErrCode = WriteRetainMeterValue(m_szRetainFile, flMeterValue);
            if (ErrCode == GasMeter::kSuccess)
            {
                m_flSavedMeterValue = flMeterValue;
            }
            else
            {
                fprintf(stderr, "ERROR: Write current MeterValue to RetainFile failed. ('%s')\n", m_szRetainFile);
            }
        }
        else
        {
            TRACE0("GasMeter: Ignore writing due to insufficient value difference.\n");
        }
    }

    m_fForceRecalc = false;

    return (ErrCode);

}



//---------------------------------------------------------------------------
//  DumpCounter
//---------------------------------------------------------------------------

const char*  GasMeter::DumpCounter (void)
{

    memset(m_szCounterDump, '\0', sizeof(m_szCounterDump));

    LibSys::JoinStr(m_szCounterDump, sizeof(m_szCounterDump), "CounterValue: %5d\n", m_Measurements.m_ulCounterValue);
    LibSys::JoinStr(m_szCounterDump, sizeof(m_szCounterDump), "MeterValue:   %08.3f\n", m_Measurements.m_flMeterValue);

    TRACE2("GasMeter: DumpCounter -> %d of %d byte buffer was used\n", strlen(m_szCounterDump), sizeof(m_szCounterDump));

    return (m_szCounterDump);

}



//---------------------------------------------------------------------------
//  GetMeasurements
//---------------------------------------------------------------------------

void  GasMeter::GetMeasurements (GasMeter::tMeasurements* pMeasurements_p)
{

    *pMeasurements_p = m_Measurements;
    return;

}



//---------------------------------------------------------------------------
//  GetMeasurementsAsText
//---------------------------------------------------------------------------

const char*  GasMeter::GetMeasurementsAsText (void)
{

const char*  pszText;


    pszText = BuildTextString();
    return (pszText);

}



//---------------------------------------------------------------------------
//  GetMeasurementsAsJson
//---------------------------------------------------------------------------

const char*  GasMeter::GetMeasurementsAsJson (time_t tmTimeStamp_p /* = 0 */, bool fAddDevInfo_p /* = false */)
{

const char*  pszJson;


    pszJson = BuildJsonString(tmTimeStamp_p, fAddDevInfo_p);
    return (pszJson);

}



//---------------------------------------------------------------------------
//  GetSensorGpioNum
//---------------------------------------------------------------------------

int  GasMeter::GetSensorGpioNum (void)
{

    return (m_iSensorGpio);

}



//---------------------------------------------------------------------------
//  GetDevTypeName
//---------------------------------------------------------------------------

const char*  GasMeter::GetDevTypeName (void)
{

    return (DEV_TYPE_NAME);

}



//---------------------------------------------------------------------------
//  GetErrorText
//---------------------------------------------------------------------------

const char*  GasMeter::GetErrorText (GasMeter::tError SmError_p)
{

const char*  paszErrorText;


    switch (SmError_p)
    {
        case GasMeter::kSuccess:
        {
            paszErrorText = "No Error";
            break;
        }

        case GasMeter::kIdleRead:
        {
            paszErrorText = "Idle Read (no Data)";
            break;
        }

        case GasMeter::kInvalidArg:
        {
            paszErrorText = "Invalid Arguments";
            break;
        }

        case GasMeter::kFileReadError:
        {
            paszErrorText = "Can't read File";
            break;
        }

        case GasMeter::kFileWriteError:
        {
            paszErrorText = "Can't write File";
            break;
        }

        case GasMeter::kInvalidData:
        {
            paszErrorText = "Invalid Data in File";
            break;
        }

        case GasMeter::kSensIfSupError:
        {
            paszErrorText = "Sensor Interface Setup Error";
            break;
        }

        case GasMeter::kSensIfRdError:
        {
            paszErrorText = "Sensor Interface ReadConter Error";
            break;
        }

        case GasMeter::kInternalErr:
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
//  Private: ReadRetainMeterValue
//---------------------------------------------------------------------------

GasMeter::tError  GasMeter::ReadRetainMeterValue (const char* pszRetainFile_p, float* pflMeterValue_p)
{

std::ifstream*  pRetainFile;
std::string     strLine;
std::string     strValue;
float           flValue;
std::size_t     nPos;
int             iRes;
tError          ErrCode;


    TRACE2("GasMeter: ReadRetainMeterValue (pszRetainFile_p=0x%08lX, pflMeterValue_p=0x%08lX)...\n", pszRetainFile_p, pflMeterValue_p);
    flValue = 0.0;
    ErrCode = GasMeter::kSuccess;

    if ((pszRetainFile_p == NULL) || (pflMeterValue_p == NULL))
    {
        fprintf(stderr, "ERROR: Invalid Arguments\n");
        return (GasMeter::kInvalidArg);
    }

    // open RetainFile for reading
    pRetainFile = new std::ifstream(pszRetainFile_p);
    if (pRetainFile == NULL)
    {
        fprintf(stderr, "ERROR: Can't open File '%s'\n", pszRetainFile_p);
        return (GasMeter::kFileReadError);
    }
    if ( !pRetainFile->is_open() )
    {
        fprintf(stderr, "ERROR: Can't open File '%s'\n", pszRetainFile_p);
        delete pRetainFile;
        return (GasMeter::kFileReadError);
    }

    // read initial Counter Value from RetainFile
    while (std::getline(*pRetainFile, strLine))
    {
        // check if line starts with "MeterValue ="
        if (strLine.find(KEY_METER_VALUE) != std::string::npos)
        {
            // extract the value that follows to "MeterValue ="
            nPos = strLine.find('=');
            if (nPos != std::string::npos)
            {
                // cut off the part folowing to "=" and trim it
                strValue = strLine.substr(nPos + 1);
                strValue.erase(0, strValue.find_first_not_of(" \n\r\t"));
                strValue.erase(strValue.find_last_not_of(" \n\r\t") + 1);

                // convert the extracted string to a float
                if ( !(strValue.empty()) && !(strValue == "") )
                {
                    iRes = sscanf(strValue.c_str(), "%f", &flValue);
                    if (iRes != 1)
                    {
                        fprintf(stderr, "ERROR: MeterValue is invalid ('%s')\n", strValue.c_str());
                        ErrCode = GasMeter::kInvalidData;
                    }
                }
            }
            break;
        }
    }

    pRetainFile->close();
    delete pRetainFile;

    *pflMeterValue_p = flValue;

    return (ErrCode);

}



//---------------------------------------------------------------------------
//  Private: WriteRetainMeterValue
//---------------------------------------------------------------------------

GasMeter::tError  GasMeter::WriteRetainMeterValue (const char* pszRetainFile_p, float flMeterValue_p)
{

std::ofstream*  pRetainFile;
std::string     strLine;
char            szLine[64];
tError          ErrCode;


    TRACE2("GasMeter: WriteRetainMeterValue (pszRetainFile_p=0x%08lX, flMeterValue_p=%f)...\n", pszRetainFile_p, flMeterValue_p);
    ErrCode = GasMeter::kSuccess;

    if (pszRetainFile_p == NULL)
    {
        fprintf(stderr, "ERROR: Invalid Arguments\n");
        return (GasMeter::kInvalidArg);
    }

    // open RetainFile for writing, trunc (clear full contents) if exists
    pRetainFile = new std::ofstream(pszRetainFile_p, std::ios::trunc);          
    if (pRetainFile == NULL)
    {
        fprintf(stderr, "ERROR: Can't open File '%s'\n", pszRetainFile_p);
        return (GasMeter::kFileWriteError);
    }
    if ( !pRetainFile->is_open() )
    {
        fprintf(stderr, "ERROR: Can't open File '%s'\n", pszRetainFile_p);
        delete pRetainFile;
        return (GasMeter::kFileWriteError);
    }

    // write MeterValue to RetainFile
    snprintf(szLine, sizeof(szLine), "\n%s %.3f\n\n", KEY_METER_VALUE, flMeterValue_p);
    pRetainFile->write(szLine, strlen(szLine));
    if ( pRetainFile->fail() )
    {
        fprintf(stderr, "ERROR: Writing to File '%s' failed.\n", pszRetainFile_p);
        ErrCode = GasMeter::kFileWriteError;
    }

    pRetainFile->close();
    delete pRetainFile;

    return (ErrCode);

}



//---------------------------------------------------------------------------
//  BuildTextString
//---------------------------------------------------------------------------

const char*  GasMeter::BuildTextString (void)
{

    memset(m_szMeasurementsText, '\0', sizeof(m_szMeasurementsText));

    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "CounterValue:\t%lu [Imp]\n",  m_Measurements.m_ulCounterValue);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "MeterValue:  \t%.3f [m3]\n",  m_Measurements.m_flMeterValue);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Power:       \t%.1f [W]\n",   m_Measurements.m_flPower);
    LibSys::JoinStr(m_szMeasurementsText, sizeof(m_szMeasurementsText), "Energy:      \t%.3f [kWh]\n", m_Measurements.m_flEnergy);

    TRACE2("GasMeter: BuildTextString -> %d of %d byte buffer was used\n", strlen(m_szMeasurementsText), sizeof(m_szMeasurementsText));
    return (m_szMeasurementsText);

}



//---------------------------------------------------------------------------
//  BuildJsonString
//---------------------------------------------------------------------------

const char*  GasMeter::BuildJsonString (time_t tmTimeStamp_p, bool fAddDevInfo_p)
{

char  szTimeStamp[64];


    memset(m_szMeasurementsJson, '\0', sizeof(m_szMeasurementsJson));

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
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Label\": \"%s\",\n", m_szLabel);
        LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"PowerRangeCycles\": %d,\n", m_iPowerRangeCycles);
    }
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Counter\": %lu,\n",     m_Measurements.m_ulCounterValue);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"MeterValue\": %.3f,\n", m_Measurements.m_flMeterValue);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Power\": %.3f,\n",      m_Measurements.m_flPower);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "  \"Energy\": %.3f\n",      m_Measurements.m_flEnergy);
    LibSys::JoinStr(m_szMeasurementsJson, sizeof(m_szMeasurementsJson), "}\n");

    TRACE2("GasMeter: BuildJsonString -> %d of %d byte buffer was used\n", strlen(m_szMeasurementsJson), sizeof(m_szMeasurementsJson));
    return (m_szMeasurementsJson);

}



//  EOF


