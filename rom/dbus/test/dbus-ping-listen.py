#! /usr/bin/env python

import gtk
import dbus

bus = dbus.Bus()

def signal_callback(interface, signal_name, service, path, message):
    print "Received signal %s from %s" % (signal_name, interface)

# Catch signals from a specific interface and object, and call signal_callback
# when they arrive.
bus.add_signal_receiver(signal_callback,
                        "com.burtonini.dbus.Signal", # Interface
                        None, # Any service
                        "/" # Path of sending object
                        )

# Enter the event loop, waiting for signals
gtk.main()
