import tkinter as tk
import matplotlib.pyplot as plt
import numpy as np

from tkinter import ttk
from tkinter import *
from tkinter.ttk import *

window = tk.Tk()

window.title("Temperature Sensor by Oops All EE's")
window.geometry("750x750")

button_remote = tk.Button(window, text = "Display on Remote",height = 15 ,width = 30)
button_remote.place(x="100",y="450")

button_temp = tk.Button(window, text = "TEMP",height = 15 ,width = 30)
button_temp.place(x="450",y="450")









window.mainloop()

