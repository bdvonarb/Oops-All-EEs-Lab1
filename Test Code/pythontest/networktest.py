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
    print(int(j["entry_id"]))
    print(' ')

    window.after(1000,update_v)


window = tk.Tk()   

    



window.after(1000,update_v)

window.mainloop()