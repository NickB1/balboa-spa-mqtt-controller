//Pins
#define Controller_Data_Pin_i           13      //D7
//#define Controller_Button_Data_Pin_o    4     //D2
#define Controller_Button_Data_Pin_o    2       //D4
#define Controller_Clock_Pin_i          12      //D6  Needs to be an external interrupt capable pin, 16 cannot be used on nodemcu

#define Display_Data_Pin_o              5       //D1
#define Display_Button_Data_Pin_i       14      //D5
#define Display_Clock_Pin_o             16      //D0  Also used for LED

#define Debug_Pin_0                     2       //D4  Also used for LED
#define Debug_Pin_1                     0       //D3

//Settings
#define Byte_Count                      6
#define Bit_Count                       7
#define Button_Send_Repeats             5

#define controller_timeout_ms           2
#define controller_clock_freq_hz        1000
#define controller_lights_settings      2
#define controller_jets_settings        2
#define controller_blower_settings      1
#define controller_temperature_default  25

#define display_digits                  4
#define display_timer_int_cycles        1000
#define display_period_ms               10


#define temp_unit                       'C'
#define temp_increment                  0.5
#define temp_max                        47
#define temp_min                        25
#define data_feedthrough_enable         1
#define button_press_sequencer_enable   1
#define button_press_sequencer_timer    500     //ms between buttons presses

//Display Button Values
#define Button_Temp_Up      0x10
#define Button_Temp_Down    0x50
#define Button_Lights        0x70
#define Button_Jets         0x30
#define Button_Blower       0x60

//Struct definitions
struct s_status {
  float   temperature;
  float   temperature_desired;
  uint8_t lights;
  uint8_t jets;
  uint8_t blower;
  uint8_t status;
  char    code[5];
};

struct s_bbdata {
  uint8_t disp[Byte_Count];
  uint8_t buttons;
  uint8_t newdata;
};

String balboa_error_codes[][2] = {
  {"OH  ", ""},
  {"SN1 ", ""},
  {"SN3 ", ""},
  {"FLO ", ""},
  {"COOL", ""},
  {"ICE ", ""},
  {"PD  ", ""},
  {"O3  ", ""},
  {"ILOC", ""}
};
