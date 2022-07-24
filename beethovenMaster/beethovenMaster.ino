//Bibliotecas
#include "Arduino.h"
#include <WiFi.h>
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>


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
 
// SSID e Senha do Wifi
const char* ssid = "x";
const char* password = "x";

// Mac address do slave
uint8_t broadcastAddress[] = {0x0C, 0xB8, 0x15, 0xC4, 0x0A, 0x14};

// Estrutura da mensagem enviada
typedef struct struct_message {
  int a;
} struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;

// Callback quando a mensagem é enviada
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Mensagens enviadas
void enviaDado(int nMus) {
  Serial.println("Valor enviado: ");
  Serial.print(nMus);
  esp_err_t result;

  myData.a = nMus;
  result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

// Porta do servidor
AsyncWebServer server(80);

// Tocar musica baseada no valor recebido
void musicaPlay(int mus) {
  if (mus == 1) { 
    Serial.println("Musica1");
    Serial.println();
    audio.connecttoFS(SD,"/musica1.mp3");
  } else if (mus == 2) {
    Serial.println("Musica2");
    Serial.println();
    audio.connecttoFS(SD,"/musica2.mp3");
  } else if (mus == 3) {
    Serial.println("Pausado/Despausado");
    Serial.println();
    musRun = !musRun;
  } else if (mus == 4) {
    Serial.println("Musica3");
    Serial.println();
    audio.connecttoFS(SD,"/musica3.mp3");
  } else if (mus == 5) {
    audio.setVolume(21);
    Serial.println("Vol Alto");
    Serial.println();
  } else if (mus == 6) {
    audio.setVolume(5);
    Serial.println("Vol Alto");
    Serial.println();
  }
  delay(100);
}
 
void setup(){
  Serial.begin(9600);

  // Modo estacao
  WiFi.mode(WIFI_STA);

  // Comeca ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

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

  // Lista arquivos em /
  listDir(SD, "/", 0);

  // I2S 
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
  // Volume
  audio.setVolume(5);

  // Callbacks
  esp_now_register_send_cb(OnDataSent);
  
  // Registra peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Adiciona       
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Inicia SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Conecta na wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Paginas do servidor
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/estilo.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/estilo.css", "text/css");
  });

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/login.html", "text/html");
  });
 
  server.on("/perfis", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/perfis.html", "text/html");
  });

  server.on("/cadastrar", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/cadastrar.html", "text/html");
  });

  server.on("/senha", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/senha.html", "text/html");
  });

  server.on("/musicas", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musicas.html", "text/html");
  });

  server.on("/musica1", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
  });

  server.on("/musica2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
  });

  server.on("/musica3", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
  });

  server.on("/musica1Slave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(1);
  });

  server.on("/musica1SlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(3);
  });

  server.on("/musica1Master", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    musicaPlay(1);
  });

  server.on("/musica1MasterPause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    musicaPlay(3);
  });

  server.on("/musica1MasterSlave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(1);
    musicaPlay(1);
  });

  server.on("/musica1MasterSlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(3);
    musicaPlay(3);
  });

  server.on("/musica1MasterVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    musicaPlay(5);
  });

  server.on("/musica1MasterVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    musicaPlay(6);
  });

  server.on("/musica1SlaveVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(5);
  });

  server.on("/musica1SlaveVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(6);
  });

  server.on("/musica1MSVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(5);
    musicaPlay(5);
  });

  server.on("/musica1MSVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(6);
    musicaPlay(6);
  });

  server.on("/musica2Slave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(2);
  });

  server.on("/musica2SlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(3);
  });

  server.on("/musica2Master", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    musicaPlay(2);
  });

  server.on("/musica2MasterPause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    musicaPlay(3);
  });

  server.on("/musica2MasterSlave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(2);
    musicaPlay(2);
  });

  server.on("/musica2MasterSlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(3);
    musicaPlay(3);
  });

  server.on("/musica2MasterVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    musicaPlay(5);
  });

  server.on("/musica2MasterVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    musicaPlay(6);
  });

  server.on("/musica2SlaveVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(5);
  });

  server.on("/musica2SlaveVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(6);
  });

  server.on("/musica2MSVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(5);
    musicaPlay(5);
  });

  server.on("/musica2MSVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica2.html", "text/html");
    enviaDado(6);
    musicaPlay(6);
  });

  server.on("/musica3Slave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(4);
  });

  server.on("/musica3SlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(3);
  });

  server.on("/musica3Master", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    musicaPlay(4);
  });

  server.on("/musica3MasterPause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    musicaPlay(3);
  });

  server.on("/musica3MasterSlave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(4);
    musicaPlay(4);
  });

  server.on("/musica3MasterSlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(3);
    musicaPlay(3);
  });

  server.on("/musica3MasterVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    musicaPlay(5);
  });

  server.on("/musica3MasterVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    musicaPlay(6);
  });

  server.on("/musica3SlaveVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(5);
  });

  server.on("/musica3SlaveVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(6);
  });

  server.on("/musica3MSVolAlto", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(5);
    musicaPlay(5);
  });

  server.on("/musica3MSVolBaixo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica3.html", "text/html");
    enviaDado(6);
    musicaPlay(6);
  });

  server.on("/music-player2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/music-player2.html", "text/html");
  });

  server.on("/embed", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/embed.html", "text/html");
  });

  // Inicia o servidor
  server.begin();
}

// Pausa/despausa musica
void musicaPlaying() {
  if (musRun == true) {
    audio.loop(); 
  } else {
    
  }
}
 
void loop(){
  // Pausa/despausa musica
  musicaPlaying();
}
