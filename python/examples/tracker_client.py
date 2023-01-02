# NOTE: a vrpn tracker must call user callbacks with tracker data (pos and
#       ori info) which represent the transformation xfSourceFromSensor.
#       This means that the pos info is the position of the origin of
#       the sensor coord sys in the source coord sys space, and the
#       quat represents the orientation of the sensor relative to the
#       source space (ie, its value rotates the source's axes so that
#       they coincide with the sensor's)

import argparse
import vrpn

#############################################################################
# Callback handler

def    handle_tracker_position(userdata, t):
  '''This function gets called when the tracker's POSITION xform is updated.
  userdata is whatever you passed into the register_change_handler function.
  VRPN passes it through to the callback handler. It's not used by vrpn internally.
  '''

  print("handle_tracker_position\tSensor {:d} is at ({:.3f},{:.3f},{:.3f}) at time {}".format(
     t['sensor'],
     t['position'][0], t['position'][1], t['position'][2],
     t['time'])
  )
  print('  Callback userdata =', userdata)


def    handle_tracker_velocity(userdata, t):
  '''This function gets called when the tracker's POSITION xform is updated.
  userdata is whatever you passed into the register_change_handler function.
  VRPN passes it through to the callback handler. It's not used by vrpn internally.
  '''

  print("handle_tracker_velocity\tSensor {:d} speed ({:.3f},{:.3f},{:.3f}) at time {}".format(
     t['sensor'],
     t['velocity'][0], t['velocity'][1], t['velocity'][2],
     t['time'])
  )
  print('  Callback userdata =', userdata)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(
                    prog = 'tracker_client',
                    description = 'Connects to a tracker and prints positionr reports',
                    epilog = 'Text at the bottom of help')
  parser.add_argument('device', help='Name of the device (like Tracker0@localhost)')
  args = parser.parse_args()

  done = False

  # Open the tracker and set its position callback handler
  # (other choices are velocity, acceleration, unit2ssensor, workspace,
  # tracker2room).  An optional fourth parameter selects a particular sensor
  # from the tracker to watch.
  tkr = vrpn.receiver.Tracker(args.device);
  tkr.register_change_handler(None, handle_tracker_position, "position");
  tkr.register_change_handler("VelHandlerData", handle_tracker_velocity, "velocity");

  while not done:
    # Let the traker do it's thing.
    # It will call the callback function(s) registered above when new
    # messages are received.
    tkr.mainloop()

