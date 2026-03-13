/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Implementation of Config File Reader

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <fstream>
#include <list>
#include <configparser.hpp>
#include "CfgReader.h"
#include "Trace.h"

#ifdef _WIN32
    #pragma warning(disable : 4996)         // disable warning "This function or variable may be unsafe"
#endif





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  CfgReader                                               */
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

CfgReader::CfgReader()
{

    m_pCfgParser = NULL;

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

CfgReader::~CfgReader()
{

    if (m_pCfgParser != NULL)
    {
        delete m_pCfgParser;
        m_pCfgParser = NULL;
    }

    return;

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P U B L I C    M E T H O D E N                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  BuildRuntimeConfig
//---------------------------------------------------------------------------

int  CfgReader::BuildRuntimeConfig (const char* pszConfigFile_p, Config* pAppConfig_p)
{

bool         fCfgFileExist;
char         szSection[32];
int          iDevSectNum;
const char*  pszSection;
const char*  pszKey;
std::string  strCfgValue;
long         lCfgValue;
float        flCfgValue;
std::string  strType;
int          iModbusID;
std::string  strLabel;
CfgDevice    CfgDev;
bool         fGasMeterCfg;
int          iRes;

#ifdef _GASMETER_
int          iCfgValue;
#endif


    // check parameters
    if ((pszConfigFile_p == NULL) || (pAppConfig_p == NULL))
    {
        TRACE0("\nERROR: Invalid Params!\n");
        return (-1);
    }

    // check if ConfigFile exist
    fCfgFileExist = CheckFileExit(pszConfigFile_p);
    if ( !fCfgFileExist )
    {
        fprintf(stderr, "ERROR: Can't open Config '%s'\n", pszConfigFile_p);
        return (-2);
    }


    // clear configuration
    pAppConfig_p->m_CfgSettings.m_lBaudrate = 0;
    pAppConfig_p->m_CfgSettings.m_lQueryInterval = 0;
    pAppConfig_p->m_CfgSettings.m_strMqttBroker = "";
    pAppConfig_p->m_lstCfgDevList.clear();

    #ifdef _GASMETER_
    {
        pAppConfig_p->m_CfgGasMeter.m_iSensorGpio = -1;
    }
    #endif
    fGasMeterCfg = false;

    // create new Parser object
    m_pCfgParser = new ConfigParser(std::string(pszConfigFile_p));


    //------------------------------------------------------------------------
    // read Application settings "[Settings]"
    //------------------------------------------------------------------------
    pszSection = "Settings";

    pszKey = "MbInterface";             // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_strMbInterface = strCfgValue;

    pszKey = "Baudrate";                // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    iRes = sscanf(strCfgValue.c_str(), "%ld", &lCfgValue);
    if (iRes != 1)
    {
        fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_lBaudrate = lCfgValue;

    pszKey = "QueryInterval";           // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    iRes = sscanf(strCfgValue.c_str(), "%ld", &lCfgValue);
    if (iRes != 1)
    {
        fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_lQueryInterval = lCfgValue;

    pszKey = "InterDevPause";           // optional parameter
    flCfgValue = 0;
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ( !(strCfgValue.empty()) && !(strCfgValue == "") )
    {
        iRes = sscanf(strCfgValue.c_str(), "%f", &flCfgValue);
        if (iRes != 1)
        {
            fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
            return (-3);
        }
    }
    pAppConfig_p->m_CfgSettings.m_lInterDevPause = (long)(flCfgValue * 1000);

    pszKey = "MqttBroker";              // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_strMqttBroker = strCfgValue;

    pszKey = "ClientName";              // optional parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    pAppConfig_p->m_CfgSettings.m_strClientName = strCfgValue;

    pszKey = "UserName";                // optional parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    pAppConfig_p->m_CfgSettings.m_strUserName = strCfgValue;

    pszKey = "Password";                // optional parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    pAppConfig_p->m_CfgSettings.m_strPassword = strCfgValue;

    pszKey = "DataTopic";               // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_strDataTopic = strCfgValue;

    pszKey = "StatTopic";               // mandatory parameter
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ((strCfgValue.empty()) || (strCfgValue == ""))
    {
        fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
        return (-3);
    }
    pAppConfig_p->m_CfgSettings.m_strStatTopic = strCfgValue;

    pszKey = "JsonInfoLevel";           // optional parameter
    lCfgValue = (long)tJsonInfoLevel::kNone;
    strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
    if ( !(strCfgValue.empty()) && !(strCfgValue == "") )
    {
        iRes = sscanf(strCfgValue.c_str(), "%ld", &lCfgValue);
        if (iRes != 1)
        {
            fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
            return (-3);
        }
        if ((lCfgValue < (long)tJsonInfoLevel::kNone) || (lCfgValue > tJsonInfoLevel::kAddDevInfo))
        {
            fprintf(stderr, "ERROR: Parameter is out of range (Section: '[%s]', Key: '%s', Value=%ld)\n", pszSection, pszKey, lCfgValue);
            return (-4);
        }
    }
    pAppConfig_p->m_CfgSettings.m_JsonInfoLevel = (tJsonInfoLevel)lCfgValue;


    //------------------------------------------------------------------------
    // read Modbus Device sections "[DeviceNNN]"
    //------------------------------------------------------------------------
    // Reading the device sections always starts with [Device001]. After this,
    // an attempt is then made to read the [Device002] section, then the
    // [Device003] section and so on. As soon as a section is not available,
    // reading is terminated. This means that the sections must always be
    // numbered consecutively, gaps in the numbering are not possible.
    //------------------------------------------------------------------------
    iDevSectNum = 1;
    while (true)
    {
        snprintf(szSection, sizeof(szSection), "Device%03d", iDevSectNum);

        pszKey = "Type";                // control parameter, terminate reading of further [DeviceXxx] sectionsin if not existing
        strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(szSection, pszKey));
        if ((strCfgValue.empty()) || (strCfgValue == ""))
        {
            // if no further entry was found, then interpret this as the end of the list with Modbus Devices
            break;
        }
        strType = strCfgValue;

        pszKey = "ModbusID";            // mandatory parameter
        strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(szSection, pszKey));
        if ((strCfgValue.empty()) || (strCfgValue == ""))
        {
            fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
            return (-3);
        }
        iRes = sscanf(strCfgValue.c_str(), "%ld", &lCfgValue);
        if (iRes != 1)
        {
            fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
            return (-3);
        }
        if ((lCfgValue < 1) || (lCfgValue > 247))
        {
            fprintf(stderr, "ERROR: Parameter is out of range (Section: '[%s]', Key: '%s', Value=%ld)\n", pszSection, pszKey, lCfgValue);
            return (-4);
        }
        iModbusID = (int)lCfgValue;

        pszKey = "Label";               // optional parameter
        strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(szSection, pszKey));
        strLabel = strCfgValue;

        // append Modbus Device at the end of list
        CfgDev = CfgDevice(strType, iModbusID, strLabel);
        pAppConfig_p->m_lstCfgDevList.push_back(CfgDev);

        iDevSectNum++;
    }


    //------------------------------------------------------------------------
    // read GasMeter (S0) Settings "[GasMeter]"
    //------------------------------------------------------------------------
    // A GasMeter is an optional device. Therefore, the section "[GasMeter]"
    // is also optional. If a GasMeter is present or not ist determined by
    // the parameter "SensorGpio=". If a valid value is specified for this
    // parameter (plausibility check), the following parameters are also read
    // and GasMeter support is activated in the application.
    //------------------------------------------------------------------------
    #ifdef _GASMETER_
    {
        pszSection = "GasMeter";

        pszKey = "SensorGpio";              // optional section
        iCfgValue = -1;
        pAppConfig_p->m_CfgGasMeter.m_iSensorGpio = iCfgValue;
        strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
        if ( !(strCfgValue.empty()) && !(strCfgValue == "") )
        {
            iRes = sscanf(strCfgValue.c_str(), "%d", &iCfgValue);
            if (iRes != 1)
            {
                fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                return (-3);
            }
            if ((iCfgValue >= 0) && (iCfgValue <= 63))      // plausibility check
            {
                fGasMeterCfg = true;
            }
            else
            {
                fprintf(stderr, "ERROR: Parameter is out of range (Section: '[%s]', Key: '%s', Value=%d)\n", pszSection, pszKey, iCfgValue);
                return (-4);
            }
            pAppConfig_p->m_CfgGasMeter.m_iSensorGpio = iCfgValue;

            pszKey = "Elasticity";          // optional parameter
            iCfgValue = 1;
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            if ( !(strCfgValue.empty()) && !(strCfgValue == "") )
            {
                iRes = sscanf(strCfgValue.c_str(), "%d", &iCfgValue);
                if (iRes != 1)
                {
                    fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                    return (-3);
                }
                if ((iCfgValue < 1) || (iCfgValue > 100))
                {
                    fprintf(stderr, "ERROR: Parameter is out of range (Section: '[%s]', Key: '%s', Value=%d)\n", pszSection, pszKey, iCfgValue);
                    return (-4);
                }
            }
            pAppConfig_p->m_CfgGasMeter.m_iElasticity = iCfgValue;

            pszKey = "CounterDelta";        // optional parameter
            iCfgValue = 1;
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            if ( !(strCfgValue.empty()) && !(strCfgValue == "") )
            {
                iRes = sscanf(strCfgValue.c_str(), "%d", &iCfgValue);
                if (iRes != 1)
                {
                    fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                    return (-3);
                }
                if ((iCfgValue < 1) || (iCfgValue > 10))
                {
                    fprintf(stderr, "ERROR: Parameter is out of range (Section: '[%s]', Key: '%s', Value=%d)\n", pszSection, pszKey, iCfgValue);
                    return (-4);
                }
            }
            pAppConfig_p->m_CfgGasMeter.m_iCounterDelta = iCfgValue;

            pszKey = "VolumeFactor";        // mandatory parameter
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            if ((strCfgValue.empty()) || (strCfgValue == ""))
            {
                fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                return (-3);
            }
            iRes = sscanf(strCfgValue.c_str(), "%f", &flCfgValue);
            if (iRes != 1)
            {
                fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                return (-3);
            }
            pAppConfig_p->m_CfgGasMeter.m_flVolumeFactor = flCfgValue;

            pszKey = "EnergyFactor";        // mandatory parameter
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            if ((strCfgValue.empty()) || (strCfgValue == ""))
            {
                fprintf(stderr, "ERROR: Required parameter does't exist (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                return (-3);
            }
            iRes = sscanf(strCfgValue.c_str(), "%f", &flCfgValue);
            if (iRes != 1)
            {
                fprintf(stderr, "ERROR: Parameter is invalid (Section: '[%s]', Key: '%s')\n", pszSection, pszKey);
                return (-3);
            }
            pAppConfig_p->m_CfgGasMeter.m_flEnergyFactor = flCfgValue;

            pszKey = "RetainFile";          // optional parameter
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            pAppConfig_p->m_CfgGasMeter.m_strRetainFile = strCfgValue;

            pszKey = "Label";               // optional parameter
            strCfgValue = TrimValueLine(m_pCfgParser->aConfig<std::string>(pszSection, pszKey));
            pAppConfig_p->m_CfgGasMeter.m_strLabel = strCfgValue;
        }
    }
    #endif  // #ifdef _GASMETER_


    // check if a valid configuration has been detected for at least one device
    if ((pAppConfig_p->m_lstCfgDevList.size() == 0) && (!fGasMeterCfg))
    {
        fprintf(stderr, "ERROR: No Device Section found\n");
        return (-5);
    }


    return (0);

}



//---------------------------------------------------------------------------
//  GetRuntimeConfigAsText
//---------------------------------------------------------------------------

std::string  CfgReader::GetRuntimeConfigAsText (Config* pAppConfig_p)
{

std::list<CfgDevice>::iterator  lstCfgDevIterator;
std::string  strConfigText;
char         szBuff[128];
int          iDevSectNum;
const char*  pszJsonInfoLevel;
CfgDevice    CfgDev;


    // check parameters
    if (pAppConfig_p == NULL)
    {
        strConfigText = "Parameter Error";
        return (strConfigText);
    }


    //------------------------------------------------------------------------
    // get Application settings "[Settings]"
    //------------------------------------------------------------------------
    strConfigText  = "\n";
    strConfigText += "[Settings]\n";
    snprintf(szBuff, sizeof(szBuff), "MbInterface = %s\n", pAppConfig_p->m_CfgSettings.m_strMbInterface.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "Baudrate = %d\n", pAppConfig_p->m_CfgSettings.m_lBaudrate);
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "QueryInterval = %d\n", pAppConfig_p->m_CfgSettings.m_lQueryInterval);
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "InterDevPause = %.2f\n", ((float)pAppConfig_p->m_CfgSettings.m_lInterDevPause) / 1000);
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "MqttBroker = %s\n", pAppConfig_p->m_CfgSettings.m_strMqttBroker.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "ClientName = %s\n", pAppConfig_p->m_CfgSettings.m_strClientName.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "UserName = %s\n", pAppConfig_p->m_CfgSettings.m_strUserName.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "Password = %s\n", pAppConfig_p->m_CfgSettings.m_strPassword.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "DataTopic = %s\n", pAppConfig_p->m_CfgSettings.m_strDataTopic.c_str());
    strConfigText += szBuff;
    snprintf(szBuff, sizeof(szBuff), "StatTopic = %s\n", pAppConfig_p->m_CfgSettings.m_strStatTopic.c_str());
    strConfigText += szBuff;
    switch (pAppConfig_p->m_CfgSettings.m_JsonInfoLevel)
    {
        case tJsonInfoLevel::kNone:         pszJsonInfoLevel = "0 (None)";          break;
        case tJsonInfoLevel::kTimeStamp:    pszJsonInfoLevel = "1 (TimeStamp)";     break;
        case tJsonInfoLevel::kAddDevInfo:   pszJsonInfoLevel = "2 (AddDevInfo)";    break;
        default:                            pszJsonInfoLevel = "? (unknown)";       break;
    }
    snprintf(szBuff, sizeof(szBuff), "JsonInfoLevel = %s\n", pszJsonInfoLevel);
    strConfigText += szBuff;


    //------------------------------------------------------------------------
    // get Modbus Device sections "[DeviceNNN]"
    //------------------------------------------------------------------------
    iDevSectNum = 1;
    for (lstCfgDevIterator=pAppConfig_p->m_lstCfgDevList.begin(); lstCfgDevIterator!=pAppConfig_p->m_lstCfgDevList.end(); lstCfgDevIterator++)
    {
        CfgDev = *lstCfgDevIterator;

        strConfigText += "\n";
        snprintf(szBuff, sizeof(szBuff), "[Device%03d]\n", iDevSectNum++);
        strConfigText += szBuff;
        snprintf(szBuff, sizeof(szBuff), "Type = %s\n", CfgDev.m_strType.c_str());
        strConfigText += szBuff;
        snprintf(szBuff, sizeof(szBuff), "ModbusID = %d\n", CfgDev.m_iModbusID);
        strConfigText += szBuff;
        snprintf(szBuff, sizeof(szBuff), "Label = %s\n", CfgDev.m_strLabel.c_str());
        strConfigText += szBuff;
    }


    //------------------------------------------------------------------------
    // get Gasmeter Device section "[GasMeter]"
    //------------------------------------------------------------------------
    #ifdef _GASMETER_
    {
        if (pAppConfig_p->m_CfgGasMeter.m_iSensorGpio >= 0)
        {
            strConfigText += "\n";
            strConfigText += "[GasMeter]\n";
            snprintf(szBuff, sizeof(szBuff), "SensorGpio = %d\n", pAppConfig_p->m_CfgGasMeter.m_iSensorGpio);
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "Elasticity = %d\n", pAppConfig_p->m_CfgGasMeter.m_iElasticity);
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "CounterDelta = %d\n", pAppConfig_p->m_CfgGasMeter.m_iCounterDelta);
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "VolumeFactor = %f\n", pAppConfig_p->m_CfgGasMeter.m_flVolumeFactor);
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "EnergyFactor = %f\n", pAppConfig_p->m_CfgGasMeter.m_flEnergyFactor);
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "RetainFile = %s\n", pAppConfig_p->m_CfgGasMeter.m_strRetainFile.c_str());
            strConfigText += szBuff;
            snprintf(szBuff, sizeof(szBuff), "Label = %s\n", pAppConfig_p->m_CfgGasMeter.m_strLabel.c_str());
            strConfigText += szBuff;
        }
    }
    #endif


    return (strConfigText);

}




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E    M E T H O D E N                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: Check if File exist
//---------------------------------------------------------------------------

bool  CfgReader::CheckFileExit (const char* pszConfigFile_p)
{

std::ifstream*  pCfgFile;
bool            fCfgFileExist;


    if (pszConfigFile_p == NULL)
    {
        return (false);
    }

    pCfgFile = new std::ifstream(pszConfigFile_p);
    fCfgFileExist = pCfgFile->good();
    delete pCfgFile;

    return (fCfgFileExist);

}



//---------------------------------------------------------------------------
//  Private: Trim Value Line by removing comments at the end of line
//---------------------------------------------------------------------------

std::string  CfgReader::TrimValueLine (std::string strValueLine_p)
{

size_t  nPos;


    nPos = strValueLine_p.find(';');    // search for comment separator ';'
    if ((nPos != std::string::npos) && (nPos > 0))
    {
        strValueLine_p = strValueLine_p.substr(0, nPos);
    }

    return (strValueLine_p);

}



//  EOF


