/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Extension for S0 based GasMeter)
  Description:  Implementation of GasMeterSensorInterface
                (Version for sysWORXX Pi-AM62x with Smart Meter HAT)

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#define _SENSOR_IMPULSE_ANALYZER_


#include <stdio.h>
#include "GMSensIf.h"
#include "sysworxx_io.h"
#include "Trace.h"


#ifdef _SENSOR_IMPULSE_ANALYZER_
    #include <fstream>
    #include <string.h>
    #include <errno.h>
    #include <time.h>
    #include <pthread.h>
#endif



//=========================================================================//
//                                                                         //
//          G L O B A L   D E F I N I T I O N S                            //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Definitions
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

    static  const  char*    ANALYZER_FILE_PATH_NAME     = "/home/ramdisk/SensorImpAnalyzer.log";
    static  const  char*    ANALYZER_FILE_HEADER_LINE   = "SensorCounterValue;TimeStamp\r\n";

    static  const  int      MEASUREMENT_TABLE_ENTRIES   = 100;

    typedef struct
    {
        unsigned long       m_ulSensorCounterValue;
        uint64_t            m_u64TimeStamp;

    } tMeasurementEvent;

#endif



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  std::atomic<unsigned long>*  pulSensorCounterValue_l;
static  int                          iSensorGpio_l;


#ifdef _SENSOR_IMPULSE_ANALYZER_

    tMeasurementEvent   aMeasurementTable_l[MEASUREMENT_TABLE_ENTRIES]  = { 0 };
    unsigned int        uiMeasurementTableIdx_l                         = 0;
    pthread_mutex_t     mutTableMutex_l;
    std::ofstream       ofsAnalyzerFile_l;

#endif



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  void  CbSensorGpioEvent (uint8_t uChannel_p, IoBool fEdge_p);


#ifdef _SENSOR_IMPULSE_ANALYZER_

    static  void      SiaSetupMeasurementAnalyzer (const char* pszAnalyzerFilePathName_p);
    static  void      SiaCloseMeasurementAnalyzer (void);
    static  void      SiaLogMeasurementEvent (unsigned long ulSensorCounterValue_p);
    static  void      SiaSaveMeasurementTable (const char* pszAnalyzerFilePathName_p);
    static  uint64_t  SiaGetTimeStamp (void);

#endif





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//-----------------------------------------------------------------------
//  SensorInterfaceSetup
//-----------------------------------------------------------------------

int  SensorInterfaceSetup (int iSensorGpio_p, std::atomic<unsigned long>* pulSensorCounterValue_p)
{

IoResult  IoRes;


    TRACE2("GasMeterSensorInterface: SensorInterfaceSetup (SensorGpio=%d, pulSensorCounterValue=0x%08lX)...\n", iSensorGpio_p, (unsigned long)pulSensorCounterValue_p);

    if (pulSensorCounterValue_p == NULL)
    {
        fprintf(stderr, "ERROR: Invalid Parameter 'pulSensorCounterValue_p'\n");
        return (-1);
    }

    // save parameters
    pulSensorCounterValue_l = pulSensorCounterValue_p;
    iSensorGpio_l = iSensorGpio_p;

    // open sysWORXX IO Driver
    IoRes = IoInit();
    if (IoRes != IoResult_Success)
    {
        fprintf(stderr, "ERROR: Failed to initialize sysWORXX IO Driver (%d)\n", (int)IoRes);
        return (-2);
    }

    // register Sensor GPIO Callback Handler
    IoRes = IoRegisterInputCallback (iSensorGpio_l, CbSensorGpioEvent, IoInputTrigger_RisingEdge);
    if (IoRes != IoResult_Success)
    {
        fprintf(stderr, "ERROR: Failed to register Sensor GPIO Callback Handler (%d)\n", (int)IoRes);
        return (-2);
    }

    #ifdef _SENSOR_IMPULSE_ANALYZER_
    {
        // setup Sensor Impuls Analyzer
        SiaSetupMeasurementAnalyzer(ANALYZER_FILE_PATH_NAME);
    }
    #endif

    return (0);

}



//-----------------------------------------------------------------------
//  SensorInterfaceClose
//-----------------------------------------------------------------------

int  SensorInterfaceClose (void)
{

IoResult  IoRes;


    TRACE0("GasMeterSensorInterface: SensorInterfaceClose...\n");

    #ifdef _SENSOR_IMPULSE_ANALYZER_
    {
        // close Sensor Impuls Analyzer
        SiaCloseMeasurementAnalyzer();
    }
    #endif

    // unregister Sensor GPIO Callback Handler
    IoRes = IoUnregisterInputCallback(iSensorGpio_l);
    if (IoRes != IoResult_Success)
    {
        fprintf(stderr, "ERROR: Failed to unregister Sensor GPIO Callback Handler (%d)\n", (int)IoRes);

        // even if there is an error, do not cancel, but continue with 'IoShutdown()'
        // in the following statement
    }

    // close sysWORXX IO Driver
    IoRes = IoShutdown();
    if (IoRes != IoResult_Success)
    {
        fprintf(stderr, "ERROR: Failed to close sysWORXX IO Driver (%d)\n", (int)IoRes);
        return (-1);
    }

    return (0);

}



//-----------------------------------------------------------------------
//  SensorInterfaceReadCounter
//-----------------------------------------------------------------------

int  SensorInterfaceReadCounter (void)
{

    TRACE0("GasMeterSensorInterface: SensorInterfaceReadCounter...\n");

    TRACE1("  SensorCounterValue: %lu\n", pulSensorCounterValue_l->load());

    #ifdef _SENSOR_IMPULSE_ANALYZER_
    {
        // save Measurement Table to file
        SiaSaveMeasurementTable(ANALYZER_FILE_PATH_NAME);
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
//  Callback function to handle events at Sensor GPIO (S0 Input)
//---------------------------------------------------------------------------

static  void  CbSensorGpioEvent (uint8_t uChannel_p, IoBool fEdge_p)
{

    TRACE2("\nGasMeterSensorInterface: CbSensorGpioEvent(Channel=%u, fEdge_p=%u)\n", (unsigned int)uChannel_p, (unsigned int)fEdge_p);

    // increment CounterValue
    if (uChannel_p == iSensorGpio_l)
    {
        if (pulSensorCounterValue_l != NULL)
        {
            pulSensorCounterValue_l->fetch_add(1, std::memory_order_relaxed);
            TRACE1("  -> New SensorCounterValue: %lu\n\n", pulSensorCounterValue_l->load());

            #ifdef _SENSOR_IMPULSE_ANALYZER_
            {
                SiaLogMeasurementEvent(pulSensorCounterValue_l->load());
            }
            #endif
        }
    }
    else
    {
        TRACE2("WARNING: Event from unexpected GPIO Channel (expected Channel: %u <-> rised Channel: %u\n\n", (unsigned int)iSensorGpio_l, (unsigned int)uChannel_p);
    }

    return;

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   H E L P E R   F U N C T I O N S                //
//          (used by SensorImpulseAnalyzer only)                           //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  SensorImpulseAnalyzer:  Setup Sensor Impuls Analyzer
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

static  void  SiaSetupMeasurementAnalyzer (const char* pszAnalyzerFilePathName_p)
{

    // mark MeasurementTable as empty
    uiMeasurementTableIdx_l = 0;

    // initialze mutex to support read/write operation in separate threads
    pthread_mutex_init(&mutTableMutex_l, NULL);

    // open Analyzer Logfile in append mode, create file if it does not already exist
    ofsAnalyzerFile_l.open(pszAnalyzerFilePathName_p, std::ios::app);
    if ( ofsAnalyzerFile_l.is_open() )
    {
        // write Header line
        ofsAnalyzerFile_l.write(ANALYZER_FILE_HEADER_LINE, strlen(ANALYZER_FILE_HEADER_LINE));
        ofsAnalyzerFile_l.flush();
        ofsAnalyzerFile_l.close();
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to open/create Analyzer Logfile ('%s'): '%s' (errno=%d)\n", pszAnalyzerFilePathName_p, strerror(errno), errno);
    }

    return;

}

#endif



//---------------------------------------------------------------------------
//  SensorImpulseAnalyzer:  Close Sensor Impuls Analyzer
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

static  void  SiaCloseMeasurementAnalyzer (void)
{

    // relase mutex to support read/write operation in separate threads
    pthread_mutex_destroy(&mutTableMutex_l);

    return;

}

#endif



//---------------------------------------------------------------------------
//  SensorImpulseAnalyzer:  Log Measurement Event (CounterValue/Timestamp)
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

static  void  SiaLogMeasurementEvent (unsigned long ulSensorCounterValue_p)
{

uint64_t  u64TimeStamp;


    // fetch system time in [ms]
    u64TimeStamp = SiaGetTimeStamp();

    // prevent further write operations if the MeasurementTable is already full
    if (uiMeasurementTableIdx_l >= (sizeof(aMeasurementTable_l)/sizeof(tMeasurementEvent)))
    {
        // too bad - MeasurementTable is already full
        return;
    }

    // write entry into MeasurementTable
    pthread_mutex_lock(&mutTableMutex_l);
    {
        aMeasurementTable_l[uiMeasurementTableIdx_l].m_ulSensorCounterValue = ulSensorCounterValue_p;
        aMeasurementTable_l[uiMeasurementTableIdx_l].m_u64TimeStamp = u64TimeStamp;

        uiMeasurementTableIdx_l++;
    }
    pthread_mutex_unlock(&mutTableMutex_l);
    
    return;
    
}

#endif



//---------------------------------------------------------------------------
//  SensorImpulseAnalyzer:  Save Measurement Table to File
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

static  void  SiaSaveMeasurementTable (const char* pszAnalyzerFilePathName_p)
{

char          szLineBuffer[128];
unsigned int  uiIdx;


    // Have any Measurement Entry been written to the Table since the last call?
    if (uiMeasurementTableIdx_l == 0)
    {
        // Table is empty -> no entries to process
        return;
    }

    // write all Measurement Entry from Table to File
    ofsAnalyzerFile_l.open(pszAnalyzerFilePathName_p, std::ios::app);
    if ( ofsAnalyzerFile_l.is_open() )
    {
        pthread_mutex_lock(&mutTableMutex_l);
        {
            for (uiIdx=0; uiIdx<uiMeasurementTableIdx_l; uiIdx++)
            {
                // convert Table Entry in CSV format and write it into File
                snprintf(szLineBuffer, sizeof(szLineBuffer), "%lu;%lu\n",
                         (unsigned long)aMeasurementTable_l[uiIdx].m_ulSensorCounterValue,
                         (unsigned long)aMeasurementTable_l[uiIdx].m_u64TimeStamp);

                ofsAnalyzerFile_l.write(szLineBuffer, strlen(szLineBuffer));
            }

            // mark Table as empty
            uiMeasurementTableIdx_l = 0;
        }
        pthread_mutex_unlock(&mutTableMutex_l);

        ofsAnalyzerFile_l.flush();
        ofsAnalyzerFile_l.close();
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to open/write Analyzer Logfile ('%s'): '%s' (errno=%d)\n", pszAnalyzerFilePathName_p, strerror(errno), errno);
    }

    return;

}

#endif



//---------------------------------------------------------------------------
//  SensorImpulseAnalyzer:  Get Timestamp in [ms]
//---------------------------------------------------------------------------

#ifdef _SENSOR_IMPULSE_ANALYZER_

static  uint64_t  SiaGetTimeStamp (void)
{

struct timespec  TimeSpec;
uint64_t         u64TimeStamp;
int              iRes;


    iRes = clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
    if (iRes == 0)
    {
        u64TimeStamp = (TimeSpec.tv_sec * 1000) + (TimeSpec.tv_nsec / 1000000);
    }
    else
    {
        u64TimeStamp = 0;
    }

    return (u64TimeStamp);

}

#endif



// EOF


