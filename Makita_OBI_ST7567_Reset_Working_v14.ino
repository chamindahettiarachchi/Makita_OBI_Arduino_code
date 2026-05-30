/*OBI.ino*/
#include "OneWire2.h"
#include "st7567.h"

#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_OK    4
#define RESET_LED 11


bool reset_check_done = false;
uint8_t page = 0;
#define PAGE_COUNT 8
byte led_r1 = 0;
byte led_r2 = 0;
bool led_response_ok = false;



#ifdef ESP_BUILD
#define ONEWIRE_PIN ESP_OW_PIN
#define ENABLE_PIN ESP_EN_PIN
#else
#define ONEWIRE_PIN 6
#define ENABLE_PIN 8
#endif


OneWire makita(ONEWIRE_PIN);


/* -------- BATTERY LOW LEVEL COMMANDS -------- */

bool makita_clear_errors()
{
    byte status_cmd[] = {0xAA, 0x00};
    byte model_cmd[]  = {0xDC, 0x0C};

    byte test_cmd[]   = {0xD9, 0x96, 0xA5};  // TESTMODE
    byte reset_cmd[]  = {0xDA, 0x04};        // CLEAR ERRORS

    byte rsp[50];

    byte test_ack  = 0xFF;
    byte reset_ack = 0xFF;

    // -------- OBI step 1: read status --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_33(status_cmd, 2, rsp, 40);

    digitalWrite(ENABLE_PIN, LOW);
    delay(50);

    // -------- OBI step 2: read model --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_cc(model_cmd, 2, rsp, 16);

    digitalWrite(ENABLE_PIN, LOW);
    delay(50);

    // -------- OBI step 3: enter test mode --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_33(test_cmd, 3, rsp, 9);
    test_ack = rsp[8];

    digitalWrite(ENABLE_PIN, LOW);
    delay(50);

    // -------- OBI step 4: clear errors --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_33(reset_cmd, 2, rsp, 9);
    reset_ack = rsp[8];

    digitalWrite(ENABLE_PIN, LOW);

    // OBI shows both should return 0x06
    if (test_ack == 0x06 && reset_ack == 0x06)
        return true;

    return false;
}

void cmd_and_read_33(byte *cmd, uint8_t cmd_len, byte *rsp, uint8_t rsp_len) {
	int i;
	makita.reset();
	delayMicroseconds(400);
	makita.write(0x33,0);

	for (i=0; i < 8; i++) {
		delayMicroseconds(90);
		rsp[i] = makita.read();
	}

	for (i=0; i < cmd_len; i++) {
		delayMicroseconds(90);
		makita.write(cmd[i],0);
	}

	for (i=8; i < rsp_len + 8; i++) {
		delayMicroseconds(90);
		rsp[i] = makita.read();
	}
}

void cmd_and_read_cc(byte *cmd, uint8_t cmd_len, byte *rsp, uint8_t rsp_len) {
	int i;
	makita.reset();
	delayMicroseconds(400);
	makita.write(0xcc,0);

	for (i=0; i < cmd_len; i++) {
		delayMicroseconds(90);
		makita.write(cmd[i],0);
	}

	for (i=0; i < rsp_len; i++) {
		delayMicroseconds(90);
		rsp[i] = makita.read();
	}
}


void makita_led_on()
{
    byte test_cmd[] = {0xD9, 0x96, 0xA5};
    byte led_cmd[]  = {0xDA, 0x31};
    byte rsp[20];

    led_r1 = 0xFF;   // test mode ACK
    led_r2 = 0xFF;   // LED ON ACK
    led_response_ok = false;

    // -------- OBI request 1: TEST MODE --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_33(test_cmd, 3, rsp, 9);

    // OBI response: 8 ROM bytes + 1 ACK byte
    // ACK is at rsp[8]
    led_r1 = rsp[8];

    digitalWrite(ENABLE_PIN, LOW);

    delay(50);

    // -------- OBI request 2: LED ON --------
    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    cmd_and_read_33(led_cmd, 2, rsp, 9);

    // ACK is at rsp[8]
    led_r2 = rsp[8];

    digitalWrite(ENABLE_PIN, LOW);

    led_response_ok = true;
}

void makita_led_off()
{
    byte test_cmd[] = {0xD9, 0x96, 0xA5};
    byte led_cmd[]  = {0xDA, 0x34};
    byte rsp[20];

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);
        return;
    }

    // Enter test mode
    cmd_and_read_33(test_cmd, 3, rsp, 9);
    delay(100);

    // LED OFF command
    cmd_and_read_33(led_cmd, 2, rsp, 9);

    digitalWrite(ENABLE_PIN, LOW);
}


void show_clear_errors_result_on_lcd()
{
    lcd_clear();
    lcd_string(0, 0, "Clear Errors");
    lcd_string(0, 1, "Sending...");

    bool ok = makita_clear_errors();

    lcd_clear();

    if (ok)
    {
        lcd_string(0, 0, "Clear Errors");
        lcd_string(0, 1, "Command sent");
        lcd_string(0, 2, "Read status");
        lcd_string(0, 3, "OK=Back");
    }
    else
    {
        lcd_string(0, 0, "Clear Errors");
        lcd_string(0, 1, "Failed");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Back");
    }
}

void show_reset_execute_on_lcd()
{
    byte model_cmd[]  = {0xDC, 0x0C};
    byte volt_cmd[]   = {0xD7, 0x00, 0x00, 0xFF};
    byte temp_cmd[]   = {0xD7, 0x0E, 0x00, 0x02};
    byte status_cmd[] = {0xAA, 0x00};

    byte dummy_model[16];
    byte volt_rsp[30];
    byte temp_rsp[3];
    byte status_rsp[48];

    lcd_clear();
    lcd_string(0, 0, "Reset Execute");
    lcd_string(0, 1, "Checking...");

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "Comm Error");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    cmd_and_read_cc(model_cmd, 2, dummy_model, 16);
    delay(50);

    cmd_and_read_cc(volt_cmd, 4, volt_rsp, 30);
    delay(50);

    cmd_and_read_cc(temp_cmd, 4, temp_rsp, 3);
    delay(50);

    cmd_and_read_33(status_cmd, 2, status_rsp, 40);

    digitalWrite(ENABLE_PIN, LOW);

    if ((volt_rsp[0] == 0xFF && volt_rsp[1] == 0xFF) ||
        (temp_rsp[0] == 0xFF && temp_rsp[1] == 0xFF))
    {
        lcd_clear();
        lcd_string(0, 0, "Bad Data");
        lcd_string(0, 1, "Read Failed");
        lcd_string(0, 2, "No reset");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    uint16_t c1_mv = (uint16_t)volt_rsp[2]  | ((uint16_t)volt_rsp[3]  << 8);
    uint16_t c2_mv = (uint16_t)volt_rsp[4]  | ((uint16_t)volt_rsp[5]  << 8);
    uint16_t c3_mv = (uint16_t)volt_rsp[6]  | ((uint16_t)volt_rsp[7]  << 8);
    uint16_t c4_mv = (uint16_t)volt_rsp[8]  | ((uint16_t)volt_rsp[9]  << 8);
    uint16_t c5_mv = (uint16_t)volt_rsp[10] | ((uint16_t)volt_rsp[11] << 8);

    uint16_t min_mv = c1_mv;
    uint16_t max_mv = c1_mv;

    if (c2_mv < min_mv) min_mv = c2_mv;
    if (c3_mv < min_mv) min_mv = c3_mv;
    if (c4_mv < min_mv) min_mv = c4_mv;
    if (c5_mv < min_mv) min_mv = c5_mv;

    if (c2_mv > max_mv) max_mv = c2_mv;
    if (c3_mv > max_mv) max_mv = c3_mv;
    if (c4_mv > max_mv) max_mv = c4_mv;
    if (c5_mv > max_mv) max_mv = c5_mv;

    uint16_t diff_mv = max_mv - min_mv;

    uint16_t temp_10k = (uint16_t)temp_rsp[0] | ((uint16_t)temp_rsp[1] << 8);
    int16_t temp_10c = (int16_t)temp_10k - 2731;
    int16_t temp_c = temp_10c / 10;

    uint8_t lock_nibble = status_rsp[28] & 0x0F;

    bool cells_ok = true;
    bool temp_ok = true;

    if (min_mv < 3000) cells_ok = false;
    if (diff_mv > 50)  cells_ok = false;

    if (temp_c < 0 || temp_c > 50) temp_ok = false;

    if (lock_nibble == 0)
    {
        lcd_clear();
        lcd_string(0, 0, "UNLOCKED");
        lcd_string(0, 1, "No reset");
        lcd_string(0, 2, "needed");
        lcd_string(0, 3, "OK=Back");
        return;
    }

    if (!cells_ok || !temp_ok)
    {
        lcd_clear();
        lcd_string(0, 0, "RESET BLOCKED");

        if (!cells_ok)
            lcd_string(0, 1, "Cell problem");
        else
            lcd_string(0, 1, "Temp problem");

        lcd_string(0, 3, "OK=Back");
        return;
    }

    lcd_clear();
    lcd_string(0, 0, "Clear Errors");
    lcd_string(0, 1, "Sending...");

    bool ok = makita_clear_errors();

    lcd_clear();

    if (ok)
    {
        lcd_string(0, 0, "Clear Errors");
        lcd_string(0, 1, "Command sent");
        lcd_string(0, 2, "Read status");
        lcd_string(0, 3, "OK=Back");
    }
    else
    {
        lcd_string(0, 0, "Clear Errors");
        lcd_string(0, 1, "Failed");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Back");
    }
}

/* -------- LCD PAGE FUNCTIONS -------- */

void show_led_menu_on_lcd()
{
    lcd_clear();
    lcd_string(0, 0, "LED Test");
    lcd_string(0, 1, "OK=LED ON");
    lcd_string(0, 2, "Hold OK=OFF");
    lcd_string(0, 3, "UP/DN=Exit");
}

void show_battery_model_on_lcd()
{
    byte cmd[] = {0xDC, 0x0C};
    byte rsp[16];

    char model[9];

    lcd_clear();
    lcd_string(0, 0, "Reading model");

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "Comm Error");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    cmd_and_read_cc(cmd, 2, rsp, 16);

    // If first read failed, try one more time
    if (rsp[0] == 0xFF && rsp[1] == 0xFF)
    {
        delay(50);
        cmd_and_read_cc(cmd, 2, rsp, 16);
    }

    digitalWrite(ENABLE_PIN, LOW);

    if (rsp[0] == 0xFF && rsp[1] == 0xFF)
{
    lcd_clear();
    lcd_string(0, 0, "Bad Model");
    lcd_string(0, 1, "Read Failed");
    lcd_string(0, 2, "Check pins");
    lcd_string(0, 3, "OK=Retry");
    return;
}

    for (int i = 0; i < 8; i++)
    {
        model[i] = (char)rsp[i];
    }
    model[8] = '\0';

    lcd_clear();
    lcd_string(0, 0, "Model");
    lcd_string(0, 1, model);
    lcd_string(0, 3, "OK=Retry");
}



void show_cell_voltages_on_lcd()
{
    byte cmd[] = {0xD7, 0x00, 0x00, 0xFF};
    byte rsp[30];

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "or Comm Error");
        lcd_string(0, 2, "Check contact");
        lcd_string(0, 3, "OK=Retry");
        return;
    }
    byte model_cmd[] = {0xDC, 0x0C};
    byte dummy_model[16];

    cmd_and_read_cc(model_cmd, 2, dummy_model, 16);
    delay(50);
    
    cmd_and_read_cc(cmd, 4, rsp, 30);

    digitalWrite(ENABLE_PIN, LOW);

    // Check for failed response: all first 12 bytes are 0xFF
    bool all_ff = true;

    for (int i = 0; i < 12; i++)
    {
        if (rsp[i] != 0xFF)
        {
            all_ff = false;
            break;
        }
    }

    if (all_ff)
    {
        lcd_clear();
        lcd_string(0, 0, "Bad Data");
        lcd_string(0, 1, "Read Failed");
        lcd_string(0, 2, "Check contact");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    uint16_t pack_mv = (uint16_t)rsp[0]  | ((uint16_t)rsp[1]  << 8);
    uint16_t c1_mv   = (uint16_t)rsp[2]  | ((uint16_t)rsp[3]  << 8);
    uint16_t c2_mv   = (uint16_t)rsp[4]  | ((uint16_t)rsp[5]  << 8);
    uint16_t c3_mv   = (uint16_t)rsp[6]  | ((uint16_t)rsp[7]  << 8);
    uint16_t c4_mv   = (uint16_t)rsp[8]  | ((uint16_t)rsp[9]  << 8);
    uint16_t c5_mv   = (uint16_t)rsp[10] | ((uint16_t)rsp[11] << 8);

    uint16_t min_mv = c1_mv;
    uint16_t max_mv = c1_mv;

    if (c2_mv < min_mv) min_mv = c2_mv;
    if (c3_mv < min_mv) min_mv = c3_mv;
    if (c4_mv < min_mv) min_mv = c4_mv;
    if (c5_mv < min_mv) min_mv = c5_mv;

    if (c2_mv > max_mv) max_mv = c2_mv;
    if (c3_mv > max_mv) max_mv = c3_mv;
    if (c4_mv > max_mv) max_mv = c4_mv;
    if (c5_mv > max_mv) max_mv = c5_mv;

    uint16_t diff_mv = max_mv - min_mv;

    char line[22];

    lcd_clear();

    sprintf(line, "Pack:%u.%03uV", pack_mv / 1000, pack_mv % 1000);
    lcd_string(0, 0, line);

    sprintf(line, "C1:%u.%03u C2:%u.%03u", c1_mv / 1000, c1_mv % 1000, c2_mv / 1000, c2_mv % 1000);
    lcd_string(0, 1, line);

    sprintf(line, "C3:%u.%03u C4:%u.%03u", c3_mv / 1000, c3_mv % 1000, c4_mv / 1000, c4_mv % 1000);
    lcd_string(0, 2, line);

    if (diff_mv <= 50)
    {
        sprintf(line, "C5:%u.%03u D:%um OK",
                c5_mv / 1000, c5_mv % 1000,
                diff_mv);
    }
    else
    {
        sprintf(line, "C5:%u.%03u D:%um BAD",
                c5_mv / 1000, c5_mv % 1000,
                diff_mv);
    }

    lcd_string(0, 3, line);
   }
/* -------- BUTTON FUNCTIONS -------- */
   
bool button_pressed(uint8_t pin)
{
    if (digitalRead(pin) == LOW)
    {
        delay(30);   // debounce

        if (digitalRead(pin) == LOW)
        {
            while (digitalRead(pin) == LOW);  // wait release
            delay(30);
            return true;
        }
    }

    return false;
}

bool button_long_pressed(uint8_t pin, uint16_t hold_ms)
{
    if (digitalRead(pin) == LOW)
    {
        delay(30);   // debounce

        if (digitalRead(pin) == LOW)
        {
            unsigned long start_time = millis();

            while (digitalRead(pin) == LOW)
            {
                if (millis() - start_time >= hold_ms)
                {
                    while (digitalRead(pin) == LOW);  // wait release
                    delay(30);
                    return true;
                }
            }
        }
    }

    return false;
}

uint8_t button_short_or_long(uint8_t pin, uint16_t hold_ms)
{
    if (digitalRead(pin) == LOW)
    {
        delay(30);   // debounce

        if (digitalRead(pin) == LOW)
        {
            unsigned long start_time = millis();

            while (digitalRead(pin) == LOW)
            {
                if (millis() - start_time >= hold_ms)
                {
                    while (digitalRead(pin) == LOW);  // wait release
                    delay(30);
                    return 2;   // long press
                }
            }

            delay(30);
            return 1;   // short press
        }
    }

    return 0;   // no press
}

/* NOTE:
   Reset command is included.
   It is protected by long press
   and safety checks.
*/

void show_reset_menu_on_lcd()
{
    lcd_clear();
    lcd_string(0, 0, "Reset Menu");
    lcd_string(0, 1, "Guarded");
    lcd_string(0, 2, "Hold OK 3s");
    lcd_string(0, 3, "2x hold OK");
}


void show_about_on_lcd()
{
    lcd_clear();
    lcd_string(0, 0, "FW:Study v11");
    lcd_string(0, 1, "MCU:ATmega328P");
    lcd_string(0, 2, "LCD:ST7567");
    lcd_string(0, 3, "Reset guarded");
}

void show_current_page()
{
    if (page == 0)
    {
        show_summary_on_lcd();
    }
    else if (page == 1)
    {
        show_battery_model_on_lcd();
    }
    else if (page == 2)
    {
        show_cell_voltages_on_lcd();
    }
    else if (page == 3)
    {
        show_temperature_on_lcd();
    }
    else if (page == 4)
    {
        show_status_on_lcd();
    }
    else if (page == 5)
    {
        show_reset_menu_on_lcd();
    }
    else if (page == 6)
    {
        show_about_on_lcd();
    }
    else if (page == 7)
    {
        show_led_menu_on_lcd();
    }
}

void show_temperature_on_lcd()
{
    byte temp_cmd[]  = {0xD7, 0x0E, 0x00, 0x02};
    byte model_cmd[] = {0xDC, 0x0C};

    byte rsp[3];
    byte dummy_model[16];

    char line[22];

    lcd_clear();
    lcd_string(0, 0, "Reading T...");

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "Comm Error");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    // Warm-up / identify command first
    cmd_and_read_cc(model_cmd, 2, dummy_model, 16);
    delay(50);

    // Now read temperature
    cmd_and_read_cc(temp_cmd, 4, rsp, 3);

    digitalWrite(ENABLE_PIN, LOW);

    if (rsp[0] == 0xFF && rsp[1] == 0xFF)
    {
        lcd_clear();
        lcd_string(0, 0, "Bad T Data");
        lcd_string(0, 1, "Read Failed");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    uint16_t temp_10k = (uint16_t)rsp[0] | ((uint16_t)rsp[1] << 8);

    int16_t temp_10c = (int16_t)temp_10k - 2731;

    int16_t temp_c_whole = temp_10c / 10;
    int16_t temp_c_dec   = temp_10c % 10;

    if (temp_c_dec < 0) temp_c_dec = -temp_c_dec;

    lcd_clear();
    lcd_string(0, 0, "Temp");

    sprintf(line, "Raw:%u", temp_10k);
    lcd_string(0, 1, line);

    sprintf(line, "T:%d.%dC", temp_c_whole, temp_c_dec);
    lcd_string(0, 2, line);

    lcd_string(0, 3, "OK=Retry");
}

void show_summary_on_lcd()
{

    lcd_clear();
    lcd_string(0, 0, "Reading...");
    lcd_string(0, 1, "Please wait");


    byte model_cmd[] = {0xDC, 0x0C};
    byte volt_cmd[]  = {0xD7, 0x00, 0x00, 0xFF};
    byte temp_cmd[]  = {0xD7, 0x0E, 0x00, 0x02};

    byte model_rsp[16];
    byte volt_rsp[30];
    byte temp_rsp[3];

    char model[9];
    char line[22];

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    // Check if battery responds on 1-Wire line
    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "or Comm Error");
        lcd_string(0, 2, "Check contact");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    cmd_and_read_cc(model_cmd, 2, model_rsp, 16);

    // If first model read failed, try one more time
    if (model_rsp[0] == 0xFF && model_rsp[1] == 0xFF)
    {
        delay(50);
        cmd_and_read_cc(model_cmd, 2, model_rsp, 16);
    }

    delay(50);

cmd_and_read_cc(volt_cmd, 4, volt_rsp, 30);

   cmd_and_read_cc(temp_cmd, 4, temp_rsp, 3);

// If temperature looks invalid, try one more time
uint16_t test_temp_10k = (uint16_t)temp_rsp[0] | ((uint16_t)temp_rsp[1] << 8);
int16_t test_temp_10c = (int16_t)test_temp_10k - 2731;
int16_t test_temp_c = test_temp_10c / 10;

if (test_temp_c < 0 || test_temp_c > 60)
{
    delay(50);
    cmd_and_read_cc(temp_cmd, 4, temp_rsp, 3);
}

digitalWrite(ENABLE_PIN, LOW);


    // Check voltage response
if (volt_rsp[0] == 0xFF && volt_rsp[1] == 0xFF)
{
    lcd_clear();
    lcd_string(0, 0, "Bad V Data");
    lcd_string(0, 1, "Read Failed");
    lcd_string(0, 2, "Check pins");
    lcd_string(0, 3, "OK=Retry");
    return;
}

// Check temperature response
if (temp_rsp[0] == 0xFF && temp_rsp[1] == 0xFF)
{
    lcd_clear();
    lcd_string(0, 0, "Bad T Data");
    lcd_string(0, 1, "Read Failed");
    lcd_string(0, 2, "Check pins");
    lcd_string(0, 3, "OK=Retry");
    return;
}

// Check model response
if (model_rsp[0] == 0xFF && model_rsp[1] == 0xFF)
{
    lcd_clear();
    lcd_string(0, 0, "Bad Model");
    lcd_string(0, 1, "Read Failed");
    lcd_string(0, 2, "Check pins");
    lcd_string(0, 3, "OK=Retry");
    return;
}

    // Model text
    for (int i = 0; i < 8; i++)
    {
        model[i] = (char)model_rsp[i];
    }
    model[8] = '\0';

    // Pack and cell voltages
    uint16_t pack_mv = (uint16_t)volt_rsp[0]  | ((uint16_t)volt_rsp[1]  << 8);
    uint16_t c1_mv   = (uint16_t)volt_rsp[2]  | ((uint16_t)volt_rsp[3]  << 8);
    uint16_t c2_mv   = (uint16_t)volt_rsp[4]  | ((uint16_t)volt_rsp[5]  << 8);
    uint16_t c3_mv   = (uint16_t)volt_rsp[6]  | ((uint16_t)volt_rsp[7]  << 8);
    uint16_t c4_mv   = (uint16_t)volt_rsp[8]  | ((uint16_t)volt_rsp[9]  << 8);
    uint16_t c5_mv   = (uint16_t)volt_rsp[10] | ((uint16_t)volt_rsp[11] << 8);

    uint16_t min_mv = c1_mv;
    uint16_t max_mv = c1_mv;

    if (c2_mv < min_mv) min_mv = c2_mv;
    if (c3_mv < min_mv) min_mv = c3_mv;
    if (c4_mv < min_mv) min_mv = c4_mv;
    if (c5_mv < min_mv) min_mv = c5_mv;

    if (c2_mv > max_mv) max_mv = c2_mv;
    if (c3_mv > max_mv) max_mv = c3_mv;
    if (c4_mv > max_mv) max_mv = c4_mv;
    if (c5_mv > max_mv) max_mv = c5_mv;

    uint16_t diff_mv = max_mv - min_mv;

    bool low_cell = false;

    if (min_mv < 3000)
    {
        low_cell = true;
    }

    // Temperature
    uint16_t temp_10k = (uint16_t)temp_rsp[0] | ((uint16_t)temp_rsp[1] << 8);
    int16_t temp_10c = (int16_t)temp_10k - 2731;

    int16_t temp_c_whole = temp_10c / 10;
    int16_t temp_c_dec = temp_10c % 10;
    if (temp_c_dec < 0) temp_c_dec = -temp_c_dec;

    if (temp_c_whole < 0 || temp_c_whole > 60)
{
    lcd_clear();
    lcd_string(0, 0, "Bad T Data");
    lcd_string(0, 1, "Temp invalid");
    lcd_string(0, 2, "OK=Retry");
    return;
}

    lcd_clear();

    lcd_string(0, 0, model);

    sprintf(line, "P:%u.%03uV", pack_mv / 1000, pack_mv % 1000);
    lcd_string(0, 1, line);

            if (low_cell)
        {
            sprintf(line, "LOW CELL!");
        }
        else if (diff_mv <= 50)
        {
            sprintf(line, "D:%um OK", diff_mv);
        }
        else
        {
            sprintf(line, "D:%um BAD", diff_mv);
        }

        lcd_string(0, 2, line);

            sprintf(line, "T:%d.%dC", temp_c_whole, temp_c_dec);
            lcd_string(0, 3, line);
        }

        uint8_t nibble_swap(uint8_t b)
        {
            return ((b & 0x0F) << 4) | ((b & 0xF0) >> 4);
        }

    void show_status_on_lcd()
    {
    byte cmd[] = {0xAA, 0x00};
    byte rsp[48];

    char line[22];

    lcd_clear();
    lcd_string(0, 0, "Reading status");

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "or Comm Error");
        lcd_string(0, 2, "Check contact");
        lcd_string(0, 3, "OK=Retry");
        return;
    }
    byte model_cmd[] = {0xDC, 0x0C};
    byte dummy_model[16];

    cmd_and_read_cc(model_cmd, 2, dummy_model, 16);
    delay(50);
    cmd_and_read_33(cmd, 2, rsp, 40);

    digitalWrite(ENABLE_PIN, LOW);

    // Check bad response
    bool all_ff = true;
    for (int i = 0; i < 40; i++)
    {
        if (rsp[i] != 0xFF)
        {
            all_ff = false;
            break;
        }
    }

    if (all_ff)
    {
        lcd_clear();
        lcd_string(0, 0, "Bad Data");
        lcd_string(0, 1, "Read Failed");
        lcd_string(0, 2, "Check contact");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    uint8_t status_code = rsp[27];
    uint8_t lock_nibble = rsp[28] & 0x0F;
    uint8_t capacity_raw = nibble_swap(rsp[24]);
    uint8_t battery_type = nibble_swap(rsp[19]);

    lcd_clear();

    if (lock_nibble > 0)
    lcd_string(0, 0, "LOCKED");
    else
        lcd_string(0, 0, "UNLOCKED");

    sprintf(line, "S:0x%02X", status_code);
    lcd_string(0, 1, line);

    sprintf(line, "Cap:%u.%uAh", capacity_raw / 10, capacity_raw % 10);
    lcd_string(0, 2, line);

    sprintf(line, "Ty:%u", battery_type);
    lcd_string(0, 3, line);
    }

    void show_reset_check_on_lcd()
{
    byte model_cmd[]  = {0xDC, 0x0C};
    byte volt_cmd[]   = {0xD7, 0x00, 0x00, 0xFF};
    byte temp_cmd[]   = {0xD7, 0x0E, 0x00, 0x02};
    byte status_cmd[] = {0xAA, 0x00};

    byte dummy_model[16];
    byte volt_rsp[30];
    byte temp_rsp[3];
    byte status_rsp[48];

    char line[22];

    lcd_clear();
    lcd_string(0, 0, "Reset Check");
    lcd_string(0, 1, "Reading...");

    digitalWrite(ENABLE_PIN, HIGH);
    delay(400);

    if (makita.reset() == 0)
    {
        digitalWrite(ENABLE_PIN, LOW);

        lcd_clear();
        lcd_string(0, 0, "No Battery");
        lcd_string(0, 1, "Comm Error");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    // Warm-up read
    cmd_and_read_cc(model_cmd, 2, dummy_model, 16);
    delay(50);

    // Read cell voltages
    cmd_and_read_cc(volt_cmd, 4, volt_rsp, 30);
    delay(50);

    // Read temperature
    cmd_and_read_cc(temp_cmd, 4, temp_rsp, 3);
    delay(50);

    // Read lock/status data
    cmd_and_read_33(status_cmd, 2, status_rsp, 40);

    digitalWrite(ENABLE_PIN, LOW);

    // Basic bad data check
    if ((volt_rsp[0] == 0xFF && volt_rsp[1] == 0xFF) ||
        (temp_rsp[0] == 0xFF && temp_rsp[1] == 0xFF))
    {
        lcd_clear();
        lcd_string(0, 0, "Bad Data");
        lcd_string(0, 1, "Read Failed");
        lcd_string(0, 2, "Check pins");
        lcd_string(0, 3, "OK=Retry");
        return;
    }

    uint16_t c1_mv = (uint16_t)volt_rsp[2]  | ((uint16_t)volt_rsp[3]  << 8);
    uint16_t c2_mv = (uint16_t)volt_rsp[4]  | ((uint16_t)volt_rsp[5]  << 8);
    uint16_t c3_mv = (uint16_t)volt_rsp[6]  | ((uint16_t)volt_rsp[7]  << 8);
    uint16_t c4_mv = (uint16_t)volt_rsp[8]  | ((uint16_t)volt_rsp[9]  << 8);
    uint16_t c5_mv = (uint16_t)volt_rsp[10] | ((uint16_t)volt_rsp[11] << 8);

    uint16_t min_mv = c1_mv;
    uint16_t max_mv = c1_mv;

    if (c2_mv < min_mv) min_mv = c2_mv;
    if (c3_mv < min_mv) min_mv = c3_mv;
    if (c4_mv < min_mv) min_mv = c4_mv;
    if (c5_mv < min_mv) min_mv = c5_mv;

    if (c2_mv > max_mv) max_mv = c2_mv;
    if (c3_mv > max_mv) max_mv = c3_mv;
    if (c4_mv > max_mv) max_mv = c4_mv;
    if (c5_mv > max_mv) max_mv = c5_mv;

    uint16_t diff_mv = max_mv - min_mv;

    uint16_t temp_10k = (uint16_t)temp_rsp[0] | ((uint16_t)temp_rsp[1] << 8);
    int16_t temp_10c = (int16_t)temp_10k - 2731;
    int16_t temp_c = temp_10c / 10;

    uint8_t lock_nibble = status_rsp[28] & 0x0F;

    bool cells_ok = true;
    bool temp_ok = true;

    if (min_mv < 3000) cells_ok = false;
    if (diff_mv > 50)  cells_ok = false;

    if (temp_c < 0 || temp_c > 50) temp_ok = false;

    lcd_clear();
    lcd_string(0, 0, "Reset Check");

    if (cells_ok)
        sprintf(line, "Cell OK D:%um", diff_mv);
    else
        sprintf(line, "Cell BAD D:%um", diff_mv);

    lcd_string(0, 1, line);

    if (temp_ok)
        sprintf(line, "T OK:%dC", temp_c);
    else
        sprintf(line, "T BAD:%dC", temp_c);

    lcd_string(0, 2, line);

   if (lock_nibble == 0)
    {
        lcd_string(0, 3, "UNLK:No reset");
    }
    else if (cells_ok && temp_ok)
    {
        lcd_string(0, 3, "LOCK:Study OK");
    }
    else
    {
        lcd_string(0, 3, "RESET BLOCKED");
    }
}

/* -------- SETUP -------- */

        void setup()
        {
        pinMode(BTN_UP, INPUT_PULLUP);
        pinMode(BTN_DOWN, INPUT_PULLUP);
        pinMode(BTN_OK, INPUT_PULLUP);
        
        pinMode(RESET_LED, OUTPUT);


     

        pinMode(ENABLE_PIN, OUTPUT);
        digitalWrite(ENABLE_PIN, LOW);

        lcd_init();
        delay(10);

        lcd_clear();
        lcd_string(0, 0, "Makita Tester");
        lcd_string(0, 1, "Study FW v11");
        lcd_string(0, 2, "Starting...");
        delay(1000);

        page = 0;              // start from summary page
        show_current_page();
}



/* -------- MAIN LOOP -------- */


void loop()
{     
    if (button_pressed(BTN_UP))
    {
        if (page == 0)
            page = PAGE_COUNT - 1;
        else
            page--;
        show_current_page();
        reset_check_done = false;
    }

    if (button_pressed(BTN_DOWN))
    {
        page++;

        if (page >= PAGE_COUNT)
            page = 0;

        show_current_page();
        reset_check_done = false;
    }

   if (page == 5)
{
    if (button_long_pressed(BTN_OK, 3000))
    {
        if (reset_check_done == false)
        {
            show_reset_check_on_lcd();     // first long press: check only
            reset_check_done = true;
        }
        else
        {
            show_reset_execute_on_lcd();   // second long press: safe wrapper
            reset_check_done = false;
        }
    }
}
    else if (page == 7)
    {
        uint8_t press_type = button_short_or_long(BTN_OK, 1500);

        if (press_type == 1)
        {
            makita_led_on();

        char line[22];

        lcd_clear();
        lcd_string(0, 0, "LED ON Debug");

        if (led_response_ok)
        {
           // sprintf(line, "R:%02X %02X", led_r1, led_r2);
           sprintf(line, "A:%02X %02X", led_r1, led_r2);
            lcd_string(0, 1, line);
        }
        else
        {
            lcd_string(0, 1, "No response");
        }

lcd_string(0, 3, "OK=ON Hold=OFF");
        }
        else if (press_type == 2)
        {
            makita_led_off();

            lcd_clear();
            lcd_string(0, 0, "LED Test");
            lcd_string(0, 1, "LED OFF sent");
            lcd_string(0, 3, "OK=ON Hold=OFF");
        }
    }
    else
    {
        if (button_pressed(BTN_OK))
        {
            show_current_page();         // refresh current page
        }
    }
}
