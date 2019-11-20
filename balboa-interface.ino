//Global variables

s_bbdata bbdata_display, bbdata_controller;

s_status bbstatus;

volatile uint8_t balboa_controller_done = 0, balboa_controller_int = 0, balboa_controller_int_cnt = 0, balboa_display_done = 0;
volatile uint8_t balboa_controller_bit_skipped = 0;
volatile uint32_t balboa_controller_clock_cycle_cnt = 0, balboa_controller_clock_cycle_cnt_measured = 0;


//Function declarations
extern void          ICACHE_RAM_ATTR   isr_0(void);
//extern void          ICACHE_RAM_ATTR   isr_1(void);
extern uint8_t       ICACHE_RAM_ATTR   balboa_controller_comm(uint8_t reset_routine);

//Initialisation

void balboa_interface_init(void)
{
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  pinMode(Controller_Data_Pin_i, INPUT);
  pinMode(Controller_Data_Pin_i, INPUT_PULLUP);
  pinMode(Controller_Button_Data_Pin_o, OUTPUT);
  pinMode(Controller_Clock_Pin_i, INPUT);
  pinMode(Controller_Clock_Pin_i, INPUT_PULLUP);

  pinMode(Display_Data_Pin_o, OUTPUT);
  pinMode(Display_Button_Data_Pin_i, INPUT);
  //pinMode(Display_Button_Data_Pin_i, INPUT_PULLUP);
  pinMode(Display_Clock_Pin_o, OUTPUT);
  pinMode(Debug_Pin_0, OUTPUT);
  pinMode(Debug_Pin_1, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(Controller_Clock_Pin_i), isr_0, RISING);

  balboa_controller_clock_cycle_cnt = ((cpu_freq_mhz() * 1000000) / controller_clock_freq_hz);

  bbdata_controller.buttons = 0x00;

  __digitalWrite(Debug_Pin_0, HIGH); //Default high
  __digitalWrite(Controller_Button_Data_Pin_o, HIGH); //Default high

  bbdata_display.disp[0] = SevenSegmentASCII['3' - 0x20];
  bbdata_display.disp[1] = SevenSegmentASCII['7' - 0x20];
  bbdata_display.disp[2] = SevenSegmentASCII['0' - 0x20];
  bbdata_display.disp[3] = SevenSegmentASCII['C' - 0x20];
  bbdata_display.disp[4] = 0x00;
  bbdata_display.disp[5] = 0x00;

  convert_error_codes_to_sevenseg(balboa_error_codes, (int) (sizeof(balboa_error_codes) / sizeof(balboa_error_codes[0])), balboa_error_codes[0][0].length() + 1);

  bbstatus.temperature_desired = 35.0;

}

//ISR routines

extern void ICACHE_RAM_ATTR isr_0(void)
{
  //__digitalWrite(Debug_Pin_0, HIGH);
  balboa_controller_done = balboa_controller_comm(false);
  balboa_controller_int = 1;
  //__digitalWrite(Debug_Pin_0, LOW);
}

//-------- Commands --------

void balboa_set_desired_temperature(float temperature)
{
  float temperature_difference = temperature - bbstatus.temperature_desired;
  uint8_t button_presses = fabs(temperature_difference) / temp_increment;

  if (temperature_difference > 0)
  {
    balboa_button_press_sequencer(true, Button_Temp_Up, button_presses);
  }
  else
  {
    balboa_button_press_sequencer(true, Button_Temp_Down, button_presses);
  }
  
  bbstatus.temperature_desired = temperature;
}

void balboa_set_lights(uint8_t lights)
{
  uint8_t button_presses = 0;

  if (lights > controller_lights_settings)
  {
    lights = controller_lights_settings;
  }

  if (bbstatus.lights == lights)
  {
    return;
  }
  if (lights > bbstatus.lights)
  {
    button_presses = lights - bbstatus.lights;
  }
  if (lights < bbstatus.lights)
  {
    button_presses = (lights - bbstatus.lights) + controller_lights_settings + 1;
  }

  bbstatus.lights = lights;
  balboa_button_press_sequencer(true, Button_Lights, button_presses);
}

void balboa_set_jets(uint8_t jets)
{
  uint8_t button_presses = 0;

  if (jets > controller_jets_settings)
  {
    jets = controller_jets_settings;
  }

  if (bbstatus.jets == jets)
  {
    return;
  }
  if (jets > bbstatus.jets)
  {
    button_presses = jets - bbstatus.jets;
  }
  if (jets < bbstatus.jets)
  {
    button_presses = (jets - bbstatus.jets) + controller_jets_settings + 1;
  }

  bbstatus.jets = jets;
  balboa_button_press_sequencer(true, Button_Jets, button_presses);
}

void balboa_set_blower(uint8_t blower)
{
  uint8_t button_presses = 0;

  if (blower > controller_blower_settings)
  {
    blower = controller_blower_settings;
  }

  if (bbstatus.blower == blower)
  {
    return;
  }
  if (blower > bbstatus.blower)
  {
    button_presses = blower - bbstatus.blower;
  }
  if (blower < bbstatus.blower)
  {
    button_presses = (blower - bbstatus.blower) + controller_blower_settings + 1;
  }

  bbstatus.blower = blower;
  balboa_button_press_sequencer(true, Button_Blower, button_presses);
}

//-------- Data Processing --------

void balboa_data_process(void)
{
  if (data_feedthrough_enable)
  {
    //Display
    if (bbdata_controller.newdata == 1)
    {
      //display_send_data(bbdata_controller.disp, 0);
      balboa_status_decode();
      bbdata_controller.newdata = 0;
    }
    //Controller
    if (bbdata_display.newdata == 1)
    {
      //controller_send_button(bbdata_display.buttons);
      bbdata_display.newdata = 0;
    }
  }
}

void balboa_button_press_sequencer(uint8_t load_new_command, uint8_t button_command, uint8_t number_of_button_presses)
{
  static uint8_t button_command_buf = 0, number_of_button_presses_buf = 0;
  static unsigned long time_now = 0, time_prv = 0;

  if (load_new_command)
  {
    button_command_buf = button_command;
    number_of_button_presses_buf = number_of_button_presses;
  }
  else
  {
    if (button_press_sequencer_enable)
    {
      time_now = millis();

      if (((time_now - time_prv) > button_press_sequencer_timer) && (number_of_button_presses_buf > 0))
      {
        balboa_controller_send_button(button_command_buf);
        number_of_button_presses_buf--;
        time_prv = millis();
      }
    }
  }
}

//-------- Status Decode --------

void balboa_status_decode(void)
{
  String error_buf;

  bbstatus.temperature = 0;
  bbstatus.temperature_desired = 35.0;
  bbstatus.lights = 0;
  bbstatus.jets = 0;
  bbstatus.blower = 0;
  bbstatus.status = 0;

  error_buf = check_for_error_code(balboa_error_codes, (int) (sizeof(balboa_error_codes) / sizeof(balboa_error_codes[0])), (char *) bbdata_controller.disp, display_digits);

  if (error_buf == "NULL")
  {
    bbstatus.temperature = (sevenseg_to_char(bbdata_controller.disp[0]) - '0') * 10;
    bbstatus.temperature = bbstatus.temperature + (sevenseg_to_char(bbdata_controller.disp[1]) - '0');
    bbstatus.temperature = bbstatus.temperature + ((sevenseg_to_char(bbdata_controller.disp[2]) - '0') * 0.1);
  }
  else
  {
    error_buf.toCharArray(bbstatus.code, sizeof(bbstatus.code));
  }
}

//-------- Controller Communication --------

void balboa_controller_send_button(uint8_t buttons)
{
  bbdata_controller.buttons = buttons;
}

extern uint8_t ICACHE_RAM_ATTR  balboa_controller_comm(uint8_t reset_routine)
{
  static uint8_t    bit_cntr = 0, byte_cntr = 0, bit_data = 0, data = 0, data_write_holdoff = 0, newdata = 0, button_data = 0, button_send_data = 0, button_data_repeats = 0;
  static uint32_t   cyclecount_now = 0, cyclecount_prv = 0;

  if (reset_routine) //Reset controller_comm routine when controller_timeout_ms of no interrupts has passed
  {
    bit_cntr = 0;
    balboa_controller_bit_skipped = 0;
    balboa_controller_int_cnt = 0;
    byte_cntr = 0;
    data = 0;
    data_write_holdoff = 0;
    newdata = 0;
    __digitalWrite(Controller_Button_Data_Pin_o, HIGH); //Default high
    return 0;
  }

  bit_data = 0x01 & __digitalRead(Controller_Data_Pin_i);

  balboa_controller_int_cnt++;


  if (bit_cntr == 0) //First bit only saves current tick count because clock is not continuous
  {
    cyclecount_prv = ESP.getCycleCount();
  }
  else
  {
    cyclecount_now = ESP.getCycleCount();
    balboa_controller_clock_cycle_cnt_measured = (cyclecount_now - cyclecount_prv);
    if (balboa_controller_clock_cycle_cnt_measured > (balboa_controller_clock_cycle_cnt + 4000)) //clock cycle missed
    {
      balboa_controller_bit_skipped = 1;
    }
    cyclecount_prv = cyclecount_now;
  }


  //Check if previous data has been read, otherwise hold off new writes
  if ((byte_cntr == 0) && (bbdata_controller.newdata))
  {
    data_write_holdoff = 1;
  }

  write_byte_bit(&data, bit_data, bit_cntr);


  //Button Data

  //Set low at 5th byte

  if ((byte_cntr >= (Byte_Count - 2)) && (bit_cntr >= 0))
  {
    __digitalWrite(Controller_Button_Data_Pin_o, 0);
  }

  //Set first button data bit
  if ((byte_cntr >= (Byte_Count - 2)) && (bit_cntr >= (Bit_Count  - 1)))
  {
    //Only clock out button data when no clock cycles where skipped
    if (balboa_controller_bit_skipped == 0)
    {
      __digitalWrite(Controller_Button_Data_Pin_o, 0x01 & bbdata_controller.buttons);
    }
    else
    {
      __digitalWrite(Controller_Button_Data_Pin_o, 0);
    }
  }

  //Clock out rest of button data
  if ((byte_cntr >= (Byte_Count - 1)) && (button_send_data) && (balboa_controller_bit_skipped == 0))
  {
    //Only clock out button data when no clock cycles where skipped
    if (balboa_controller_bit_skipped == 0)
    {
      __digitalWrite(Controller_Button_Data_Pin_o, 0x01 & (bbdata_controller.buttons >> (bit_cntr + 1)));
    }
    else
    {
      __digitalWrite(Controller_Button_Data_Pin_o, 0);
    }
  }


  bit_cntr++;

  if (bit_cntr >= Bit_Count)
  {
    bit_cntr = 0;

    if ((bbdata_controller.disp[byte_cntr] != data) && (data_write_holdoff == 0))
    {
      bbdata_controller.disp[byte_cntr] = data;
      newdata = 1;
    }
    data = 0;

    //Done
    if (byte_cntr >= (Byte_Count - 1))
    {
      byte_cntr = 0;

      //Signal new data
      if (newdata)
      {
        bbdata_controller.newdata = 1;
        newdata = 0;
      }

      //Button push send routine
      if (button_send_data == 0)
      {
        if (bbdata_controller.buttons != 0)
        {
          button_send_data = 1;
          button_data = bbdata_controller.buttons;
        }
      }
      else
      {
        if (button_data_repeats++ == Button_Send_Repeats)
        {
          button_send_data = 0;
          button_data_repeats = 0;
          button_data = 0;
          bbdata_controller.buttons = 0;
        }
      }
      return 1;
    }
    else
    {
      byte_cntr++;
    }
  }
  return 0;
}

void balboa_controller_reset_data_pin(void)
{
  static uint8_t holdoff = 0, first_error = 1;
  static unsigned long time_int = 0;

  __digitalWrite(Debug_Pin_1, LOW);

  //The last rising edge pulse of the external interrupt on the controller clock is used to output data (to slow on falling edge) so button data line must be reset high out of the interrupt routine

  if (balboa_controller_int) //Check if interrupt has been triggered
  {
    balboa_controller_int = 0;
    holdoff = 0;
    time_int = millis();
  }
  else
  {
    if (holdoff == 0)
    {
      if ((millis() - time_int) >= 2)
      {
        holdoff = 1;
        if (balboa_controller_done == 0)
        {
          if (!first_error) //Do not print first error, this one is normal
          {
            __digitalWrite(Debug_Pin_1, HIGH);
            Serial.println("---- ERROR ----");
            Serial.println();
            Serial.print("Controller routine was not ready, interrupt count (should be 42): ");
            Serial.println(balboa_controller_int_cnt);
            Serial.println();
            serial_print_debug();
            Serial.println();
            Serial.println();
          }
          first_error = 0;
        }
        balboa_controller_int_cnt = 0;

        //Reset controller routine
        balboa_controller_comm(true);
      }
    }
  }
}

//-------- Display Communication --------

void balboa_display_send_data(uint8_t data[], uint8_t holdoff_s)
{
  static uint8_t holdoff_enable = 0, holdoff_duration_s = 0;
  static unsigned long time_now = 0, time_holdoff_start = 0;

  time_now = millis();

  if (holdoff_enable)
  {
    if ((time_now - time_holdoff_start) >= (holdoff_duration_s * 1000))
    {
      holdoff_enable = false;
    }
    else
    {
      return;
    }
  }

  if (holdoff_s > 0)
  {
    holdoff_enable = true;
    holdoff_duration_s = holdoff_s;
    time_holdoff_start = millis();
  }

  memcpy(bbdata_display.disp, data, sizeof(bbdata_display.disp));
}

void balboa_display_poll(void)
{
  static uint8_t idle = 0;
  static unsigned long time_now = 0, time_prv = 0, time_poll = 0;

  time_now = millis();

  if (idle) //idle time between transmissions
  {

    if ((time_now - time_prv) >= display_period_ms)
    {
      idle = 0;
    }
  }
  else
  {
    /*if ((time_now - time_poll) > 1)
      {*/
    time_poll = millis();
    if (balboa_display_comm(true, Display_Data_Pin_o, Display_Button_Data_Pin_i, Display_Clock_Pin_o))
    {
      balboa_display_done = 1;
      idle = 1;
      time_prv = millis();
    }
    /*}*/
  }
}

extern uint8_t balboa_display_comm(uint8_t start, uint8_t data_pin_o, uint8_t button_data_pin_i, uint8_t clock_pin_o)
{
  static uint8_t state = 0, bit_cntr = 0, byte_cntr = 0, button_data = 0;

  // 5 states per period (25uS) -> 5uS clock -> 200kHz
  switch (state)
  {
    case 0: //reset variables
      bit_cntr = 0;
      byte_cntr = 0;
      button_data = 0;
      if (start)
        state = 1;
      break;

    case 1: //set data
      if (bit_cntr >= Bit_Count) //neglect last bit
        __digitalWrite(data_pin_o, LOW);
      else
      {
        __digitalWrite(data_pin_o, (0x01 & (bbdata_display.disp[byte_cntr] >> bit_cntr))); //LSB first
      }
      state = 2;
      break;

    case 2: //set clock
      if (bit_cntr >= Bit_Count) //neglect first bit
        __digitalWrite(clock_pin_o, LOW);
      else
      {
        __digitalWrite(clock_pin_o, HIGH);
        //Clock in button data
        if (byte_cntr == (Byte_Count - 1))
        {
          write_byte_bit(&button_data, (0x01 & __digitalRead(button_data_pin_i)), bit_cntr); //Read in bit (LSB first)
        }
      }
      state = 3;
      break;

    case 3: //reset clock
      __digitalWrite(clock_pin_o, LOW);
      if ((bit_cntr == 0) && (byte_cntr == 0) && (__digitalRead(button_data_pin_i) == 0)) //No display detected
      {
        state = 0;
        return 1;
      }
      else
        state = 4;
      break;

    case 4: //reset data
      __digitalWrite(data_pin_o, LOW);
      bit_cntr++;
      state = 1;
      break;
  }

  if (bit_cntr == (Bit_Count + 1))
  {
    if (byte_cntr == (Byte_Count - 1)) //done
    {
      state = 0;
      if ((button_data != 0) && (bbdata_display.newdata == 0)) //Only write new button data when previous is processed
      {
        bbdata_display.buttons = button_data;
        bbdata_display.newdata = 1;
      }
      return 1;
    }
    else
    {
      bit_cntr = 0;
      byte_cntr++;
    }
  }
  return 0;
}

//Helper Functions

extern uint8_t ICACHE_RAM_ATTR write_byte_bit(uint8_t* write_byte, uint8_t write_bit, uint8_t bit_location)
{
  uint8_t temp;

  if (write_bit)
  {
    *write_byte = *write_byte | (1 << bit_location);
  }
  else
  {
    temp = (1 << bit_location);
    temp = ~temp;
    *write_byte = *write_byte & temp;
  }
}

char sevenseg_to_char(uint8_t data)
{
  for (int i = 16; i < sizeof(SevenSegmentASCII); i++) //Skip signs
  {
    if (data == SevenSegmentASCII[i])
    {
      return (char) (i + 0x20);
    }
  }
}

String string_to_sevenseg(String string, int string_length)
{
  char char_array_in[string_length];
  char char_array_out[string_length];

  string.toCharArray(char_array_in, string_length);

  for (int i = 0; i < string_length; i++)
  {
    if (char_array_in[i] >= 0x20)
    {
      char_array_out[i] = SevenSegmentASCII[char_array_in[i] - 0x20];
    }
    else
    {
      char_array_out[i] = 0x00;
    }
  }

  return String(char_array_out);
}

void convert_error_codes_to_sevenseg(String string_array[][2], int array_length, int string_length)
{
  for (int i = 0; i < array_length; i++)
  {
    string_array[i][1] = string_to_sevenseg(string_array[i][0], string_length);
  }
}

String check_for_error_code(String string_array[][2], int array_length, char data[], uint8_t char_array_length)
{
  char buffer_0[char_array_length + 1];
  char buffer_1[char_array_length];
  bool compare_cnt = false;

  for (int i = 0; i < array_length; i++)
  {
    string_array[i][1].toCharArray(buffer_0, char_array_length + 1);
    memcpy(buffer_1, data, char_array_length);
    compare_cnt = true;

    for (int j = 0; j < char_array_length; j++)
    {
      if (buffer_0[j] == 0x00)
      {
        break;
      }
      if (buffer_0[j] != buffer_1[j])
      {
        compare_cnt &= false;
      }
    }
    if (compare_cnt)
    {
      return string_array[i][0];
    }
  }

  return "NULL";
}
