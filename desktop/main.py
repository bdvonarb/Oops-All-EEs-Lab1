import tkinter as tk
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.figure as fig
from matplotlib.animation import FuncAnimation
import random as rd
import tkinter.font as tkFont


from tkinter import ttk
from tkinter import *
from tkinter.ttk import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


def trigger_remote():
    print("HI!")
    return

def update():
    temp_.config(text=temp[len(temp)-1])

a=0
def swi_temp():
    global a
    if a == 0:
        for x in range(0,301):
            temp[x]=(temp[x]*1.8)+32
        a=1
        update()

    else:
        for x in range(0,301):
            temp[x]=(temp[x]-32)/1.8
        a=0
        update()
    return

window = tk.Tk()

window.title("Temperature Sensor by Oops All EE's")
window.geometry("750x750")

button_remote = tk.Button(window, text = "Display on Remote",height = 15 ,width = 30, command = trigger_remote)
button_remote.place(x="82",y="450")

button_temp = tk.Button(window, text = "",height = 15 ,width = 30, command = swi_temp)
button_temp.place(x="450",y="450")

time_s = [-300]
temp = [0]
for x in range (0, 300):
    time_s.append(x-299)
    if x>=0:
        temp.append(0)

siz=tkFont.Font(size=30)

graph = tk.Canvas(window, bg="white", height=430, width= 600)
graph.pack()

temp_figure = fig.plot(time_s,temp, color= "black")

canvas = FigureCanvasTkAgg(temp_figure,master= window)
canvas.draw()

temp_=tk.Label(window, text = temp[len(temp)-1], font= siz)
temp_.place(x="537",y="545")

window.mainloop()

