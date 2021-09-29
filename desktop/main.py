from numpy.core.numeric import NaN
from numpy.lib.function_base import diff
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
import datetime

from tkinter import ttk
from tkinter import *
from tkinter.ttk import *
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg   

MaxT=100
MinT=0
Phone=4205556969
def enter_v():
    def enter_but():
        global MaxT
        global MinT
        global Phone
        MaxT=int(Over_Temp.get())
        MinT=int(Under_Temp.get())
        Phone=int(phone_num.get())
        if MaxT<150 and MinT>-30 and Phone>1000000000 and Phone<10000000000:
            r = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1':'0', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
            window2.destroy()
        else:
            MaxT=63
            MinT=0
            Phone=4205556969
            enter_but()

    window2=tk.Tk()
    window2.title("Temperature Sensor Data Entry")
    window2.geometry("180x180")

    label_OT=tk.Label(window2, text = "Upper Temp Bound:")
    label_OT.place(x="10",y="40")

    label_UT=tk.Label(window2, text = "Lower Temp Bound:")
    label_UT.place(x="10",y="80")

    label_Pnum=tk.Label(window2, text = "Phone # :")
    label_Pnum.place(x="10",y="120")        

    Over_Temp=tk.Entry(window2)
    Over_Temp.place(x="120",y="40",height = 21, width=45)

    Under_Temp=tk.Entry(window2)
    Under_Temp.place(x="120",y="80",height = 21, width=45)

    phone_num=tk.Entry(window2)
    phone_num.place(x="65",y="120",height = 21, width=100)

    rule=tk.Label(window2, text = "Enter Temperature in Celsius")
    rule.place(x="10",y="10")  

    enter=tk.Button(window2,height=1,width=20,text="Enter",command=enter_but)
    enter.place(x='14',y='150')

    window2.mainloop()

dispon=0
def trigger_remote():
    global dispon
    global MaxT
    global MinT
    global Phone
    if dispon == 0:
        r1 = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1': '1', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
        #print(r1.status_code)
        #print(r1.json())
        dispon=1
    elif dispon == 1:
        r1 = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1': '0', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
        #print(r1.status_code)
        #print(r1.json())
        dispon=0       
    return

def update():
    temp_.config(text=round(temp[len(temp)-1],1))
    if not temp[-1] < 600 and not temp[-1] > -600:
        temp_.config(text="No Data")
        temp_.place(x="490",y="545")

    fig.clf()#clear graph to allow other graph to be placed over
    update_g()

def update_g():
    temp_figure = fig.add_subplot(111) #assigning the figure a plot 

    temp_figure.plot(time_s,temp, color= "black") #plotting time vs temp
    temp_figure.set_title("Temperature")
    temp_figure.set_xlabel("Last 300 Seconds")
    
    if a==1:
        temp_figure.set_ylabel("Degrees Fahrenheit")
        temp_figure.set_ybound(lower=50,upper=122)
    else:
        temp_figure.set_ylabel("Degrees Celsius")
        temp_figure.set_ybound(lower=10,upper=50)

    #displaying the plot to the window
    graph = FigureCanvasTkAgg(fig,master= window)
    graph.get_tk_widget().place(x=75,y=0)
    graph.draw()

def update_v():
    r = requests.get("https://api.thingspeak.com/channels/"+ THINGSPEAK_CHANNELID + "/fields/1/last.json", params={"api_key":THINGSPEAK_API_READKEY})
    j = r.json()
    global ID_value
    global a

    compare_to_utc=str(datetime.datetime.utcnow())
    compare_to_utc=compare_to_utc[-15:]
    compare_to_utc=int(compare_to_utc[0:2])*3600+int(compare_to_utc[3:5])*60+int(compare_to_utc[6:8])

    t_reading=str(j["created_at"])
    t_reading=t_reading[-9:]
    t_reading=int(t_reading[0:2])*3600+int(t_reading[3:5])*60+int(t_reading[6:8])
    
    diff_t=compare_to_utc-t_reading

    
    if diff_t<3:
        if ID_value!=int(j["entry_id"]):
            for x in range(0,299):
                temp[x]=temp[x+1]

            if a==1:
                temp[299]=(float(j["field1"])*1.8)+32
            else:
                temp[299]=float(j["field1"])
        update()
    
 
    else:
        for x in range(0,299):
            temp[x]=temp[x+1]
        temp[299]=NaN
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

enter_v()

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
    if entry["field1"] != None:
        temp.append(float(entry["field1"]))
    else:
        temp.append(0)

ID_value = 0

siz=tkFont.Font(size=30) #size of temperature font in the window

fig = fi.Figure(figsize=(6,4.3)) #size of figure that represents the graph

update_g()

#creating and placing the temperature display value on the button
temp_=tk.Label(window, text = temp[len(temp)-1], font= siz)
temp_.place(x="520",y="545")

window.after(1000,update_v)

window.mainloop()

