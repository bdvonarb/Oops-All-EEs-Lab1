from password import THINGSPEAK_API_READKEY, THINGSPEAK_CHANNELID
import tkinter as tk
import numpy as np
import matplotlib.figure as fi
import matplotlib.animation as ani
from matplotlib.animation import FuncAnimation
import random as rd
import tkinter.font as tkFont
import requests
import json
import math
import time
import threading

import datetime

from tkinter import ttk
from tkinter import *
from tkinter.ttk import *
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg        


def update_v():
    #r = requests.get("https://api.thingspeak.com/channels/" + THINGSPEAK_CHANNELID + "/fields/1.json", params={"api_key":THINGSPEAK_API_READKEY,"results":"300"})
    r = requests.get("https://api.thingspeak.com/channels/"+ THINGSPEAK_CHANNELID + "/fields/1/last.json", params={"api_key":THINGSPEAK_API_READKEY})
    j = r.json()

    #for entry in j["feeds"]:
    #   print(int(entry["entry_id"]))
    #print(int(j["entry_id"]))
    #print(j["created_at"])
    compare_to_utc=str(datetime.datetime.utcnow())
    compare_to_utc=compare_to_utc[-15:]

    compare_to_utc=int(compare_to_utc[0:2])*3600+int(compare_to_utc[3:5])*60+int(compare_to_utc[6:8])

    t_reading=str(j["created_at"])
    t_reading=t_reading[-9:]
    t_reading=int(t_reading[0:2])*3600+int(t_reading[3:5])*60+int(t_reading[6:8])
    print('New Read')
    print(t_reading)
    print(' ')
    print(compare_to_utc)
    print(' ')
    
    diff_t=compare_to_utc-t_reading
    print(diff_t)
    print(' ')



    if diff_t>2:
        print("oh NO! OoO")

    window.after(1000,update_v)

window = tk.Tk()   

    



window.after(1000,update_v)

window.mainloop()