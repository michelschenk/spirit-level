#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  #include "Wire.h"
#endif

const char *SSID = "";
const char *PWD = "";

#define I2C_SDA 15
#define I2C_SCL 2
#define INTERRUPT_PIN 13

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0"/><style>canvas { border:1px solid #d3d3d3; background-color: #edfb27;}</style><script src="script.js"></script></head><body onload="startGame()"><p></p></body></html>
)rawliteral";

MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false; // set true if DMP init was successful
uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
uint8_t devStatus; // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount; // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q; // [w, x, y, z] quaternion container
VectorFloat gravity; // [x, y, z] gravity vector
float ypr[3]; // [yaw, pitch, roll] yaw/pitch/roll container and gravity vector

float xyz[4];

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void ICACHE_RAM_ATTR dmpDataReady() {
    mpuInterrupt = true;
}

// Web server running on port 80
AsyncWebServer server(80);
// Async Events
AsyncEventSource events("/events");

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void configureEvents() {
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client connections. Id: %u\n", client->lastId());
    }
    // and set reconnect delay to 1 second
    client->send("hello from ESP32",NULL,millis(),1000);
  });
  server.addHandler(&events);
}

void setup()
{
Serial.begin(115200);
// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

mpu.initialize();
pinMode(INTERRUPT_PIN, INPUT);

devStatus = mpu.dmpInitialize();

// supply your own gyro offsets here, scaled for min sensitivity
mpu.setXGyroOffset(140);
mpu.setYGyroOffset(-6);
mpu.setZGyroOffset(43);
mpu.setZAccelOffset(1585); // 1688 factory default for my test chip

// make sure it worked (returns 0 if so)
  if (devStatus == 0)
  {
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();

  }
  else
  {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  connectToWiFi();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, NULL);
  });
  configureEvents();
  server.begin();
  xyz[2] = 0;
  xyz[3] = 0;
}


void loop()
{
  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // wait for MPU interrupt or extra packet(s) available
  while (!mpuInterrupt && fifoCount < packetSize);

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024)
  {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    Serial.println(F("FIFO overflow!")); //happens, why?

  // otherwise, check for DMP data ready interrupt (this should happen frequently)
  }
  else if (mpuIntStatus & 0x02)
  {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);
    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    //Serial.print("Yaw: ");
    //Serial.print(ypr[0] * 180/M_PI);
    //Serial.print("  Pitch: ");
    //Serial.print(ypr[1] * 180/M_PI);
    //Serial.print("  Roll: ");
    //Serial.print(ypr[2] * 180/M_PI);
    //Serial.print("  Temp: ");
    //Serial.println(float((mpu.getTemperature()+521)/340+35.0));
    xyz[0] = ypr[1] * 180/M_PI;
    xyz[1] = ypr[2] * 180/M_PI;
    float yaw = ypr[0] * 180/M_PI;
    float temp = (mpu.getTemperature()+521.0)/340.0+35.0;
    //const e = '{"x":25.3, "y":2.0}'
    char xy[120];
    if (if abs(xyz[2]-xyz[0]) > 0.2 || abs(xyz[3]-xyz[1]) > 0.2 || millis() % 1000 < 10) {
      sprintf(xy,"{\"x\":%f, \"y\":%f, \"z\":%f, \"t\":%f}",xyz[0],xyz[1],yaw,temp);
      events.send(String(xy).c_str(), "xy", millis());
      xyz[2] = xyz[0];
      xyz[3] = xyz[1];
    }//delay(20);
  }
}
