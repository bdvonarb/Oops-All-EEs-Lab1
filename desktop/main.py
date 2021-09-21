
import tkinter as tk
import matplotlib.pyplot as plt
#import matplotlib.figure as Figure
import numpy as np
from matplotlib.animation import FuncAnimation
import random as rd
import tkinter.font as tkFont

from tkinter import ttk
from tkinter import *
from tkinter.ttk import *

window = tk.Tk()

window.title("Temperature Sensor by Oops All EE's")
window.geometry("750x750")

button_remote = tk.Button(window, text = "Display on Remote",height = 15 ,width = 30)
button_remote.place(x="100",y="450")

button_temp = tk.Button(window, text = "",height = 15 ,width = 30)
button_temp.place(x="450",y="450")

I=True
time_s = [-300]
temp = [0]
for x in range (0, 300):
    time_s.append(x-299)
    if x>0:
        temp.append(0)

siz=tkFont.Font(size=30)

temp_=tk.Label(window, text = temp[len(temp)-1], font= siz)
temp_.place(x="537",y="545")


#def animate(i):
#    plt.cla()
#    plt.plot(time_s,temp)

#ani = FuncAnimation(plt.gcf(),animate)

#plt.show()








window.mainloop()

