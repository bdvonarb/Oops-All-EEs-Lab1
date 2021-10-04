from numpy.core.numeric import NaN
from password import THINGSPEAK_API_READKEY, THINGSPEAK_CHANNELID, THINGSPEAK_API_WRITEKEY
import tkinter as tk
import numpy as np
import matplotlib.figure as fi
import tkinter.font as tkFont
import requests
import datetime

from tkinter import *
from tkinter.ttk import *
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg   
#default temp values and fake phone number
MaxT=100
MinT=0
Phone=4205556969

#first window function that request you put in the upper temp, lower temp and a phone number
def enter_v():
    def enter_but():

        #global variables for lower and upper temp and phone so we can change them in the function
        global MaxT
        global MinT
        global Phone

        #get the values from the dialogue boxes of the first window
        MaxT=int(Over_Temp.get())
        MinT=int(Under_Temp.get())
        Phone=int(phone_num.get())

        #only allow temp values of 150 to -30 to be entered and forces the length of a phone number to be correct
        if MaxT<150 and MinT>-30 and Phone>1000000000 and Phone<10000000000:
            r = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1':'0', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
            window2.destroy()#destroys window after sending information to the server
        
        #puts in default values if incorrect numbers are input
        else:
            MaxT=63
            MinT=0
            Phone=4205556969
            window2.destroy()

    #create window and title
    window2=tk.Tk()
    window2.title("Temperature Sensor Data Entry")
    window2.geometry("180x180")

    #create label for upper temp bound 
    label_OT=tk.Label(window2, text = "Upper Temp Bound:")
    label_OT.place(x="10",y="40")

    #create label for lower temp bound
    label_UT=tk.Label(window2, text = "Lower Temp Bound:")
    label_UT.place(x="10",y="80")

    #create label for phone number
    label_Pnum=tk.Label(window2, text = "Phone # :")
    label_Pnum.place(x="10",y="120")        

    #create input box for upper temp
    Over_Temp=tk.Entry(window2)
    Over_Temp.place(x="120",y="40",height = 21, width=45)

    #create input box for lower temp
    Under_Temp=tk.Entry(window2)
    Under_Temp.place(x="120",y="80",height = 21, width=45)

    #create input box for phone number
    phone_num=tk.Entry(window2)
    phone_num.place(x="65",y="120",height = 21, width=100)

    #input information
    rule=tk.Label(window2, text = "Enter Temperature in Celsius")
    rule.place(x="10",y="10")  

    #button that enters the data
    enter=tk.Button(window2,height=1,width=20,text="Enter",command=enter_but)
    enter.place(x='14',y='150')

    window2.mainloop()

#function to turn on the remote display
dispon=0 #variable that keeps track of the switch being on or off
def trigger_remote():
    #we send the upper and lower temp bounds during this cycle in order to only have one data line
    global dispon
    global MaxT
    global MinT
    global Phone
    if dispon == 0:
        #sends data to server with field 1 being 1 indicating the MC should turn on the MOSFET allowing the display to be on
        r1 = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1': '1', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
        dispon=1
    elif dispon == 1:
        #same as before except now we send a 0 to turn off the MOSFET 
        r1 = requests.get('https://api.thingspeak.com/update.json', params={'api_key':THINGSPEAK_API_WRITEKEY, 'field1': '0', 'field2': str(Phone), 'field3': str(MinT), 'field4': str(MaxT)})
        dispon=0       
    return

#function to update the text
def update():
    temp_.config(text=round(temp[len(temp)-1],1))#label reconfigure to update the Temp text every time we recieve a new value
    temp_.place(x=520,y=545)

    #if statement to determine if the MC is turned off
    if not temp[-1] < 600 and not temp[-1] > -600:
        temp_.config(text="No Data")
        temp_.place(x="490",y="545")

    fig.clf()#clear graph to allow other graph to be placed over
    update_g()#call graph function to plot

#function for graphing the temp vector vs the time vector
def update_g():

    #pull the size of the window and set empty vectors for width and height
    wbyh = str(window.geometry())
    width=[]
    height=[]

    check=0 #check to see if we hit x because the size is in the form 000x000 which shows the transition to height
    for x in range(0,len(wbyh)):
        #check if the value in the string is a number and see if we have crossed the x yet
        if wbyh[x].isnumeric() and check==0:
            width.append(int(wbyh[x]))
        #if we have crossed the x then only add numbers to the height vector
        if wbyh[x].isnumeric() and check==1:
            height.append(int(wbyh[x]))
        #ensure that nothing is done when the for loop hits the x and assign check to 1 so we can append to the height
        elif wbyh[x]=="x":
            check=1
        #once we hit the + sign we have recieved all the information we want about the dimensions of the window
        elif wbyh[x]=="+":
            break

    nwidth=""#create an empty string for width to convert out of vector form (side note this could have probably been compressed with the code above)
    #go through the length of the width vector and convert it to a string
    for x in range(0,len(width)):
        nwidth+=str(width[x])
    
    #same as just above but for height
    nheight=""
    for x in range(0,len(height)):
        nheight+=str(height[x])
    


    fig.set_size_inches((int(nwidth)/1000)*8,(int(nheight)/1000)*5.73)#function that has the size of the garph set as a ratio based on the size of the window

    temp_figure = fig.add_subplot(111) #assigning the figure a plot 
    temp_figure.clear()#clear the plot or we get graphs overlapping on each other
    temp_figure.plot(time_s,temp, color= "black") #plotting time vs temp

    #set information about the graph
    temp_figure.set_title("Temperature")
    temp_figure.set_xlabel("Last 300 Seconds")
    temp_figure.set_xbound(lower=-300,upper=0)
    
    #depending on the state of "a" being triggered tells us if temp is being scaled in which case we need to switch the y axis accordingly
    if a==1:
        temp_figure.set_ylabel("Degrees Fahrenheit")
        temp_figure.set_ybound(lower=50,upper=122)
    else:
        temp_figure.set_ylabel("Degrees Celsius")
        temp_figure.set_ybound(lower=10,upper=50)

    #this for loop goes through all of the widgets every second and destroys all but the most recent canvas object in order to not have Canvases overlapping when were trying to resize the plot
    for widget in window.winfo_children():
        if widget.widgetName == "canvas":
            widget.destroy()

    #displaying the plot to the graph object via the the figure object
    graph = FigureCanvasTkAgg(fig,master= window)
    graph.get_tk_widget().place(x=75,y=0)
    graph.draw()
    



#update function that pings the server for the most recent measured value
def update_v():
    r = requests.get("https://api.thingspeak.com/channels/"+ THINGSPEAK_CHANNELID + "/fields/1/last.json", params={"api_key":THINGSPEAK_API_READKEY})#request most recent data point
    j = r.json()#assign data to vector j

    global ID_value#ID variable to determine if we are indeed recieving new information on every request for new information
    global a #the variable that represents if we are in Farenheit or Celsiu, basically if the change temp button has been pressed

    compare_to_utc=str(datetime.datetime.utcnow()) #pull the current UTC time

    #slicing the time so I only have the time exempting the date
    compare_to_utc=compare_to_utc[-15:]
    compare_to_utc=int(compare_to_utc[0:2])*3600+int(compare_to_utc[3:5])*60+int(compare_to_utc[6:8])

    t_reading=str(j["created_at"])#request most recent measurement time

    #slicing the measurement time so I only have hours:minutes:sec
    t_reading=t_reading[-9:]
    t_reading=int(t_reading[0:2])*3600+int(t_reading[3:5])*60+int(t_reading[6:8])
    
    diff_t=compare_to_utc-t_reading #compare utc time and system time to determine the length of time that has elapsed since a new reading

    #compare the length of time to a new measurement and determine if its been longer then 3 seconds, if not execute as long as the most recent value is not NaN
    if diff_t<3 and not np.isnan(float(j["field1"])):
        if ID_value!=int(j["entry_id"]):
            #shift all the values down the vector like popping the 0th place
            for x in range(0,299):
                temp[x]=temp[x+1]

            #then push on the most recent value depending on if we want to know F instead C
            if a==1:
                temp[299]=(float(j["field1"])*1.8)+32
            else:
                temp[299]=float(j["field1"])
        update()#update plot and temp reading
    
    #if the temperatur reading is NaN then I know that the probe has been unplugged because thats what we coded as the response to the backend
    elif np.isnan(float(j["field1"])):

        #for loop that updates all the value locations
        for x in range(0,299):
            temp[x]=temp[x+1]
        temp[299]=NaN #assign NaN to the most recent reading and display probe error for the temp reading       
        temp_.config(text="Probe Er")
        temp_.place(x="480",y="545")
        fig.clf()#clear figure or plots overlap
        update_g()#update plot

    #else statement that catches if the box has been turned off because our difference in UTC and system time are different
    else:
        for x in range(0,299):
            temp[x]=temp[x+1]
        temp[299]=NaN
        update()#update plot and temp reading

    window.after(1000,update_v)#keep us in this loop until the window has been closed

#function for temperature switching
a=0#variable "a" which determines if the button has been pressed is a global variable as to allow other functions to know if were in farenheit or celsius
def swi_temp():
    global a
    #if the button has just been pressed convert the entire temperature vector to farenheit
    if a == 0:
        for x in range(0,300):
            temp[x]=(temp[x]*1.8)+32
        a=1
        update()#update plot and temp reading
    #if the button is pressed again then change the temperature back to Celsius
    else:
        for x in range(0,300):
            temp[x]=(temp[x]-32)/1.8
        a=0
        update()#update plot and temp reading
    return

#get the first 300 values data points from the server and put them into a vector j
r = requests.get("https://api.thingspeak.com/channels/" + THINGSPEAK_CHANNELID + "/fields/1.json", params={"api_key":THINGSPEAK_API_READKEY,"results":"300"})
j = r.json()

enter_v() #call the 2nd window I created but first window that pops up that allows you to enter your upper temp bound and lower temp bound as well as a phone number to text when the system goes outside those bounds

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

#make the time vector a vector from -300 to 0 representing the last 300 seconds
for x in range (1, 300):
    time_s.append(x-299)

#seperate the temperature readings our of the vector j and place themp into the temp vector
for entry in j["feeds"]:
    if entry["field1"] != None:
        temp.append(float(entry["field1"]))
    else:
        temp.append(0)

ID_value = 0 #initialize the ID_value to be reassigned in the graph function

siz=tkFont.Font(size=30) #size of temperature font in the window

fig = fi.Figure(figsize=(6,4.3)) #size of figure that represents the graph

update_g()#update plot

#creating and placing the temperature display value on the button
temp_=tk.Label(window, text = temp[len(temp)-1], font= siz)
temp_.place(x="520",y="545")

window.after(50,update_v)#enter into the update_v function after 1 second

window.mainloop()

