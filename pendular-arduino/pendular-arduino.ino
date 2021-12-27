
#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

const int PIXELS_PIN_1 = 2;
const int PIXELS_PIN_2 = 3;

const int NUM_PIXELS_1 = 5;
const int NUM_PIXELS_2 = 6;

const int PHOTO_RESISTOR_PIN_1 = A7;
const int PHOTO_RESISTOR_PIN_2 = A6;

const int NUM_IMUS = 2;
const int NUM_PHOTO_RESISTORS = 2;

float elapsedTime, currentTime, previousTime;

MPU6050 mpu1(Wire);
MPU6050 mpu2(Wire);

Adafruit_NeoPixel pixels1(NUM_PIXELS_1, PIXELS_PIN_1, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel pixels2(NUM_PIXELS_2, PIXELS_PIN_2, NEO_GRBW + NEO_KHZ800);


unsigned long timer = 0;

#define TCAADDR 0x70

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use by strtok() function

// variables to hold the parsed data
char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;

boolean newData = false;

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.print("This just in ... ");
    Serial.println(receivedChars);
    newData = false;
  }
}

//----------------------------------------------------------

void tcaselect(uint8_t i)
{
  if (i > 7)
    return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}


void initMPU1() {
  Serial.println("Init mpu 1");
  byte status = mpu1.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  if (status == 0) {
    Serial.println(F("Calculating offsets, do not move MPU6050"));
    delay(1000);
    mpu1.calcOffsets(); // gyro and accelero
    Serial.println("Done!\n");
  } else {
    Serial.println("Error connecting");
  }
}

void initMPU2() {
  Serial.println("Init mpu 2");
  byte status = mpu2.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  if (status == 0) {
    Serial.println(F("Calculating offsets, do not move MPU6050"));
    delay(1000);
    mpu2.calcOffsets(); // gyro and accelero
    Serial.println("Done!\n");
  } else {
    Serial.println("Error connecting");
  }
}

//=============================================================================


float xMult = 0.0;
float yMult = 0.0;

int v1 = 0;
int v2 = 0;


void parseData() {
  // split the data into its parts
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(tempChars, ",");     // get the first part - the string
  v1 = atoi(strtokIndx);     // convert this part to an integer

  strtokIndx = strtok(NULL, ",");
  v2 = atof(strtokIndx);     // convert this part to a float
}

void showParsedData() {
  Serial.print("v1 ");
  Serial.print(v1);
  Serial.print(" v2 ");
  Serial.println(v2);
}


void setup()
{
  Serial.begin(9600);

  while (!Serial);


  parseData();

  pinMode(PHOTO_RESISTOR_PIN_1, INPUT);
  pinMode(PHOTO_RESISTOR_PIN_2, INPUT);

  Serial.println("Hello v3");

  Wire.begin();
  // TODO make function
  initMPU1();
  initMPU2();

  pixels1.begin();
  pixels2.begin();

  pixels1.clear();
  pixels2.clear();

//  for (int i = 0; i < NUM_PIXELS_1; i++) {
//    pixels1.setPixelColor(i, pixels1.Color(255, 255, 255, 255));
//    pixels1.show();
//  }
//
//
//  for (int i = 0; i < NUM_PIXELS_2; i++) {
//    pixels2.setPixelColor(i, pixels1.Color(255, 0, 5, 255));
//    pixels2.show();
//  }
}

void printMPUData(int i) {
  MPU6050 mpu = mpu1;
  if (i == 2) {
    mpu = mpu2;
  }
  tcaselect(i);
  float x = mpu.getAngleX();
  float y = mpu.getAngleY();
  float z = mpu.getAngleZ();

  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  float totalAcc = sqrt(pow(ax, 2) + pow(ay, 2) + pow(az, 2));
  if (i == 1) {
    xMult = x + 90.0;
    yMult = y + 90.0;

  }

  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.print(z);
  Serial.print(" ");
  Serial.print(totalAcc - 1.0);
}

int arrivingdatabyte = 0;

void loop()
{

  recvWithStartEndMarkers();
  if (newData == true) {
    strcpy(tempChars, receivedChars);
    // this temporary copy is necessary to protect the original data
    //   because strtok() replaces the commas with \0
    parseData();
    //    showParsedData();
    newData = false;

    pixels1.setBrightness(v1);
    pixels2.setBrightness(v2);
  }

  //  Serial.print(v1);
  //  Serial.print(" ");
  //
  //  Serial.println(v2);

  //  return;
  tcaselect(1);
  mpu1.update();

  tcaselect(2);
  mpu2.update();

  int light1 = analogRead(PHOTO_RESISTOR_PIN_1);
  int light2 = analogRead(PHOTO_RESISTOR_PIN_2);

  // print data every 10ms
  if ((millis() - timer) > 10)
  {

    printMPUData(1);
    Serial.print(" ");
    Serial.print(light2);
    Serial.print(" ");
    printMPUData(2);
    Serial.print(" ");
    Serial.print(light1);
    Serial.println();
    timer = millis();
  }

  float brightness1 = light1 / 4.0;
  float brightness2 = light2 / 4.0;

//  pixels1.clear();

  int lightOn = 0;
  if (arrivingdatabyte > 10) {
    lightOn = 1;
  }

  //  float v1Percent = v1 / 127.0;
  //  float v2Percent = v2 / 127.0;

  // CIRCLE
  for (int i = 0; i < NUM_PIXELS_1; i++) {
    pixels1.setPixelColor(i, pixels1.Color(2, 15, 250, brightness1));
    pixels1.show();
  }

  // CUBE
  for (int i = 0; i < NUM_PIXELS_2; i++) {
    if (i < 3) {
      pixels2.setPixelColor(i, pixels2.Color(250, 50, 5, brightness2));

    } else {
      pixels2.setPixelColor(i, pixels2.Color(250, 5, 5, brightness2));

    }
    pixels2.show();
  }
  


}
