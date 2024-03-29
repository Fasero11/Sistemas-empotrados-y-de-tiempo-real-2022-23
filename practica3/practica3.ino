#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>
#include <avr/wdt.h>

// Constants
const int ARRANQUE = 0, SERVICIO = 1, ADMIN = 2;
char *ITEMS[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
char *ADMIN_ITEMS[] = {"Ver temperatura", "Ver distancia sensor", "Ver contador", "Modificar Precios"};

// Pins
const int RS = 8, EN = 9, D4 = 10, D5 = 11, D6 = 12, D7 = 13, TRIGGER = 6, ECHO = 7, DHT_PIN = 4, 
          LED_1 = A3, LED_2 = 5, JOY_BTN = 2, JOY_X = A4, JOY_Y = A5, BTN = 3;

// Variables
int state = ARRANQUE , joystick_read = 0, lcd_cleared = 0, scroll_count = 0, already_scrolled = 0, previous_second = 0, item_id = 0,
    make_time = 0, led2_brightness = 0, menu_back = 0, distance_cm = 0, prev_dist_digits = 0, change_price_id = 0, change_price_selected = 0, confirm_price = 0;
unsigned long ms_timer_start = 0, btn_timer = 0, prev_btn_timer = 0, btn_cooldown = 0;
double maketime_2_255 = 0.0;
float price_increment = 0;
float prices[] = {1, 1.10, 1.25, 1.50, 2};

// Variables modified in ISR
volatile short ledstate = LOW, is_client = 0, item_selected = 0, joyBTN_just_pressed = 0, BTN_just_pressed = 0;
volatile short led_1_counter = 6, BTN_interrupts = 0;

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
DHT dht(DHT_PIN, DHT11);
Thread T_H_Thread = Thread();
Thread Prep_Thread = Thread();
Thread LED_2_Thread = Thread();

void setup() {
    Serial.begin(9600);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(TRIGGER, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(JOY_BTN , INPUT_PULLUP);                                                                                                                                                                                                                                                                                                 
    pinMode(BTN, INPUT); 

    digitalWrite(LED_1, LOW);

    Timer1.initialize(500000);
    Timer1.attachInterrupt(blink_led_1);

    lcd.begin(16,2);
    dht.begin();

    T_H_Thread.enabled = true;
    T_H_Thread.setInterval(100);
    T_H_Thread.onRun(show_t_and_h);

    Prep_Thread.setInterval(100);
    Prep_Thread.onRun(preparando_cafe);
    Prep_Thread.enabled = false;

    LED_2_Thread.setInterval(100);
    LED_2_Thread.onRun(light_led_2);
    LED_2_Thread.enabled = false;

    wdt_disable();
    wdt_enable(WDTO_1S);

    attachInterrupt(digitalPinToInterrupt(BTN), read_btn, CHANGE);
}

void loop() {
    //Serial.println("Watchdog reset");
    wdt_reset();
    
    check_btn_hold();
  
    if (state == ARRANQUE){
        if (led_1_counter > 0){
            lcd.setCursor(0,0);
            lcd.print("CARGANDO...");
            //Serial.println(led_1_counter);
            digitalWrite(LED_1, ledstate);
        } else{
            digitalWrite(LED_1, 0);
            detachInterrupt(blink_led_1);
            state = 1;
            lcd.clear();
        }
    }

    if (state == SERVICIO){
        if (!is_client){
            distance_cm = get_distance();
            if (distance_cm < 100){
                is_client = 1;
            }
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
            if (T_H_Thread.enabled && (millis() - ms_timer_start) / 1000 < 5){
                if(T_H_Thread.shouldRun()){
                    T_H_Thread.run();
                }    
            } else {
                if (T_H_Thread.enabled){
                    T_H_Thread.enabled = false;
                    attachInterrupt(digitalPinToInterrupt(JOY_BTN), update_joystick_btn, LOW);  // Allows button interruption (For selecting item)
                }
                //Serial.print("Item Selected: ");
                //Serial.println(item_selected);
                if (joyBTN_just_pressed){
                    item_selected = 1;
                    joyBTN_just_pressed = 0;
                    lcd_cleared = 0;
                    Prep_Thread.enabled = true;
                    LED_2_Thread.enabled = true;
                    make_time = random(4000,8001);
                    ms_timer_start = millis();
                    maketime_2_255 = 255.0 / make_time;
                }
                
                if (!item_selected){
                    show_items();        
                } else{
                    //Serial.print("Prep_Thread: ");
                    //Serial.println(Prep_Thread.enabled);
                    //Serial.print("TIME: ");
                    //Serial.println((millis() - ms_timer_start)/1000);
                    if (Prep_Thread.enabled && (((millis() - ms_timer_start)) < make_time) ){
                        if (Prep_Thread.shouldRun()){
                            Prep_Thread.run(); 
                        }
                        if (LED_2_Thread.shouldRun()){
                            //Serial.println((millis() - ms_timer_start));
                            //Serial.println(maketime_2_255);
                            led2_brightness = int((millis() - ms_timer_start)) * maketime_2_255;
                            LED_2_Thread.run(); 
                        }
                    } else {
                        if (Prep_Thread.enabled){
                            Serial.println("RETIRE BEBIDA");
                            Prep_Thread.enabled = false;
                            LED_2_Thread.enabled = false;
                            analogWrite(LED_2, 0); // Turn off LED_2
                            lcd.clear();
                            lcd.setCursor(0,0);
                            lcd.print("RETIRE BEBIDA");
                            ms_timer_start = millis();
                        }
                        Serial.println(joyBTN_just_pressed);
                        if (((millis() - ms_timer_start)) > 3000){
                            reset_servicio();
                        }
                    }
                }
            }
        }
    }
    if (state == ADMIN){
        Serial.print("Item Selected: ");
        Serial.print(item_selected);
        Serial.print("| menu_back: ");
        Serial.print(menu_back);
        Serial.print("| item_ID: ");
        Serial.print(item_id);
        Serial.print("| change_price_selected: ");
        Serial.print(change_price_selected);
        Serial.print("| joyBTN_just_pressed: ");
        Serial.print(joyBTN_just_pressed);
        Serial.print("| confirm_price: ");
        Serial.println(confirm_price);
        
        if (!item_selected){
            show_admin();
            if (joyBTN_just_pressed){
                item_selected = 1;
                joyBTN_just_pressed = 0;
                lcd_cleared = 0;
                change_price_selected = 0;
            }       
        } else {
            if (!lcd_cleared){
                Serial.println("CLEARED");
                lcd.clear();
                lcd_cleared = 1;
            }
          
            update_joystick_x();
          
            switch (item_id){
                case 0:
                    show_temp();
                    break;
              
                case 1:
                    show_dist();
                    break;
              
                case 2:
                    lcd.setCursor(0,0);
                    lcd.print("Segundos: ");
                    lcd.setCursor(0,1);
                    lcd.print(millis()/1000);
                    break;
              
                case 3:
                    //Serial.println("ID 3");
                    if (joyBTN_just_pressed && !change_price_selected){
                        Serial.println("change_price_selected: ");
                        change_price_selected = 1;
                        lcd.clear();
                        joyBTN_just_pressed = 0;
                        btn_cooldown = millis();  
                    }
    
                    if (!change_price_selected){
                        show_item_change();
                        attachInterrupt(digitalPinToInterrupt(JOY_BTN), update_joystick_btn, LOW);
                    } else {
                        // Reactivate button after 500ms to avoid false positives.
                        if ((millis() - btn_cooldown) > 500){
                            attachInterrupt(digitalPinToInterrupt(JOY_BTN), update_joystick_btn, LOW);
                        }
                        Serial.print("change_price_id: ");
                        Serial.println(change_price_id);
                        change_price();
                        if (confirm_price){
                            prices[change_price_id] = prices[change_price_id] + price_increment;
                            menu_back = 1;  
                            lcd.clear();  
                        }
    
                        if (joyBTN_just_pressed){
                            confirm_price = 1;
                            joyBTN_just_pressed = 0;   
                        }
                    }
                      
                    break;
              
                 default:
                    break;
          }
           
        if (menu_back){
            confirm_price = 0;
            price_increment = 0;
            lcd.clear();
            item_selected = 0;
            change_price_selected = 0;
            change_price_id = 0;
            // Allows button to be used again.
            attachInterrupt(digitalPinToInterrupt(JOY_BTN), update_joystick_btn, LOW);
            menu_back = 0;
            lcd_cleared = 0;
          }
        }
  }
}
