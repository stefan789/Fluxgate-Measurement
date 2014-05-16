import pynedm
import adsinstantmod as ads
import time
import cloudant

acct = cloudant.Account(uri="http://raid.nedm1:5984")
res = acct.login("mapperfluxgate_writer", "cluster")
assert res.status_code == 200
db = acct["nedm%2Ffluxgate"]
des = db.design("nedm_default")
the_view = des.view("latest_value")


_running = False
_myprocess = None

def is_running():
    return _running

def _measure():
    print "measure"
    print is_running()
    adoc = {
            "type": "data",
            "value": {
                "runstatus": 1
                }
            }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print r.json()
    while is_running():
        print is_running()
        reading = ads.readFGvalue()
        time.sleep(1)
        adoc = {
            "type": "data",
            "value": {
                "Bx": reading[0],
                "By": reading[1],
                "Bz": reading[2]
                }
            }
        r = des.post("_update/insert_with_timestamp", params = adoc)
        print reading
    print is_running()
    adoc = {
            "type": "data",
            "value": {
                "runstatus": 0
                }
            }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print r.json()

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
        "stop_measure": stop_measure
        }

pynedm.listen(execute_dict, "nedm%2Ffluxgate", username="mapperfluxgate_writer", password="cluster", uri = "http://raid.nedm1:5984")

pynedm.wait()
