/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Implementation of Main Module

  -------------------------------------------------------------------------

    To include a new SmartMeter type, simply add the Class for
    this new Device in the <BuildSmDeviceList> function.

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <list>
#include <fstream>
#include <random>
#include <modbus.h>
#include <configparser.hpp>
#include "CfgReader.h"
#include "LibMqtt.h"
#include "LibSys.h"
#include "SmartMeter.h"
#include "SDM230.h"
#include "SDM630.h"
#include "Trace.h"

#ifdef _GASMETER_
    #include "GasMeter.h"
#endif

#ifdef _WIN32
    // WIN32: allow comfortable debugging in Visual Studio by using RS485/USB Adapter
    #pragma warning(disable : 4996)         // disable warning "This function or variable may be unsafe"
    #include <windows.h>
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
#else
    #include <unistd.h>
#endif





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Configuration
//---------------------------------------------------------------------------

static  const  int              APP_VER_MAIN                    = 1;        // Version 1.xx
static  const  int              APP_VER_REL                     = 0;        // Version x.00
static  const  char             APP_BUILD_TIMESTAMP[]           = __DATE__ " " __TIME__;

static  const  int              MQTT_DEF_HOST_PORTNUM           = 1883;
static  const  char*            MQTT_DEF_CLIENT_NAME            = "SmartMeterMon_<uid>";
static  const  char*            MQTT_DEF_USER_NAME              = "{empty}";
static  const  char*            MQTT_DEF_PASSWORD               = "{empty}";
static  const  unsigned int     MQTT_KEEPALIVE_INTERVAL         = 30;



//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------

// SmartMeter Device Instanze
class SmDevice
{

    public:

        std::string     m_strType;                      // SmartMeter Type Name (eg. "SDM230", "SDM630", ...)
        int             m_iModbusSlaveID;               // Modbus Slave ID
        std::string     m_strLabel;                     // Label / Description of the Purpose of Use
        std::string     m_strDataTopic;                 // MQTT Topic to publish Measurement Data (as JSON Record)
        SmartMeter*     m_pSmartMeter;                  // Pointer to Runtime Object of SmartMeter Class Instanze (eg. SDM230, SDM630, ...)

};


// GasMeter Device Instanze
#ifdef _GASMETER_
class GmDevice
{

    public:

        std::string     m_strType;                      // GasMeter Type Name (eg. "ES-GAS-2")
        int             m_iSensorGpio;                  // Number of GPIO Pin connected to Sensor
        std::string     m_strLabel;                     // Label / Description of the Purpose of Use
        std::string     m_strDataTopic;                 // MQTT Topic to publish Measurement Data (as JSON Record)
        GasMeter*       m_pGasMeter;                    // Pointer to Runtime Object of GasMeter Class Instanze

};
#endif


// Login Data for access to MQTT Broker
class LoginData
{

    public:

        std::string     m_strClientName;
        std::string     m_strUserName;
        std::string     m_strPassword;

};



//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  const char*             pszCfgFileName_l        = NULL;
static  const char*             pszMsgFileName_l        = NULL;
static  int                     fOffline_l              = false;
static  bool                    fVerbose_l              = false;
static  bool                    fRunMainLoop_l          = false;
static  std::ofstream           ofsMsgFile_l;



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  void         AppSigHandler (int iSignalNum_p);
static  bool         AppEvalCmdlnArgs (int iArgCnt_p, char* apszArg_p[]);
static  void         AppPrintHelpScreen  (const char* pszArg0_p);
static  bool         SplitupUrlString (const char* pszUrlString_p, char* pszIpAddr_p, size_t nIpAddrBufSize_p, int* piPortNum_p);
static  int          BuildSmDeviceList (Config* pAppConfig_p, std::list<SmDevice>* plstSmDevList_p, const char* pszRtuInterface_p, long lBaudrate_p);
static  void         ReleaseSmDeviceList (std::list<SmDevice>* plstSmDevList_p);

#ifdef _GASMETER_
    static  int      BuildGmDevice (Config* pAppConfig_p, GmDevice* pGmDev_p);
    static  void     ReleaseGmDevice (GmDevice* pGmDev_p);
#endif

static  std::string  BuildMqttDataPublishTopic (std::string strDataTopicTemplate_p, std::string strLabel_p, int iIdNum_p);
static  std::string  BuildMqttBootupPublishTopic (CfgSettings CfgSettings_p);
static  std::string  BuildMqttKeepAlivePublishTopic (CfgSettings CfgSettings_p);
static  std::string  GetBootupInfoAsJson (const char* pszClientName_p = NULL, time_t tmTimeStamp_p = 0);
static  int          MessageFileOpen (const char* pszMsgFileName_p);
static  int          MessageFileClose ();
static  int          MessageFileWriteData (const char* strMessage_p);
static  LoginData    GetMqttLoginData (CfgSettings CfgSettings_p, const char* pszDefClientName_p, const char* pszDefUserName_p, const char* pszDefPassword_p);
static  std::string  GetUiniqueID (int iLength_p = 16);
static  void         Delay (unsigned int uiTimeSpan_p);





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Main function of this application
//---------------------------------------------------------------------------

int  main (int iArgCnt_p, char* apszArg_p[])
{

std::list<SmDevice>::iterator  lstSmDevIterator;
std::list<SmDevice>  lstSmDevList;
SmartMeter::tError   SmError;
CfgReader            AppCfgReader;
Config               AppConfig;
std::string          strConfigAsText;
char                 szHostAddr[32];
int                  iPortNum;
LoginData            MqttLoginData;
std::string          strMqttBootupTopic;
std::string          strKeepAliveTopic;
std::string          strBootupInfo;
SmDevice             SmDev;
time_t               tmTimeStamp;
char                 szTimeStamp[64];
time_t               tmTimeNow;
time_t               tmTimeLastQuery;
time_t               tmTimeStampJson;
bool                 fAddDevInfo;
const char*          pszModbusRegDump;
const char*          pszJsonMessage;
const char*          pszMeasurements;
unsigned int         fRetainedFlag;
bool                 fMqttReconnect;
int                  iRes;
bool                 fRes;

#ifdef _GASMETER_
GmDevice             GmDev;
GasMeter::tError     GmError;
const char*          pszCounterDump;
#endif


    //-------------------------------------------------------------------
    // Step(1): Setup
    //-------------------------------------------------------------------
    printf("\n");
    printf("********************************************************************\n");
    printf("  SmartMeterMonitor\n");
    printf("  (c) 2025 Ronald Sieber\n");
    printf("  Version: %u.%02u\n", APP_VER_MAIN, APP_VER_REL);
    printf("  Build:   %s\n", APP_BUILD_TIMESTAMP);
    printf("********************************************************************\n");
    printf("\n");


    // setup Workspace
    pszCfgFileName_l = NULL;
    pszMsgFileName_l = NULL;
    fOffline_l       = false;
    fVerbose_l       = false;
    fRunMainLoop_l   = false;
    tmTimeNow        = 0;
    tmTimeLastQuery  = 0;
    fMqttReconnect   = false;


    // register Ctrl+C Handler
    #ifndef _WIN32
    {
        signal(SIGINT, AppSigHandler);
    }
    #endif


    // evaluate Command Line Arguments
    fRes = AppEvalCmdlnArgs(iArgCnt_p, apszArg_p);
    if ( !fRes )
    {
        AppPrintHelpScreen(apszArg_p[0]);
        return (-1);
    }


    // show sytem start time and runtime configuration
    LibSys::GetTimeStamp(szTimeStamp, sizeof(szTimeStamp));
    printf("Systen Time: %s\n", szTimeStamp);
    printf("\n");
    printf("Runtime Configuration:\n");
    printf("  '-o' Offline = %s\n", (fOffline_l ? "yes" : "no"));
    printf("  '-v' Verbose = %s\n", (fVerbose_l ? "yes" : "no"));
    printf("\n");


    // parse ConfigFile, build binary representation of ConfigSettings
    if (pszCfgFileName_l == NULL)
    {
        fprintf(stderr, "ERROR: No Configuration specified (no '-c=' given at Command Line, use --help for Help)!\n\n");
        return (-1);
    }
    printf("Read Config File '%s'...\n", pszCfgFileName_l);
    iRes = AppCfgReader.BuildRuntimeConfig(pszCfgFileName_l, &AppConfig);
    if (iRes != 0)
    {
        fprintf(stderr, "ERROR: Invalid Configuration (iRes=%d)!\n\n", iRes);
        return (iRes);
    }

    // print binary representation of ConfigSettings (-> "what did the parser recognize?")
    strConfigAsText = AppCfgReader.GetRuntimeConfigAsText(&AppConfig);
    printf("Processed Settings:\n%s\n", strConfigAsText.c_str());

    // split-up URL string for MQTT Broker into IP-Address and PortNum
    fRes = SplitupUrlString(AppConfig.m_CfgSettings.m_strMqttBroker.c_str(), szHostAddr, sizeof(szHostAddr), &iPortNum);
    if ( !fRes )
    {
        fprintf(stderr, "ERROR: Invalid URL for MQTT Broker!\n\n");
        return (-1);
    }

    // build SmartMeter DeviceList as a basis for cyclicaly query of measurement values
    iRes = BuildSmDeviceList(&AppConfig, &lstSmDevList, AppConfig.m_CfgSettings.m_strMbInterface.c_str(), AppConfig.m_CfgSettings.m_lBaudrate);
    if (iRes != 0)
    {
        fprintf(stderr, "ERROR: Creating SmartMeter DeviceList failed (iRes=%d)!\n\n", iRes);
        return (iRes);
    }

    // build GasMeter Device (if present) as a basis for cyclicaly query of measurement values
    #ifdef _GASMETER_
    {
        if (AppConfig.m_CfgGasMeter.m_iSensorGpio >= 0)
        {
            iRes = BuildGmDevice(&AppConfig, &GmDev);
            if (iRes != 0)
            {
                fprintf(stderr, "ERROR: Creating GasMeter Device failed (iRes=%d)!\n\n", iRes);
                return (iRes);
            }
        }
    }
    #endif

    // create/open MessageFile
    if (pszMsgFileName_l != NULL)
    {
        printf("Create/Open MessageFile ('%s')... ", pszMsgFileName_l);
        iRes = MessageFileOpen(pszMsgFileName_l);
        if (iRes >= 0)
        {
            printf("done.\n");
        }
        else
        {
            printf("failed (iRes=%d)!\n\n", iRes);
            pszMsgFileName_l = NULL;
        }
    }


    // get Login Data for access to MQTT Broker (either user entries from Config File or default values)
    MqttLoginData = GetMqttLoginData (AppConfig.m_CfgSettings, MQTT_DEF_CLIENT_NAME, MQTT_DEF_USER_NAME, MQTT_DEF_PASSWORD);

    // Build MQTT Topics to publish Bootup and KeepAlive Messages
    strMqttBootupTopic = BuildMqttBootupPublishTopic(AppConfig.m_CfgSettings);
    strKeepAliveTopic  = BuildMqttKeepAlivePublishTopic(AppConfig.m_CfgSettings);

    // connect to MQTT Broker
    if ( !fOffline_l )
    {
        printf("Connect to MQTT Broker...\n");
        printf("  HostUrl     = '%s'\n",     szHostAddr);
        printf("  HostPortNum = %u\n",       iPortNum);
        printf("  ClientName  = '%s'\n",     MqttLoginData.m_strClientName.c_str());
        printf("  UserName    = '%s'\n",     MqttLoginData.m_strUserName.c_str());
        printf("  Password    = '%s'\n",     MqttLoginData.m_strPassword.c_str());
        printf("  KeepAlive   = %u [sec]\n", MQTT_KEEPALIVE_INTERVAL);
        iRes = MqttConnect(szHostAddr, iPortNum, MqttLoginData.m_strClientName.c_str(), MQTT_KEEPALIVE_INTERVAL,
                           MqttLoginData.m_strUserName.c_str(), MqttLoginData.m_strPassword.c_str());
        if (iRes != 0)
        {
            fprintf(stderr, "\nERROR: MqttConnect() failed (iRes=%d)!\n\n", iRes);
            return (-4);
        }
        printf("done.\n");
    }
    else
    {
        printf("Running in Offline Mode, without MQTT connection.\n");
    }
    fMqttReconnect = false;


    // send MQTT Bootup Message
    if ( !fOffline_l )
    {
        printf("Send MQTT Bootup Message...\n");
        tmTimeStamp = LibSys::GetTimeStamp();
        strBootupInfo = GetBootupInfoAsJson(MqttLoginData.m_strClientName.c_str(), tmTimeStamp);
        fRetainedFlag = 1;
        iRes = MqttPublishMessage(strMqttBootupTopic.c_str(), (const unsigned char*)strBootupInfo.c_str(), strBootupInfo.length(), kMqttQoS0, fRetainedFlag);
        if (iRes < 0)
        {
            fprintf(stderr, "\nERROR: MqttPublishMessage() failed (iRes=%d)!\n\n", iRes);
            fMqttReconnect = true;
        }
    }


    //-------------------------------------------------------------------
    // Step(2): Main Loop
    //-------------------------------------------------------------------
    printf("\n\n---- Entering Main Loop ----\n");
    fRunMainLoop_l = true;
    while ( fRunMainLoop_l )
    {
        // check if Query Interval is elapsed
        tmTimeNow = LibSys::GetTimeStamp();
        if ((tmTimeNow - tmTimeLastQuery) < AppConfig.m_CfgSettings.m_lQueryInterval)
        {
            if ( fVerbose_l )
            {
                printf(".");
                fflush(stdout);
            }
        }
        else
        {
            tmTimeLastQuery = tmTimeNow;

            // ======== (2a): Process SmartMeter Query Batch ========
            for (lstSmDevIterator=lstSmDevList.begin(); lstSmDevIterator!=lstSmDevList.end(); lstSmDevIterator++)
            {
                SmDev = *lstSmDevIterator;

                printf("\n==== Read '%s' at ModbusID=%u ====\n", SmDev.m_strType.c_str(), SmDev.m_iModbusSlaveID);
                tmTimeStamp = LibSys::GetTimeStamp(szTimeStamp, sizeof(szTimeStamp));
                printf("Systen Time: %s\n", szTimeStamp);

                fAddDevInfo = false;
                tmTimeStampJson = 0;
                switch (AppConfig.m_CfgSettings.m_JsonInfoLevel)
                {
                    case tJsonInfoLevel::kAddDevInfo:   fAddDevInfo = true;                 // fall through to next statement without 'break'
                    case tJsonInfoLevel::kTimeStamp:    tmTimeStampJson = tmTimeStamp;      // fall through to next statement without 'break'
                    default:                            break;
                }

                // query data from SmartMeter
                SmError = SmDev.m_pSmartMeter->ReadModbusRegs();
                if (SmError == SmartMeter::kSuccess)
                {
                    if ( fVerbose_l )
                    {
                        pszModbusRegDump = SmDev.m_pSmartMeter->DumpModbusRegs();
                        printf("\n---- Modbus Register Dump ----\n%s\n", pszModbusRegDump);
                    }

                    pszMeasurements = SmDev.m_pSmartMeter->GetMeasurementsAsText();
                    printf("\n---- Measured Values ----\n%s\n", pszMeasurements);

                    pszJsonMessage = SmDev.m_pSmartMeter->GetMeasurementsAsJson(tmTimeStampJson, fAddDevInfo);
                    if ( fVerbose_l )
                    {
                        printf("\n---- JSON Message ----\n%s\n", pszJsonMessage);
                    }

                    // log JSON Record with measurement data to MessageFile
                    if (pszMsgFileName_l != NULL)
                    {
                        printf("Log Measurements to MessageFile ('%s')... ", pszMsgFileName_l);
                        MessageFileWriteData(pszJsonMessage);
                        printf("done.\n");
                    }

                    // send JSON Record with measurement data to MQTT Broker
                    if ( !fOffline_l )
                    {
                        printf("Send Measurements to MQTT Broker... ");
                        fRetainedFlag = 0;
                        iRes = MqttPublishMessage(SmDev.m_strDataTopic.c_str(), (const unsigned char*)pszJsonMessage, strlen(pszJsonMessage), kMqttQoS0, fRetainedFlag);
                        if (iRes >= 0)
                        {
                            printf("done.\n\n");
                        }
                        else
                        {
                            fprintf(stderr, "\nERROR: MqttPublishMessage() failed (iRes=%d)!\n\n", iRes);
                            fMqttReconnect = true;
                        }
                    }
                }
                else if (SmError == SmartMeter::kIdleRead)
                {
                    if ( fVerbose_l )
                    {
                        printf("\nModbus: Idle Read\n\n");
                    }
                }
                else
                {
                    fprintf(stderr, "\nERROR: SmartMeter::ReadModbusRegs failed ('%s')!\n\n", SmDev.m_pSmartMeter->GetErrorText(SmError));
                }

                // insert a waiting pause between Query of two SmartMeters
                Delay(AppConfig.m_CfgSettings.m_lInterDevPause);
            }   // for(<List of all SmartMeter Devices>)

            // ======== (2b): Process GasMeter Query ========
            #ifdef _GASMETER_
            {
                if (AppConfig.m_CfgGasMeter.m_iSensorGpio >= 0)
                {
                    printf("\n==== Read '%s' at SensorGpio=%u ====\n", GmDev.m_strType.c_str(), GmDev.m_iSensorGpio);
                    tmTimeStamp = LibSys::GetTimeStamp(szTimeStamp, sizeof(szTimeStamp));
                    printf("Systen Time: %s\n", szTimeStamp);

                    fAddDevInfo = false;
                    tmTimeStampJson = 0;
                    switch (AppConfig.m_CfgSettings.m_JsonInfoLevel)
                    {
                        case tJsonInfoLevel::kAddDevInfo:   fAddDevInfo = true;                 // fall through to next statement without 'break'
                        case tJsonInfoLevel::kTimeStamp:    tmTimeStampJson = tmTimeStamp;      // fall through to next statement without 'break'
                        default:                            break;
                    }

                    // query data from GasMeter
                    GmError = GmDev.m_pGasMeter->ReadCounter();
                    if (GmError == GasMeter::kSuccess)
                    {
                        if ( fVerbose_l )
                        {
                            pszCounterDump = GmDev.m_pGasMeter->DumpCounter();
                            printf("\n---- Counter Dump ----\n%s\n", pszCounterDump);
                        }

                        pszMeasurements = GmDev.m_pGasMeter->GetMeasurementsAsText();
                        printf("\n---- Measured Values ----\n%s\n", pszMeasurements);

                        pszJsonMessage = GmDev.m_pGasMeter->GetMeasurementsAsJson(tmTimeStampJson, fAddDevInfo);
                        if ( fVerbose_l )
                        {
                            printf("\n---- JSON Message ----\n%s\n", pszJsonMessage);
                        }

                        // log JSON Record with measurement data to MessageFile
                        if (pszMsgFileName_l != NULL)
                        {
                            printf("Log Measurements to MessageFile ('%s')... ", pszMsgFileName_l);
                            MessageFileWriteData(pszJsonMessage);
                            printf("done.\n");
                        }

                        // send JSON Record with measurement data to MQTT Broker
                        if ( !fOffline_l )
                        {
                            printf("Send Measurements to MQTT Broker... ");
                            fRetainedFlag = 0;
                            iRes = MqttPublishMessage(GmDev.m_strDataTopic.c_str(), (const unsigned char*)pszJsonMessage, strlen(pszJsonMessage), kMqttQoS0, fRetainedFlag);
                            if (iRes >= 0)
                            {
                                printf("done.\n\n");
                            }
                            else
                            {
                                fprintf(stderr, "\nERROR: MqttPublishMessage() failed (iRes=%d)!\n\n", iRes);
                                fMqttReconnect = true;
                            }
                        }
                    }
                    else if (GmError == GasMeter::kIdleRead)
                    {
                        if ( fVerbose_l )
                        {
                            printf("\nGasMeter: Idle Read\n\n");
                        }
                    }
                    else
                    {
                        fprintf(stderr, "\nERROR: GasMeter::ReadCounter failed ('%s')!\n\n", GmDev.m_pGasMeter->GetErrorText(GmError));
                    }
                }   // if (AppConfig.m_CfgGasMeter.m_iSensorGpio >= 0)
            }
            #endif  // #ifdef _GASMETER_
        }   // Query Interval is elapsed


        // send KeepAlive
        if ( !fOffline_l )
        {
            iRes = MqttKeepAlive(strKeepAliveTopic.c_str());
            if (iRes < 0)
            {
                fprintf(stderr, "\nERROR: MqttKeepAlive() failed (iRes=%d)!\n\n", iRes);
                fMqttReconnect = true;
            }
            if (iRes > 0)
            {
                if ( fVerbose_l )
                {
                    printf("[KA]");
                }
            }
        }

        // ErrorHandling: reconnet to MQTT Broker if necessary
        if ( !fOffline_l )
        {
            if ( fMqttReconnect )
            {
                printf("\nReanimation: Reconnect to MQTT Broker... ");
                iRes = MqttReconnect();
                if (iRes != 0)
                {
                    fprintf(stderr, "\nERROR: MqttReconnect() failed (iRes=%d)!\n\n", iRes);
                    fMqttReconnect = true;
                }
                else
                {
                    printf("done.\n");
                    if ( fVerbose_l )
                    {
                        printf("\n");
                    }
                    fMqttReconnect = false;
                }
            }
        }

        fflush(stdout);
        fflush(stderr);
        Delay(1000);
    }


    //-------------------------------------------------------------------
    // Step(3): Exit Application
    //-------------------------------------------------------------------
    printf("\n\nExit Application...\n");

    // disconnect from MQTT Broker
    if ( !fOffline_l )
    {
        printf("Disconnect from MQTT Broker... ");
        iRes = MqttDisconnect();
        if (iRes != 0)
        {
            fprintf(stderr, "\nERROR: MqttDisconnect() failed (iRes=%d)!\n\n", iRes);
        }
        else
        {
            printf("done.\n");
        }
    }   // while ( fRunMainLoop_l )

    // close MessageFile
    if (pszMsgFileName_l != NULL)
    {
        printf("Close MessageFile ('%s')... ", pszMsgFileName_l);
        MessageFileClose();
        printf("done.\n");
    }

    // release List of SmartMeter Devices
    ReleaseSmDeviceList(&lstSmDevList);

    // release GasMeter Device
    #ifdef _GASMETER_
    {
        if (AppConfig.m_CfgGasMeter.m_iSensorGpio >= 0)
        {
            ReleaseGmDevice(&GmDev);
        }
    }
    #endif


    return (0);

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Application signal handler
//---------------------------------------------------------------------------

static  void  AppSigHandler (int iSignalNum_p)
{

    printf("\n\nTerminate Application\n\n");
    if ( fRunMainLoop_l )
    {
        fRunMainLoop_l = false;
    }
    else
    {
        exit (-1);
    }

    return;

}


//---------------------------------------------------------------------------
//  Evaluate command line arguments
//---------------------------------------------------------------------------

static  bool  AppEvalCmdlnArgs (int iArgCnt_p, char* apszArg_p[])
{

char*  pszArg;
int    iIdx;
bool   fRes;


    fRes = true;

    for (iIdx=1; iIdx<iArgCnt_p; iIdx++)
    {
        pszArg = apszArg_p[iIdx];
        if (pszArg != NULL)
        {
            // argument '-c=' -> ConfigurationFile
            if ( !strncasecmp("-c=", pszArg, sizeof("-c=")-1) )
            {
                pszArg += sizeof("-c=")-1;
                pszCfgFileName_l = pszArg;
                continue;
            }

            // argument '-l=' -> MessageFile
            if ( !strncasecmp("-l=", pszArg, sizeof("-l=")-1) )
            {
                pszArg += sizeof("-l=")-1;
                pszMsgFileName_l = pszArg;
                continue;
            }

            // argument '-o' -> Offline
            if ( !strncasecmp("-o", pszArg, sizeof("-o")-1) )
            {
                fOffline_l = true;
                continue;
            }

            // argument '-v' -> Verbose
            if ( !strncasecmp("-v", pszArg, sizeof("-v")-1) )
            {
                fVerbose_l = true;
                continue;
            }
        }

        fRes = false;
    }

    return (fRes);

}



//---------------------------------------------------------------------------
//  Show Help Screen
//---------------------------------------------------------------------------

static  void  AppPrintHelpScreen (const char* pszArg0_p)
{

    //     |    10   |    20   |    30   |    40   |    50   |    60   |    70   |    80   |
    printf("Usage:\n");
    printf("   %s [OPTION]\n", pszArg0_p);
    printf("   OPTION:\n");
    printf("\n");
    printf("       -c=<cfg_file>   Path/Name of Configuration File\n");
    printf("\n");
    printf("       -l=<msg_file>   Logs all Messages sent via MQTT to the specified file\n");
    printf("                       (the file is opened always in APPEND mode)\n");
    printf("\n");
    printf("       -o              Run in Offline Mode, without MQTT connection\n");
    printf("\n");
    printf("       -v              Run in Verbose Mode\n");
    printf("\n");
    printf("       --help          Shows this Help Screen\n");
    printf("\n");

    return;

}



//---------------------------------------------------------------------------
//  Split-up URL string into IP-Address and PortNum
//---------------------------------------------------------------------------

static  bool  SplitupUrlString (const char* pszUrlString_p, char* pszIpAddr_p, size_t nIpAddrBufSize_p, int* piPortNum_p)
{

char*  pszSeparator;
int    iPortNum;
int    iRes;


    *pszIpAddr_p = '\0';
    *piPortNum_p = 0;

    if (strlen(pszUrlString_p) >= nIpAddrBufSize_p)
    {
        return (false);
    }

    strncpy(pszIpAddr_p, pszUrlString_p, nIpAddrBufSize_p-1);
    pszIpAddr_p[nIpAddrBufSize_p-1] = '\0';
    pszSeparator = strstr(pszIpAddr_p, ":");
    if (pszSeparator != NULL)
    {
        *pszSeparator = '\0';

        pszSeparator++;
        iRes = sscanf(pszSeparator, "%d", &iPortNum);
        if (iRes != 1)
        {
            return (false);
        }
    }
    else
    {
        iPortNum = MQTT_DEF_HOST_PORTNUM;
    }

    *piPortNum_p = iPortNum;

    return (true);

}



//---------------------------------------------------------------------------
//  Build List of SmartMeter Devices
//---------------------------------------------------------------------------

static  int  BuildSmDeviceList (Config* pAppConfig_p, std::list<SmDevice>* plstSmDevList_p, const char* pszRtuInterface_p, long lBaudrate_p)
{

std::list<CfgDevice>::iterator  lstCfgDevIterator;
SmartMeter::tError  SmError;
CfgDevice  CfgDev;
SmDevice   SmDev;
int        iNumOfDevices;


    if ((pAppConfig_p == NULL) || (plstSmDevList_p == NULL))
    {
        TRACE0("\nERROR: Invalid Params!\n");
        return (-1);
    }

    iNumOfDevices = pAppConfig_p->m_lstCfgDevList.size();
    if (iNumOfDevices == 0)
    {
        TRACE0("\nINFO: No ModbusDevices specified!\n");
        return (0);
    }


    plstSmDevList_p->clear();

    for (lstCfgDevIterator=pAppConfig_p->m_lstCfgDevList.begin(); lstCfgDevIterator!=pAppConfig_p->m_lstCfgDevList.end(); lstCfgDevIterator++)
    {
        CfgDev = *lstCfgDevIterator;

        SmDev.m_iModbusSlaveID = CfgDev.m_iModbusID;
        SmDev.m_strLabel = CfgDev.m_strLabel;
        SmDev.m_strDataTopic = BuildMqttDataPublishTopic (pAppConfig_p->m_CfgSettings.m_strDataTopic, CfgDev.m_strLabel, CfgDev.m_iModbusID);

        if (strcasecmp(CfgDev.m_strType.c_str(), "SDM230") == 0)
        {
            // setup a SDM230
            SmDev.m_pSmartMeter = new SDM230();
        }
        else if (strcasecmp(CfgDev.m_strType.c_str(), "SDM630") == 0)
        {
            // setup a SDM630
            SmDev.m_pSmartMeter = new SDM630();
        }
        else
        {
            TRACE1("\nERROR: Unknown Device Type ('%s')!\n", CfgDev.m_strType.c_str());
            return (-3);
        }

        SmDev.m_strType = SmDev.m_pSmartMeter->GetDevTypeName();

        SmError = SmDev.m_pSmartMeter->Setup(pszRtuInterface_p, lBaudrate_p, SmDev.m_iModbusSlaveID, SmDev.m_strLabel.c_str());
        if (SmError != SmartMeter::kSuccess)
        {
            TRACE1("\nERROR: Setup SmartMeter failed (ErrCode=%d)!\n", (int)SmError);
            return (-4);
        }

        plstSmDevList_p->push_back(SmDev);
    }


    return (0);

}



//---------------------------------------------------------------------------
//  Release List of SmartMeter Devices
//---------------------------------------------------------------------------

static  void  ReleaseSmDeviceList (std::list<SmDevice>* plstSmDevList_p)
{

std::list<SmDevice>::iterator  lstSmDevIterator;
SmDevice  SmDev;


    if (plstSmDevList_p == NULL)
    {
        TRACE0("\nERROR: Invalid Params!\n");
        return;
    }

    for (lstSmDevIterator=plstSmDevList_p->begin(); lstSmDevIterator!=plstSmDevList_p->end(); lstSmDevIterator++)
    {
        SmDev = *lstSmDevIterator;
        delete SmDev.m_pSmartMeter;
    }
 
    return;

}



//---------------------------------------------------------------------------
//  Build GasMeter Device
//---------------------------------------------------------------------------

#ifdef _GASMETER_
static  int  BuildGmDevice (Config* pAppConfig_p, GmDevice* pGmDev_p)
{

GasMeter::tError  GmError;
CfgGasMeter  CfgDev;


    if ((pAppConfig_p == NULL) || (pGmDev_p == NULL))
    {
        TRACE0("\nERROR: Invalid Params!\n");
        return (-1);
    }

    CfgDev = pAppConfig_p->m_CfgGasMeter;

    pGmDev_p->m_iSensorGpio = CfgDev.m_iSensorGpio;
    pGmDev_p->m_strLabel = CfgDev.m_strLabel;
    pGmDev_p->m_strDataTopic = BuildMqttDataPublishTopic (pAppConfig_p->m_CfgSettings.m_strDataTopic, CfgDev.m_strLabel, CfgDev.m_iSensorGpio);

    // setup a GasMeter
    pGmDev_p->m_pGasMeter = new GasMeter();
    pGmDev_p->m_strType = pGmDev_p->m_pGasMeter->GetDevTypeName();

    GmError = pGmDev_p->m_pGasMeter->Setup(CfgDev.m_iSensorGpio, CfgDev.m_iElasticity, CfgDev.m_iCounterDelta, CfgDev.m_flVolumeFactor, CfgDev.m_flEnergyFactor, CfgDev.m_strRetainFile.c_str(), CfgDev.m_strLabel.c_str());
    if (GmError != GasMeter::kSuccess)
    {
        TRACE1("\nERROR: Setup GasMeter failed (ErrCode=%d)!\n", (int)GmError);
        return (-4);
    }

    return (0);

}
#endif



//---------------------------------------------------------------------------
//  Release GasMeter Device
//---------------------------------------------------------------------------

#ifdef _GASMETER_
static  void  ReleaseGmDevice (GmDevice* pGmDev_p)
{

    if (pGmDev_p == NULL)
    {
        TRACE0("\nERROR: Invalid Params!\n");
        return;
    }

    delete pGmDev_p->m_pGasMeter;
 
    return;

}
#endif



//---------------------------------------------------------------------------
//  Build MQTT Topic to publish Measurement Data Message
//---------------------------------------------------------------------------

static  std::string  BuildMqttDataPublishTopic (std::string strDataTopicTemplate_p, std::string strLabel_p, int iIdNum_p)
{

std::string  strDataTopic;
char         szIdNum[16];
std::string  strIdNum;
size_t       nStartPos;


    strDataTopic = strDataTopicTemplate_p;

    // check if TopicTemplate uses format "<label>" (replace placeholder with Label of SmartMeter Device)
    nStartPos = strDataTopic.find("<label>");
    if (nStartPos != std::string::npos)
    {
        // is 'Label' defined in Device Section inside Config File?
        if (strLabel_p.length() > 0)
        {
            // 'Label' is defined -> use it
            strDataTopic = strDataTopic.replace(nStartPos, strlen("<label>"), strLabel_p);
        }
        else
        {
            // 'Label' isn't defined -> Fallback: replace Template with "<id>"
            strDataTopic = strDataTopic.replace(nStartPos, strlen("<label>"), std::string("<id>"));
        }
    }

    // check if TopicTemplate uses format "<id>" (replace placeholder with ModbusID of SmartMeter Device or GpioNum of GasMeter Device)
    nStartPos = strDataTopic.find("<id>");
    if (nStartPos != std::string::npos)
    {
        snprintf(szIdNum, sizeof(szIdNum), "%03d", iIdNum_p);
        strIdNum = std::string(szIdNum);

        strDataTopic = strDataTopic.replace(nStartPos, strlen("<id>"), strIdNum);
    }

    return (strDataTopic);

}



//---------------------------------------------------------------------------
//  Build MQTT Topic to publish Bootup Message
//---------------------------------------------------------------------------

static  std::string  BuildMqttBootupPublishTopic (CfgSettings CfgSettings_p)
{

std::string  strBootupTopic;


    strBootupTopic = CfgSettings_p.m_strStatTopic;

    // make sure that Topi String ends with "/"
    if (strBootupTopic.empty() || strBootupTopic.compare(strBootupTopic.length()-1, 1, "/") != 0)
    {
        strBootupTopic += "/";
    }

    strBootupTopic += "Bootup";

    return (strBootupTopic);

}



//---------------------------------------------------------------------------
//  Build MQTT Topic to publish KeepAlive Message
//---------------------------------------------------------------------------

static  std::string  BuildMqttKeepAlivePublishTopic (CfgSettings CfgSettings_p)
{

std::string  strBootupTopic;


    strBootupTopic = CfgSettings_p.m_strStatTopic;

    // make sure that Topi String ends with "/"
    if (strBootupTopic.empty() || strBootupTopic.compare(strBootupTopic.length()-1, 1, "/") != 0)
    {
        strBootupTopic += "/";
    }

    strBootupTopic += "KeepAlive";

    return (strBootupTopic);

}



//---------------------------------------------------------------------------
//  GetBootupInfoAsJson
//---------------------------------------------------------------------------

static  std::string  GetBootupInfoAsJson (const char* pszClientName_p /* = NULL */, time_t tmTimeStamp_p /* = 0 */)
{

char         szBootupInfo[1024];
std::string  strBootupInfo;
char         szTimeStamp[64];


    memset(szBootupInfo, '\0', sizeof(szBootupInfo));

    LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "{\n");
    LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "  \"Version\": \"%u.%02u\",\n", APP_VER_MAIN, APP_VER_REL);
    if (tmTimeStamp_p != 0)
    {
        LibSys::FormatTimeStamp(tmTimeStamp_p, szTimeStamp, sizeof(szTimeStamp));
        LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "  \"TimeStamp\": %u,\n", (unsigned)tmTimeStamp_p);
        LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "  \"TimeStampFmt\": \"%s\",\n", szTimeStamp);
    }
    if (pszClientName_p != NULL)
    {
        LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "  \"ClientName\": \"%s\",\n", pszClientName_p);
    }
    LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "  \"KeepAlive\": %d\n", MQTT_KEEPALIVE_INTERVAL);
    LibSys::JoinStr(szBootupInfo, sizeof(szBootupInfo), "}\n");

    strBootupInfo = szBootupInfo;

    return (strBootupInfo);

}



//---------------------------------------------------------------------------
//  MessageFileOpen
//---------------------------------------------------------------------------

static  int  MessageFileOpen (const char* pszMsgFileName_p)
{

    ofsMsgFile_l.open(pszMsgFileName_p, std::ios_base::app);
    if ( !ofsMsgFile_l.is_open() )
    {
        return (-1);
    }

    return (0);

}



//---------------------------------------------------------------------------
//  MessageFileClose
//---------------------------------------------------------------------------

static  int  MessageFileClose ()
{

    if ( !ofsMsgFile_l.is_open() )
    {
        return (-1);
    }

    ofsMsgFile_l.flush();
    ofsMsgFile_l.close();

    return (0);

}



//---------------------------------------------------------------------------
//  MessageFileWriteData
//---------------------------------------------------------------------------

static  int  MessageFileWriteData (const char* strMessage_p)
{

const char*  pszDelimiter = "\n\n";


    if ( !ofsMsgFile_l.is_open() )
    {
        return (-1);
    }

    ofsMsgFile_l.write(strMessage_p, strlen(strMessage_p));
    ofsMsgFile_l.write(pszDelimiter, strlen(pszDelimiter));
    if ( ofsMsgFile_l.fail() )
    {
        return (-2);
    }

    ofsMsgFile_l.flush();

    return (0);

}



//---------------------------------------------------------------------------
//  Get Login Data for access to MQTT Broker
//---------------------------------------------------------------------------

static  LoginData  GetMqttLoginData (CfgSettings CfgSettings_p, const char* pszDefClientName_p, const char* pszDefUserName_p, const char* pszDefPassword_p)
{

LoginData    MqttLoginData;
std::string  strUid;
size_t       nStartPos;


    // provide ClientName
    if ( !(CfgSettings_p.m_strClientName.empty()) && !(CfgSettings_p.m_strClientName == "") )
    {
        MqttLoginData.m_strClientName = CfgSettings_p.m_strClientName;
    }
    else
    {
        MqttLoginData.m_strClientName = pszDefClientName_p;
    }

    // check if ClientName uses format "<uid>" (replace placeholder with a UniqueID)
    nStartPos = MqttLoginData.m_strClientName.find("<uid>");
    if (nStartPos != std::string::npos)
    {
        strUid = GetUiniqueID();
        MqttLoginData.m_strClientName = MqttLoginData.m_strClientName.replace(nStartPos, strlen("<uid>"), strUid);
    }


    // provide UserName
    if ( !(CfgSettings_p.m_strUserName.empty()) && !(CfgSettings_p.m_strUserName == "") )
    {
        MqttLoginData.m_strUserName = CfgSettings_p.m_strUserName;
    }
    else
    {
        MqttLoginData.m_strUserName = pszDefUserName_p;
    }

    // provide Password
    if ( !(CfgSettings_p.m_strPassword.empty()) && !(CfgSettings_p.m_strPassword == "") )
    {
        MqttLoginData.m_strPassword = CfgSettings_p.m_strPassword;
    }
    else
    {
        MqttLoginData.m_strPassword = pszDefPassword_p;
    }
        

    return (MqttLoginData);

}



//---------------------------------------------------------------------------
//  Get IniqueID as String
//---------------------------------------------------------------------------

static  std::string  GetUiniqueID (int iLength_p /* = 16 */)
{

const  int  UID_LEN = 16;

std::uniform_int_distribution<int>  UniformDistib(0, 15);       // scale equally distributed random numbers between 0 and 15
std::random_device  RndDev;                                     // Random Device
std::mt19937        MtRndGen;                                   // Mersenne-Twister Random Algorithm
int                 iRandomNumber;
char                szDigit[2];
std::string         strUid;
int                 iIdx;


    MtRndGen.seed(RndDev());
    for (iIdx=0; iIdx<iLength_p; iIdx++)
    {
        iRandomNumber = UniformDistib(MtRndGen);
        snprintf(szDigit, sizeof(szDigit), "%X", iRandomNumber);
        strUid += szDigit;
    }

    return (strUid);

}



//---------------------------------------------------------------------------
//  Wait/Sleep
//---------------------------------------------------------------------------

static  void  Delay (unsigned int uiTimeSpan_p)
{

    #ifdef _WIN32
    {
        Sleep(uiTimeSpan_p);
    }
    #else
    {
        usleep(uiTimeSpan_p * 1000);        // us -> ms
    }
    #endif

    return;

}




// EOF


