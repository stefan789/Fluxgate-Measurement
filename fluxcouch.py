import pynedm
import ads
import cloudant
from threading import Lock

acct = cloudant.Account(uri="http://raid.nedm1:5984")
res = acct.login("mapperfluxgate_writer", "cluster")
assert res.status_code == 200
db = acct["nedm%2Ffluxgate"]
des = db.design("nedm_default")
the_view = des.view("latest_value")

class Fluxgate():
    lock = Lock()
    def __init__(self):
        self.a = ads.Ads(samples = 32684, clkRate = 32684)

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
    print "measure"
    adoc = {
            "type": "data",
            "value": {
                "runstatus": 1
                }
            }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print r.json()
    while is_running():
        once_measure(True)

    adoc = {
            "type": "data",
            "value": {
                "runstatus": 0
                }
            }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print r.json()

def once_measure(verbose=False):
    reading = _fg.measureonce()
    if verbose: print reading
    adoc = {
            "type": "data",
            "value": {
                "Bx": reading[0],
                "By": reading[1],
                "Bz": reading[2]
                }
            }
    return des.post("_update/insert_with_timestamp", params = adoc).json()

def log_and_measure(alog):
    measure = once_measure()
    if "ok" not in measure:
         raise Exception("Error measuring")
    doc = {
            "type" : "log",
            "log" : alog,
            "dataids" : [ measure["id"] ]
    }
    return des.post("_update/insert_with_timestamp", params = doc).json()

def start_measure():
    global _running, _myprocess
    if _myprocess is not None:
        raise Exception("Measurement already running")

    _running = True
    _myprocess = pynedm.start_process(_measure)
    return True

def stop_measure():
    print "stop_measure"
    global _running, _myprocess
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
    pynedm.listen(execute_dict, "nedm%2Ffluxgate", username="mapperfluxgate_writer", password="cluster", uri = "http://raid.nedm1:5984")

    pynedm.wait()
