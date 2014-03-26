#!/usr/bin/python3.2

import vrpn

def callback(userdata, data):
    print(userdata, " => ", data);

tracker=vrpn.receiver.Tracker("GTK@localhost")
tracker.register_change_handler("tracker", callback, "position")
analog=vrpn.receiver.Analog("GTK@localhost")
analog.register_change_handler("analog", callback)
button=vrpn.receiver.Button("GTK@localhost")
button.register_change_handler("button", callback)

while 1:
    tracker.mainloop()
    analog.mainloop()
    button.mainloop()

#del(essai)
