/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Read Measurements using Modbus/RTU)
  Description:  Declarations of Config File Reader

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _CFGREADER_H_
#define _CFGREADER_H_



//=========================================================================//
//                                                                         //
//          C O N F I G   S E T T I N G   C L A S S E S                    //
//                                                                         //
//=========================================================================//



typedef enum
{
    kNone           = 0,
    kTimeStamp      = 1,
    kAddDevInfo     = 2

} tJsonInfoLevel;



//---------------------------------------------------------------------------
//  Application Settings "[Settings]"
//---------------------------------------------------------------------------

class CfgSettings
{

    public:

        std::string     m_strMbInterface;
        long            m_lBaudrate;
        long            m_lQueryInterval;               // [sec]
        long            m_lInterDevPause;               // [ms]
        std::string     m_strMqttBroker;
        std::string     m_strClientName;
        std::string     m_strUserName;
        std::string     m_strPassword;
        std::string     m_strDataTopic;
        std::string     m_strStatTopic;
        tJsonInfoLevel  m_JsonInfoLevel;

};



//---------------------------------------------------------------------------
//  SmartMeter Device Settings "[DeviceNNN]"
//---------------------------------------------------------------------------

class CfgDevice
{

    public:

        CfgDevice ()
        {
            m_strType = "";
            m_iModbusID = 0;
            m_strLabel = "";
        };

        CfgDevice (std::string strType_p, int iModbusID_p, std::string strLabel_p)
        {
            m_strType = strType_p;
            m_iModbusID = iModbusID_p;
            m_strLabel = strLabel_p;
        }

        std::string     m_strType;
        int             m_iModbusID;
        std::string     m_strLabel;

};



//---------------------------------------------------------------------------
//  GasMeter (S0) Settings "[GasMeter]"
//---------------------------------------------------------------------------

#ifdef _GASMETER_
class CfgGasMeter
{

    public:

        int             m_iSensorGpio;
        int             m_iElasticity;
        int             m_iCounterDelta;
        float           m_flVolumeFactor; 
        float           m_flEnergyFactor;
        std::string     m_strRetainFile;
        std::string     m_strLabel;

};
#endif



//---------------------------------------------------------------------------
//  Full Config Data (all settings from ConfigFile)
//---------------------------------------------------------------------------

class Config
{

    public:

        CfgSettings             m_CfgSettings;
        std::list<CfgDevice>    m_lstCfgDevList;

        #ifdef _GASMETER_
            CfgGasMeter         m_CfgGasMeter;
        #endif
};





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  CfgReader                                               */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class CfgReader
{

    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        CfgReader();
        ~CfgReader();

        int          BuildRuntimeConfig (const char* pszConfigFile_p, Config* pAppConfig_p);
        std::string  GetRuntimeConfigAsText (Config* pAppConfig_p);


    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        ConfigParser*       m_pCfgParser    = NULL;



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        bool         CheckFileExit (const char* pszConfigFile_p);
        std::string  TrimValueLine (std::string strValueLine_p);

};



#endif  // _CFGREADER_H_


