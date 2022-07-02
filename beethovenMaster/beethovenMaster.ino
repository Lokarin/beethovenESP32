#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>

 
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

// Porta do servidor
AsyncWebServer server(80);
 
void setup(){
  Serial.begin(9600);

  // Modo estacao
  WiFi.mode(WIFI_STA);

  // Comeca ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

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

  server.on("/musica1Slave", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(1);
  });

  server.on("/musica1SlavePause", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/musica1.html", "text/html");
    enviaDado(3);
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
 
void loop(){
}
