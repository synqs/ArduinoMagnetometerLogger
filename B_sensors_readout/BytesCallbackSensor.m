function BytesCallbackSensor(obj,event, serialW)
%AVAILABLEBYTES This is a callbackfunction when there is a terminator read

%display('callback!');
if serialW.BytesAvailable > 0
%     disp('bytes here');
    data = fscanf(serialW);     % reads out the data until newline or terminator char
%     x = str2double(data);
    fid=fopen('C:\arduino_logs\B_field_sensors.txt','w');
%     fprintf(fid,'%4.3f',x);
    fprintf(fid,data);
    fclose(fid);
    %type text\wavelength.txt
    display('File written');
else
    display('No data');
end
    
