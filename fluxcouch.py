import pynedm
import ads
from threading import Lock

_db = "nedm%2Ffluxgate"
po = pynedm.ProcessObject("http://raid.nedm1:5984",
   "mapperfluxgate_writer",
   "cluster")

class Fluxgate():
    lock = Lock()
    def __init__(self):
        self.a = ads.Ads(clkRate=16384 , chStart=0, chCount=3, samples=65536)
        self.a.setRangeTo625()
        #self.a = ads.Ads(samples = 10240, clkRate = 1024)

    def measureonce(self):
        Fluxgate.lock.acquire()
        try:
            reading = self.a.readFGvalue()
        finally:
            Fluxgate.lock.release()

        return reading

_running = False
_myprocess = None

_fg = Fluxgate()

def is_running():
    return _running

def _measure():
    print("measure")
    while is_running():
        once_measure(True)

def once_measure(verbose=False):
    reading = _fg.measureonce()
    if verbose: print([i for i in reading])
    adoc = {
            "type": "data",
            "value": {
                "Bx1": reading[0],
                "By1": reading[1],
                "Bz1": reading[2]
                }
           }
    return po.write_document_to_db(adoc, _db)

def log_and_measure(alog):
    measure = once_measure(verbose=True)
    if "ok" not in measure:
         raise Exception("Error measuring")
    ldoc = {
            "type" : "log",
            "log" : alog,
            "dataids" : [measure["id"]]
           }
    print(alog)
    return po.write_document_to_db(ldoc, _db)

def start_measure():
    print("start measure")
    global _running, _myprocess
    if _myprocess is not None:
        raise Exception("Measurement already running")
    adoc = {
            "type": "data",
            "value": {
                "runstatus": 1
                }
            }
    print(po.write_document_to_db(adoc, _db))

    _running = True
    _myprocess = pynedm.start_process(_measure)
    return True

def stop_measure():
    print("stop_measure")
    global _running, _myprocess
    adoc = {
            "type": "data",
            "value": {
                "runstatus": 0
                }
            }
    print(po.write_document_to_db(adoc, _db))

    if _myprocess is None:
        raise Exception("Measurement not running")

    _running = False
    retVal = _myprocess.result.get()
    _myprocess = None
    return retVal

execute_dict =  {
        "start_measure": start_measure,
        "stop_measure": stop_measure,
        "log_and_measure": log_and_measure
        }


if __name__ == "__main__":
    o = pynedm.listen(execute_dict, _db, username="mapperfluxgate_writer", password="cluster", uri = "http://raid.nedm1:5984")

    o.wait()
