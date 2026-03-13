/****************************************************************************

  Copyright (c) 2026 Ronald Sieber

  Project:      SmartMeterMonitor (Extension for S0 based GasMeter)
  Description:  Declarations for GasMeterSensorInterface

  -------------------------------------------------------------------------

  Revision History:

  2026/01/17 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _GMSENSIF_H_
#define _GMSENSIF_H_


#include <atomic>



//-----------------------------------------------------------------------
//  Prototypes of GasMeterSensorInterface Functions
//-----------------------------------------------------------------------

int  SensorInterfaceSetup (int iSensorGpio_p, std::atomic<unsigned long>* pulSensorCounterValue_p);
int  SensorInterfaceClose (void);
int  SensorInterfaceReadCounter (void);



#endif  // _GMSENSIF_H_


