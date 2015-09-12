/*
  Made by Korakos
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

// Constantes en el sistema
#define TIME_TO_SWITCH 1800//TODO
#define PRESSURE_LIMIT 570
#define PRESSURE_TIME 5000//TODO
#define MIN_WATER 500//TODO
#define LOOP_MS 1000
#define NUM_AVG 10
#define TIME_PER_READ 100
#define MAX_TIME_TO_MOTOR_OFF 900

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

unsigned long time_of_change;
unsigned long pressure_check;
unsigned long last_time = 0;
boolean motor_enabled;
boolean motor_turn;
boolean low_water;
double pressure;
int water_level;
int counter;
int new_pressure;

// INPUTS
int pressure_pin = A0;
int water_level_pin = A1;

// OUTPUTS
int motor_one = 11;//TODO
int motor_two = 12;//TODO

void setup()   {                
  Serial.begin(9600);
  
  // Initializing values
  time_of_change = 0;
  counter = 0;
  pressure = 100.0;
  new_pressure = 100;
  motor_enabled = false;
  motor_turn = false;
  low_water = false;
  
  // Initializing pins
  pinMode(motor_one, OUTPUT);
  pinMode(motor_two, OUTPUT);
  digitalWrite(motor_one, LOW);
  digitalWrite(motor_two, LOW);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  last_time = millis();
}


void loop() {
  if((millis() - last_time) >= TIME_PER_READ){
    last_time += TIME_PER_READ;
    counter++;
    new_pressure = analogRead(pressure_pin);
    pressure = (((double)pressure * (((double)NUM_AVG - 1) / NUM_AVG)) + ((double)new_pressure / NUM_AVG));
  }
  if(counter >= (LOOP_MS / TIME_PER_READ)){
    counter = 0;
    unsigned long time_elapsed = (millis() - time_of_change)/1000;
    
    // Despues de TIME TO SWITCH milisegundos de estar apagado el motor, lo prendemos cambiando de motor
    
    if(time_elapsed > TIME_TO_SWITCH && !motor_enabled){
      motor_enabled = true;
      motor_turn = !motor_turn;
      time_of_change = millis();
      pressure_check = millis();
      time_elapsed = 0;
    }
    int secs;
    
    water_level = analogRead(water_level_pin);
    
    // Motor Enabled: Verificar que el limite de presion no sea sobrepasado por mas de PRESSURE TIME milisegundos, prender motores
    
    if(motor_enabled){
      secs = time_elapsed;
      
      if(motor_turn){
        digitalWrite(motor_one, HIGH);
      }else{
        digitalWrite(motor_two, HIGH);
      }
      
      // si la presion esta por debajo de PRESSURE LIMIT, contador igual a millis
      
      if(pressure < PRESSURE_LIMIT){
        pressure_check = millis();
      }
      
      // si el contador difiere de millis mas que PRESSURE TIME significa que la presion fue alta todo ese tiempo, y apagamos motores
      
      if(millis() - pressure_check > PRESSURE_TIME || time_elapsed > MAX_TIME_TO_MOTOR_OFF){
        time_of_change = millis();
        secs = TIME_TO_SWITCH - time_elapsed;
        digitalWrite(motor_one, LOW);
        digitalWrite(motor_two, LOW);
        motor_enabled = !motor_enabled;
      }
    }else{      // Motor Disabled: apagar los motores
      secs = TIME_TO_SWITCH - time_elapsed;
      digitalWrite(motor_one, LOW);
      digitalWrite(motor_two, LOW);
    }
    String str = "pressure: ";
    str += new_pressure;
    str += "\npromedio: ";
    str += pressure;
    str += "\nA1: ";
    str += water_level;
    
    String error = "";
    if(water_level < MIN_WATER){
      error += "Low Water Level";
    }
    displayStatus(error, motor_enabled, motor_turn, secs, str);
  }
  
}

void displayStatus(String error, boolean status, boolean motor, unsigned long time, String data){
  // Clear the buffer
  display.clearDisplay();
  display.setCursor(0,0);
  
  // Display error.
  if(error.length() != 0){
    display.setTextColor(BLACK,WHITE);
    
    display.println("ERROR:");
    display.println(error);
    
    display.setTextColor(WHITE);
  }  
  
  
  // Display Status.
  if(status){
    display.print("Motor ");
    display.print((int)motor + 1);
    display.println(" Prendido");
  }else{
    display.println("Tiempo para prender");
  }
  
  // Display Time.
  display.print(time);
  display.println(" segs");
  
  // Display Data.
  if(data.length() != 0){
    display.println(data);
  }
  display.display();
}


