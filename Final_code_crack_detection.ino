/*  THREE‑SENSOR IR + BUZZER + OLED DISPLAY
 *  ───────────────────────────────────────
 *  • Buzz only when object ≤ 4 cm — using the four‑case truth‑table below
 *  • Show a plain‑language status message on a 128×64 I²C OLED
 *
 *  ─── WIRING ───
 *  IR1 AO → A0         (pin 23 on Nano)
 *  IR2 AO → A1         (pin 24)
 *  IR3 AO → A2         (pin 25)
 *  All IR VCC → 5 V,  GND → GND
 *
 *  OLED VCC  → 5 V (OK for most breakout boards¹)
 *  OLED GND  → GND
 *  OLED SCL  → A5  (Arduino Nano I²C clock)
 *  OLED SDA  → A4  (Arduino Nano I²C data)
 *
 *  Active buzzer + → D5  (pin 11)
 *  Active buzzer − → GND
 *
 *  ¹ If your module is 3.3 V only you must use a 3.3 V rail or level shifter.
 *  -----------------------------------------------------------
 *  One‑time calibration (unchanged):
 *   – Upload, open Serial Plotter, hold target at 4 cm,
 *     write down the plateau reading for each sensor,
 *     then set THRESH_1/2/3 = plateau − 25 and re‑upload.
 *  -----------------------------------------------------------
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const uint8_t IR1_PIN_A = A0;
const uint8_t IR2_PIN_A = A1;
const uint8_t IR3_PIN_A = A2;
const uint8_t BUZZER_PIN = 5;          // active buzzer (+)

// ─── Per‑sensor 4 cm thresholds (0…1023) ───
uint16_t THRESH_1 = 600;   // ← replace with your values
uint16_t THRESH_2 = 610;
uint16_t THRESH_3 = 590;
// ───────────────────────────────────────────

// ─── OLED setup ───
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_ADDR      0x3C   // 0x3C is the most common; use 0x3D if needed
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// ──────────────────

String lastMsg = "";        // remember what’s on‑screen to avoid flicker

void showMessage(const String &msg)
{
  if (msg == lastMsg) return;      // nothing new → skip OLED update
  lastMsg = msg;

  display.clearDisplay();
  display.setCursor(0, 24);        // roughly centre‑vertically
  display.setTextSize(2);          // 2× font → ~16 px tall
  display.setTextColor(SSD1306_WHITE);
  display.println(msg);
  display.display();
}

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(9600);              // only needed for calibration

  // OLED initialisation
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    // If we get here, something is wrong—stop execution
    while (true);   // hang forever
  }
  display.clearDisplay();
  display.display();
  showMessage("STARTING…");
  delay(1000);
}

void loop()
{
  const uint16_t val1 = analogRead(IR1_PIN_A);
  const uint16_t val2 = analogRead(IR2_PIN_A);
  const uint16_t val3 = analogRead(IR3_PIN_A);

  // During calibration: stream values to Serial Plotter
  // (comment these three lines out once thresholds are fixed)
  Serial.print(val1); Serial.print('\t');
  Serial.print(val2); Serial.print('\t');
  Serial.println(val3);

  const bool obj1 = (val1 < THRESH_1);   // true → object ≤ 4 cm
  const bool obj2 = (val2 < THRESH_2);
  const bool obj3 = (val3 < THRESH_3);

  /*  FOUR OPERATING CASES
   *   case | IR1 | IR2 | IR3 | Buzzer |   OLED message
   *  ──────┼─────┼─────┼─────┼────────┼──────────────────────────
   *     1  | obj | obj | obj |  OFF   |   TRACK OK
   *     2  | gap | obj | obj |  ON    |   CRACK DETECTED
   *     3  | gap | gap | obj |  OFF   |   NORMAL GAP
   *     4  | gap | gap | gap |  ON    |   NON‑ALIGNMENT
   */

  bool buzzerOn = (!obj1 &&  obj2 &&  obj3) ||   // case 2
                  (!obj1 && !obj2 && !obj3);     // case 4
  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);

  if ( obj1 &&  obj2 &&  obj3)            showMessage("TRACK OK");
  else if (!obj1 &&  obj2 &&  obj3)       showMessage("CRACK DETECT");
  else if (!obj1 && !obj2 &&  obj3)       showMessage("NORMAL GAP");
  else if (!obj1 && !obj2 && !obj3)       showMessage("NON ALIGN");
  else                                    showMessage("---");

  delay(10);   // 1 kHz loop
}