#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// ---------- Reseau Wi-Fi cree par l'ESP32 (mode point d'acces) ----------
const char* AP_SSID = "SafeStade-ESP32";
const char* AP_PASSWORD = "safestade2030";

// ---------- Broches CONFIRMEES sur la carte ----------
const int PIN_LED1    = 33;  // LED1
const int PIN_LED2    = 32;  // LED2
const int PIN_BUZZER  = 26;  // BUZZER (confirme)
const int PIN_RELAIS1 = 23;  // RELAY1
const int PIN_RELAIS2 = 27;  // RELAY2
const int PIN_IR      = 4;   // INFRARED

Adafruit_BME280 bmp;         // capteur temperature/pression/humidite (I2C)
bool capteurTempOk = false;

WebServer server(80);

// ---------- Minuterie (fonctionnalite logicielle, pas de broche dediee) ----------
unsigned long minuterieFin = 0;
bool minuterieActive = false;

void envoyerCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

// ---- LED (on utilise LED1=rouge, LED2=verte par convention) ----
void ledRouge() {
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, LOW);
  envoyerCORS(); server.send(200, "text/plain", "LED_RED");
}
void ledVerte() {
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(PIN_LED1, LOW);
  envoyerCORS(); server.send(200, "text/plain", "LED_GREEN");
}
void ledOff() {
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  envoyerCORS(); server.send(200, "text/plain", "LED_OFF");
}

// ---- Buzzer ----
void buzzerOn() {
  digitalWrite(PIN_BUZZER, HIGH);
  envoyerCORS(); server.send(200, "text/plain", "BUZZER_ON");
}
void buzzerOff() {
  digitalWrite(PIN_BUZZER, LOW);
  envoyerCORS(); server.send(200, "text/plain", "BUZZER_OFF");
}

// ---- Alerte generale (utilise aussi le relais comme sortie supplementaire) ----
void alertOn() {
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  digitalWrite(PIN_RELAIS1, HIGH);
  envoyerCORS(); server.send(200, "text/plain", "ALERT_ON");
}
void alertOff() {
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_RELAIS1, LOW);
  envoyerCORS(); server.send(200, "text/plain", "ALERT_OFF");
}

// ---- Etat general (utilise par l'app pour verifier la connexion) ----
void gererStatus() {
  envoyerCORS();
  server.send(200, "application/json", "{\"status\":\"ONLINE\"}");
}

// ---- Temperature / humidite (capteur I2C) ----
void gererTemperature() {
  envoyerCORS();
  if (!capteurTempOk) {
    server.send(200, "application/json", "{\"error\":\"capteur non detecte\"}");
    return;
  }
  float t = bmp.readTemperature();
  float p = bmp.readPressure() / 100.0F; // en hPa
  float h = bmp.readHumidity();          // en %
  String json = "{\"temperature\":" + String(t, 1) + ",\"pression\":" + String(p, 1) + ",\"humidite\":" + String(h, 1) + "}";
  server.send(200, "application/json", json);
}

// ---- Infrarouge (lecture instantanee) ----
void gererInfrarouge() {
  envoyerCORS();
  bool detecte = digitalRead(PIN_IR) == HIGH;
  server.send(200, "application/json", detecte ? "{\"detected\":true}" : "{\"detected\":false}");
}

// ---- Minuterie : demarre un compte a rebours, declenche une alerte a la fin ----
void gererMinuterieStart() {
  envoyerCORS();
  int secondes = 10;
  if (server.hasArg("s")) secondes = server.arg("s").toInt();
  if (secondes < 1) secondes = 1;
  minuterieFin = millis() + (unsigned long)secondes * 1000UL;
  minuterieActive = true;
  server.send(200, "application/json", "{\"status\":\"demarree\",\"secondes\":" + String(secondes) + "}");
}
void gererMinuterieStatus() {
  envoyerCORS();
  if (!minuterieActive) {
    server.send(200, "application/json", "{\"active\":false}");
    return;
  }
  long restant = (long)(minuterieFin - millis()) / 1000;
  if (restant < 0) restant = 0;
  server.send(200, "application/json", "{\"active\":true,\"restant\":" + String(restant) + "}");
}
void gererMinuterieStop() {
  envoyerCORS();
  minuterieActive = false;
  server.send(200, "application/json", "{\"status\":\"arretee\"}");
}

void gererOptions() {
  envoyerCORS();
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.send(204);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_RELAIS1, OUTPUT);
  pinMode(PIN_RELAIS2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_IR, INPUT);

  Wire.begin(21, 22); // SDA=21, SCL=22 (broches I2C standard de l'ESP32)
  capteurTempOk = bmp.begin(0x76) || bmp.begin(0x77); // les 2 adresses possibles du capteur
  if (capteurTempOk) {
    Serial.println("Capteur temperature/pression detecte.");
  } else {
    Serial.println("ATTENTION : capteur temperature/pression non detecte (verifiez le branchement I2C).");
  }

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("Point d'acces cree : " + String(AP_SSID));
  Serial.print("Adresse IP a utiliser dans l'application : ");
  Serial.println(WiFi.softAPIP());

  server.on("/status", HTTP_GET, gererStatus);
  server.on("/led/red", HTTP_GET, ledRouge);
  server.on("/led/green", HTTP_GET, ledVerte);
  server.on("/led/off", HTTP_GET, ledOff);
  server.on("/buzzer/on", HTTP_GET, buzzerOn);
  server.on("/buzzer/off", HTTP_GET, buzzerOff);
  server.on("/alert/on", HTTP_GET, alertOn);
  server.on("/alert/off", HTTP_GET, alertOff);
  server.on("/temperature", HTTP_GET, gererTemperature);
  server.on("/infrared", HTTP_GET, gererInfrarouge);
  server.on("/timer/start", HTTP_GET, gererMinuterieStart);
  server.on("/timer/status", HTTP_GET, gererMinuterieStatus);
  server.on("/timer/stop", HTTP_GET, gererMinuterieStop);
  server.onNotFound(gererOptions);

  server.begin();
  Serial.println("Serveur pret.");
}

void loop() {
  server.handleClient();

  static bool etatPrecedent = false;
  bool etat = digitalRead(PIN_IR) == HIGH;
  if (etat && !etatPrecedent) {
    Serial.println("Detection infrarouge !");
    ledRouge();
  }
  etatPrecedent = etat;

  if (minuterieActive && millis() >= minuterieFin) {
    minuterieActive = false;
    Serial.println("Minuterie terminee -> declenchement de l'alerte");
    alertOn();
  }
}