#! /usr/bin/env python

import gtk
import dbus

# Connect to the bus
bus = dbus.Bus()

# Create a service on the bus
service = dbus.Service("com.burtonini.dbus.SignalService", bus)

# Define a D-BUS object
class SignalObject(dbus.Object):
    def __init__(self, service):
        dbus.Object.__init__(self, "/", [], service)

# Create an instance of the object, which is part of the service
signal_object = SignalObject(service)

def send_ping():
    signal_object.broadcast_signal("com.burtonini.dbus.Signal", "Ping")
    print "Ping!"
    return gtk.TRUE

# Call send_ping every second to send the signal
gtk.timeout_add(1000, send_ping)
gtk.main()
