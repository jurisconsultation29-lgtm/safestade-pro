/*
  SafeStade Pro — Firmware USB (Web Serial), sans Wi-Fi
  ========================================================
  Aucune connexion reseau : l'ESP32 recoit ses commandes
  directement par cable USB. C'est l'application (Chrome/Edge)
  qui ouvre le port serie via Web Serial API.

  Broches confirmees sur le kit "Maker Beginner Kit" :
    LED1=33, LED2=32, BUZZER=26, RELAIS1=23, RELAIS2=27, INFRAROUGE=4
    Capteur temperature/humidite (BME280) : bus I2C (SDA=21, SCL=22)

  Bibliotheque a installer : "Adafruit BME280 Library"
  (deja listee dans platformio.ini si vous utilisez PlatformIO)

  Commandes acceptees (une par ligne, terminee par un retour a la ligne) :
    LED_RED, LED_GREEN, LED_OFF, BUZZER_ON, BUZZER_OFF,
    ALERT_ON, ALERT_OFF, GET_TEMP, GET_IR,
    TIMER_START:<secondes>, TIMER_STOP
  Toute commande hors de cette liste est ignoree (securite).
*/

#include <Wire.h>
#include <Adafruit_BME280.h>

const int PIN_LED1 = 33, PIN_LED2 = 32, PIN_BUZZER = 26;
const int PIN_RELAIS1 = 23, PIN_RELAIS2 = 27, PIN_IR = 4;

Adafruit_BME280 bme;
bool capteurOk = false;

String ligne = "";
bool minuterieActive = false;
unsigned long minuterieFin = 0;
unsigned long dernierEnvoiTimer = 0;
bool etatIRPrecedent = false;

void appliquer(String cmd) {
  if (cmd == "LED_RED")        { digitalWrite(PIN_LED1, HIGH); digitalWrite(PIN_LED2, LOW); }
  else if (cmd == "LED_GREEN") { digitalWrite(PIN_LED2, HIGH); digitalWrite(PIN_LED1, LOW); }
  else if (cmd == "LED_OFF")   { digitalWrite(PIN_LED1, LOW);  digitalWrite(PIN_LED2, LOW); }
  else if (cmd == "BUZZER_ON")  digitalWrite(PIN_BUZZER, HIGH);
  else if (cmd == "BUZZER_OFF") digitalWrite(PIN_BUZZER, LOW);
  else if (cmd == "ALERT_ON")  { digitalWrite(PIN_LED1, HIGH); digitalWrite(PIN_BUZZER, HIGH); digitalWrite(PIN_RELAIS1, HIGH); }
  else if (cmd == "ALERT_OFF") { digitalWrite(PIN_LED1, LOW);  digitalWrite(PIN_BUZZER, LOW);  digitalWrite(PIN_RELAIS1, LOW); }
  else if (cmd == "GET_TEMP") {
    if (capteurOk) Serial.println("TEMP:" + String(bme.readTemperature(), 1) + ":" + String(bme.readHumidity(), 1));
    else Serial.println("TEMP:--:--");
  }
  else if (cmd == "GET_IR") {
    Serial.println("IR:" + String(digitalRead(PIN_IR) == HIGH ? "1" : "0"));
  }
  else if (cmd.startsWith("TIMER_START:")) {
    int s = cmd.substring(12).toInt();
    if (s < 1) s = 1;
    minuterieFin = millis() + (unsigned long)s * 1000UL;
    minuterieActive = true;
  }
  else if (cmd == "TIMER_STOP") {
    minuterieActive = false;
  }
  // toute autre commande est silencieusement ignoree
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED1, OUTPUT); pinMode(PIN_LED2, OUTPUT); pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RELAIS1, OUTPUT); pinMode(PIN_RELAIS2, OUTPUT); pinMode(PIN_IR, INPUT);

  Wire.begin(21, 22);
  capteurOk = bme.begin(0x76) || bme.begin(0x77);
}

void loop() {
  // Lecture des commandes envoyees par l'application (une ligne = une commande)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') { appliquer(ligne); ligne = ""; }
    else if (c != '\r') { ligne += c; }
  }

  // Detection infrarouge autonome (reaction locale meme sans commande de l'app)
  bool etatIR = digitalRead(PIN_IR) == HIGH;
  if (etatIR && !etatIRPrecedent) {
    appliquer("LED_RED"); appliquer("BUZZER_ON");
    Serial.println("IR:1");
  }
  etatIRPrecedent = etatIR;

  // Minuterie : envoie le temps restant chaque seconde, declenche l'alerte a la fin
  if (minuterieActive) {
    if (millis() - dernierEnvoiTimer >= 1000) {
      dernierEnvoiTimer = millis();
      long restant = (long)(minuterieFin - millis()) / 1000;
      if (restant < 0) restant = 0;
      Serial.println("TIMER:" + String(restant));
    }
    if (millis() >= minuterieFin) {
      minuterieActive = false;
      appliquer("ALERT_ON");
      Serial.println("TIMER:DONE");
    }
  }
}