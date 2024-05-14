# Arduino trigger box

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

### Serial commands
  ``?``     : Show current settings

  ``DT...`` : Set the pulse interval `DT` to ... msecs

  ``T...``  : Set the measurement time `T_meas` to ... msecs

  ``s``     : Start / stop

### Hardware
  * Adafruit Feather M4 Express
  * Adafruit TermBlock FeatherWing #2926
  * 74AHCT125 'Quad Level-Shifter': To increase 3.3 V digital out of pins D05, D06 and D09 to 5 V.

  Or:

  * Arduino Uno (Does not have an onboard RGB LED but is native 5 V logic)