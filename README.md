# Wheelie Leveler

A device that tracks and displays real-time wheelie metrics for bike enthusiasts.

## Features

- **Real-time Pitch Angle Display**: Visualizes the current wheelie angle on a vertical bar graph.
- **Dynamic Color Coding**:
  - Blue: 0-44 degrees
  - Green: 45-60 degrees (balance point range)
  - Yellow: 61-79 degrees
  - Red: 80+ degrees
- **Extreme Wheelie Indicator**: Secondary bar activates for wheelies over 80 degrees.
- **Timing Features**:
  - Longest wheelie time (>10 degrees)
  - Longest extreme wheelie time (80+ degrees)
  - Running stopwatch for total ride time
- **Performance Metrics**:
  - Percentage of time spent in a wheelie
  - Top 3 times for regular wheelies
  - Top 3 times for extreme wheelies

## Hardware

- Arduino Nano RP2040 Connect
- TFT ST7735 Display
- LSM6DSOX IMU Sensor

## How It Works

1. The device continuously measures the pitch angle using the IMU sensor.
2. The TFT display shows the current wheelie angle and updates in real-time.
3. Timers track various wheelie durations and update top performances.
4. A running stopwatch keeps track of the total ride time.
5. The device calculates and displays the percentage of time spent in a wheelie.

## Usage

Mount the device securely on your motorcycle. Power on to start tracking your wheelie performance. The display provides immediate feedback during your ride.

## Safety Notice

This device is for recreational use only. Always prioritize safety while riding. Wear appropriate protective gear and follow all traffic laws and regulations.
