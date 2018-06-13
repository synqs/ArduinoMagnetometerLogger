import os, pty
import time
import numpy as np

def test_serial():
    master, slave = pty.openpty()
    s_name = os.ttyname(slave)
    Nsensors = 6;
    x = np.zeros(Nsensors);
    y = np.zeros(Nsensors);
    z = np.zeros(Nsensors);
    print(s_name)
    while True:
        for ii in np.arange(Nsensors):
            x[ii] = np.random.randint(1024);
            y[ii] = np.random.randint(1024);
            z[ii] = np.random.randint(1024);
        mode = os.read(master, 1);
        if mode:
            print('mode {}'.format(mode))
            if mode == b'w':
                '''
                write the data to the buffer on trigger
                '''
                ard_str = '';
                for ii in np.arange(Nsensors):
                    ard_str = ard_str + str(x[ii]) + ' ';
                    ard_str = ard_str +  str(y[ii]) + ' ';
                    ard_str = ard_str + str(z[ii]) + ' ' + '\r\n';
                out = ard_str.encode('windows-1252')
                os.write(master, out)
        time.sleep(0.1)
if __name__=='__main__':
    test_serial()
