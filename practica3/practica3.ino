#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>

// Constants
const int ARRANQUE = 0, SERVICIO = 1, ADMIN = 2;

// Variables
int state = ARRANQUE, ms_timer_start = 0, joystick_read = 0, lcd_cleared = 0, scroll_count = 0;

// Pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 10, trigger = 6, echo = 7, DHT_pin = 9, 
LED_1 = 8, joy_BTN = 2, joy_x = A4, joy_y = A5;

// Variables modified in ISR
volatile byte ledstate = LOW, LED_1_counter = 6, is_client = 0,  item_selected = 0;

char *items[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
float prices[] = {1, 1.10, 1.25, 1.50, 2};
int item_id = 0; // which item is the user on.

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
DHT dht(DHT_pin, DHT11);
Thread myThread = Thread();

void scroll_text(){
  lcd.scrollDisplayLeft();
}

void update_joystick_btn(){
  item_selected = 1;
}

int update_joystick_y(){
  int updated = 0;
  int val_y = analogRead(joy_y);
  if (val_y < 100 & !joystick_read){
    item_id++;
    updated = 1;
    joystick_read = 1;
  } else if (val_y > 900 & !joystick_read){
    item_id--;
    updated = 1;
    joystick_read = 1;
  } else if (val_y > 100 & val_y < 900){
    joystick_read = 0;
  }

  if (item_id < 0){
    item_id = 0;
  } else if (item_id > 4){
    item_id = 4;
  }
 
  return updated;
}

void blinkLED_1() {
  ledstate = !ledstate;
  LED_1_counter--;
}

void showItems(){
  if (update_joystick_y()){
    lcd.clear();
  }
  Serial.println(item_id);
  int i;
  for (i = 0; i < 2; i++){
    lcd.setCursor(0,i);
    lcd.print(items[item_id+i]);
  }
  lcd.setCursor(15,0);
  lcd.print("*");
}

void show_T_and_H(){
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  lcd.setCursor(0,0);
  lcd.print("T: ");
  lcd.print(temp);
  lcd.setCursor(0,1);
  lcd.print("H: ");
  lcd.print(humidity);
}

int detectClient(){
   long pulse_delay, distance_cm;
   
   digitalWrite(trigger, LOW);
   delayMicroseconds(4);
   digitalWrite(trigger, HIGH);
   delayMicroseconds(10);
   digitalWrite(trigger, LOW);
   
   pulse_delay = pulseIn(echo, HIGH);
   
   distance_cm = pulse_delay / 59;
   Serial.println(distance_cm);
   if (distance_cm < 100){
      is_client = 1;
   }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_1, OUTPUT);
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(joy_BTN , INPUT_PULLUP); 
  
  digitalWrite(LED_1, LOW);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(blinkLED_1);
  
  myThread.setInterval(100);
  myThread.onRun(show_T_and_H);
  myThread.enabled = true;

  lcd.begin(16,2);
  dht.begin();

  attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);
}

void loop() {
  if (state == ARRANQUE){
    if (LED_1_counter > 0){
        lcd.setCursor(0,0);
        lcd.print("CARGANDO...");
        Serial.println(LED_1_counter);
        digitalWrite(LED_1, ledstate);
    } else{
      detachInterrupt(blinkLED_1);
      state = 1;
      lcd.clear();
    }
  }

  if (state == SERVICIO){
    Serial.println(item_selected);
    if (!is_client){
      detectClient();
      lcd.setCursor(0,0);
      lcd.print("ESPERANDO");
      lcd.setCursor(0,1);
      lcd.print("CLIENTE");
      if (is_client){
        // Client was detected in this iteration.
        lcd.clear();
        ms_timer_start = millis();  
      }
    } else {
        if(millis() - ms_timer_start < 5000){
          show_T_and_H();
          delay(500);
          lcd.clear();
        } else {
          if (!item_selected){
            showItems();           
          } else{
            if (!lcd_cleared){
              lcd.clear();
              lcd_cleared = 1;
              lcd.setCursor(0,0);
              lcd.print("Preparando Cafe...");
            }
            // There are two extra characters. (Scroll twice)
            if (scroll_count < 3){
              delay(750);
              scroll_text();
              scroll_count++;
            } else {
              lcd_cleared = 0;
              scroll_count = 0;
            }
          }
        }
      }
  }
}
