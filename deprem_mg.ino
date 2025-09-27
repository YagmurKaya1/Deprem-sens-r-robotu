#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

Adafruit_MPU6050 mpu;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int BUZZER_PIN = 10;
const float ACC_THRESHOLD_MG = 50.0; // mg cinsinden eşik
const float ACC_MAX_MG = 2000.0;      // PWM için üst limit (mg)

bool isShaking = false;
bool lcdNeedsClear = true;
unsigned long shakeStart = 0;
unsigned long shakeDuration = 0;
float bias = 0;
float maxAccMg = 0;

float calibrateBias(unsigned long ms = 2000) {
  unsigned long t0 = millis();
  float total = 0;
  int n = 0;
  while (millis() - t0 < ms) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float accX = a.acceleration.x;
    float accY = a.acceleration.y;
    float accZ = a.acceleration.z;
    float acc_mg = sqrt(accX*accX + accY*accY + accZ*accZ) / 9.80665 * 1000.0;
    total += acc_mg;
    n++;
    delay(5);
  }
  return (n > 0) ? (total / n) : 0;
}

void printReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hazir, izleniyor");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcdNeedsClear = false;
}

void printShakeEnd(float duration, float magnitude) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sure: ");
  lcd.print(duration / 1000.0, 1);
  lcd.print(" sn    ");
  lcd.setCursor(0, 1);
  lcd.print("Mag: ");
  lcd.print(magnitude, 1);  // 1.5, 3.1 gibi
  lcd.print("        ");
  delay(4000);
  printReady();
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);
  analogWrite(BUZZER_PIN, 0);

  if (!mpu.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("MPU6050 yok!");
    while (1) delay(10);
  }

  lcd.setCursor(0, 0);
  lcd.print("Kalibrasyon...");
  bias = calibrateBias(2000);
  printReady();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accX = a.acceleration.x;
  float accY = a.acceleration.y;
  float accZ = a.acceleration.z;

  float acc_mg = sqrt(accX*accX + accY*accY + accZ*accZ) / 9.80665 * 1000.0;
  float shake_mg = abs(acc_mg - bias);

  bool axisExceeded = (shake_mg > ACC_THRESHOLD_MG);

  if (axisExceeded) {
    if (!isShaking) {
      shakeStart = millis();
      maxAccMg = 0;
      isShaking = true;
    }
    shakeDuration = millis() - shakeStart;
    if (shake_mg > maxAccMg) maxAccMg = shake_mg;
    // Buzzer isteğe bağlı, istersen kapatabilirsin
    
    analogWrite(BUZZER_PIN, 100);
  } else {
    if (isShaking) {
      // Burda Kandilli tarzı büyüklük kullandık
      float magnitude = (maxAccMg > 0) ? log10(maxAccMg) - 1.7 : 0.0;
      if (magnitude < 0) magnitude = 0;
      printShakeEnd(shakeDuration, magnitude);
      isShaking = false;
      shakeDuration = 0;
      analogWrite(BUZZER_PIN, 0);
    }
  }
  delay(20);
}