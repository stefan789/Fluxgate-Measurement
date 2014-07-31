#include <Python.h>
#include "structmember.h"
#include <stdlib.h>
#include <stdio.h>
#include "inc/compatibility.h"
#include "inc/bdaqctrl.h"
using namespace Automation::BDaq;


typedef struct {
    PyObject_HEAD
    int32_t samples;
    int clkRate;
    int chStart;
    int chCount;
    BufferedAiCtrl *bfdAiCtrl;
} Ads;

static void
Ads_dealloc(Ads* self) 
{
    self->bfdAiCtrl->Cleanup();
    self->bfdAiCtrl->Dispose();

    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * 
Ads_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Ads *self;

    self = (Ads *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->samples = 16384;
        self->clkRate = 16384;
        self->chStart = 0;
        self->chCount = 3;
        self->bfdAiCtrl = NULL;
    }

    return (PyObject *)self;
}

static int
Ads_init(Ads *self, PyObject *args, PyObject *kwds)
{
    int tmp;

    static char *kwlist[] = {"samples", "clkRate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ii", kwlist, &self->samples, &self->clkRate))
        return -1;


    ErrorCode ret = Success;

    self->bfdAiCtrl = AdxBufferedAiCtrlCreate();
    DeviceInformation devInfo(0);
    ret = self->bfdAiCtrl->setSelectedDevice(devInfo);
    CHK_RESULT(ret);
    printf("ErrorCode: 0x%X. \n", ret);

    AiChannelCollection *channels = self->bfdAiCtrl->getChannels();
    for(int i=0; i < channels->getCount(); ++i)
    {
        channels->getItem(i).setValueRange(mV_Neg625To625);
    }
    self->bfdAiCtrl->getScanChannel()->setChannelStart(self->chStart);
    self->bfdAiCtrl->getScanChannel()->setChannelCount(self->chCount);
    self->bfdAiCtrl->getScanChannel()->setSamples(self->samples);
    self->bfdAiCtrl->getScanChannel()->setIntervalCount(self->samples);
    
    self->bfdAiCtrl->getConvertClock()->setRate(self->clkRate);
    
    self->bfdAiCtrl->setStreaming(false);
    ret = self->bfdAiCtrl->Prepare();
    CHK_RESULT(ret);
    printf("ErrorCode: 0x%X. \n", ret);
    
    return 0;
}

static PyMemberDef Ads_members[] = {
/*    {"samples", T_INT, offsetof(Ads, samples), 0, "samples"},
    {"clkRate", T_INT, offsetof(Ads, clkRate), 0, "clkRate"},
    {"chStart", T_INT, offsetof(Ads, chStart), 0, "chStart"},
    {"chCount", T_INT, offsetof(Ads, chCount), 0, "chCount"}, */
    {NULL} /* Sentinel */
};

static PyObject *
Ads_readFGvalue(Ads* self)
{
    double Data[self->samples * self->chCount];

    self->bfdAiCtrl->RunOnce();
    SLEEP(1);
    self->bfdAiCtrl->GetData(self->samples * self->chCount,Data);
    double sum = 0;
    double scale = 7000;
    for(int32 i = 0; i < self->samples; ++i)
    {
        sum = sum + Data[3*i];
    }
    double meanBx = scale * sum/self->samples;
    sum = 0;
    for(int32 i = 0; i < self->samples; ++i)
    {
        sum = sum + Data[3*i+1];
    }
    double meanBy = scale * sum/self->samples;
    sum = 0;
    for(int32 i = 0; i < self->samples; ++i)
    {
        sum = sum + Data[3*i+2];
    }
    double meanBz = scale * sum/self->samples;
    return Py_BuildValue("ddd", meanBx, meanBy, meanBz);
}

static PyObject *
Ads_getSamples(Ads* self)
{
    return Py_BuildValue("i", self->samples);
}

static PyObject *
Ads_getclkRate(Ads* self)
{
    return Py_BuildValue("i", self->clkRate);
}

static PyMethodDef Ads_methods[] = {
    {"readFGvalue", (PyCFunction)Ads_readFGvalue, METH_NOARGS,
        "read fg values and return result"},
    {"getSamples", (PyCFunction)Ads_getSamples, METH_NOARGS, "return sample number"},
    {"getclkRate", (PyCFunction)Ads_getclkRate, METH_NOARGS, "return clock rate"},

    {NULL}
};

static PyTypeObject AdsType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "ads.Ads",             /*tp_name*/
    sizeof(Ads),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Ads_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Ads objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
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

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initads(void)
{
    PyObject* m;

    if (PyType_Ready(&AdsType) < 0)
        return;

    m = Py_InitModule3("ads", module_methods, "Module to handle ADS DAQ for fluxgate readout");

    if (m == NULL)
        return;

    Py_INCREF(&AdsType);
    PyModule_AddObject(m, "Ads", (PyObject *)&AdsType);
}

