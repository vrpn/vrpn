#!/usr/bin/python3.2

import vrpn

def callback(userdata, data):
    print(userdata, " => ", data);

tracker=vrpn.receiver.Tracker("Tracker0@localhost")
tracker.register_change_handler("position", callback, "position")

analog=vrpn.receiver.Analog("Touchpad@localhost")
analog.register_change_handler("dev input", callback)

button=vrpn.receiver.Button("Touchpad@localhost")
button.register_change_handler("dev input", callback)

while 1:
    tracker.mainloop()
    analog.mainloop()
    button.mainloop()

#del(essai)
