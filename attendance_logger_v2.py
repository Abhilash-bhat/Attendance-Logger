import datetime
from time import time
from tabulate import tabulate
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db


log_path = "logs/"
name_path = "users/"

cred = credentials.Certificate(
    "attendance-logger-436e5-firebase-adminsdk-ca5rj-2793ac7daa.json")

firebase_admin.initialize_app(cred, {
    'databaseURL': "https://attendance-logger-436e5-default-rtdb.asia-southeast1.firebasedatabase.app/"
})


def fetch_logs(names, date):
    date = date.replace("-", "")
    date_ref = db.reference(log_path + date + "/")
    try:
        records_fetch = dict(date_ref.order_by_child("time").get().items())
        records = [["name", "rfid_code", "status", "timestamp"]]
        for i in records_fetch.keys():
            rfid_code = records_fetch[i]["rfid_code"]
            status = records_fetch[i]["status"]
            timestamp = records_fetch[i]["time"]
            try:
                name = names[rfid_code]
                records += [[name, rfid_code, status, timestamp]]
            except KeyError:
                records += [["null", rfid_code, status, timestamp]]
        print(tabulate(records))
    except:
        print("No records exist.\n")

def main():
    help_str = [
        ["Action", "Syntax"],
        ["Add users", "add <rfid_code> <username>"],
        ["Rename users", "rename <rfid_code> <username>"],
        ["Delete users", "delete <rfid_code>"],
        ["Fetch logs for one date", "logd <date[DD-MM-YYYY]>"],
        ["Fetch logs for a date range", "logr <start_date[DD-MM-YYYY]> <end_date[DD-MM-YYYY]>"]
    ]
    
    print(tabulate(help_str))
    name_ref = db.reference(name_path)
    names = dict(name_ref.get())
    while (True):
        x = input("> ")
        if "help" in x:
            print(tabulate(help_str))
        elif "logd" in x:
            date = x.split(" ")[1]
            print(f"Logs for: {date}")
            fetch_logs(names, date)
        elif "logr" in x:
            date1 = x.split(" ")[1]
            date2 = x.split(" ")[2]
            start = datetime.datetime.strptime(date1, "%d-%m-%Y")
            end = datetime.datetime.strptime(date2, "%d-%m-%Y")
            date_generated = [
                start + datetime.timedelta(days=i) for i in range(0, (end-start).days + 1)]
            for i in date_generated:
                print(f"Logs for date: " + i.strftime("%d-%m-%Y"))
                fetch_logs(names, i.strftime("%d-%m-%Y"))
        elif "add" in x:
            rfid_code = x.split(" ")[1]
            name = x.split(" ")[2]
            name_ref.child(rfid_code).set(name)
            names = dict(name_ref.get())
        elif "rename" in x:
            rfid_code = x.split(" ")[1]
            name = x.split(" ")[2]
            name_ref.child(rfid_code).set(name)
            names = dict(name_ref.get())
        elif "delete" in x:
            rfid_code = x.split(" ")[1]
            name_ref.child(rfid_code).delete()
            names = dict(name_ref.get())
        elif "exit" in x:
            break
        else:
            print("Invalid command")

if __name__ == '__main__':
    main()