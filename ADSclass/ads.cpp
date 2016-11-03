#include <Python.h>
#include "structmember.h"
#include <stdlib.h>
#include <stdio.h>
#include "inc/compatibility.h"
#include "inc/bdaqctrl.h"
using namespace Automation::BDaq;

#define deviceDescription  L"USB-4716,BID#0"
const wchar_t* profilePath = L"../../profile/DemoDevice.xml";

typedef struct {
  PyObject_HEAD
  double clkRate;
  int chStart;
  int chCount;
  int samples;
  double scale;
  int sectionLength;
  int sectionCount;
  int Buffersize;
  WaveformAiCtrl *wfAiCtrl;
} Ads;

static void Ads_dealloc(Ads* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Ads_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Ads *self;

  self = (Ads *)type->tp_alloc(type, 0);
  if (self != NULL) {
      self->clkRate = 1024;
      self->chStart = 0;
      self->chCount = 3;
      self->sectionLength = 1024;
      self->scale = 7000;
      self->samples = self->sectionLength;
      self->sectionCount = 1;
      self->wfAiCtrl = NULL;
  }

  return (PyObject *)self;
}

static int
Ads_init(Ads *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"clkRate", "chStart", "chCount", "samples", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|diii", kwlist,
                                      &self->clkRate, &self->chStart,
                                      &self->chCount, &self->sectionLength))
        return -1;

    ErrorCode ret = Success;

    self->wfAiCtrl = WaveformAiCtrl::Create();
    DeviceInformation devInfo(deviceDescription);
    ret = self->wfAiCtrl->setSelectedDevice(devInfo);
    //printf("SelectDevice ErrorCode: 0x%X. \n", ret);
		ret = self->wfAiCtrl->LoadProfile(profilePath);//Loads a profile to initialize the device.
		//CHK_RESULT(ret);

    // Get AI channels property for parameters setting.
    Array<AiChannel>* channels = self->wfAiCtrl->getChannels();
    // Set signal connection type.
    for(int i=0; i < channels->getCount(); ++i)
    {
      channels->getItem(i).setSignalType(SingleEnded);
    // Set value range.
      //channels->getItem(i).setValueRange(mV_Neg625To625);
      channels->getItem(i).setValueRange(V_Neg10To10);
    }

    Conversion* conversion = self->wfAiCtrl->getConversion();
    conversion->setChannelStart(self->chStart);
    conversion->setChannelCount(self->chCount);
    conversion->setClockRate(self->clkRate);
    //printf("SetClockRate ErrorCode: 0x%X. \n", ret);

    Record* record = self->wfAiCtrl->getRecord();
    ret = record->setSectionCount(self->sectionCount);
    ret = record->setSectionLength(self->sectionLength);
    //CHK_RESULT(ret);
    //printf("SectionLength ErrorCode: 0x%X. \n", ret);
    ret = self->wfAiCtrl->Prepare();
    //printf("Prepare ErrorCode: 0x%X. \n", ret);
    ret = self->wfAiCtrl->Start();
    printf("Start ErrorCode: 0x%X. \n", ret);

    return 0;
}

static PyObject *
Ads_params(Ads* self)
{
    if (self->clkRate == NULL) {
        PyErr_SetString(PyExc_AttributeError, "clkRate");
        return NULL;
    }

    if (self->chStart == NULL) {
        PyErr_SetString(PyExc_AttributeError, "chStart");
        return NULL;
    }

    return PyUnicode_FromFormat("%i %i", self->clkRate, self->chStart);
}

static PyObject*
Ads_readFGvalue(Ads* self)
{
      // Get Data
      ErrorCode ret = Success;
      int32 returnedCount = 0;

      ret = self->wfAiCtrl->Start();
      //printf("Start ErrorCode: 0x%X. \n", ret);

      int le = self->samples * self->chCount;

      double* Data = new double[le];
      ret = self->wfAiCtrl->GetData(self->samples * self->chCount, Data, -1, &returnedCount);
      //printf("# values: %i \n", returnedCount);

      //printf("GetData ErrorCode: 0x%X. \n", ret);
      /*for(int32 i = 0; i < self->chCount; ++i)
      {
        printf("channel %i: %10.6f \n", (i + self->chStart), Data[i]);
      }
      */
      ret = self->wfAiCtrl->Stop();
      //printf("Stop ErrorCode: 0x%X. \n", ret);

      double sum = 0;
      for(int32 i = 0; i < self->samples; ++i)
      {
          sum = sum + Data[3*i];
      }
      double meanBx = self->scale * sum/self->samples;
      sum = 0;
      for(int32 i = 0; i < self->samples; ++i)
      {
          sum = sum + Data[3*i+1];
      }
      double meanBy = self->scale * sum/self->samples;
      sum = 0;
      for(int32 i = 0; i < self->samples; ++i)
      {
          sum = sum + Data[3*i+2];
      }
      double meanBz = self->scale * sum/self->samples;

      delete[] Data;
      return Py_BuildValue("ddd", meanBx, meanBy, meanBz);
}

static PyObject*
Ads_readFGfast(Ads* self)
{
      // Get Data
      ErrorCode ret = Success;
      int32 returnedCount = 0;

      ret = self->wfAiCtrl->Start();
      //printf("Start ErrorCode: 0x%X. \n", ret);

      int le = self->samples * self->chCount;

      double* Data = new double[le];
      ret = self->wfAiCtrl->GetData(self->samples * self->chCount, Data, -1, &returnedCount);
      //printf("# values: %i \n", returnedCount);

      //printf("GetData ErrorCode: 0x%X. \n", ret);
      /*for(int32 i = 0; i < self->chCount; ++i)
      {
        printf("channel %i: %10.6f \n", (i + self->chStart), Data[i]);
      }
      */
      ret = self->wfAiCtrl->Stop();
      //printf("Stop ErrorCode: 0x%X. \n", ret);

      PyObject *lst = PyList_New(self->samples * self->chCount);
      if (!lst)
          return NULL;
      for (int i = 0; i < self->samples * self->chCount; i++) {
          PyObject *num = PyFloat_FromDouble(self->scale*Data[i]);
          if (!num) {
              Py_DECREF(lst);
              return NULL;
          }
          PyList_SET_ITEM(lst, i, num);   // reference to num stolen
      }
      return lst;
}


static PyObject*
Ads_getClkRate(Ads* self)
{
  double rate;
  Conversion* conversion = self->wfAiCtrl->getConversion();
  rate = conversion->getClockRate();
  return Py_BuildValue("d", rate);
}

static PyObject*
Ads_setClkRate(Ads* self, PyObject *args)
{
  //printf("setclockrate \n");
  //PyObject *tmp;
  ErrorCode ret = Success;
  double clkRate;

  if (! PyArg_ParseTuple(args, "d", &clkRate))
      return NULL;

  //printf("pyarg: %d. \n", clkRate);
  Conversion* conversion = self->wfAiCtrl->getConversion();
  ret = conversion->setClockRate(clkRate);
  //printf("setclockrate ErrorCode: 0x%X. \n", ret);
  self->clkRate = clkRate;
  return Py_BuildValue("d", clkRate);
}

static PyObject*
Ads_setRangeTo10(Ads* self)
{
  // Get AI channels property for parameters setting.
  Array<AiChannel>* channels = self->wfAiCtrl->getChannels();
  // Set signal connection type.
  for(int i=0; i < channels->getCount(); ++i)
  {
    channels->getItem(i).setSignalType(SingleEnded);
  // Set value range.
    //channels->getItem(i).setValueRange(mV_Neg625To625);
    channels->getItem(i).setValueRange(V_Neg10To10);
  }
  return Py_BuildValue("d", 0);
}

static PyObject*
Ads_setRangeTo625(Ads* self)
{
  // Get AI channels property for parameters setting.
  Array<AiChannel>* channels = self->wfAiCtrl->getChannels();
  // Set signal connection type.
  for(int i=0; i < channels->getCount(); ++i)
  {
    channels->getItem(i).setSignalType(SingleEnded);
  // Set value range.
    channels->getItem(i).setValueRange(mV_Neg625To625);
    //channels->getItem(i).setValueRange(V_Neg10To10);
  }
  return Py_BuildValue("d", 0);
}


static PyObject*
Ads_getSectionLength(Ads* self)
{
  double sl;
  Record* record = self->wfAiCtrl->getRecord();
  sl = record->getSectionLength();
  return Py_BuildValue("d", sl);
}

static PyMethodDef Ads_methods[] = {
    {"readFGvalue", (PyCFunction)Ads_readFGvalue, METH_NOARGS,
    "Return params"
    },
    {"readFGfast", (PyCFunction)Ads_readFGfast, METH_NOARGS,
    "Return params"
    },
    {"setRangeTo10", (PyCFunction)Ads_setRangeTo10, METH_NOARGS,
    "Return params"
    },
    {"setRangeTo625", (PyCFunction)Ads_setRangeTo625, METH_NOARGS,
    "Return params"
    },
    {"getClkRate", (PyCFunction)Ads_getClkRate, METH_NOARGS,
    "Return clkrate"
    },
    {"setClkRate", (PyCFunction)Ads_setClkRate, METH_VARARGS,
    "Return clkrate"
    },
    {"getSectionLength", (PyCFunction)Ads_getSectionLength, METH_NOARGS,
    "Return sectionlength"
    },
    {"params", (PyCFunction)Ads_params, METH_NOARGS,
     "Return params"
    },
    {NULL}  /* Sentinel */
};

static PyMemberDef Ads_members[] = {
    {"clkRate", T_INT, offsetof(Ads, clkRate), 0,
     "sampling freq"},
    {"chStart", T_INT, offsetof(Ads, chStart), 0,
     "first channel"},
    {"chCount", T_INT, offsetof(Ads, chCount), 0,
     "number of channels"},
    {"samples", T_INT, offsetof(Ads, samples), 0,
      "samples to read"},
    {"scale", T_DOUBLE, offsetof(Ads, scale), 0,
      "V to nT"},
    {NULL}  /* Sentinel */
};

static PyTypeObject AdsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ads.Ads",             /* tp_name */
    sizeof(Ads),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)Ads_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Ads objects",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Ads_methods,             /* tp_methods */
    Ads_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Ads_init,      /* tp_init */
    0,                         /* tp_alloc */
    Ads_new,                 /* tp_new */
};

static PyModuleDef adsmodule = {
  PyModuleDef_HEAD_INIT,
  "ads",
  "ads module",
  -1,
  NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_ads(void)
{
  PyObject* m;

  if (PyType_Ready(&AdsType) < 0)
      return NULL;

  m = PyModule_Create(&adsmodule);
  if (m == NULL)
      return NULL;

  Py_INCREF(&AdsType);
  PyModule_AddObject(m, "Ads", (PyObject *)&AdsType);
  return m;
}
