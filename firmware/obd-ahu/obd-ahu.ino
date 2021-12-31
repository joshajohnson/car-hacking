#include <SPI.h>
#include <mcp2515.h>

// #define PRINT_MSG

// Pin Defs
#define CAN_nCS   10

#define SWC_OUTPUT 9

#define LED_0     5
#define LED_1     6
#define LED_2     7
#define LED_3     8

// CAN Signals
#define MSG_STEERING      0x206

#define STATE_UNPRESSED   0x00
#define STATE_PRESSED     0x01
#define STATE_ROTATIONAL  0x08

#define SW_LEFT_KNOB      0x83
#define SW_RIGHT_KNOB     0x93
#define SW_LEFT_CENTER    0x84
#define SW_LEFT_UPPER     0x81
#define SW_LEFT_LOWER     0x82
#define SW_RIGHT_UPPER    0x91
#define SW_RIGHT_LOWER    0x92

#define DIR_LEFT_UP       0xFF
#define DIR_LEFT_DOWN     0x01
#define DIR_RIGHT_DOWN    0xFF
#define DIR_RIGHT_UP      0x01

// Vars
struct can_frame can_msg;

MCP2515 mcp2515(CAN_nCS);

bool new_msg = true;

bool skip_next_knob = false;
bool knob_pressed = false;

uint8_t led0_state = 0;
uint8_t led1_state = 0;
uint8_t led2_state = 0;
uint8_t led3_state = 0;

uint64_t previous_time_ms = 0;
const uint32_t timeout_ms = 2000;

// Kenwood head unit things

enum e_commands
{
  PLAY_PAUSE,
  TRK_PREV,
  TRK_NEXT,
  VOL_DOWN,
  VOL_UP
};

unsigned char cmd_common[] = { 0,1,0,0,0,1,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,0,1,0,0 };
unsigned char play_pause[] = { 0,1,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0 };
unsigned char trk_prev[] = { 0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,0,1,0,1,0 };
unsigned char trk_next[] = { 1,0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,1,0,1,0 };
unsigned char vol_down[] = { 1,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,0,1,0 };
unsigned char vol_up[]   = { 0,0,1,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,1,0 };


// Function Defs
void get_message(void);
void decode_press(void);
void action_switch(void);
void action_knob(void);
void send_command(boolean *command);
void send_bit(boolean bit);

void setup() {
  pinMode(SWC_OUTPUT, OUTPUT);
  digitalWrite(SWC_OUTPUT, LOW);

  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);

  digitalWrite(LED_0, LOW);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);

  Serial.begin(115200);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_95KBPS);
  mcp2515.setNormalMode();

}

void loop() {

  get_message();
  if (new_msg == true)
  {
    decode_press();
    new_msg = false;
  }

  // Knobs have rate limiting as they send messages too quickly
  // If no press for timeout_ms, reset rate limit
  uint64_t current_time_ms = millis();

  if ((current_time_ms - previous_time_ms >= timeout_ms) && knob_pressed)
  {
    // skip_next_knob = false;
    previous_time_ms = current_time_ms;
  }

  // Toggle LED each cycle as a heartbeat
  led0_state ^= 1;
  digitalWrite(LED_0, led0_state);
}

// Reads incoming messages and sets new_msg flag true if it's from the steering wheel switches
void get_message(void)
{
  if (mcp2515.readMessage(&can_msg) == MCP2515::ERROR_OK) {
    if (can_msg.can_id == MSG_STEERING)
    {
      new_msg = true;
#ifdef PRINT_MSG
      Serial.print(can_msg.can_id, HEX); // print ID
      Serial.print(" "); 
      for (int i = 0; i<can_msg.can_dlc; i++)  {  // print the data
        Serial.print(can_msg.data[i],HEX);
        Serial.print(" ");
      }
      Serial.println();
#endif
      // Toggle LED each time a packet is read
      led1_state ^= 1;
      digitalWrite(LED_1, led1_state);
    }

  }
}

// Determines if a message is a knob or switch, and if we should action it
void decode_press(void)
{
  // Action knobs
  if (can_msg.data[1] == SW_LEFT_KNOB || can_msg.data[1] == SW_RIGHT_KNOB)
  {
    action_knob();
    // if (skip_next_knob == false)
    // {
    //   action_knob();
    //   skip_next_knob = true;
    // }
    // else 
    // {
    //   skip_next_knob = false;
    // }
  }
  // Action switches
  else
  {
    action_switch();
  }

}

// Determines which switch is pressed and calls the relevant function
void action_switch(void)
{
  if (can_msg.data[1] == SW_LEFT_CENTER)
  {
    if (can_msg.data[0] == STATE_PRESSED && can_msg.data[2] == STATE_PRESSED)
    {
      send_command(PLAY_PAUSE);
      Serial.println("Left Center Pressed");
    }
  }
  else if (can_msg.data[1] == SW_LEFT_UPPER)
  {
    if (can_msg.data[0] == STATE_PRESSED && can_msg.data[2] == STATE_PRESSED)
    {
      send_command(TRK_NEXT);
      Serial.println("Left Upper Pressed");
    }
  }
  else if (can_msg.data[1] == SW_LEFT_LOWER)
  {
    if (can_msg.data[0] == STATE_PRESSED && can_msg.data[2] == STATE_PRESSED)
    {
      send_command(TRK_PREV);
      Serial.println("Left Lower Pressed");
    }
  }
  else if (can_msg.data[1] == SW_RIGHT_UPPER)
  {
    if (can_msg.data[0] == STATE_PRESSED && can_msg.data[2] == STATE_PRESSED)
    {
      send_command(TRK_NEXT);
      Serial.println("Right Upper Pressed");
    }
  }
  else if (can_msg.data[1] == SW_RIGHT_LOWER)
  {
    if (can_msg.data[0] == STATE_PRESSED && can_msg.data[2] == STATE_PRESSED)
    {
      send_command(TRK_PREV);
      Serial.println("Right Lower Pressed");
    }
  }
}

// Determines which switch is pressed and calls the relevant function
void action_knob(void)
{
  if (can_msg.data[1] == SW_LEFT_KNOB)
  {
    if (can_msg.data[0] == STATE_ROTATIONAL)
    {
      if (can_msg.data[2] == DIR_LEFT_UP)
      {
        send_command(VOL_UP);
        Serial.println("Left Knob Up");
      }
      else if (can_msg.data[2] == DIR_LEFT_DOWN)
      {
        send_command(VOL_DOWN);
        Serial.println("Left Knob Down");
      }
    }
  }

  else if (can_msg.data[1] == SW_RIGHT_KNOB)
  {
    if (can_msg.data[0] == STATE_ROTATIONAL)
    {
      if (can_msg.data[2] == DIR_RIGHT_UP)
      {
        send_command(VOL_UP);
        Serial.println("Right Knob Up");
      }
      else if (can_msg.data[2] == DIR_RIGHT_DOWN)
      {
        send_command(VOL_DOWN);
        Serial.println("Right Knob Down");
      }
    }
  }

  knob_pressed = true;
}

// Functions to send out the NEC commands
void send_command(enum e_commands command)
{
  digitalWrite(LED_2, HIGH);
  Serial.print("Sending Command: ");
  Serial.println(command);

  pinMode(SWC_OUTPUT, INPUT);
  pinMode(SWC_OUTPUT, OUTPUT);
  digitalWrite(SWC_OUTPUT, HIGH);
  delay(10);
  digitalWrite(SWC_OUTPUT, LOW);
  delayMicroseconds(4500);

  for (int i = 0; i < 25; i++)
    send_bit(cmd_common[i]);

  switch (command)
  {
    case PLAY_PAUSE:
    for (int i = 0; i < 24; i++)
    send_bit(play_pause[i]);
    break;

    case TRK_PREV:
    for (int i = 0; i < 24; i++)
    send_bit(trk_prev[i]);
    break;

    case TRK_NEXT:
    for (int i = 0; i < 24; i++)
    send_bit(trk_next[i]);
    break;

    case VOL_UP:
    for (int i = 0; i < 24; i++)
    send_bit(vol_up[i]);
    break;

    case VOL_DOWN:
    for (int i = 0; i < 24; i++)
    send_bit(vol_down[i]);
    break;
  }  
  digitalWrite(LED_2, LOW);
}

void send_bit(boolean bit)
{
  digitalWrite(SWC_OUTPUT, bit ^ 1);
  delayMicroseconds(1000);
  digitalWrite(SWC_OUTPUT, LOW);
  delayMicroseconds(200);
}