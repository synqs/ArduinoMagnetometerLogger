/* FILE:    selfwritten_magnetometer
   DATE:    24.11.2015
   Based on Marcell, extended by Arno.

  GY-273 module with Honeywell HMC5883L 3-Axis Digital Compass IC

  Connections to arduino
  VCC       3.3V
  GND       GND
  SCL       A5
  SDA       A4
  DRDY      N/A
*/

// math library for rounding
#include <math.h>

// include the I2C library
#include <Wire.h>

// include the library for progmem (store in flash instead of ram
//#include <avr/pgmspace.h>
//_____________________________________________________________________________________________________________

// -----Global variables-----


//--------------------------------------------------------------------------------------------------
// ------------------------------ fast adjustment HMC5883L------------------------------------------
// Gain setting choosen
byte sample = 3;     // Sampled average: 1 ; 2 ; 4 ; 8 (sample = 3 => 8 values averaged)
byte rate = 6;       // rates in Hertz: 0.75 ; 1.5 ; 3 ; 7.5 ; 15 ; 30 ; 75; (rate = 3 => rate = 7.5Hz)
byte pos_bias = 0;   // Settings for positive bias coils (1 for on)
byte neg_bias = 0;   // Settings for negative bias coils (1 for on)
byte single = 0;     // single meassurement if set to 1
byte gain = 3;       // global gain setting => can be overwritten in the configuration function, multiplicative factor is in array defined below // was 7
float gain_array[] = {0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35}; // amplification due to gain setting: 100 digital units correspond to the gain in milliGauss
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------

// Possibility to use a digital averaging over the values to get a lower statistical variation
int N_averaging = 50; // 50
int wait = 0;       // Wait time between every sample, set to 0 if not wa nted

// self calbiration on or off
boolean selfcal = false; // currently NOT WORKING

// We declare our variables for the different values of the magnetic field
float x = 0;
float y = 0;
float z = 0;
float total;

// declaring the sensitivity adjustments for each sensor (default = 100)
float x_m = 1.00;
float y_m = 1.00;
float z_m = 1.00;

// declaring the offsets of each axis (default = 0)
float x_o = 00;//00; Gain 3  //-25; Gain 2
float y_o = 0 * 90; //90;         // 90; Gain 2
float z_o = 0 * 70; //70;         //80; Gain 2

//helping variable to use for getting the bytes
float x_uc = 0;
float y_uc = 0;
float z_uc = 0;

float x_[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
float y_[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
float z_[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
//float x_1, y_1, z_1, x_2, y_2, z_2, x_3, y_3, z_3, x_4, y_4, z_4, x_5, y_5, z_5, x_6, y_6, z_6;

float sum_x = 0;
float sum_y = 0;
float sum_z = 0;

int x_show;
int y_show;
int z_show;

byte x1, x2, y1, y2, z1, z2;

// value to be transmitted
int x_unmod;

// check the bnc input
boolean digital_input;

// input charakter, defines communication with the arduino
char input;

// array length for transfer to PC is defined here
const int ArraySize = 750;  //2625 -> 35s
int DataArray[ArraySize];

//_____________________________________________________________________________________________________________

bool trigstat, oldtrigstat, take_sample = 0;

// channels for setting an un-setting the relais
//int phi[9] = {0, 3, 5, 14, 16, 13, 11, 8}; //, 7}; // we start indexing at 1, so array has 9 entries
int phi[9] = {0, 3, 5, 14, 16, 7, 11, 8}; //, 7}; // we start indexing at 1, so array has 9 entries
int plo[9] = {0, 2, 4, 15, 17, 12, 10, 9}; //, 6}; // first one is dummy

// function for setting and unsetting:
void set(int i) {
  digitalWrite(phi[i], 1); delay(20); digitalWrite(phi[i], 0);
}
void unset(int i) {
  digitalWrite(plo[i], 1); delay(20); digitalWrite(plo[i], 0);
}

void setup()
{
  // Start Communication with PC and MatLab
  Serial.begin(115200);
  Serial.println("Hello, I'm the Arduino prog to read out the HML5883L sensors.");

  for (int i = 2; i < 18; i++) {
    pinMode(i, OUTPUT); // set 2 to 14 (= A5) as output
  }
  pinMode(6, INPUT); // trigger!

  for (int i = 1; i < 8; i++) {
    set(i);
  }
  //start the I2C bus service
  Wire.begin();
  // set the initial values for the configuration of the chip
  adjust_mag();
  delay(20);
  for (int i = 1; i < 8; i++) {
    unset(i);
  }

  // one readout to check everything:
  for (int sens = 1; sens < 7; sens++) {
    set(sens);
    //initalize the internal variables for digital averaging, first sum up, then devide through the no of values
    sum_x = 0;
    sum_y = 0;
    sum_z = 0;

    // digital sampling loop, reads out a number of values, then averages over them to get a lower statistical uncertainty (we assume it is better than the internal averaging)
    for (int i = 0; i < N_averaging; i++) {
      data_ready();
      get_data();     // requests set of data, including scaling with the scaling constants
      sum_x = sum_x + x;
      sum_y = sum_y + y;
      sum_z = sum_z + z;
      delay(wait);      // delay the cycle to get let the data be more readable
    }

    // set the x y z values on the average
    x_[sens] = sum_x / N_averaging;
    y_[sens] = sum_y / N_averaging;
    z_[sens] = sum_z / N_averaging;
    unset(sens);
  }
  for (int sens = 1; sens < 7; sens++) {
    //      Serial.print("s#"); Serial.print(sens); Serial.print(" ");
    Serial.print(x_[sens]); Serial.print(" ");
    Serial.print(y_[sens]); Serial.print(" ");
    Serial.print(z_[sens]); Serial.print(" ");
    //    Serial.print(sqrt(x_1 * x_1 + y_1 * y_1 + z_1 * z_1));
  }
  Serial.println("");

}


//_____________________________________________________________________________________________________________



// Main Programm
void loop()
{
  oldtrigstat = trigstat;
  if (digitalRead(6)) {
    trigstat = 1; // long pulse => no trig!
  } else {
    trigstat = 0; // short pulse => yes trig!
  }
  if (oldtrigstat != trigstat) { // it's a change!
    if (trigstat == false) {     // it's a falling slope, since the S&H box needs this.
      take_sample = true;
    };
  }

  if (take_sample == true) {
    for (int sens = 1; sens < 7; sens++) {
      set(sens);
      //initalize the internal variables for digital averaging, first sum up, then devide through the no of values
      sum_x = 0;
      sum_y = 0;
      sum_z = 0;

      // digital sampling loop, reads out a number of values, then averages over them to get a lower statistical uncertainty (we assume it is better than the internal averaging)
      for (int i = 0; i < N_averaging; i++) {
        data_ready();
        get_data();     // requests set of data, including scaling with the scaling constants
        sum_x = sum_x + x;
        sum_y = sum_y + y;
        sum_z = sum_z + z;
        delay(wait);      // delay the cycle to get let the data be more readable
      }

      // set the x y z values on the average
      x_[sens] = sum_x / N_averaging;
      y_[sens] = sum_y / N_averaging;
      z_[sens] = sum_z / N_averaging;
      unset(sens);
    }
    for (int sens = 1; sens < 7; sens++) {
      //      Serial.print("s#"); Serial.print(sens); Serial.print(" ");
      Serial.print(x_[sens]); Serial.print(" ");
      Serial.print(y_[sens]); Serial.print(" ");
      Serial.print(z_[sens]); Serial.print(" ");
      //    Serial.print(sqrt(x_1 * x_1 + y_1 * y_1 + z_1 * z_1));
    }
    Serial.println("");
    delay(5000);  // prevent from new take_samplement while seq is still running
    take_sample = false;
  }
}



//_____________________________________________________________________________________________________________

// This function writes a byte into the 3 configuration registers
// Register A: sample rate, averages, bias fields on (positive/negative)
// Register B: gain
// Register M(ode): single or continuous meassurement
void adjust_mag() {
  byte RegA = (sample << 5) | (rate << 2) | (neg_bias << 1) | pos_bias;
  byte RegB = gain << 5;
  byte RegM = single;

  // set the configuration register A
  Wire.beginTransmission(0x1E);  // the adress is 0x1E for the magnetometer
  Wire.write(B00000000);         // set to write first register-> configuration register A
  Wire.write(RegA);
  Wire.endTransmission();

  // set the configuration register B
  Wire.beginTransmission(0x1E);  // the adress is 0x1E for the magnetometer
  Wire.write(B00000001);         // set to write second register-> configuration register B
  Wire.write(RegB);
  Wire.endTransmission();

  // set the mode register
  Wire.beginTransmission(0x1E); // the adress is 0x1E for the magnetometer
  Wire.write(B00000010);        // set to write second register-> configuration register A
  Wire.write(RegM);             // 1.bit HighSpeed I2C 0-> off 7.-8.bit operating mode 00 -> set to continous meassurement mode
  Wire.endTransmission();
}


//_____________________________________________________________________________________________________________

// This function calls the data registers and reads them out. First it has to set the device pointer to the right address (data X A => position 3) and then requests 6 bytes (1.X 2.Z 3.Y)
// All values are then written on the global variables x,y,z and total
void get_data() {

  // wait until data is ready (wait in ms -> optional, default = 0)
  data_ready();

  // get the data -> go to the first data register
  Wire.beginTransmission(0x1E);
  Wire.write(B00000011);       // register 3 -> X   5 -> Z     7 -> Y
  Wire.endTransmission();

  // Request 6 byte from the address of the magnetometer. After every read process, the register is shifted by one -> one time read and get all of it
  Wire.requestFrom(0x1E, 6);
  x1 = Wire.read();
  x2 = Wire.read();
  z1 = Wire.read();
  z2 = Wire.read();
  y1 = Wire.read();
  y2 = Wire.read();

  // uc stands for uncorrected -> different scaling
  x_uc = (x1 << 8) | x2;
  z_uc = (z1 << 8) | z2;
  y_uc = (y1 << 8) | y2;

  // scaling with two factors: the total scaling due to different gain settings tot_m, and the different scaling for each axis x_m and adjusts for an offset x_o

  x = (x_uc * gain_array[gain]) + x_o;
  y = (y_uc * gain_array[gain]) + y_o;
  z = (z_uc * gain_array[gain]) + z_o;

  return;
}

//_____________________________________________________________________________________________________________

//function to check if new data available with the a defined loop wait time
void data_ready() {
  boolean Nready = true;  // this is set to false when data is ready
  while (Nready)
  {
    // check if there is new data -> go to status register
    Wire.beginTransmission(0x1E);
    Wire.write(B00001001);       // register 9
    Wire.endTransmission();

    // request 1 byte from the magnometer device
    Wire.requestFrom(0x1E, 1);
    byte status = Wire.read();
    if (status & (1 << 0))
    {
      Nready = false;
    }
  }
  return;
}
