#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>

const int ARRANQUE = 0, SERVICIO = 1, ADMIN = 2;
int LED_1 = 8, LED_1_counter = 6, ledstate = LOW, state = ARRANQUE, is_client = 0, DHT_pin = 9, ms_timer_start = 0;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2, trigger = 6, echo = 7;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
DHT dht(DHT_pin, DHT11);
Thread myThread = Thread();

void blinkLED_1() {
  ledstate = !ledstate;
  LED_1_counter--;
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
  
  digitalWrite(LED_1, LOW);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(blinkLED_1);
  
  myThread.setInterval(100);
  myThread.onRun(show_T_and_H);
  myThread.enabled = true;

  lcd.begin(16,2);
  dht.begin();
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
        }
      }
  }
}
