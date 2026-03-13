/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Extension for S0 based GasMeter)
  Description:  Declarations for GasMeter

  -------------------------------------------------------------------------

  When implementing this class, top priority was given to robust and
  reliable 24/7 continuous operation. For this reason, types such as
  std::string, which work with dynamic memory management at runtime,
  were not used. Static buffers are used instead.

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _GASMETER_H_
#define _GASMETER_H_


#include <atomic>



/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  GasMeter                                                */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class GasMeter
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
            kFileReadError  = 3,
            kFileWriteError = 4,
            kInvalidData    = 5,
            kSensIfSupError = 6,
            kSensIfRdError  = 7,

            kInternalErr    = 255

        } tError;


        typedef struct
        {
            unsigned long   m_ulCounterValue;               // Counting unit pulses
            float           m_flMeterValue;                 // MeterValue [m3]
            float           m_flPower;                      // Power [W]
            float           m_flEnergy;                     // Energy [kWh]

        } tMeasurements;


    private:

        const char*         DEV_TYPE_NAME       = "ES-GAS-2";
        const char*         KEY_METER_VALUE     = "MeterValue =";
        const float         SAVE_DELTA_VALUE    = 1.0;      // Minimum increase of MeterValue before the ReatinFile is rewritten



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        GasMeter();
        ~GasMeter();

        GasMeter::tError    Setup (int iSensorGpio_p, int iElasticity_p, int iCounterDelta_p, float flVolumeFactor_p, float flEnergyFactor_p, const char* pszRetainFile_p, const char* pszLabel_p = NULL);
        GasMeter::tError    Close (void);

        GasMeter::tError    ReadCounter (void);
        const char*         DumpCounter (void);

        void                GetMeasurements (GasMeter::tMeasurements* pMeasurements_p);
        const char*         GetMeasurementsAsText (void);
        const char*         GetMeasurementsAsJson (time_t tmTimeStamp_p = 0, bool fAddDevInfo_p = false);

        int                 GetSensorGpioNum (void);
        const char*         GetDevTypeName (void);
        const char*         GetErrorText (GasMeter::tError GasMeterErr_p);


    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        // Free Running Counter to count hardware impulses on Sensor GPIO Port
        std::atomic<unsigned long>      m_ulSensorCounterValue      = 0;

        // Configurations/Settings Variables
        int                 m_iSensorGpio                           = -1;
        int                 m_iElasticity                           = 0;
        int                 m_iCounterDelta                         = 0;
        float               m_flVolumeFactor                        = 0.0;
        float               m_flEnergyFactor                        = 0.0;
        char                m_szRetainFile[256]                     = { 0 };
        char                m_szLabel[128]                          = { 0 };

        // Workspace Variables
        tMeasurements       m_Measurements                          = { 0 };        // current Measurement Data Record
        unsigned long       m_ulPrevReadCounterValue                = 0;
        time_t              m_tmPrevReadTimeStamp                   = 0;
        int                 m_iPowerCalcCycleCounter                = 0;
        int                 m_iPowerRangeCycles                     = 0;
        float               m_flSavedMeterValue                     = 0.0;
        bool                m_fForceRecalc                          = false;
        char                m_szCounterDump[256]                    = { 0 };
        char                m_szMeasurementsText[1024]              = { 0 };
        char                m_szMeasurementsJson[1024]              = { 0 };



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        GasMeter::tError    ReadRetainMeterValue (const char* pszRetainFile_p, float* pflMeterValue_p);
        GasMeter::tError    WriteRetainMeterValue (const char* pszRetainFile_p, float flMeterValue_p);
        const char*         BuildTextString (void);
        const char*         BuildJsonString (time_t tmTimeStamp_p, bool fAddDevInfo_p);

};



#endif  // _GASMETER_H_


