#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include "HX711.h"

#define LED_COUNT 24

///PIN ASSIGNATION

#define PIN_BUZZER 10
#define PIN_CONTRAST 44
#define PIN_LED 24
#define PIN_START 19
#define PIN_SKIP 20
#define PIN_STOP 21

Adafruit_NeoPixel strip(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);
LiquidCrystal lcd(12,11,5,4,3,2); //RS,EN,D4,D5,D6,D7
HX711 scale(38,39); //38 - data, 39 - clock

typedef struct ins
{
  String action;
  String name;
  String measurementUnit;
  double quantity;
  int duration;
  int points;
}INSTRUCTION;

INSTRUCTION recipe[40];
volatile int index = 0;
int dimension = 0;
volatile bool recipe_started = false;
volatile bool recipe_ended = false;

const int start_size = 8;
const int start_song[] = {587, 523, 493, 440, 392, 493, 587, 784};
const int start_duration[] = {8,8,8,8,4,8,8,4};
const float start_pause_f = 1.3;

const int wrong_size = 3;
const int wrong_song[] = {494, 494, 349};
const int wrong_duration[] = {8,8,2};
const float wrong_pause_f = 1.7;

const int end_size = 6;
const int end_song[] = {147, 196, 247, 293, 293, 392};
const int end_duration[] = {8,8,8,4,8,1};
const float end_pause_f = 1.3;

bool did_i_sing = false;

void setup() {
  // put your setup code here, to run once:
  initializeStrip();
  initializeLCD();
  Serial.begin(9600);
  Serial2.begin(9600);
  pinMode(PIN_START,INPUT_PULLUP);
  pinMode(PIN_SKIP,INPUT_PULLUP);
  pinMode(PIN_STOP,INPUT_PULLUP);
  pinMode(PIN_BUZZER,OUTPUT);
  play_song(start_song,start_duration,start_size,start_pause_f);
  attachInterrupt(digitalPinToInterrupt(PIN_START),startRecipe,RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_SKIP),skipInstruction,RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_STOP),stopRecipe,RISING);
  recipe_started=false;
  recipe_ended = false;
}

void loop() {
 ///TODO: display a welcoming message on the LCD
 if(!recipe_started)
 {
    friendlyMessage();
    if(Serial2.available())
      readRecipe();
 }
 else
 {
   if(!did_i_sing)
   {
    play_song(start_song,start_duration,start_size,start_pause_f);
    did_i_sing = true;  
   }

   //displayInstruction(recipe[index]);
   index++;
   delay(1000);
 }
 ///TODO: when button 'START' is pressed, interrupt catches the loading of the recipe
 ///TODO: display a ready, steady, go
 ///TODO: display each instruction on the LCD
 ///TODO: when instruction is skipped, index incremented
}

void initializeStrip()
{
  strip.begin();
  strip.show();
  strip.setBrightness(10);

  for(int k=0;k<3;k++)
  {
      for(int i=0;i<LED_COUNT;i++)
  {
    strip.setPixelColor(i,strip.Color(255,0,0));
    strip.show();
  }
  delay(600);
  
  for(int i=0;i<LED_COUNT;i++)
  {
    strip.setPixelColor(i,strip.Color(0,0,0));
    strip.show();
  }

    delay(600);
  }
}

void initializeLCD()
{
  lcd.begin(16,2);
  lcd.clear();
  pinMode(PIN_CONTRAST, OUTPUT);
  analogWrite(PIN_CONTRAST, 100); 
}
void initializeScale()
{
  scale.set_scale();
  scale.tare();
}

void play_song(int song[],int duration[],int nr_notes,float pause_f)
{
  int song_size = sizeof(song)/sizeof(int);
  
  for(int i=0;i<nr_notes;i++)
  {
    int note_duration = 1000/duration[i];
    tone(PIN_BUZZER,song[i],note_duration);
    int pause = note_duration*pause_f;
    delay(pause);
    noTone(PIN_BUZZER);
  }
}

void setColor(unsigned int quantity, unsigned int quantity_t,bool isFirst)
{
  int first,last;
  unsigned long color;
  unsigned long shut_off = strip.Color(0,0,0);
  if(isFirst)
  {
    first = 0;
    last = LED_COUNT/2;
    color = strip.Color(70, 255, 81);
  }
  else
  {
    first = LED_COUNT/2;
    last = LED_COUNT;
    color = strip.Color(255,75,0);
  }
  double index = LED_COUNT/2*(double)quantity/quantity_t;
  
  if(index>LED_COUNT/2){
     index = first;
     for(int i=first;i<last;i++)
     {
      strip.setPixelColor(i,shut_off);
      strip.show();
     }
  }
    
  else
  {
    for(int i=first;i<first+index;i++)
     {
      strip.setPixelColor(i,color);
      strip.show();
      delay(30);
     }
  }
}

void readRecipe()
{

  String instruction;
  String text;
  
  while(Serial2.available())
  {
    String text;
    instruction = Serial2.readString();
    text+=instruction;
  }
    
  while(text.length()>0)
  {
    instruction = text.substring(0,text.indexOf('\n'));
        Serial.println(instruction);
        int startingPoint = 0;
        if(instruction[0] == '@')
        {
          recipe[dimension].action = "ADD"; 
          int firstSpace;
          startingPoint = 1;
          firstSpace = instruction.indexOf(' ');
          if(instruction[1] == '@')
          {
            int secondSpace = instruction.indexOf(' ', firstSpace + 1);
            recipe[dimension].name = instruction.substring(startingPoint, secondSpace);
            startingPoint = secondSpace + 1;
          }
          else
          {
            recipe[dimension].name = instruction.substring(startingPoint,firstSpace);
            startingPoint = firstSpace + 1;
          }

          firstSpace = instruction.indexOf(' ',startingPoint);
          recipe[dimension].quantity = instruction.substring(startingPoint, firstSpace).toDouble();
          int secondSpace = instruction.indexOf(' ', firstSpace + 1);
          recipe[dimension].measurementUnit = instruction.substring(firstSpace + 1, secondSpace);
          int thirdSpace = instruction.indexOf(' ', secondSpace + 1);
          recipe[dimension].duration = instruction.substring(secondSpace + 1, thirdSpace).toInt();
          recipe[dimension].points = instruction.substring(thirdSpace + 1).toInt();
        }
        else if(instruction[0] == '~')
        {
          recipe[dimension].action = "MIX";
          int firstSpace = instruction.indexOf(' ');
          recipe[dimension].duration = instruction.substring(1, firstSpace).toInt();
          recipe[dimension].points = instruction.substring(firstSpace + 1).toInt();
        }
        else if(instruction[0] == '!')
        {
          recipe[dimension].action = "COOK";
          int firstSpace = instruction.indexOf(' ');
          recipe[dimension].duration = instruction.substring(1, firstSpace).toInt();
        }
        else if(instruction[0] == '#')
          break;
     dimension++;
     text = text.substring(text.indexOf('\n')+1);
  } 
}

void friendlyMessage()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Welcome to ");
  lcd.setCursor(0,1);
  lcd.print("CHEF DETECTOR");
  delay(5000);
  lcd.clear();
  delay(1000);
}

void startRecipe()
{
  index = 0;
  recipe_started = true;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("READY!");
  lcd.setCursor(7,0);
  lcd.print("STEADY!");
  lcd.setCursor(5,1);
  lcd.print("GO...");
}

void skipInstruction()
{
  if(index==dimension-1)
  {
    recipe_ended = true;
    index = 0;
  }
  else index++;
}

void stopRecipe()
{
  index = dimension;
}

void displayInstruction()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(recipe[index].action);
  Serial.println(recipe[index].action);
  lcd.setCursor(13,0);
  lcd.print(recipe[index].duration);
  Serial.println(recipe[index].duration);
  lcd.setCursor(0,1);
  lcd.print(recipe[index].name);
  Serial.println(recipe[index].name);
}
