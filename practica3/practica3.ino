#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>

// Constants
const int ARRANQUE = 0, SERVICIO = 1, ADMIN = 2;
char *items[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
char *admin_items[] = {"Ver temperatura", "Ver distancia sensor", "Ver contador", "Modificar Precios"};
float prices[] = {1, 1.10, 1.25, 1.50, 2};

// Variables
int state = ARRANQUE , joystick_read = 0, lcd_cleared = 0, scroll_count = 0, already_scrolled = 0, previous_second = 0, item_id = 0,
    make_time = 0, LED2_bright = 0, menu_back = 0, distance_cm = 0, prev_dist_digits = 0, change_price_ID = 0, change_price_selected = 0, confirm_price = 0;
unsigned long ms_timer_start = 0, BTN_timer = 0, prev_BTN_timer = 0, BTN_cooldown = 0;
double scale_factor = 0.0;
float price_increment = 0;

// Pins
const int rs = 8, en = 9, d4 = 10, d5 = 11, d6 = 12, d7 = 13, trigger = 6, echo = 7, DHT_pin = 4, 
          LED_1 = A3, LED_2 = 5, joy_BTN = 2, joy_x = A4, joy_y = A5, BTN = 3;

// Variables modified in ISR
volatile byte ledstate = LOW, LED_1_counter = 6, is_client = 0,  item_selected = 0, joyBTN_just_pressed = 0, BTN_just_pressed = 0, BTN_interrupts = 0;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
DHT dht(DHT_pin, DHT11);
Thread T_H_Thread = Thread();
Thread Prep_Thread = Thread();
Thread LED_2_Thread = Thread();

void show_temp(){
    float temp_C = dht.readTemperature();
    float humidity = dht.readHumidity();
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.print(temp_C);
    

    lcd.setCursor(0,1);
    lcd.print("Hum: ");
    lcd.print(humidity);
    lcd.print("%");
}

void show_dist(){
    int dist_digits;
    distance_cm = get_distance();

    if (distance_cm >= 1000){
        dist_digits = 4; 
    } else if (distance_cm >= 100) {
        dist_digits = 3;  
    } else if (distance_cm >= 10) {
        dist_digits = 2;  
    } else {
        dist_digits = 1;
    }

    if (prev_dist_digits > dist_digits){
        lcd.clear();
    }

    lcd.setCursor(0,0);
    lcd.print("Distancia: ");
    lcd.setCursor(0,1);
    lcd.print(distance_cm);
    lcd.print(" cm");

    Serial.print("Dist.(cm): ");
    Serial.println(distance_cm);

    prev_dist_digits = dist_digits;
}

void reset_servicio(){
    // Reset State
    Serial.println("RESET");
    item_id = 0;
    is_client = 0;
    T_H_Thread.enabled = true;
    item_selected = 0;
    lcd.clear();
    digitalWrite(LED_2, 0);
    digitalWrite(LED_1, 0);
    Prep_Thread.enabled = false;
    LED_2_Thread.enabled = false;  
}

void check_btn_hold(){
    if (BTN_just_pressed){
    BTN_timer = millis();

    Serial.print("BTN_PRESSED: ");
    Serial.println((BTN_timer - prev_BTN_timer));

    Serial.print("BTN_interrupts: ");
    Serial.println(BTN_interrupts);
    
    if ( (((BTN_timer - prev_BTN_timer)) > 5000) && BTN_interrupts > 1 && state != ADMIN){
        Serial.println("CHANGE TO ADMIN");
        attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);
        item_selected = 0;
        state = ADMIN;
        lcd.clear();
        digitalWrite(LED_1, 1);
        digitalWrite(LED_2, 1);
        lcd_cleared = 0;
      
    } else if ( (((BTN_timer - prev_BTN_timer)) > 5000) && BTN_interrupts > 1 && state == ADMIN){
        detachInterrupt(digitalPinToInterrupt(joy_BTN));
        Serial.println("FROM ADMIN TO SERV");
        state = SERVICIO;
        reset_servicio();
      
    } else if ( (((BTN_timer - prev_BTN_timer)) > 2000) && (((BTN_timer - prev_BTN_timer)) < 3000) && BTN_interrupts > 1 && state == SERVICIO){
        Serial.println("GO TO SERV");
        reset_servicio();
    }

    if (BTN_interrupts >= 2){
        BTN_interrupts = 0;
    }
    
    prev_BTN_timer = BTN_timer;
    BTN_just_pressed = 0;
  }
}

void read_BTN(){
    BTN_just_pressed = 1;
    BTN_interrupts++;
}

void light_LED2(){
    Serial.print("BRIGHT: ");
    Serial.println(LED2_bright);
    if (LED2_bright > 255){
        LED2_bright = 255;
    }
    if (LED2_bright < 0){
        LED2_bright = 0;
    }
    analogWrite(LED_2, LED2_bright);
}

void preparando_cafe(){
    //Serial.println(make_time);
    //Serial.print("Scroll count: ");
    //Serial.println(scroll_count);
    if (!lcd_cleared){
        Serial.println("Cleared preparando_café");
        lcd.clear();
        lcd_cleared = 1;
        lcd.setCursor(0,0);
        lcd.print("Preparando Cafe...");
        scroll_count = 0;
    }
    // There are two extra characters. (Scroll three times)
    if (scroll_count < 4){
        if (scroll_text_sec()){
            scroll_count++;  
        }     
    } else {
        lcd_cleared = 0;
    }  
}

int scroll_text_sec(){
    // Scrolls text only once per second.
    // If called twice during the same second, won't do anything.
    // Returns 1 if text was scrolled, 0 otherwise
    int was_scrolled = 0;
    int current_second = (millis() / 1000) % 2;

    if (current_second != previous_second){
        lcd.scrollDisplayLeft();
        was_scrolled = 1;  
    }
    previous_second = current_second;
    return(was_scrolled);
}

void update_joystick_btn(){
    Serial.println("Button Pressed");
    // Cooldown to avoid false positives
    if ((millis() - BTN_cooldown) > 500){
        // To ignore additional interruptions.
        detachInterrupt(digitalPinToInterrupt(joy_BTN)); // As recommended in the documentation.
        joyBTN_just_pressed = 1;
        BTN_cooldown = millis();
    }
}

void update_joystick_x(){
    int val_x = analogRead(joy_x);
    if (val_x > 900){
        menu_back = 1;  
    }
    }

int update_joystick_y(){
    int updated = 0;
    int val_y = analogRead(joy_y);
    if (val_y > 900 & !joystick_read){
        updated = 1;
        joystick_read = 1;

    } else if (val_y < 100 & !joystick_read){
        updated = -1;
        joystick_read = 1;
    } else if (val_y > 100 & val_y < 900){
        joystick_read = 0;
    }
    return updated;
}

void blinkLED_1() {
    ledstate = !ledstate;
    LED_1_counter--;
}

void showItems(){
    char item_and_price[40]; // 40 is MAX of LCD buffer
    char str_price[40];
    int i, str_len, max_len = 0;

    int joy_y_update = update_joystick_y();

    if (joy_y_update > 0){
        item_id++;
    } else if (joy_y_update < 0){
        item_id--;
    }

    if (joy_y_update != 0){
        lcd.clear();
        scroll_count = 0;
        if (item_id < 0){
            item_id = 0;
        } else if (item_id > 4){
            item_id = 4;
        }
    }

    for (i = 0; i < 2; i++){
        dtostrf(prices[item_id+i], 5, 2, str_price); // Cast price from float to string

        if (i == 0) { // Item selected
            str_len = snprintf(item_and_price, 40, "--> %s:%s", items[item_id+i], str_price); // Create string containing item and price
        } else if (item_id < 4) {
            str_len = snprintf(item_and_price, 40, "    %s:%s", items[item_id+i], str_price); // Create string containing item and price
        } else {
            item_and_price[0] = '\0';
            str_len = 0;
        }
        lcd.setCursor(0,i);
        lcd.print(item_and_price);

        if (str_len > max_len){
            max_len = str_len;
        }
    }
    int scrolls_needed = max_len - 14; // 16 are the LCD positions. We want to leave some space at the end.
    if (scroll_count < scrolls_needed){
        if (scroll_text_sec()){
            scroll_count++;
        }
    } else if (scrolls_needed >= 0) {
        scroll_count = 0;
        lcd.clear();
    }
}

void showAdmin(){
    char admin_item[40]; // 40 is MAX of LCD buffer
    int i, str_len, max_len = 0;

    int joy_y_update = update_joystick_y();

    if (joy_y_update > 0){
        item_id++;
    } else if (joy_y_update < 0){
        item_id--;
    }

    if (joy_y_update != 0){
        lcd.clear();
        scroll_count = 0;
    if (item_id < 0){
        item_id = 0;
    } else if (item_id > 3){
        item_id = 3;
    }
    }

    for (i = 0; i < 2; i++){
    if (i == 0) { // Item selected
        str_len = snprintf(admin_item, 40, "--> %s", admin_items[item_id+i]);
    } else if (item_id < 3) {
        str_len = snprintf(admin_item, 40, "    %s", admin_items[item_id+i]);
    } else {
        admin_item[0] = '\0';
        str_len = 0;
    }
    lcd.setCursor(0,i);
    lcd.print(admin_item);

    if (str_len > max_len){
        max_len = str_len;
    }
    }
    int scrolls_needed = max_len - 14; // 16 are the LCD positions. We want to leave some space at the end.
    if (scroll_count < scrolls_needed){
        if (scroll_text_sec()){
            scroll_count++;
        }
    } else if (scrolls_needed >= 0) {
        scroll_count = 0;
        lcd.clear();
    }
}

void show_item_change(){
    char item[40]; // 40 is MAX of LCD buffer
    int i, str_len, max_len = 0;

    int joy_y_update = update_joystick_y();

    if (joy_y_update > 0){
        change_price_ID++;
    } else if (joy_y_update < 0){
        change_price_ID--;
    }

    if (joy_y_update != 0){
        lcd.clear();
        scroll_count = 0;
    if (change_price_ID < 0){
        change_price_ID = 0;
    } else if (change_price_ID > 4){
        change_price_ID = 4;
    }
    }

    for (i = 0; i < 2; i++){
        if (i == 0) { // Item selected
            str_len = snprintf(item, 40, "--> %s", items[change_price_ID+i]);
        } else if (change_price_ID < 4) {
            str_len = snprintf(item, 40, "    %s", items[change_price_ID+i]);
        } else {
            item[0] = '\0';
            str_len = 0;
        }
        lcd.setCursor(0,i);
        lcd.print(item);

        if (str_len > max_len){
            max_len = str_len;
        }
    }
    int scrolls_needed = max_len - 14; // 16 are the LCD positions. We want to leave some space at the end.
    if (scroll_count < scrolls_needed){
        if (scroll_text_sec()){
            scroll_count++;
        }
    } else if (scrolls_needed >= 0) {
        scroll_count = 0;
        lcd.clear();
    }
}

void change_price(){
    int joy_y_update = update_joystick_y();

    if (joy_y_update > 0){
        price_increment = price_increment - 0.05;
    } else if (joy_y_update < 0){
        price_increment = price_increment + 0.05;
    }

    if (joy_y_update != 0){
        lcd.clear();
        if ((price_increment + prices[change_price_ID]) < 0.05){
            price_increment = price_increment + 0.05;
        } else if ((price_increment + prices[change_price_ID]) > 9.95){
            price_increment = price_increment - 0.05;
        }
    }
  
    lcd.setCursor(0,0);
    lcd.print("Price: ");
    lcd.print(price_increment + prices[change_price_ID]);
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

int get_distance(){
    long pulse_delay, distance_cm;

    digitalWrite(trigger, LOW);
    delayMicroseconds(4);
    digitalWrite(trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);

    pulse_delay = pulseIn(echo, HIGH);

    distance_cm = pulse_delay / 59;
    //Serial.println(distance_cm);

    return(distance_cm);
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(trigger, OUTPUT);
    pinMode(echo, INPUT);
    pinMode(joy_BTN , INPUT_PULLUP);                                                                                                                                                                                                                                                                                                 
    pinMode(BTN, INPUT); 

    digitalWrite(LED_1, LOW);

    Timer1.initialize(1000000);
    Timer1.attachInterrupt(blinkLED_1);

    lcd.begin(16,2);
    dht.begin();

    T_H_Thread.enabled = true;
    T_H_Thread.setInterval(100);
    T_H_Thread.onRun(show_T_and_H);

    Prep_Thread.setInterval(100);
    Prep_Thread.onRun(preparando_cafe);
    Prep_Thread.enabled = false;

    LED_2_Thread.setInterval(100);
    LED_2_Thread.onRun(light_LED2);
    LED_2_Thread.enabled = false;

    attachInterrupt(digitalPinToInterrupt(BTN), read_BTN, CHANGE);
}

void loop() {
    check_btn_hold();
  
    if (state == ARRANQUE){
        if (LED_1_counter > 0){
            lcd.setCursor(0,0);
            lcd.print("CARGANDO...");
            //Serial.println(LED_1_counter);
            digitalWrite(LED_1, ledstate);
        } else{
            detachInterrupt(blinkLED_1);
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
                    attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);  // Allows button interruption (For selecting item)
                }
                //Serial.print("Item Selected: ");
                //Serial.println(item_selected);
                if (joyBTN_just_pressed){
                    item_selected = 1;
                    joyBTN_just_pressed = 0;
                    make_time = random(4,9);
                    ms_timer_start = millis();
                    lcd_cleared = 0;
                    Prep_Thread.enabled = true;
                    LED_2_Thread.enabled = true;
                    scale_factor = 255.0 / (make_time*1000);
                }
                
                if (!item_selected){
                    showItems();        
                } else{
                    //Serial.print("Prep_Thread: ");
                    //Serial.println(Prep_Thread.enabled);
                    //Serial.print("TIME: ");
                    //Serial.println((millis() - ms_timer_start)/1000);
                    if (Prep_Thread.enabled && (((millis() - ms_timer_start)/1000) < make_time) ){
                        if (Prep_Thread.shouldRun()){
                            Prep_Thread.run(); 
                        }
                        if (LED_2_Thread.shouldRun()){
                            //Serial.println((millis() - ms_timer_start));
                            //Serial.println(scale_factor);
                            LED2_bright = int((millis() - ms_timer_start)) * scale_factor;
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
                        if (((millis() - ms_timer_start)/1000) > 3){
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
        showAdmin();
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
                    BTN_cooldown = millis();
                    attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);   
                }

                if (!change_price_selected){
                    show_item_change();
                    attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);
                } else {
                    Serial.print("change_price_ID: ");
                    Serial.println(change_price_ID);
                    change_price();
                    if (confirm_price){
                        prices[change_price_ID] = prices[change_price_ID] + price_increment;
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
        change_price_ID = 0;
        // Allows button to be used again.
        attachInterrupt(digitalPinToInterrupt(joy_BTN), update_joystick_btn, LOW);
        menu_back = 0;
        lcd_cleared = 0;
      }
    }
  }
}
