#include "ascii_to_7seg.h"
#include "balboa-interface.h"

//Balboa interface variables

extern s_status bbstatus;
extern s_bbdata bbdata_display, bbdata_controller;
extern volatile uint32_t balboa_controller_clock_cycle_cnt, balboa_controller_clock_cycle_cnt_measured;
extern volatile uint8_t balboa_controller_bit_skipped;

//Function declarations

extern void          IRAM_ATTR __digitalWrite(uint8_t pin, uint8_t val);
extern int           IRAM_ATTR __digitalRead(uint8_t pin);
extern uint8_t       IRAM_ATTR   write_byte_bit(uint8_t* write_byte, uint8_t write_bit, uint8_t bit_location);

void setup()
{
  Serial.begin(115200);
  delay(10);

  balboa_interface_init();

  mqtt_init();
  mqtt_connect();

  serial_print_header();
}

void loop()
{
  serial_read();

  balboa_display_poll();
  balboa_controller_reset_data_pin();
  balboa_button_press_sequencer(false, 0, 0);
  balboa_data_process();

  //mqtt_loop();
}

//Serial Debug

void serial_print_header(void)
{
  Serial.println();
  Serial.println("Balboa Spa - MQTT Controller by Nick Blommen");
  Serial.println();
  Serial.println("Button Presses:");
  Serial.println();
  Serial.println("0: Temperature Up");
  Serial.println("1: Temperature Down");
  Serial.println("2: Toggle Lighting");
  Serial.println("3: Toggle Jets");
  Serial.println("4: Toggle Blower");
  Serial.println();
  Serial.println("Options:");
  Serial.println();
  Serial.println("5: Set Temperature (e.g: 5 37.0;)");
  Serial.println("6: Set Lighting (e.g: 6 2;)");
  Serial.println("7: Set Jets (e.g: 7 2;)");
  Serial.println("8: Set Blower (e.g: 8 1;)");
  Serial.println("9: Print Status");
  Serial.println("a: Debug");
  Serial.println("b: Print Error Codes");
  Serial.println();
}

void serial_read(void)
{
  if (Serial.available())
  {
    int inByte = Serial.read();
    switch (inByte)
    {
      case '0':
        balboa_controller_send_button(Button_Temp_Up);
        Serial.println("Button Press: Temperature Up");
        break;

      case '1':
        balboa_controller_send_button(Button_Temp_Down);
        Serial.println("Button Press: Temperature Down");
        break;

      case '2':
        balboa_controller_send_button(Button_Lights);
        Serial.println("Button Press: Toggle Lighting");
        break;

      case '3':
        balboa_controller_send_button(Button_Jets);
        Serial.println("Button Press: Toggle Jets");
        break;

      case '4':
        balboa_controller_send_button(Button_Blower);
        Serial.println("Button Press: Toggle Blower");
        break;

      case '5':
        balboa_set_desired_temperature(Serial.parseFloat());
        break;

      case '6':
        balboa_set_lights(Serial.parseInt());
        break;

      case '7':
        balboa_set_jets(Serial.parseInt());
        break;

      case '8':
        balboa_set_blower(Serial.parseInt());
        break;

      case '9':
        Serial.print("Current Water Temperature: ");
        Serial.print(bbstatus.temperature, 2);
        Serial.println("°C");
        Serial.print("Desired Water Temperature: ");
        Serial.print(bbstatus.temperature_desired, 2);
        Serial.println("°C");
        Serial.print("Lights: ");
        Serial.println(bbstatus.lights);
        Serial.print("Jets: ");
        Serial.println(bbstatus.jets);
        Serial.print("Blower: ");
        Serial.println(bbstatus.blower);
        Serial.print("Last Code: ");
        Serial.println(bbstatus.code);
        break;

      case 'a':
        serial_print_debug();
        break;

      case 'b':
        serial_print_error_codes(balboa_error_codes, (int) (sizeof(balboa_error_codes) / sizeof(balboa_error_codes[0])));
        break;

      default:
        break;
    }
    Serial.println();
  }
}

void serial_print_debug(void)
{
  char DisplayString[20] = {0};
  char ControllerString[20] = {0};
  sprintf(DisplayString, "%02X:%02X:%02X:%02X:%02X:%02X", bbdata_display.disp[0], bbdata_display.disp[1], bbdata_display.disp[2], bbdata_display.disp[3], bbdata_display.disp[4], bbdata_display.disp[5]);
  sprintf(ControllerString, "%02X:%02X:%02X:%02X:%02X:%02X", bbdata_controller.disp[0], bbdata_controller.disp[1], bbdata_controller.disp[2], bbdata_controller.disp[3], bbdata_controller.disp[4], bbdata_controller.disp[5]);

  Serial.println("---- Debug Data ----");
  Serial.println();
  Serial.println("Controller:");
  Serial.println();
  Serial.print("Data: ");
  Serial.println(ControllerString);
  Serial.print("Button data: ");
  Serial.println(bbdata_controller.buttons, HEX);
  Serial.print("Clock cycle count calculated: ");
  Serial.println(balboa_controller_clock_cycle_cnt);
  Serial.print("Clock cycle count measured: ");
  Serial.println(balboa_controller_clock_cycle_cnt_measured);
  Serial.print("Bit skip detection: ");
  Serial.println(balboa_controller_bit_skipped);
  Serial.println();
  Serial.println("Display:");
  Serial.println();
  Serial.print("Data: ");
  Serial.println(DisplayString);
  Serial.print("Button data: ");
  Serial.println(bbdata_display.buttons, HEX);
}

void serial_print_error_codes(String string_array[][2], int array_length)
{
  char char_buffer_1[5];
  char char_buffer_2[5];

  for (int i = 0; i < array_length; i++)
  {
    string_array[i][0].toCharArray(char_buffer_1, string_array[i][0].length() + 1);

    Serial.print(string_array[i][0]);
    Serial.print(" - ");
    Serial.print(char_buffer_1[0], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_1[1], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_1[2], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_1[3], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_1[4], HEX);

    string_array[i][1].toCharArray(char_buffer_2, string_array[i][1].length() + 1);

    Serial.print(" ---- Converted ---- ");
    Serial.print(char_buffer_2[0], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_2[1], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_2[2], HEX);
    Serial.print(" ");
    Serial.print(char_buffer_2[3], HEX);
    Serial.print(" ");
    Serial.println(char_buffer_2[4], HEX);
  }
}

//Helper Functions

extern void IRAM_ATTR __digitalWrite(uint8_t pin, uint8_t val) {
  //pwm_stop_pin(pin);
  if (pin < 16) {
    if (val) GPOS = (1 << pin);
    else GPOC = (1 << pin);
  } else if (pin == 16) {
    if (val) GP16O |= 1;
    else GP16O &= ~1;
  }
}

extern int IRAM_ATTR __digitalRead(uint8_t pin) {
  //pwm_stop_pin(pin);
  if (pin < 16) {
    return GPIP(pin);
  } else if (pin == 16) {
    return GP16I & 0x01;
  }
  return 0;
}

uint8_t cpu_freq_mhz()
{
  return ESP.getCpuFreqMHz();
}
