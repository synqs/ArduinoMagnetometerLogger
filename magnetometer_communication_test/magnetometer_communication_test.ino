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

char mode;

void setup()
{
  // Start Communication with PC and MatLab
  Serial.begin(115200);
}

//_____________________________________________________________________________________________________________

// Main Programm
void loop()
{
  for (int sens = 1; sens < 7; sens++) {
          x_[sens] = random(300);y_[sens] = random(300); z_[sens] = random(300);
  }
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    // say what you got:
    mode = Serial.read();
    if (mode == 'w') {
        for (int sens = 1; sens < 7; sens++) {
          Serial.print(x_[sens]); Serial.print(" ");
          Serial.print(y_[sens]); Serial.print(" ");
          Serial.print(z_[sens]); Serial.print(" ");
        }
        Serial.println("");
    }
  }
  delay(500);
}
