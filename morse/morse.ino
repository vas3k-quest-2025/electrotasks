const char WORD_TO_TRANSLATE[] = "МОДУЛЬ"; // Передаваемое сообщение, между словами по шесть пробелов
const int piezoPin = 10; // Пин пищалки
const long dotDuration = 250; // Длительность точки, мс, чем короче, тем сложнее.
const int toneFrequency = 1320; // Частота тона, Гц
const int transmissionDelay = 15000; // Перерыв между повторами, мс

const char* morseCode[] = {
  ".-",     // А
  "-...",   // Б
  ".--",    // В
  "--.",    // Г
  "-..",    // Д
  ".",      // Е
  "...-",   // Ж
  "--..",   // З
  "..",     // И
  ".---",   // Й
  "-.-",    // К
  ".-..",   // Л
  "--",     // М
  "-.",     // Н
  "---",    // О
  ".--.",   // П
  ".-.",    // Р
  "...",    // С
  "-",      // Т
  "..-",    // У
  "..-.",   // Ф
  "....",   // Х
  "-.-.",   // Ц
  "---.",   // Ч
  "----",   // Ш
  "--.-",   // Щ
  ".--.-.", // Ъ
  "-.--",   // Ы
  "-..-",   // Ь
  "...-...",// Э
  "..--",   // Ю
  ".-.-",   // Я
  "", // пустота для пробела — ставь шесть пробелов подряд
};
const char cyrillicAlphabet[] = "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ  ";

const int WORD_LENGTH = (sizeof(WORD_TO_TRANSLATE) - 1) / 2; // Символы по два байта (на самом деле не все)
void setup() {
  pinMode(piezoPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Начинаем передачу...");
}

void loop() {
  for (int i = 0; i < WORD_LENGTH; i++) {
    char currentCharUTF8[3];
    strncpy(currentCharUTF8, WORD_TO_TRANSLATE + (i * 2), 2);
    currentCharUTF8[2] = '\0';
    String currentChar = String(currentCharUTF8);
    int charIndex = -1;

    for (int j = 0; j < sizeof(cyrillicAlphabet) - 1; j += 2) {
      char alphabetCharUTF8[3];
      strncpy(alphabetCharUTF8, cyrillicAlphabet + j, 2);
      alphabetCharUTF8[2] = '\0';
      String alphabetChar = String(alphabetCharUTF8);

      if (currentChar == alphabetChar) {
        charIndex = j / 2;
        break;
      }
      if (j + 2 >= sizeof(cyrillicAlphabet) - 1) {
        break;
      }
    }

    if (charIndex != -1) {
      String morse = morseCode[charIndex];
      Serial.print(currentChar);
      Serial.print(": ");
      Serial.println(morse);
      playMorse(morse);
    } else {
      Serial.print("Символ не найден в словаре: ");
      Serial.println(currentChar);
    }

    delay(dotDuration * 3);
  }
  Serial.println("Передача завершена.");
  delay(transmissionDelay);
}

void playMorse(String morse) {
  for (int i = 0; i < morse.length(); i++) {
    int letterDuration = 0;
    if (morse[i] == '.') {
      letterDuration = dotDuration;
    } else if (morse[i] == '-') {
      letterDuration = dotDuration * 3;
    }
  
    tone(piezoPin, toneFrequency);
    delay(letterDuration);
    noTone(piezoPin);
    delay(dotDuration);
  }
}