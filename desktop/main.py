from _typeshed import NoneType
from password import THINGSPEAK_API_READKEY, THINGSPEAK_CHANNELID, THINGSPEAK_API_WRITEKEY
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

dispon=0
def trigger_remote():
    global dispon
    r1 = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field2': str(dispon)})
    print(r1.json())
    dispon ^= 1
    return

def update():
    temp_.config(text=round(temp[len(temp)-1],1))
    fig.clf()#clear graph to allow other graph to be placed over
    update_g()

def update_g():
    temp_figure = fig.add_subplot(111) #assigning the figure a plot 

    temp_figure.plot(time_s,temp, color= "black") #plotting time vs temp

    #displaying the plot to the window
    graph = FigureCanvasTkAgg(fig,master= window)
    graph.get_tk_widget().place(x=75,y=0)
    graph.draw()

def update_v():
    r = requests.get("https://api.thingspeak.com/channels/"+ THINGSPEAK_CHANNELID + "/fields/1/last.json", params={"api_key":THINGSPEAK_API_READKEY})
    j = r.json()
    global ID_value
    global a

    if ID_value!=int(j["entry_id"]):
        for x in range(0,299):
            temp[x]=temp[x+1]

        if a==1:
            temp[299]=(float(j["field1"])*1.8)+32


    update()
    window.after(1000,update_v)

a=0
def swi_temp():
    global a
    if a == 0:
        for x in range(0,300):
            temp[x]=(temp[x]*1.8)+32
        a=1
        update()

    else:
        for x in range(0,300):
            temp[x]=(temp[x]-32)/1.8
        a=0
        update()
    return

r = requests.get("https://api.thingspeak.com/channels/" + THINGSPEAK_CHANNELID + "/fields/1.json", params={"api_key":THINGSPEAK_API_READKEY,"results":"300"})
j = r.json()

window = tk.Tk() #create window

#window characteristics
window.title("Temperature Sensor by Oops All EE's")
window.geometry("750x750")

#create buttons, place them on the screen and set the command to be called when the button triggers an event
button_remote = tk.Button(window, text = "Display on Remote",font=tkFont.Font(size=9),height = 15 ,width = 30, command = trigger_remote)
button_remote.place(x="82",y="450")
button_temp = tk.Button(window, text = "",height = 15 ,width = 30, command = swi_temp)
button_temp.place(x="450",y="450")

#initialize time vector to represent the last 299 seconds plus the at the moment time
#initialize temp vector to be an empty vector of 300 spaces to be filled when data is read from the microcontroller
time_s = [-299]
temp = []
for x in range (1, 300):
    time_s.append(x-299)

for entry in j["feeds"]:
    if entry["field1"] != NoneType:
        temp.append(float(entry["field1"]))

ID_value = 0

siz=tkFont.Font(size=30) #size of temperature font in the window

fig = fi.Figure(figsize=(6,4.3)) #size of figure that represents the graph

update_g()

#creating and placing the temperature display value on the button
temp_=tk.Label(window, text = temp[len(temp)-1], font= siz)
temp_.place(x="520",y="545")

window.after(1000,update_v)

window.mainloop()

