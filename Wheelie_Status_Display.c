/***************************************************
 * TFT_ST7735 nano RP2040 Connect
 * (CIPO/MISO)  - D12
 * (COPI/MOSI)  - D11
 * (SCK)        - D13
 * (CS/SS) - Any GPIO (except for A6/A7)
 * VCC        3V3
 * GND        GND
 * CS         10
 * RESET      9
 * A0(DC)     8
 * SDA        11
 * SCK        13
 * LED        3V3
 ****************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>
#include <Arduino_LSM6DSOX.h>
#include <cmath>
#include <vector>
#include <algorithm>

// TFT display using hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_DC   8  // Data/command line for TFT
#define TFT_RST  9  // Reset line for TFT (or connect to +5V)

Adafruit_ST7735 tft_ST7735 = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
int centerX = 80;
int centerY = 64;
int crosshairSize = 5; // Adjust the size of the crosshair
int LcenterX = 7;
int LcenterY = 125;
int cx = 79;
int cy = 64;
int r = tft_ST7735.height()/3; // coefficient of friction

float x, y, z;
float Gx, Gy, Gz;
float sumx = 0.0;
float sumy = 0.0;
int position = 0;
int oldsumx = 0;
int oldsumy = 0;
float pitch = 0.0;
float maxPitch = 75.0; // Maximum pitch angle for normal bar display
float absoluteMaxPitch = 110.0; // Absolute maximum pitch to display
int barHeight = 80;   // Height of the vertical bar
int mainBarWidth = 20;     // Width of the main vertical bar
int mainBarX = 10;         // X position of the main bar
int secondBarWidth = 10;   // Width of the secondary bar
int secondBarX = 35;       // X position of the secondary bar

float mountAngle = 25.0; // Mounting angle offset in degrees

unsigned long startTime = 0;
bool isTimerRunning = false;
std::vector<float> topTimes;
float currentTime = 0;

// Add these global variables
unsigned long wheelieStartTime = 0;
bool isWheelieTimerRunning = false;
float wheelieCurrentTime = 0;
std::vector<float> topWheelieTimes;

// Add these global variables
unsigned long rideStartTime = 0;
bool isRiding = false;
unsigned long totalWheelieTime = 0;
unsigned long lastWheelieEndTime = 0;

// Add this function to format the ride time
String formatRideTime(unsigned long seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, secs);
  return String(buffer);
}

// Update this function to move the stopwatch and add wheelie percentage
void updateRideTime() {
  if (!isRiding) {
    rideStartTime = millis();
    isRiding = true;
  }
  
  unsigned long currentTime = millis();
  unsigned long rideTime = (currentTime - rideStartTime) / 1000;
  String timeStr = formatRideTime(rideTime);
  
  // Update total wheelie time if currently in a wheelie
  if (isWheelieTimerRunning) {
    totalWheelieTime += currentTime - lastWheelieEndTime;
  }
  lastWheelieEndTime = currentTime;
  
  // Calculate wheelie percentage
  float wheeliePercentage = (rideTime > 0) ? (float)totalWheelieTime / (rideTime * 10) : 0;
  
  // Display ride time (moved 15 pixels to the right)
  tft_ST7735.fillRect(15, tft_ST7735.height() - 10, 70, 10, ST77XX_BLACK);
  tft_ST7735.setTextColor(ST77XX_WHITE);
  tft_ST7735.setTextSize(1);
  tft_ST7735.setCursor(15, tft_ST7735.height() - 10);
  tft_ST7735.print(timeStr);
  
  // Display wheelie percentage with dynamic decimal places
  tft_ST7735.fillRect(85, tft_ST7735.height() - 10, 45, 10, ST77XX_BLACK);
  tft_ST7735.setCursor(85, tft_ST7735.height() - 10);
  
  if (wheeliePercentage < 10) {
    tft_ST7735.print(wheeliePercentage, 1);
  } else if (wheeliePercentage < 100) {
    tft_ST7735.print(wheeliePercentage, 1);
  } else {
    tft_ST7735.print(wheeliePercentage, 0);
  }
  tft_ST7735.print("%");
}

// Add this helper function to format time
String formatTime(float seconds) {
  int minutes = (int)(seconds / 60);
  int secs = (int)seconds % 60;
  int millis = (int)((seconds - (int)seconds) * 100);
  
  char buffer[20];
  if (minutes > 0) {
    if (seconds >= 600) { // 10 minutes or more
      sprintf(buffer, "%d:%02d", minutes, secs);
    } else {
      sprintf(buffer, "%d:%02d.%02d", minutes, secs, millis);
    }
  } else if (seconds >= 10) {
    sprintf(buffer, "%d.%02d", secs, millis);
  } else {
    sprintf(buffer, "%d.%02d", secs, millis);
  }
  return String(buffer);
}

// Update this helper function to right-align text
void printRightAligned(float value, int y, uint16_t color) {
  tft_ST7735.setTextColor(color);
  tft_ST7735.setTextSize(2);
  
  String timeStr = formatTime(value);
  
  // Calculate width of the text
  int16_t x1, y1;
  uint16_t w, h;
  tft_ST7735.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  
  // Add some padding to the width
  int clearWidth = w + 10;  // 5 pixels padding on each side
  
  // Clear only the area needed for the current number
  int clearX = tft_ST7735.width() - clearWidth;
  tft_ST7735.fillRect(clearX, y, clearWidth, h + 4, ST77XX_BLACK);
  
  // Print the text right-aligned within the cleared area
  tft_ST7735.setCursor(tft_ST7735.width() - 5 - w, y + 2);  // 5 pixels padding on the right
  tft_ST7735.print(timeStr);
}

// Update this function to right-align results
void updateTopTimes(float newTime) {
  topTimes.push_back(newTime);
  std::sort(topTimes.begin(), topTimes.end(), std::greater<float>());
  if (topTimes.size() > 3) {
    topTimes.resize(3);
  }
  
  // Display top times
  tft_ST7735.fillRect(tft_ST7735.width() - 90, 0, 90, 30, ST77XX_BLACK);
  tft_ST7735.setTextColor(ST77XX_WHITE);
  tft_ST7735.setTextSize(1);
  for (int i = 0; i < topTimes.size(); i++) {
    String timeStr = formatTime(topTimes[i]);
    int16_t x1, y1;
    uint16_t w, h;
    tft_ST7735.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft_ST7735.setCursor(tft_ST7735.width() - w - 5, i * 10);
    tft_ST7735.print(timeStr);
  }
}

// Update this function to right-align wheelie results
void updateTopWheelieTimes(float newTime) {
  topWheelieTimes.push_back(newTime);
  std::sort(topWheelieTimes.begin(), topWheelieTimes.end(), std::greater<float>());
  if (topWheelieTimes.size() > 3) {
    topWheelieTimes.resize(3);
  }
  
  // Display top wheelie times
  tft_ST7735.fillRect(tft_ST7735.width() - 90, 90, 90, 30, ST77XX_BLACK);
  tft_ST7735.setTextColor(ST77XX_YELLOW);
  tft_ST7735.setTextSize(1);
  for (int i = 0; i < topWheelieTimes.size(); i++) {
    String timeStr = formatTime(topWheelieTimes[i]);
    int16_t x1, y1;
    uint16_t w, h;
    tft_ST7735.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft_ST7735.setCursor(tft_ST7735.width() - w - 5, 90 + i * 10);
    tft_ST7735.print(timeStr);
  }
}

void clearTimerArea(int y, int height) {
  tft_ST7735.fillRect(0, y, tft_ST7735.width(), height, ST77XX_BLACK);
}

void setup(void) {
  delay(1000);
  




  if (!IMU.begin()) {
    //Serial.println("Failed to initialize IMU!");
    while (1);{
    }
  }
  delay(1000);
  tft_ST7735.initR(INITR_BLACKTAB);
  tft_ST7735.fillScreen(ST7735_BLACK);
  tft_ST7735.setTextWrap(true);
  tft_ST7735.setTextColor(ST77XX_WHITE);
  tft_ST7735.setCursor(0, 0);
  
  tft_ST7735.setRotation(3);
  tft_ST7735.print("Wheelie Display Using Arduino Nano RP2040 Connect");
  delay(1000);
  tft_ST7735.println("\n");
  tft_ST7735.setCursor(45, 50);
  tft_ST7735.println("\n");
  tft_ST7735.print("Wear A Helmet!");
  delay(2000);

  tft_ST7735.fillScreen(ST77XX_RED);
  tft_ST7735.setCursor(50, 50);
  tft_ST7735.print("Booting");
  delay(1000);
  tft_ST7735.fillScreen(ST77XX_BLUE);
  tft_ST7735.setCursor(50, 50);
  tft_ST7735.setTextColor(ST77XX_BLACK);
  tft_ST7735.print("Shadow 750W");

  delay(1000);
  tft_ST7735.fillScreen(ST77XX_BLACK);
  tft_ST7735.drawRect(mainBarX, centerY - barHeight/2, mainBarWidth, barHeight, ST77XX_WHITE);
  tft_ST7735.drawRect(secondBarX, centerY - barHeight/2, secondBarWidth, barHeight, ST77XX_WHITE);

  // Initialize ride start time
  rideStartTime = millis();
  isRiding = true;
}


void loop() {
  float sumPitch = 0.0;
  int iterations = 0;

  // Draw Libruh Logo
  tft_ST7735.drawCircle(8, 123, 2, ST77XX_WHITE);
  tft_ST7735.drawLine(LcenterX - crosshairSize, LcenterY, LcenterX + crosshairSize-3, LcenterY-12, ST7735_WHITE);
  tft_ST7735.drawLine(LcenterX - crosshairSize, LcenterY, LcenterX + crosshairSize, LcenterY, ST7735_WHITE);

  while (iterations < 6) {
    if (IMU.accelerationAvailable()) {
      iterations++;
      IMU.readAcceleration(x, y, z);
      float rawPitch = atan2(-x, sqrt(y*y + z*z)) * 180.0 / PI;
      pitch = rawPitch - mountAngle;
      sumPitch += pitch;
    }
  }
  
  pitch = sumPitch / 6.0; // Average pitch over 6 readings
  
  // Map pitch to main bar height (0-75 degrees)
  int mainFillHeight = map(constrain(abs(pitch), 0, maxPitch), 0, maxPitch, 0, barHeight);
  int mainFillY = centerY + barHeight/2 - mainFillHeight;
  
  // Clear previous fills
  tft_ST7735.fillRect(mainBarX + 1, centerY - barHeight/2 - 20, mainBarWidth - 2, barHeight + 20, ST77XX_BLACK);
  tft_ST7735.fillRect(secondBarX + 1, centerY - barHeight/2 - 20, secondBarWidth - 2, barHeight + 20, ST77XX_BLACK);
  
  // Determine bar colors and handle overflow
  uint16_t mainBarColor;
  if (pitch > 0) {
    // Red bar for positive pitch (on the ground)
    mainBarColor = ST77XX_RED;
    mainFillHeight = map(constrain(pitch, 0, maxPitch), 0, maxPitch, 0, barHeight);
    mainFillY = centerY + barHeight/2 - mainFillHeight;
  } else if (abs(pitch) >= 80) {
    mainBarColor = ST77XX_RED;
  } else if (abs(pitch) >= 60) {
    mainBarColor = ST77XX_YELLOW;
  } else if (abs(pitch) >= 45 && abs(pitch) <= 60) {
    mainBarColor = ST77XX_GREEN;
  } else {
    mainBarColor = ST77XX_BLUE;
  }
  
  // Fill the main bar
  tft_ST7735.fillRect(mainBarX + 1, mainFillY, mainBarWidth - 2, mainFillHeight, mainBarColor);
  
  // Draw tick markers for balance point range
  int balanceLowY = centerY + barHeight/2 - map(45, 0, maxPitch, 0, barHeight);
  int balanceHighY = centerY + barHeight/2 - map(60, 0, maxPitch, 0, barHeight);
  tft_ST7735.drawFastHLine(mainBarX - 3, balanceLowY, 5, ST77XX_WHITE);
  tft_ST7735.drawFastHLine(mainBarX - 3, balanceHighY, 5, ST77XX_WHITE);
  
  // Fill the secondary bar if pitch is over 80 degrees
  if (abs(pitch) > 80) {
    int secondFillHeight = map(constrain(abs(pitch), 80, absoluteMaxPitch), 80, absoluteMaxPitch, 0, barHeight);
    int secondFillY = centerY + barHeight/2 - secondFillHeight;
    tft_ST7735.fillRect(secondBarX + 1, secondFillY, secondBarWidth - 2, secondFillHeight, ST77XX_MAGENTA);
  }
  
  // Redraw bar outlines
  tft_ST7735.drawRect(mainBarX, centerY - barHeight/2, mainBarWidth, barHeight, ST77XX_WHITE);
  tft_ST7735.drawRect(secondBarX, centerY - barHeight/2, secondBarWidth, barHeight, ST77XX_WHITE);
  
  // Display pitch value
  tft_ST7735.fillRect(10, 4, 60, 20, ST77XX_BLACK);  // Moved 10 pixels right and 2 pixels down
  tft_ST7735.setCursor(10, 4);  // Moved 10 pixels right and 2 pixels down
  tft_ST7735.setTextColor(ST77XX_WHITE);
  tft_ST7735.setTextSize(2);
  int displayPitch = round(pitch * -1); // Multiply by -1 to swap the sign
  if (abs(displayPitch) < 10) {
    tft_ST7735.print(" "); // Add a space for single-digit numbers
  }
  if (displayPitch < 0) {
    tft_ST7735.print("-");
  }
  tft_ST7735.print(abs(displayPitch));
  tft_ST7735.print((char)247); // Degree symbol

  // Handle 80-degree timer
  if (abs(pitch) >= 80) {
    if (!isTimerRunning) {
      startTime = millis();
      isTimerRunning = true;
      clearTimerArea(35, 20);  // Clear the entire 80-degree timer area
    }
    currentTime = (millis() - startTime) / 1000.0;
    
    // Display 80-degree timer (right-aligned)
    printRightAligned(currentTime, 35, ST77XX_WHITE);
  } else if (isTimerRunning) {
    isTimerRunning = false;
    updateTopTimes(currentTime);
  }

  // Handle wheelie timer (pitch < -10 degrees)
  if (pitch < -10) {
    if (!isWheelieTimerRunning) {
      wheelieStartTime = millis();
      isWheelieTimerRunning = true;
      lastWheelieEndTime = wheelieStartTime;
      clearTimerArea(65, 20);  // Clear the entire wheelie timer area
    }
    
    wheelieCurrentTime = (millis() - wheelieStartTime) / 1000.0;
    
    // Display wheelie timer (right-aligned)
    printRightAligned(wheelieCurrentTime, 65, ST77XX_YELLOW);
  } else if (isWheelieTimerRunning) {
    isWheelieTimerRunning = false;
    updateTopWheelieTimes(wheelieCurrentTime);
    totalWheelieTime += millis() - lastWheelieEndTime;
  }

  // Update and display ride time and wheelie percentage
  updateRideTime();
}
