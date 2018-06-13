from app import app, socketio
from app.forms import UpdateForm, DisconnectForm, ConnectForm, SerialWaitForm, ReConnectForm
import serial
import h5py
import git
import numpy as np
from flask import render_template, flash, redirect, url_for, session

import time

from flask_socketio import emit, disconnect
import eventlet

# for subplots
import numpy as np
from datetime import datetime

arduinos = [];
ard_str = '';

class SerialSocketProtocol(object):
    '''
    A class which combines the serial connection and the socket into a single
    class, such that we can handle these things more properly.
    '''

    serial = None
    switch = False
    unit_of_work = 0
    name = '';
    id = 0;
    ard_str = '';

    def __init__(self, socketio):
        """
        assign socketio object to emit
        """
        self.serial = serial.Serial()
        self.socketio = socketio

    def __init__(self, socketio, name):
        """
        assign socketio object to emit
        """
        self.serial = serial.Serial()
        self.socketio = socketio
        self.name = name;

    def is_open(self):
        '''
        test if the serial connection is open
        '''
        return self.serial.is_open

    def stop(self):
        """
        stop the loop and later also the serial port
        """
        self.unit_of_work = 0
        if self.is_open():
            self.serial.close()

    def open_serial(self, port, baud_rate, timeout = 1):
        """
        open the serial port
        """
        self.serial = serial.Serial(port, baud_rate, timeout = 1)

    def start(self):
        """
        start to listen to the serial port of the Arduino
        """
        if not self.switch:
            if not self.is_open():
                print('the serial port should be open right now')
            else:
                self.switch = True
                thread = self.socketio.start_background_task(target=self.do_work)
        else:
            print('Already running')

    def do_work(self):
        """
        do work and emit message
        """
        while self.switch:

            # must call emit from the socketio
            # must specify the namespace

            if self.is_open():
                if self.serial.in_waiting:
                    self.unit_of_work += 1
                    try:
                        timestamp, ard_str = self.pull_data()
                        vals = ard_str.split(',');
                        self.socketio.emit('my_response',
                            {'data': ard_str, 'count': self.unit_of_work})
                    except Exception as e:
                        print('{}'.format(e))
                        self.socketio.emit('my_response',
                            {'data': '{}'.format(e), 'count': self.unit_of_work})
                        self.switch = False
            else:
                self.switch = False
                # TODO: Make this a link
                error_str = 'Port closed. please configure one properly under config.'
                self.socketio.emit('log_response',
                {'data': error_str, 'count': self.unit_of_work})

            eventlet.sleep(0.1)

    def pull_data(self):
        '''
        Pulling the actual data from the arduino.
        '''
        stream = self.serial.read(self.serial.in_waiting);
        self.ard_str = stream.decode(encoding='windows-1252');
        timestamp = datetime.now().replace(microsecond=0).isoformat();
        return timestamp, self.ard_str

    def trig_measurement(self):
        '''
        Triggering a measurement on the Arduino.
        TODO: Would be good to get a confirmation that it is done.
        '''
        # only read out on ask
        o_str = 'w'
        b = o_str.encode()
        self.serial.write(b);

@app.context_processor
def git_url():
    '''
    The main function for rendering the principal site.
    '''
    repo = git.Repo(search_parent_directories=True)
    add =repo.remote().url
    add_c = add.split('.git')[0];
    comm = repo.head.object.hexsha;
    return dict(git_url = add_c + '/tree/' + comm);

@app.route('/')
@app.route('/index', methods=['GET', 'POST'])
def index():
    '''
    The main function for rendering the principal site.
    '''
    global arduinos

    n_ards = len(arduinos);
    props = [];
    for ii, arduino in enumerate(arduinos):
        # create also the name for the readout field of the temperature
        temp_field_str = 'read' + str(arduino.id);
        dict = {'name': arduino.name, 'id': arduino.id, 'port': arduino.serial.port,
        'active': arduino.is_open(), 'label': temp_field_str};
        props.append(dict)

    return render_template('index.html',n_ards = n_ards, props = props);


@app.route('/details/<ard_nr>', methods=['GET', 'POST'])
def details(ard_nr):
    '''
    The main function for rendering the principal site.
    '''
    global arduinos;
    if not arduinos:
        flash('No arduinos installed', 'error')
        return redirect(url_for('index'))

    n_ards = len(arduinos);

    arduino = arduinos[int(ard_nr)];
    n_ards = len(arduinos);
    props = [];
    for ii, arduino in enumerate(arduinos):
        # create also the name for the readout field of the temperature
        temp_field_str = 'read' + str(arduino.id);
        dict = {'name': arduino.name, 'id': arduino.id, 'port': arduino.serial.port,
        'active': arduino.is_open(), 'label': temp_field_str};
        props.append(dict)

    name = arduino.name;
    port = arduino.serial.port;
    conn_open = arduino.is_open()
    return render_template('details.html',n_ards = n_ards, props = props, ard_nr = ard_nr,
        name = name, conn_open = conn_open);

@app.route('/add_arduino', methods=['GET', 'POST'])
def add_arduino():
    '''
    Add an arduino to the set up
    '''
    global arduinos;
    cform = ConnectForm();

    if cform.validate_on_submit():
        n_port =  cform.serial_port.data;
        name = cform.name.data;
        ssProto = SerialSocketProtocol(socketio, name);
        ssProto.id = len(arduinos)
        try:
            ssProto.open_serial(n_port, 115200, timeout = 1);
            ssProto.start();
            if ssProto.is_open():
                app.config['SERIAL_PORT'] = n_port;
                arduinos.append(ssProto)
                flash('We added a new arduino {}'.format(app.config['SERIAL_PORT']))
                return redirect(url_for('index'))
            else:
                 flash('Adding the Arduino went wrong', 'error')
                 return redirect(url_for('add_arduino'))
        except Exception as e:
             flash('{}'.format(e), 'error')
             return redirect(url_for('add_arduino'))

    port = app.config['SERIAL_PORT']
    n_ards = len(arduinos)
    return render_template('add_arduino.html', port = port, cform = cform, n_ards=n_ards);

@app.route('/change_arduino/<ard_nr>')
def change_arduino(ard_nr):
    '''
    Change the parameters of a specific arduino
    '''
    global arduinos;
    if not arduinos:
        flash('No arduinos installed', 'error')
        return redirect(url_for('add_arduino'))

    n_ards = len(arduinos);
    arduino = arduinos[int(ard_nr)];
    props = {'name': arduino.name, 'id': int(ard_nr), 'port': arduino.serial.port,
            'active': arduino.is_open()};

    uform = UpdateForm(id=ard_nr)

    wform = SerialWaitForm(id=ard_nr)
    dform = DisconnectForm(id=ard_nr)
    cform = ReConnectForm(id=ard_nr)

    return render_template('change_arduino.html',
        form=uform, dform = dform, cform = cform, wform = wform, props=props);

@app.route('/update', methods=['POST'])
def update():
    '''
    Update the serial port.
    '''
    global arduinos
    if not arduinos:
        flash('No arduino yet.', 'error')
        return redirect(url_for('add_arduino'))

    sform = UpdateSetpointForm();
    uform = UpdateForm();
    wform = SerialWaitForm()
    dform = DisconnectForm()
    cform = ReConnectForm()
    gform = UpdateGainForm()
    iform = UpdateIntegralForm()
    diff_form = UpdateDifferentialForm()

    id = int(uform.id.data);
    arduino = arduinos[id];

    if uform.validate_on_submit():

        arduino = arduinos[int(id)];
        n_port =  uform.serial_port.data;
        try:
            if arduino.is_open():
                arduino.stop()
            arduino.open_serial(n_port, 115200, timeout = 1)
            if arduino.is_open():
                flash('We updated the serial to {}'.format(n_port))
            else:
                flash('Update of the serial port went wrong.', 'error')
        except Exception as e:
             flash('{}'.format(e), 'error')
        return redirect(url_for('change_arduino', ard_nr = id))
    else:
        props = {'name': arduino.name, 'id': int(ard_nr), 'port': arduino.serial.port,
            'active': arduino.is_open(), 'setpoint': arduino.setpoint,
            'gain': arduino.gain, 'tauI': arduino.integral, 'tauD': arduino.diff};

        return render_template('change_arduino.html', form=uform, dform = dform,
            cform = cform,  sform = sform, gform = gform, iform = iform,
            diff_form = diff_form, wform = wform, props=props);

@app.route('/file/<filestring>')
def file(filestring):
    '''function to save the values of the hdf5 file. It should have the following structure
    <ard_nr>+<filename>
    '''
    # first let us devide into the right parts
    print(filestring)
    parts = filestring.split('+');
    if not len(parts) == 2:
        flash('The filestring should be of the form')
        return redirect(url_for('index'))

    filename = parts[1]
    id = int(parts[0])

    global arduinos;

    if id >= len(arduinos):
        flash('Arduino Index out of range.')
        return redirect(url_for('index'))

    arduino = arduinos[id];
    # We should add the latest value of the database here. Better would be to trigger the readout.
    # Let us see how this actually works.
    vals = arduino.ard_str.split(' ');
    if vals:
        with h5py.File(filename, "a") as f:
            if 'globals' in f.keys():
                params = f['globals']
                params.attrs['x1'] = np.float(vals[0])
                flash('Added the vals {} to the file {}'.format(arduino.ard_str, filename))
            else:
                flash('The file {} did not have the global group yet.'.format(filename), 'error')
    else:
        flash('Did not have any values to save', 'error')

    return render_template('file.html', file = filename, vals = vals)

# communication with the websocket
@socketio.on('connect')
def run_connect():
    '''
    we are connecting the client to the server. This will only work if the
    Arduino already has a serial connection
    '''
    socketio.emit('my_response', {'data': 'Connected', 'count': 0})

@socketio.on('stop')
def run_disconnect():
    print('Should disconnect')

    session['receive_count'] = session.get('receive_count', 0) + 1

    global arduinos;
    # we should even kill the arduino properly.
    if arduinos:
        ssProto = arduinos[0];
        ser = ssProto.serial;
        ser.close();
        ssProto.stop();
        emit('my_response',
            {'data': 'Disconnected!', 'count': session['receive_count']})
    else:
        emit('my_response',
            {'data': 'Nothing to disconnect', 'count': session['receive_count']})

@socketio.on('my_ping')
def ping_pong():
    emit('my_pong')

@socketio.on('trig_mag')
def trig_mag():
    global arduinos;
    if arduinos:
        arduino = arduinos[0];
        arduino.trig_measurement();
        ard_str = 'Triggered a measurement.';
    else:
        ard_str = 'Nothing to connect to';

    session['receive_count'] = session.get('receive_count', 0) + 1;
    emit('my_response',
        {'data': ard_str, 'count': session['receive_count']})

# error handling
@app.errorhandler(500)
def internal_error(error):
    flash('An error occured {}'.format(error), 'error')
    return render_template('500.html'), 500
