%%
% This script should recieve the current wavelength sent by an arduino via
% serial communication line. It should save this data then to a text file
% on a specific path.

% Initalize the Communication and open the port to the Arduino
h = instrfind('Status','open','Port', 'COM8');
if isempty(h)
    serialW = serial('COM8');
    set(serialW, 'BaudRate', 115200);
    set(serialW, 'InputBufferSize', 201024);
    fopen(serialW);
    display('COM8 opened');
else
    serialW = h(1);
    display('COM8 already opened');
end
% define a callback function that is executed once a terminator is read,
% like \n in this case
serialW.BytesAvailableFcnMode = 'terminator';
serialW.BytesAvailableFcn = {@BytesCallbackSensor, serialW};



%%
% closes the ports etc of connection
% execute after use of program! 
h = instrfind('Status','closed','Port', 'COM8');
display('closed Ports deleted:')
display(length(h));
delete(h);
h = instrfind('Status','open','Port', 'COM8');
if ~isempty(h)
    display('open Ports deleted:')
    display(length(h));
    fclose(h);
end;
delete(h);


    






