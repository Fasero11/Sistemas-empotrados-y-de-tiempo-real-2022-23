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
    btn_timer = millis();

    Serial.print("BTN_PRESSED: ");
    Serial.println((btn_timer - prev_btn_timer));

    Serial.print("BTN_interrupts: ");
    Serial.println(BTN_interrupts);
    
    if ( (((btn_timer - prev_btn_timer)) > 5000) && BTN_interrupts > 1 && state != ADMIN){
        Serial.println("CHANGE TO ADMIN");
        attachInterrupt(digitalPinToInterrupt(JOY_BTN), update_joystick_btn, LOW);
        item_selected = 0;
        state = ADMIN;
        lcd.clear();
        digitalWrite(LED_1, 1);
        digitalWrite(LED_2, 1);
        lcd_cleared = 0;
      
    } else if ( (((btn_timer - prev_btn_timer)) > 5000) && BTN_interrupts > 1 && state == ADMIN){
        detachInterrupt(digitalPinToInterrupt(JOY_BTN));
        Serial.println("FROM ADMIN TO SERV");
        state = SERVICIO;
        reset_servicio();
      
    } else if ( (((btn_timer - prev_btn_timer)) > 2000) && (((btn_timer - prev_btn_timer)) < 3000) && BTN_interrupts > 1 && state == SERVICIO){
        Serial.println("GO TO SERV");
        reset_servicio();
    }

    if (BTN_interrupts >= 2){
        BTN_interrupts = 0;
    }
    
    prev_btn_timer = btn_timer;
    BTN_just_pressed = 0;
  }
}

void read_btn(){
    BTN_just_pressed = 1;
    BTN_interrupts++;
}

void light_led2(){
    Serial.print("BRIGHT: ");
    Serial.println(led2_brightness);
    if (led2_brightness > 255){
        led2_brightness = 255;
    }
    if (led2_brightness < 0){
        led2_brightness = 0;
    }
    analogWrite(LED_2, led2_brightness);
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
    // To ignore additional interruptions.
    detachInterrupt(digitalPinToInterrupt(JOY_BTN)); // As recommended in the documentation.
    joyBTN_just_pressed = 1;
}

void update_joystick_x(){
    int val_x = analogRead(JOY_X);
    if (val_x > 900){
        menu_back = 1;  
    }
    }

int update_joystick_y(){
    int updated = 0;
    int val_y = analogRead(JOY_Y);
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

void blink_led_1() {
    ledstate = !ledstate;
    led_1_counter--;
}

void show_items(){
    char item_and_price[40]; // 40 is MAX of LCD buffer
    char str_price[40];
    int i, str_len, max_len = 0;

    int JOY_Y_update = update_joystick_y();

    if (JOY_Y_update > 0){
        item_id++;
    } else if (JOY_Y_update < 0){
        item_id--;
    }

    if (JOY_Y_update != 0){
        lcd.clear();
        scroll_count = 0;
        if (item_id < 0){
            item_id = 4;
        } else if (item_id > 4){
            item_id = 0;
        }
    }

    for (i = 0; i < 2; i++){
        dtostrf(prices[item_id+i], 5, 2, str_price); // Cast price from float to string

        if (i == 0) { // Item selected
            str_len = snprintf(item_and_price, 40, "--> %s:%s", ITEMS[item_id+i], str_price); // Create string containing item and price
        } else if (item_id < 4) {
            str_len = snprintf(item_and_price, 40, "    %s:%s", ITEMS[item_id+i], str_price); // Create string containing item and price
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

void show_admin(){
    char admin_item[40]; // 40 is MAX of LCD buffer
    int i, str_len, max_len = 0;

    int JOY_Y_update = update_joystick_y();

    if (JOY_Y_update > 0){
        item_id++;
    } else if (JOY_Y_update < 0){
        item_id--;
    }

    if (JOY_Y_update != 0){
        lcd.clear();
        scroll_count = 0;
    if (item_id < 0){
        item_id = 3;
    } else if (item_id > 3){
        item_id = 0;
    }
    }

    for (i = 0; i < 2; i++){
    if (i == 0) { // Item selected
        str_len = snprintf(admin_item, 40, "--> %s", ADMIN_ITEMS[item_id+i]);
    } else if (item_id < 3) {
        str_len = snprintf(admin_item, 40, "    %s", ADMIN_ITEMS[item_id+i]);
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

    int JOY_Y_update = update_joystick_y();

    if (JOY_Y_update > 0){
        change_price_id++;
    } else if (JOY_Y_update < 0){
        change_price_id--;
    }

    if (JOY_Y_update != 0){
        lcd.clear();
        scroll_count = 0;
    if (change_price_id < 0){
        change_price_id = 4;
    } else if (change_price_id > 4){
        change_price_id = 0;
    }
    }

    for (i = 0; i < 2; i++){
        if (i == 0) { // Item selected
            str_len = snprintf(item, 40, "--> %s", ITEMS[change_price_id+i]);
        } else if (change_price_id < 4) {
            str_len = snprintf(item, 40, "    %s", ITEMS[change_price_id+i]);
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
    int JOY_Y_update = update_joystick_y();

    if (JOY_Y_update > 0){
        price_increment = price_increment - 0.05;
    } else if (JOY_Y_update < 0){
        price_increment = price_increment + 0.05;
    }

    if (JOY_Y_update != 0){
        lcd.clear();
        if ((price_increment + prices[change_price_id]) < 0.05){
            price_increment = price_increment + 0.05;
        } else if ((price_increment + prices[change_price_id]) > 9.95){
            price_increment = price_increment - 0.05;
        }
    }
  
    lcd.setCursor(0,0);
    lcd.print("Precio: ");
    lcd.print(price_increment + prices[change_price_id]);
}

void show_t_and_h(){
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

    digitalWrite(TRIGGER, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER, LOW);

    pulse_delay = pulseIn(ECHO, HIGH);

    distance_cm = pulse_delay / 59;
    //Serial.println(distance_cm);

    return(distance_cm);
}
