#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include "inc/compatibility.h"
#include "inc/bdaqctrl.h"
using namespace Automation::BDaq;

#define      deviceDescription 0 

double clkRate = 16384;
int chStart = 0;
const int chCount = 3;
const int samples = 16384;

// volt to nanotesla scale
double scale = 7000;


double       Data[chCount * samples];

/*idea to return B field means sefdefined struct
works maybe
*/
struct Bvec {
    double x;
    double y;
    double z; 
};

Bvec B;

inline void waitAnyKey()
{
   do{SLEEP(1);} while(!kbhit());
} 

static PyObject* readFGvalue(PyObject* self, PyObject* args)
{
   ErrorCode   ret = Success;

   // Step 1: Create a 'BufferedAiCtrl' for buffered AI function.
   BufferedAiCtrl * bfdAiCtrl = AdxBufferedAiCtrlCreate();
   do
   {
      DeviceInformation devInfo(deviceDescription);
      ret = bfdAiCtrl->setSelectedDevice(devInfo);
      CHK_RESULT(ret);
      
      AiChannelCollection *channels = bfdAiCtrl->getChannels();
      for(int i=0; i < channels->getCount(); ++i)
      {
        channels->getItem(i).setValueRange(mV_Neg625To625);
      }

      bfdAiCtrl->getScanChannel()->setChannelStart(chStart);
      bfdAiCtrl->getScanChannel()->setChannelCount(chCount);
      bfdAiCtrl->getScanChannel()->setSamples(samples);
      bfdAiCtrl->getScanChannel()->setIntervalCount(samples);
      
      bfdAiCtrl->getConvertClock()->setRate(clkRate);
      
      bfdAiCtrl->setStreaming(false);
    } while(false);
      
    // Step 4: prepare the buffered AI. 
    ret = bfdAiCtrl->Prepare();
    CHK_RESULT(ret);

    bfdAiCtrl->RunOnce();
    SLEEP(1);
    
    
    // Step 6: Read samples and do post-process, we show the first sample of each channel to console here.
    ret = bfdAiCtrl->GetData(samples * chCount,Data);
    CHK_RESULT(ret);
    /*
    printf("Acquisition has completed!\n\n");
    printf("the first data each channel are:\n\n");
    int32 channelCountMax  = bfdAiCtrl->getFeatures()->getChannelCountMax();
	for(int32 i = 0; i < chCount; ++i)
	{
		printf("channel %d: %10.6f \n",(i + chStart)%channelCountMax,Data[i]);   
	}
    */
    // Extract field components from dataBuffer
    /*for(int32 i=0; i < chCount; ++i)
    {
        printf("%10.6f \n", scale*Data[i]);
    }*/
    double Bx[samples];
    double sum = 0;
    for(int32 i = 0; i < samples; ++i)
    {
        Bx[i] = scale * Data[3*i];
        sum = sum + Bx[i];
    }
    double meanBx = sum/samples;
    B.x = meanBx;
    sum = 0;
    double By[samples];
    for(int32 i = 0; i < samples; ++i)
    {
        By[i] = scale * Data[3*i+1];
        sum = sum + By[i];
    }
    double meanBy = sum/samples;
    B.y = meanBy;
    sum = 0;
    double Bz[samples];
    for(int32 i = 0; i < samples; ++i)
    {
        Bz[i] = scale * Data[3*i+2];
        sum = sum + Bz[i];
    }
    double meanBz = sum/samples;
    B.z = meanBz;
    // calculate mean over last databuffer
    //
    /*double sum = 0;
    for(int32 i = 0; i < samples; ++i)
    {
        sum = sum + Bx[i];
    }
    /double meanBx = sum/samples;
    B.x = meanBx;
    sum = 0;
    for(int32 i = 0; i < samples; ++i)
    {
        sum = sum + By[i];
    }
    double meanBy = sum/samples;
    B.y = meanBy;
    sum = 0;
    for(int32 i = 0; i < samples; ++i)
    {
        sum = sum + Bz[i];
    }
    double meanBz = sum/samples;
    B.z = meanBz;
    */
	// Step 7: close device, release any allocated resource.
    bfdAiCtrl->Cleanup();
    bfdAiCtrl->Dispose();   

	// If something wrong in this execution, print the error code on screen for tracking.
   if(BioFailed(ret))
   {
      printf("Some error occurred. And the last error code is Ox%X.\n", ret);
   }
   
   return Py_BuildValue("ddd", B.x,B.y,B.z);   
}
 
static PyMethodDef ADSMethods[] =
{
     {"readFGvalue", readFGvalue, METH_VARARGS, "Greet somebody."},
     {NULL, NULL, 0, NULL}
};
 
PyMODINIT_FUNC
 
initadsinstantmod(void)
{
     (void) Py_InitModule("adsinstantmod", ADSMethods);
}
