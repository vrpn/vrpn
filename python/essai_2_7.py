#!/usr/bin/python2.7

import vrpn

def callback(userdata, data):
    print(type(userdata), userdata, " => ", data);
    print(toto)

tracker=vrpn.receiver.Tracker("Tracker0@localhost")
tracker.register_change_handler("position", callback, "position")

analog=vrpn.receiver.Analog("Input0@localhost")
analog.register_change_handler("dev input", callback)

button=vrpn.receiver.Button("Touchpad@localhost")
button.register_change_handler("dev input", callback)

while 1:
    tracker.mainloop()
    analog.mainloop()
    button.mainloop()

#del(essai)
