#include <Wire.h>
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include "Ubidots.h"

const char* UBIDOTS_TOKEN = "BBFF-jgltwaXnOkiFWr1HAuifahSHhr0olF";  // Put here your Ubidots TOKEN
const char* ssid     = "username";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "password";     // The password of the Wi-Fi network
Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);

// MPU6050 Slave Device Address
const uint8_t MPU6050SlaveAddress = 0x68;

// Select SDA and SCL pins for I2C communication 
const uint8_t scl = D6;
const uint8_t sda = D7;

// sensitivity scale factor respective to full scale setting provided in datasheet 
const uint16_t AccelScaleFactor = 16384;
const uint16_t GyroScaleFactor = 131;

// MPU6050 few configuration register addresses
const uint8_t MPU6050_REGISTER_SMPLRT_DIV   =  0x19;
const uint8_t MPU6050_REGISTER_USER_CTRL    =  0x6A;
const uint8_t MPU6050_REGISTER_PWR_MGMT_1   =  0x6B;
const uint8_t MPU6050_REGISTER_PWR_MGMT_2   =  0x6C;
const uint8_t MPU6050_REGISTER_CONFIG       =  0x1A;
const uint8_t MPU6050_REGISTER_GYRO_CONFIG  =  0x1B;
const uint8_t MPU6050_REGISTER_ACCEL_CONFIG =  0x1C;
const uint8_t MPU6050_REGISTER_FIFO_EN      =  0x23;
const uint8_t MPU6050_REGISTER_INT_ENABLE   =  0x38;
const uint8_t MPU6050_REGISTER_ACCEL_XOUT_H =  0x3B;
const uint8_t MPU6050_REGISTER_SIGNAL_PATH_RESET  = 0x68;

int16_t AccelX, AccelY, AccelZ, Temperature, GyroX, GyroY, GyroZ;
int emergency=0, top =0, bottom =0, right=0, left=0, check_right=0, check_left=0,timer=0;
char contextProg[50];
char test[10] = "2591";        //patient id

void setup() {
  Serial.begin(115200);
  delay(10);
  ubidots.wifiConnect(ssid, password);
  Serial.println('\n');
  Serial.println("Connecting to ");
  Serial.println(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.println(++i); Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  Wire.begin(sda, scl);
  MPU6050_Init();
}

void loop() {
  double Ax, Ay, Az, T, Gx, Gy, Gz;
  timer++;
  Serial.print("Timer is ");
  Serial.println(timer);
  Read_RawValue(MPU6050SlaveAddress, MPU6050_REGISTER_ACCEL_XOUT_H);
  
  //divide each with their sensitivity scale factor
  Ax = (double)AccelX/AccelScaleFactor;
  Ay = (double)AccelY/AccelScaleFactor;
  Az = (double)AccelZ/AccelScaleFactor;
  T = (double)Temperature/340+36.53;        //temperature formula
  Gx = (double)GyroX/GyroScaleFactor;
  Gy = (double)GyroY/GyroScaleFactor;
  Gz = (double)GyroZ/GyroScaleFactor;

  if (T>= 57)
  {
    Serial.println ("FIRE DETECTED!! Temperature too high!");
    sprintf(contextProg, "\"FIRE DETECTED!!\":%s",test);
    ubidots.add("Message",999, contextProg);
    ubidots.send("ROOM1");
  }
  if ( ( Ax <= -0.02 && Ax >= -0.10) && ( Ay >= -0.04 && Ay <= +0.04) && ( Az >= +1.04 && Az <= +1.11))
  {
    Serial.println("MPU is Stable");    // MPU on ground or flat surface
    emergency ++;
    Serial.println(emergency);
    top =0; bottom =0; right=0; left=0;
  }
  else if ( ( Ax <= 0.15 && Ax >= -0.15) && ( Ay >= +0.39 && Ay <= +0.55) && ( Az >= +0.90 && Az <= +1.00))
  {
    Serial.print("Call Nurse/Attendant");     // Fingers side in air by 25deg approx.
    top ++;
    Serial.println(top);
  }
  else if ( ( Ax <= 0.15 && Ax >= -0.15) && ( Ay <= -0.39 && Ay >= -0.55) && ( Az >= +0.90 && Az <= +1.00))
  {
    Serial.print("I am Hungry");     // Fingers side in air by 25deg approx.
    bottom ++;
    Serial.println(bottom);
  }
  else if ( ( Ax >= +0.35 && Ax <= +0.60) && ( Ay >= -0.10 && Ay <= +0.10) && ( Az >= +0.90 && Az <= +1.05))
  {
    Serial.print("There is an Emergency");     // Fingers side in air by 25deg approx.
    right ++;
    Serial.println(right);
    while (right/2>=6)
    {
      right = 0;
      Serial.println("-----");
      check_right ++;
      if (check_right >= 5 && timer/2 <= 120)
      {
        Serial.println("emergency,irregular movement detected!!!");
        timer = 0;
        check_right=0;
        sprintf(contextProg, "\"Emergency, irregular movement!\":%s",test);
        ubidots.add("Message",6, contextProg);
        ubidots.send("ROOM1");
      }
    }
  }
  else if ( ( Ax <= -0.35 && Ax >= -0.60) && ( Ay >= -0.10 && Ay <= +0.10) && ( Az >= +0.90 && Az <= +1.05))
  {
    Serial.print("Medicines are finished");     // Fingers side in air by 25deg approx.
    left ++;
    Serial.println(left);
    while (left/2>=6)
    {
      left = 0;
      Serial.println("-----");
      check_left ++;
      if (check_left >= 5 && timer/2 <= 120)
      {
        Serial.println("emergency,irregular movement detected!!!");
        timer = 0;
        check_left=0;
        sprintf(contextProg, "\"Emergency, irregular movement!\":%s",test);
        ubidots.add("Message",6, contextProg);
        ubidots.send("ROOM1");
      }
    }
  }
  else
  {
    Serial.print(" Ax: "); Serial.print(Ax); Serial.print("\t");
    Serial.print(" Ay: "); Serial.print(Ay);Serial.print("\t");
    Serial.print(" Az: "); Serial.print(Az);Serial.print("\t");
    Serial.print(" T: "); Serial.print(T);Serial.print("\t");
    Serial.print(" Gx: "); Serial.print(Gx);Serial.print("\t");
    Serial.print(" Gy: "); Serial.print(Gy);Serial.print("\t");
    Serial.print(" Gz: "); Serial.println(Gz);Serial.print("\t");
    top =0; bottom =0; right=0; left=0, emergency=0;
  }
  if(timer/2>=120)
  {
    timer=0;
    check_left=0;
    check_right=0;
  }
  CheckCount(top,bottom,left,right);  
  delay(500);
}

void I2C_Write(uint8_t deviceAddress, uint8_t regAddress, uint8_t data){
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.write(data);
  Wire.endTransmission();
}

// read all 14 register
void Read_RawValue(uint8_t deviceAddress, uint8_t regAddress){
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, (uint8_t)14);
  AccelX = (((int16_t)Wire.read()<<8) | Wire.read());
  AccelY = (((int16_t)Wire.read()<<8) | Wire.read());
  AccelZ = (((int16_t)Wire.read()<<8) | Wire.read());
  Temperature = (((int16_t)Wire.read()<<8) | Wire.read());
  GyroX = (((int16_t)Wire.read()<<8) | Wire.read());
  GyroY = (((int16_t)Wire.read()<<8) | Wire.read());
  GyroZ = (((int16_t)Wire.read()<<8) | Wire.read());
}

//configure MPU6050
void MPU6050_Init(){
  delay(150);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_SMPLRT_DIV, 0x07);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_PWR_MGMT_1, 0x01);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_PWR_MGMT_2, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_CONFIG, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_GYRO_CONFIG, 0x00);   //set +/-250 degree/second full scale
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_ACCEL_CONFIG, 0x00);    // set +/- 2g full scale
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_FIFO_EN, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_INT_ENABLE, 0x01);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_SIGNAL_PATH_RESET, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_USER_CTRL, 0x00);
}

void CheckCount(int top,int bottom,int left,int right)
{
  if (top/2 >= 5)
  {
    Serial.println("Nurse/Attendant"); 
    sprintf(contextProg, "\"NURSE/Attendant\":%s",test);
    ubidots.add("Message",1, contextProg);
    bool bufferSent = false;
    ubidots.send("ROOM1");
    timer+=8;
  }
  else if (bottom/2 >= 5)
  {
    Serial.println("Food");
    sprintf(contextProg, "\"Food\":%s",test);
    ubidots.add("Message",2, contextProg);
    ubidots.send("ROOM1");
    timer+=8;
  }
  else if (right/2 >= 5)
  {
    Serial.println("EMERGENCY!!!");
    sprintf(contextProg, "\"Emergency!!!\":%s",test);
    ubidots.add("Message",3, contextProg);
    ubidots.send("ROOM1");
    right+=8;
    timer+=8;
  }
  else if (left/2 >= 5)
  {
    Serial.println("Medicines");
    sprintf(contextProg, "\"Medicines\":%s",test);
    ubidots.add("Message",4, contextProg);
    ubidots.send("ROOM1");
    left+=8;
    timer+=8;
  }
  else if (emergency/2 >= 120)
  {
    Serial.println("emergency, hand is stable for 2 minutes");
    sprintf(contextProg, "\"Emergency, hand stable!\":%s",test);
    ubidots.add("Message",5, contextProg);
    ubidots.send("ROOM1");
    timer+=8;
    while (emergency/2 >= 130)
    {
      emergency = 0;
    }
  }
}
