# ArduinoMagnetometerLogger

A flask server that might make it easier to track the 6 little magnetic field sensors
 within our experiment. The website assumes that the arduinos is connected via a serial device. For the moment we have to following abilities:

- It shows data in a long list for the moment.
- It is possible to save the values to an hdf5 file.

On the technical side we use the following ingredients:
- Communication with the Arduino is done through the Serial interface. We will look into the ethernet interface at some point.
- Updates on the client are done through flask_socketio.
- The layout is made nice through flask_bootstrap.

Further, we will most likely not install saving of the data on the server as this would make the whole thing MUCH more complicated (where and who to store the data. Which data should we show etc.)

# Installation

- create a new directory
- clone the repository through 'git clone ...' in the new folder
- create a new virtualenv through 'conda create -n YOURNAME python=3.6'
- activate the virtualenv through 'source activate YOURNAME'
- install the dependencies through 'pip install -r requirements.txt'
- start it through 'start.sh'
- open it in a browser on 'localhost:5000' or using the name of the appropiate host.

# Usage

## The server itself
 - activate the virtualenv through 'source activate YOURNAME'
 - start it through 'start.sh'
 - runs on 'localhost:5000' or using the name of the appropiate host.

## Test without Arduino

 If you want to test the serial port without having an Arduino, you should just
 start another terminal, and run the _simSerialPort.py_ file through 'python simSerialPort.py'

## Saving to hdf5

We can save the last data to an hdf5 file by calling the _'file/0+FNAME.h5'_. It  is assumed that the file is already created. Most likely from some program from the  [labscriptsuite](www.labscript.org) or our _NaLi_ control system. An example for calling it from matlab is found in _matlabPythonComm.m_ .

# TODO

 [] Plot up the different values.

 [] Allow for csv export.

 [] Tidy up the connections and also the code in the back-end.

 [] Always make it look cuter.

 [] Error logger to communicate with slack or via email.

 [] make this readme the about page.
