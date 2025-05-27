const int waterBorder = 550; //Ниже этого уровня — вода попала
const int piezoPin = 10; // Пин пищалки
const int toneFrequency = 660; // Частота тона, Гц
const unsigned long ALARM_DURATION = 2000; // Длительность сигнала в миллисекундах
unsigned long alarmStartTime = 0; // Время начала сигнала

void setup() {
  pinMode(piezoPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int sensorReading = analogRead(A0);
  Serial.println(sensorReading);
 
  if (sensorReading < waterBorder) {
    if (alarmStartTime == 0) {
      // Начинаем сигнал только если он еще не начат
      alarmStartTime = millis();
      Serial.println("Rain Warning");
      tone(piezoPin, toneFrequency);
    } else if (millis() - alarmStartTime >= ALARM_DURATION) {
      // Если прошло больше 5 секунд, выключаем сигнал
      noTone(piezoPin);
    }
  } else {
    // Если вода не обнаружена, сбрасываем таймер и выключаем сигнал
    alarmStartTime = 0;
    noTone(piezoPin);
    Serial.println("Not Raining");
  }

  delay(100);
}