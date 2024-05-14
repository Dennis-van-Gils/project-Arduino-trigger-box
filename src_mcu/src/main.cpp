/*------------------------------------------------------------------------------
Arduino trigger box

A configurable TTL pulse train generator on digital outputs D05 and D06. Can be
used to e.g. trigger (Ximea) cameras to acquire pictures in sync with each
other using the camera's trigger-in port.

    <5ms>
    ┌───┐      ┌───┐      ┌───┐
    │   │      │   │      │   │
    │   │      │   │      │   │
    ┘   └──────┘   └──────┘   └────── --> T_meas
    <    DT    >

  * The pulse period `DT` can be set from 10 msec upwards to hours
    with a resolution of 1 msec.
  * The duration of the pulse train `T_meas`, i.e. the measurement
    time, can be set up to a maximum of 49.7 days.
  * The RGB LED indicates the status.
      - Blue : Idle
      - Green: Running pulse train
  * The other on-board LED #13 will flash red with each pulse.

### Hardware
  Either:
  * Adafruit Feather M4 Express
  * Adafruit TermBlock FeatherWing #2926
  * 74AHCT125 'Quad Level-Shifter': To increase 3.3 V digital out of pins D05,
    D06 and D09 to 5 V.

  Or:
  * Arduino Uno (does not have onboard RGB LED but is native 5 V logic)

https://github.com/Dennis-van-Gils/project-Arduino-trigger-box

Dennis van Gils
14-05-2024
------------------------------------------------------------------------------*/

#include "DvG_SerialCommand.h"
#include "Streaming.h"
#include <Arduino.h>

#ifdef _VARIANT_FEATHER_M4_
#include "Adafruit_NeoPixel.h"
#endif

#define PIN_CAM_1 5
#define PIN_CAM_2 6
#define PULSE_WIDTH 5 // [msec]

uint32_t now = 0;   // [msec] Will keep track of Arduino time
uint32_t DT = 1000; // [msec]

#ifdef _VARIANT_FEATHER_M4_
uint32_t T_meas = 8 * 3600 * 1000; // [msec]
#endif

bool f_running = false; // Is pulse train running?
uint32_t t_start = 0;   // Starting time of the pulse train [msec]
uint32_t pulse_idx = 0; // Pulse counter
bool f_HI = false;      // Is the pulse currently in the high state?
uint32_t t_HI = 0;      // Starting time of the high state [msec]

// Character buffer for formatted time string
const uint8_t BUFLEN_TIME = 21;
char buf_time[BUFLEN_TIME] = "";

// Character buffer for global messages
const uint8_t BUFLEN = 64;
char buf[BUFLEN] = "";

// Neopixel
#ifdef _VARIANT_FEATHER_M4_
#define NEO_BRIGHTNESS 3 // Brightness level [0 - 255]
Adafruit_NeoPixel neo(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

// Instantiate serial command listener
#define Ser Serial
DvG_SerialCommand sc(Ser);

/*------------------------------------------------------------------------------
  functions
------------------------------------------------------------------------------*/

void format_msecs(uint32_t all_msecs) {
  uint32_t all_secs = all_msecs / 1000;
  uint32_t rem_secs = all_secs % 3600;
  uint16_t h = all_secs / 3600;
  uint16_t m = rem_secs / 60;
  uint16_t s = rem_secs % 60;
  uint16_t u = all_msecs % 1000;

  snprintf(buf_time, BUFLEN_TIME, "%02d:%02d:%02d.%03d", h, m, s, u);
}

void go_HI() {
  static const uint8_t BUF_LEN = 64;
  static char buf[BUF_LEN] = "";

  f_HI = true;
  pulse_idx += 1;
  digitalWrite(PIN_CAM_1, HIGH);
  digitalWrite(PIN_CAM_2, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  format_msecs(now - t_start);
  snprintf(buf, BUF_LEN, "# %ld @ t = %s", pulse_idx, buf_time);
  Ser.println(buf);
}

void go_LO() {
  f_HI = false;
  digitalWrite(PIN_CAM_1, LOW);
  digitalWrite(PIN_CAM_2, LOW);
  digitalWrite(LED_BUILTIN, LOW);
}

void start_train() {
  Ser.println("Pulse train started.");
  now = millis();
  t_start = t_HI = now;
  pulse_idx = 0;
  go_HI();

#ifdef _VARIANT_FEATHER_M4_
  neo.setPixelColor(0, neo.Color(0, 255, 0)); // Green: running
  neo.show();
#endif
}

void stop_train() {
  Ser.println("Pulse train stopped.");
  go_LO();

#ifdef _VARIANT_FEATHER_M4_
  neo.setPixelColor(0, neo.Color(0, 0, 255)); // Blue: idle
  neo.show();
#endif
}

/*------------------------------------------------------------------------------
  setup
------------------------------------------------------------------------------*/

void setup() {
  Ser.begin(9600);

  pinMode(PIN_CAM_1, OUTPUT);
  pinMode(PIN_CAM_2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  go_LO();

#ifdef _VARIANT_FEATHER_M4_
  neo.begin();
  neo.setPixelColor(0, neo.Color(0, 0, 255)); // Blue: idle
  neo.setBrightness(NEO_BRIGHTNESS);
  neo.show();
#endif
}

/*------------------------------------------------------------------------------
  loop
------------------------------------------------------------------------------*/

void loop() {
  char *strCmd; // Incoming serial command string

  if (sc.available()) {
    strCmd = sc.getCmd();

    if (strcmp(strCmd, "?") == 0) {
      Ser.println("Current settings:");
      format_msecs(DT);
      Ser.print("  DT     = ");
      Ser.println(buf_time);

#ifdef _VARIANT_FEATHER_M4_
      format_msecs(T_meas);
      Ser.print("  T_meas = ");
      Ser.println(buf_time);
#endif

    } else if ((strncmp(strCmd, "DT", 2) == 0) ||
               (strncmp(strCmd, "dt", 2) == 0)) {
      DT = strtol(&strCmd[2], NULL, 10);
      DT = constrain(DT, 10, pow(2, 32) - 1);
      Ser.print("  DT     = ");
      format_msecs(DT);
      Ser.println(buf_time);

#ifdef _VARIANT_FEATHER_M4_
    } else if ((strncmp(strCmd, "T", 1) == 0) ||
               (strncmp(strCmd, "t", 1) == 0)) {
      T_meas = strtol(&strCmd[1], NULL, 10);
      T_meas = constrain(T_meas, 10, pow(2, 32) - 1);
      Ser.print("  T_meas = ");
      format_msecs(T_meas);
      Ser.println(buf_time);
#endif

    } else if (strcmp(strCmd, "s") == 0) {
      f_running = !f_running;

      if (f_running) {
        start_train();
      } else {
        stop_train();
      }

    } else {
      // clang-format off
      Ser << "-------------------------------------------------------------------" << endl
          << "  Arduino trigger box" << endl
          << "  https://github.com/Dennis-van-Gils/project-Arduino-trigger-box" << endl << endl
          << "  A configurable TTL pulse train generator on digital outputs" << endl
          << "  D05 and D06. Can be used to e.g. trigger (Ximea) cameras to" << endl
          << "  acquire pictures in sync with each other using the camera's" << endl
          << "  trigger-in port." << endl
          << "-------------------------------------------------------------------" << endl
          << endl
          << "  <5ms>" << endl
          << "  ┌───┐      ┌───┐      ┌───┐" << endl
          << "  │   │      │   │      │   │" << endl
          << "  │   │      │   │      │   │" << endl
          << "  ┘   └──────┘   └──────┘   └────── --> T_meas" << endl
          << "  <    DT    >" << endl
          << endl
          << "  * The pulse period `DT` can be set from 10 msec upwards to hours" << endl
          << "    with a resolution of 1 msec." << endl
#ifdef _VARIANT_FEATHER_M4_
          << endl
          << "  * The duration of the pulse train `T_meas`, i.e. the measurement" << endl
          << "    time, can be set up to a maximum of 49.7 days." << endl
          << endl
          << "  * The RGB LED indicates the status." << endl
          << "    Blue : Idle" << endl
          << "    Green: Running pulse train" << endl
#endif
          << endl
          << "  * The other onboard LED (#13) will flash red with each pulse." << endl
          << endl
          << "https://github.com/Dennis-van-Gils/project-Arduino-trigger-box" << endl
          << endl;
      Ser << "Commands:" << endl
          << "  ?     : Show current settings" << endl
          << "  DT... : Set the pulse interval `DT` to ... msecs" << endl
#ifdef _VARIANT_FEATHER_M4_
          << "  T...  : Set the measurement time `T_meas` to ... msecs" << endl
#endif
          << "  s     : Start / stop" << endl << endl;
      // clang-format on
    }
  }

  now = millis();

#ifdef _VARIANT_FEATHER_M4_
  if (f_running && (now - t_start >= T_meas)) {
    f_running = false;
    stop_train();
  }
#endif

  if (f_running) {
    if (f_HI && (now - t_HI >= PULSE_WIDTH)) {
      go_LO();
    }

    if (now - t_HI >= DT) {
      t_HI += DT; // Keep the interval strict, prevent cumulative error
      go_HI();
    }
  }
}