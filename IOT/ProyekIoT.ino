#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>

#define WIFI_SSID "5G"
#define WIFI_PASSWORD "Google18"
#define BOT_TOKEN "7902420108:AAGjYFSjEMmZQVG6pg8YS92aVR4xk5-g2Fw"
#define CHAT_ID "7572128788"

#define DHTPIN D1
#define DHTTYPE DHT11
#define PRESSURE_PIN A0
#define BUZZER_PIN D7

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

String lastCuaca = "";
bool buzzerAktif = false;
unsigned long buzzerStartTime = 0;
int modeBuzzer = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  client.setInsecure();
  client.setTimeout(15000);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus, mencoba menyambung ulang...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
    return;
  }

  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();
  float tekanan = analogRead(PRESSURE_PIN) * (3.3 / 1023.0) / 5.0 * 700.0;
  if (tekanan < 0) tekanan = 0;

  String cuaca = "mendung";
  bool ekstrem = false;
  bool hujan = false;

  if ((suhu < 5 || suhu > 50) && (kelembaban < 15 || kelembaban > 95) && (tekanan < 75 || tekanan > 125)) {
    ekstrem = true;
    cuaca = "cuaca ekstrem 🚨";
  } else {
    if (suhu > 24 && kelembaban < 85) cuaca = "cerah";
    else if (kelembaban > 85 && suhu < 24) {
      hujan = true;
      cuaca = "hujan";
    }
    if (cuaca == "mendung" && tekanan >= 100) cuaca += " berangin";
    else if (cuaca == "mendung") cuaca += " tanpa angin";
  }

  Serial.println("===================================");
  Serial.println("Suhu       : " + String(suhu, 1) + " °C");
  Serial.println("Kelembaban : " + String(kelembaban, 1) + " %");
  Serial.println("Tekanan    : " + String(tekanan, 1) + " kPa");
  Serial.println("Cuaca      : " + cuaca);

  if (cuaca != lastCuaca || ekstrem || hujan) {
    String pesan = "\xF0\x9F\x93\xA1 Deteksi Cuaca:\n";
    pesan += "\xF0\x9F\x8C\xA1 Cuaca: " + cuaca + "\n";
    pesan += "\xF0\x9F\x8C\xA1 Suhu: " + String(suhu, 1) + " °C\n";
    pesan += "\xF0\x9F\x92\x87 Kelembaban: " + String(kelembaban, 1) + " %\n";
    pesan += "\xF0\x9F\x94\xB5 Tekanan: " + String(tekanan, 1) + " kPa\n";

    if (ekstrem) {
      pesan += "\n⚠️ *ALERT!* Cuaca ekstrem terdeteksi!";
      modeBuzzer = 2;
      buzzerAktif = true;
      pinMode(BUZZER_PIN, OUTPUT);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("ALARM: CUACA EKSTREM (buzzer menyala terus)");
    } else if (hujan) {
      pesan += "\n🚨 *Hujan!* Segera waspada!";
      modeBuzzer = 1;
      buzzerAktif = true;
      buzzerStartTime = millis();
      pinMode(BUZZER_PIN, OUTPUT);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("ALARM: HUJAN (buzzer menyala 15 detik)");
    } else {
      modeBuzzer = 0;
      buzzerAktif = false;
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("ALARM: Tidak aktif (buzzer OFF)");
    }

    bool terkirim = false;
    int attempt = 0;
    while (!terkirim && attempt < 3) {
      terkirim = bot.sendMessage(CHAT_ID, pesan, "Markdown");
      if (!terkirim) {
        Serial.println("❌ Gagal kirim Telegram. Percobaan ke-" + String(attempt + 1));
        delay(5000);
      }
      attempt++;
    }

    if (terkirim) {
      Serial.println("✅ Pesan berhasil dikirim ke Telegram.");
    } else {
      Serial.println("❌ Gagal mengirim pesan setelah 3 percobaan.");
    }

    lastCuaca = cuaca;
  }

  if (modeBuzzer == 1 && buzzerAktif && millis() - buzzerStartTime >= 15000) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerAktif = false;
    Serial.println("Buzzer HUJAN dimatikan setelah 15 detik.");
  }

  if (modeBuzzer == 0 && !buzzerAktif) {
    digitalWrite(BUZZER_PIN, HIGH);
  }

  delay(30000);
}
