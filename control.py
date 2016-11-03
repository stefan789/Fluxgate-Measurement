import cloudant

acct = cloudant.Account(uri="http://raid.nedm1:5984")
res = acct.login("mapperfluxgate_writer", "cluster")
assert res.status_code == 200

db = acct["nedm%2Ffluxgate"]
des = db.design("nedm_default")

the_view = des.view("latest_value")

def start():
    adoc = {
        "type": "command",
        "execute": "start_measure",
        "arguments" : []
        }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print("start command posted")
    #print r.json()

def stop():
    adoc = {
        "type": "command",
        "execute": "stop_measure",
        "arguments" : []
        }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print("stop command posted")
    #print r.json()

def once():
    adoc = {
            "type": "command",
            "execute": "log_and_measure",
            "arguments" : [ "test comment" ]
            }
    r = des.post("_update/insert_with_timestamp", params = adoc)
    print(r.json())
    print("measured once")
    
