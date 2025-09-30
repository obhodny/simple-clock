
/**
 * @file main.c
 * @brief This is the main file for the AVR Meteo project.
 *
 * This file contains the main function which initializes the program,
 * displays a "Hello, World!" message on an LCD screen, and then clears
 * the screen every second to display the message again.
 */


#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include "main.h"
#include "uart.h"
#include "i2c.h"
#include "lcd1602.h"
#include "ds3231.h"
#include "dht22.h"
#include "bmp180.h"
#include "pwm.h"
#include "adc.h"
#include "buzzer.h"


#define MODE_BUTTON_PIN PD2
#define DISPLAY_0 0
#define DEFAULT_BUTTON 0
#define MODE_BUTTON 1
#define BACKLIGHT_PIN PD3  // pin for PWM control (display light intensity)
#define PIN_PHOTOREZISTOR 2 //A2

volatile int8_t current_display_mode = DISPLAY_0;
volatile uint8_t previous_display_mode = 0; // Track previous display mode for switching

volatile uint8_t button_pressed_flag = DEFAULT_BUTTON;
volatile uint8_t update_flag = 0; // New flag to control display updates


// Power management variables
volatile uint32_t backlight_timer = 0;
volatile uint32_t sensor_read_timer = 0;
volatile uint8_t backlight_state = 1;  // 1 = on, 0 = off
volatile uint8_t last_sensor_read_minute = 0xFF;  // Track last sensor read time
volatile uint8_t last_time_read_minute = 0xFF;  // Track last time read
volatile uint8_t system_awake = 1;  // Track if system is awake

// Variables for DS3231
uint8_t time_hour, time_minute, time_second, day_of_week, calendar_day, calendar_month, calendar_year, alarm2_hour, alarm2_minute;
int8_t ds3231_temperature;

// Variables for DHT22 sensor
uint8_t dht22_humidity_int, dht22_humidity_dec, dht22_temperature_int, dht22_temperature_dec;

// Variables for BMP180 sensor
volatile float bmp180_temperature;
volatile float bmp180_pressure;
volatile float pressure_mmHg;


void init_watchdog() {
    // Configure watchdog timer for 8-second timeout
    wdt_enable(WDTO_8S);
    
    // Enable watchdog interrupt
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);  // 8 second timeout
}

// Watchdog interrupt service routine
ISR(WDT_vect) {
    // This will wake up the system if it gets stuck
    system_awake = 1;
    
}


void init_buttons_interrupts() {
    DDRD &= ~((1 << MODE_BUTTON_PIN)); // Set buttons as input
    PORTD |= (1 << MODE_BUTTON_PIN);  // Enable pull-up resistors

    // Enable INT0 (PD2)
    EICRA |= (1 << ISC01); // Trigger on falling edge
    EIMSK |= (1 << INT0);   // Enable INT0 interrupt

}

void timer1_init() {
    /*
                    F_CPU
    OCR1A = ------------------------------- - 1
            Prescaler × Target Frequency   

    Example:
    2. 1 tick per second (1 Hz)
    OCR1A = (16,000,000 / 1024 × 1) - 1 ≈ 15624

    3. 2 ticks per second (2 Hz)
    OCR1A = (16,000,000 / 1024 × 2) - 1 ≈ 31249

    4. 1 ticks per second (1 HZ)
    OCR1A = (8,000,000 / 1024 × 1) - 1 ≈ 7812

    5. 2 ticks per second (2 HZ)
    OCR1A = (8,000,000 / 1024 × 2) - 1 ≈ 15624

    */

    TCCR1B |= (1 << WGM12); // Configure Timer1 in CTC mode    
    //OCR1A = 15624; // Set the value for a 1-second interval (assuming 16MHz clock and prescaler of 1024)    
    //OCR1A = 7812; // Set the value for a 1-second interval (assuming 8MHz clock prescaler of 1024)
    OCR1A = (F_CPU / (PRESCALER * TARGET_FREQ)) - 1;
    TIMSK1 |= (1 << OCIE1A); // Enable Timer1 compare match interrupt
    TCCR1B |= (1 << CS12) | (1 << CS10); // Start Timer1 with prescaler of 1024
}

void stop_timer1() {
    TCCR1B &= ~((1 << CS12) | (1 << CS10)); // Clear the clock source bits to stop Timer1
}

void start_timer1() {
    TCCR1B |= (1 << CS12) | (1 << CS10); // Set the clock source bits to restart Timer1 with a prescaler of 1024
}

ISR(INT0_vect) {
    // interrupt service routine for INT0. Button to change the mode
    _delay_ms(50);  // Simple debouncing
    if (!(PIND & (1 << MODE_BUTTON_PIN))) {        
        //current_mode = (current_mode + 1) % 6;  // Increment the mode     
        button_pressed_flag = MODE_BUTTON; // Set the flag to indicate that the button was pressed 
             
    }
}

ISR(TIMER1_COMPA_vect) {
    //PORTB ^= (1 << PB5); // Toggle LED on PB5 
    update_flag = 1; // Set the flag to indicate that the display has been updated
}

void read_datetime() {
    DS3231_getTime(&time_hour, &time_minute, &time_second, &day_of_week);
    DS3231_getDate(&calendar_day, &calendar_month, &calendar_year);    
    DS3231_getTemperature(&ds3231_temperature);
    DS3231_getAlarm2(&alarm2_hour, &alarm2_minute);
}

void read_sensors(){     
    // Read sensors exactly every 15 seconds
    uint8_t do_read_sensor = (time_second % 10 == 0) ? 1 : 0;

    if (!do_read_sensor) {
        return;
    }

    if (time_hour >= 1 && time_hour <= 6) {
        return;
    }

    // Get the latest sensor data from the DHT22   
    DHT22_Read(&dht22_humidity_int, &dht22_humidity_dec, &dht22_temperature_int, &dht22_temperature_dec); 

     // Get the latest sensor data from the BMP180       
    bmp180_temperature = BMP180_readTemperature(); // Read temperature first (required for pressure compensation)
    bmp180_pressure = BMP180_readPressure();   
    pressure_mmHg = bmp180_pressure * 0.75006; // Convert hPa to mmHg
}

void check_alarm2() {   

    if(day_of_week == SATURDAY) {
        return; // No alarm on Saturday and Sunday
    }
    
    if (alarm2_hour != time_hour){
        return; // No alarm on the same hour
    } 
   
    if (alarm2_minute == time_minute) {
        if (time_second % 15 == 0) {
            playBeep();
        }
    }

    if (alarm2_minute + 1 == time_minute) {
        if (time_second % 10 == 0) {
            playBeep();
        }
    } 

    if (alarm2_minute + 5 == time_minute) {
        if (time_second % 20 == 0) {
            playRingtone2();
        }
    } 

    if (alarm2_minute + 10 == time_minute) {
        if (time_second % 20 == 0) {
            playAlarmBeep();
        }
    } 

    if (alarm2_minute + 11 == time_minute) {
        if (time_second % 15 == 0) {
            playAlarmBeep();
        }
    }         



      
}

void update_display2() {
    // Sensors
    // TODO: display all sensors. temp ds3231, temp dht22, temp bmp180, pressure bmp180, humanidiat dht22.

    char buffer_lcd[20];
    char buffer_float[8]; 

    dtostrf(bmp180_temperature, 2, 1, buffer_float);   
    sprintf(buffer_lcd, "%s%c", buffer_float, 0xDF);      
    lcd_set_cursor(0, 0);        
    lcd_print(buffer_lcd);

    sprintf(buffer_lcd, "%d.%d%c", dht22_temperature_int, dht22_temperature_dec, 0xDF);
    lcd_set_cursor(0, 6);        
    lcd_print(buffer_lcd);

 
    sprintf(buffer_lcd, "%d%cC", ds3231_temperature, 0xDF);
    lcd_set_cursor(0, 12);        
    lcd_print(buffer_lcd);

    //

    sprintf(buffer_lcd, "%d.%d%c", dht22_humidity_int, dht22_humidity_dec, 0x25);
    lcd_set_cursor(1, 0);        
    lcd_print(buffer_lcd);

    dtostrf(pressure_mmHg, 3, 0, buffer_float);   
    sprintf(buffer_lcd, "%smmHg", buffer_float); 
    lcd_set_cursor(1, 9);
    lcd_print(buffer_lcd);

}

void update_display1() {
    // Calendar. full data time with seconds. day, day of week (full name), month (full name)

    //PORTB ^= (1 << PB5); // Toggle LED on PB5 

    char buffer_lcd[20];
    char day_buffer[19];
    char month_buffer[4];

    sprintf(day_buffer, "%s", DayFullMap[day_of_week-1].value);    
    sprintf(month_buffer, "%s", MonthMap[calendar_month-1].value);

    sprintf(buffer_lcd, "%02d:%02d:%02d", time_hour, time_minute, time_second);
    lcd_set_cursor(0, 0);        
    lcd_print(buffer_lcd);

    sprintf(buffer_lcd, "%s",  month_buffer);
    lcd_set_cursor(0, 13);
    lcd_print(buffer_lcd);

    sprintf(buffer_lcd, "%02d %s",  calendar_day,  day_buffer);
    lcd_set_cursor(1, 0);
    lcd_print(buffer_lcd);

    sprintf(buffer_lcd, "%02d", calendar_year);
    lcd_set_cursor(1, 14);
    lcd_print(buffer_lcd);

}

void update_display0() {
    // Main clock (big gigit)

    //PORTB ^= (1 << PB5); // Toggle LED on PB5    
    display_large_time2(time_hour, time_minute, time_second); // Display the time
}

void hello_world(int8_t mode) {
    char buffer_lcd[20];
    lcd_clear();

    switch (mode)
    {
    case 0:
        playBeep();
        sprintf(buffer_lcd, "Hello World");
        lcd_set_cursor(0, 2);        
        lcd_print(buffer_lcd);
        _delay_ms(1000);
        break;
    case 1:
        for (int i = 0; i < 4; i++) {        
            playBeep();
            display_large_digit2(i, 0); // Display the digit in the center
            _delay_ms(400);
        }
        break; 
    case 2:
        for (int i = 3; i >= 0; i--) {        
            playBeep();
            display_large_digit2(i, 0); // Display the digit in the center 
            _delay_ms(400);
        }
        break;   
    default:
        break;
    }

   lcd_clear();
}


int main() { 
    // Set PB5 (pin 13) as output. Built-in LED on Arduino
    DDRB |= (1 << PB5); // Set PB5 (pin 13) as output    

    // Initialize UART
    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));

    // Enable global interrupts
    sei();   

    // Initialize I2C
    i2c_init();   
     

    // Initialize Timer1 for 1-second interrupts
    timer1_init();  
    
    // Initialize Timer2 for buzzer
    initTimer2();

    // Initialize watchdog
    init_watchdog();        

    // Initialize LCD
    lcd_init();    
 
    create_custom_chars();  

    // Initialize DS3231
    DS3231_init();  

    // Initialize bmp180
    BMP180_init(); 

    // Initialize buttons with interrupts
    init_buttons_interrupts();

    // Set initial display mode    
    // DS3231_setTime(15, 47, 0, TUESDAY);
    // DS3231_setDate(23, 9, 25);

    // Set initial Alarm2 to 07:00
    // DS3231_clearAlarm2();
    // DS3231_setAlarm2(7, 0);

    // Initialize ADC
    adc_init();

    // Initialize PWM for backlight control
    //uint16_t light_level;
    //uint8_t brightness;
    pwm_init(BACKLIGHT_PIN); // Initialize PWM for backlight control
    pwm_set_duty_cycle(96); // Set the initial brightness level 
    
    hello_world(0);

    while (1) {         

        wdt_reset(); // Reset the watchdog timer   
        
        read_sensors();

        // Change display every 10 seconds
        // current_display_mode = (time_second % 20 < 10) ? 0 : 1;

        // Change display every 20 seconds
        // current_display_mode = (time_second % 40 < 20) ? 0 : 1;

        // Change display
        // 0 → for 0–39 seconds (40 seconds)
        // 1 → for 40–59 seconds (20 seconds)
        // current_display_mode = (time_second % 60 < 40) ? 0 : 1;

        // Change 3 display. 30, 15, 15 seconds.
        // 0 → for 0–29 seconds (30 seconds)
        // 1 → for 30–44 seconds (15 seconds)
        // 2 → for 45–59 seconds (15 seconds)
        current_display_mode = (time_second % 60 < 30) ? 0 : ((time_second % 60 < 45) ? 1 : 2);

         if (time_hour >= 0 && time_hour <= 6) {
            current_display_mode = 0; // Only display mode 0 at night
        }

        if (alarm2_hour == time_hour && (time_minute >= alarm2_minute  && time_minute <= alarm2_minute + 12)){
            current_display_mode = 0; // Only display mode 0 at alarm time
        }    

        if (button_pressed_flag) {
            // Change the display mode when the button is pressed.

            // Toggle between 0 and 1
            // current_display_mode = (current_display_mode + 1) % 2; 

            // Inverse the current mode
            // current_display_mode = (current_display_mode == 0) ? 1 : 0;
            
            playBeep();
            button_pressed_flag = DEFAULT_BUTTON;  // Reset the flag
        }        

        if (update_flag) {

            read_datetime();               

            check_alarm2();

            if (time_hour >= 21 && time_hour <= 23) {
                pwm_set_duty_cycle(64);
            } else if ((time_hour >= 23 && time_hour <= 24) || (time_hour >= 0 && time_hour <= 6)) {
                pwm_set_duty_cycle(8);
            } else if ((time_hour >= 6 && time_hour <= 7)) {
                pwm_set_duty_cycle(64);
            } else {
                pwm_set_duty_cycle(96);
            }

            if (previous_display_mode != current_display_mode) {
                lcd_clear(); // Clear the display when switching modes
            }

            switch (current_display_mode)
            {
            case 0:
                update_display0();
                break;
            case 1:
                update_display1();
                break;
            case 2:
                update_display2();
                break;
            default:
                update_display0();
                break;
            }
            
            previous_display_mode = current_display_mode;

            update_flag = 0;
        }

        

        // Read the light level from the photoresistor (assuming it's connected to ADC channel 3)
        //light_level = adc_read(PIN_PHOTOREZISTOR); 
        // Map the light level to the brightness value (invert the value for dimming)
        // Assuming light level 0-1023 maps to brightness 0-255        
        // brightness = 255 - (light_level / 4);  // Invert the value for backlight control
        // Bright room (high light_level) = bright display
        // Dark room (low light_level) = dim display
        //brightness = light_level / 20;  // Direct mapping: 0-1023 -> 0-255        
        // Set the backlight brightness
        //pwm_set_duty_cycle(brightness);
       

    }


    return 0;
}
