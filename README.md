Fluxgate-Measurement
====================

Fluxgate measurement code

fluxcouch.py

uses pynedm to listen to command documents in nedm/Fluxgate and executes the start/stop command when a document is found. Start and Stop functions also post status documents with runstatus set to 1 and 0 respectively.

control.py

provides functions that post command documents that fluxcouch listens for

ADSpythonmodule

c++ code to provide a python module to read out ADS USB 4716.
Needs installed device driver from advantech homepage
