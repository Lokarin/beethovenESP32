// Bibliotecas
#include "Arduino.h"
#include <WiFi.h>
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include <esp_now.h>
#include <esp_wifi.h>


//Pinos de conexão do ESP32 e o módulo de cartão SD
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

//Pinos de conexão do ESP32-I2S e o módulo I2S/DAC CJMCU 1334
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26
 
// Audio
Audio audio;

// Variavel para musica pausar
bool musRun = 1;

// SSID da rede
constexpr char WIFI_SSID[] = "x";

// Encontrar canal do wifi
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

// Listar diretorios
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  
}

// Estrutura da mensagem recebida
typedef struct struct_message {
  int a;
} struct_message;

struct_message myData;

// Tocar musica baseada no valor de myData.a
void musicaPlay() {
  if (myData.a == 1) { 
    Serial.println("Musica1");
    Serial.println();
    audio.connecttoFS(SD,"/musica1.mp3");
  } else if (myData.a == 2) {
    Serial.println("Musica2");
    Serial.println();
    audio.connecttoFS(SD,"/musica2.mp3");
  } else if (myData.a == 3) {
    Serial.println("Pausado/Despausado");
    Serial.println();
    musRun = !musRun;
  }
  delay(100);
}

// Callback quando recebe mensagem
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Numero: ");
  Serial.println(myData.a);

  musicaPlay();
}

void setup() {
    Serial.begin(9600);

    // Wifi modo estacao
    WiFi.mode(WIFI_AP_STA);

    // Canal do wifi
    int32_t channel = getWiFiChannel(WIFI_SSID);
    WiFi.printDiag(Serial);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    WiFi.printDiag(Serial);

    // Print mac address
    Serial.println(WiFi.macAddress());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());

    // Inicia ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }    

    // Callbacks
    esp_now_register_recv_cb(OnDataRecv);

    // Setar pino CS do cartao SD como OUTPUT e setar HIGH
    pinMode(SD_CS, OUTPUT);      
    digitalWrite(SD_CS, HIGH); 
    
    // Comeca SPI 
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
   
    // Comeca comunicacao com cartao SD
    if(!SD.begin(SD_CS))
    {
      Serial.println("Error accessing microSD card!");
      while(true); 
    }
    
    // I2S 
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // Volume
    audio.setVolume(21);

    // Lista arquivos em /
    listDir(SD, "/", 0);
}

// Pausa/despausa musica
void musicaPlaying() {
  if (musRun == true) {
    audio.loop(); 
  } else {
    
  }
}
 
void loop() {
  // Pausa/despausa musica
  musicaPlaying();
}
