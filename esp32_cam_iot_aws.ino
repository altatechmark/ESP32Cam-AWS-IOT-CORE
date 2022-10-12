#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
int WiFiConnectCount=0;
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#include <ESP32Ping.h>


#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "esp_camera.h"
#include <WiFiManager.h>

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
const char uid[] = "NT0YGG94EdcaYdjSytCIlSt5Jzs2";
//const char uid[] = "user_id_01";
const char pwd[] = "12345678";
const char cam[] = "/cam1";

#define ESP32CAM_PUBLISH_TOPIC "NT0YGG94EdcaYdjSytCIlSt5Jzs2/cam1"
bool homeWifi = false;
const int bufferSize = 1024 * 23; // 23552 bytes

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(bufferSize);


IPAddress  ip(192,168,4,4);
IPAddress  gateway(192,168,4,1);
IPAddress  subnet(255,255,255,0);

String wifilist;
//String wifiLst[]={};
//const char *SSID = "Need_for_speed";
//const char *PWD = "aforapple123";
 
// Web server running on port 80
WebServer serverr(80);
 
 
// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

void connectAWS()
{
   //Serial.println("here");  
    //Serial.begin(115200);
    //WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("\n\n=====================");
  Serial.println("Connecting to Wi-Fi");
  Serial.println("=====================\n\n");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }


  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  client.setCleanSession(true);

  Serial.println("\n\n=====================");
  Serial.println("Connecting to AWS IOT");
  Serial.println("=====================\n\n");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    ESP.restart();
    return;
  }

  Serial.println("\n\n=====================");
  Serial.println("AWS IoT Connected!");
  Serial.println("=====================\n\n");
  create_json_1("aws_connected");
  
  serverr.send(200, "application/json", buffer);
}

void cameraInit(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
}


void grabImage(){
  camera_fb_t * fb = esp_camera_fb_get();
  if(fb != NULL && fb->format == PIXFORMAT_JPEG && fb->len < bufferSize){
    Serial.print("Image Length: ");
    Serial.print(fb->len);
    Serial.print("\t Publish Image: ");
    bool result = client.publish(ESP32CAM_PUBLISH_TOPIC, (const char*)fb->buf, fb->len);
    Serial.println(result);

    if(!result){
      ESP.restart();
    }
  }
  esp_camera_fb_return(fb);
  delay(1);
}

 
void accessPoint(){
  WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(uid, pwd,7,0,3);
  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());
  }
//void connectToWiFi() {
//  Serial.print("Connecting to ");
//  Serial.println(SSID);
//  
//  WiFi.begin(SSID, PWD);
//  
//  while (WiFi.status() != WL_CONNECTED) {
//    Serial.print(".");
//    delay(500);
//    // we can even make the ESP32 to sleep
//  }
// 
//  Serial.print("Connected. IP: ");
//  Serial.println(WiFi.localIP());
//}
void setup_routing() {     
  serverr.on("/wifi", getWifiList);
  serverr.on("/uid", getUID);
  serverr.on("/aws", connectAWS);     
  serverr.on(UriBraces("/ssid/{}/pwd/{}"),connectWifiResp);

  // start server    
  serverr.begin();    
}
 
void create_json(String tag) {  
  jsonDocument.clear();  
  jsonDocument["UID"] = tag;
  //jsonDocument["value"] = value;
  //jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
}
 void create_json_1(String value1) {  
  jsonDocument.clear();  
  
  jsonDocument["WiFi"] = value1;
  //jsonDocument["value2"] = value2;
  serializeJson(jsonDocument, buffer);
}

void create_json_2(String value1,String value2) {  
  jsonDocument.clear();  
  
  jsonDocument["WiFi_status"] = value1;
  jsonDocument["Wifi_Local_IP"] = value2;
  serializeJson(jsonDocument, buffer);
}
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}
 
void getWifiList() {
   
  Serial.println("scanning");
  int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
             wifilist += WiFi.SSID(i)+":";
             
            //Serial.print(i + 1);
            //Serial.print(": ");
            //Serial.print(WiFi.SSID(i));
            //Serial.print(" (");
            //Serial.print(WiFi.RSSI(i));
            //Serial.print(")");
            //Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            //delay(10);
        }
        Serial.println(wifilist);
    }
    Serial.println("");
  Serial.println("Get temperature");
  create_json_1(wifilist);
  wifilist="";
  serverr.send(200, "application/json", buffer);
}
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}
 void connectWifiResp(){
  
    String new_ssid = serverr.pathArg(0);
    String ssid_no_space = new_ssid;
    ssid_no_space.replace("%20"," ");
    int new_ssid_length = ssid_no_space.length()+1;
    char ch_new_ssid[new_ssid_length];
    ssid_no_space.toCharArray(ch_new_ssid,new_ssid_length);
    Serial.println(ch_new_ssid);
    
    String new_pwd = serverr.pathArg(1);

    int new_pwd_length = new_pwd.length()+1;
    char ch_new_pwd[new_pwd_length];
    new_pwd.toCharArray(ch_new_pwd,new_pwd_length);
    Serial.println(ch_new_pwd);

    WiFi.begin(ch_new_ssid, ch_new_pwd);
    
  Serial.println("\n\n=====================");
  Serial.println("Connecting to Wi-Fi");
  Serial.println("=====================\n\n");

  while ((WiFi.status() != WL_CONNECTED)&&(WiFiConnectCount<=20)){
    WiFiConnectCount++;
    delay(500);
    Serial.print(WiFiConnectCount);
  }
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  client.setCleanSession(true);

  Serial.println("\n\n=====================");
  Serial.println("Connecting to AWS IOT");
  Serial.println("=====================\n\n");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    ESP.restart();
    return;
  }

  Serial.println("\n\n=====================");
  Serial.println("AWS IoT Connected!");
  Serial.println("=====================\n\n");

  if(WiFi.status() != WL_CONNECTED){
    String statuss = "Can not be connected with "+String(ch_new_ssid);
    Serial.println(statuss);
    create_json_2(statuss,"None");
  //wifilist="";
  serverr.send(200, "application/json", buffer);
    }
    if(WiFi.status() == WL_CONNECTED){
      bool success = Ping.ping("www.google.com", 3);
 
  if(!success){
    Serial.println("Ping failed");
    create_json_2("Wifi_connected","No Internet");
  }
 
  Serial.println("Ping succesful.");
      //cameraInit();
      //connectAWS();
    String statuss = "connected with "+String(ch_new_ssid);
    Serial.println(statuss);
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
   
    
    
    create_json_2(statuss,IpAddress2String(WiFi.localIP()));
  //wifilist="";
  serverr.send(200, "application/json", buffer);
    }


    
    
    
    //server.send(200, "text/plain", "SSID: '" + ssid_no_space + "' and pwd: '" + new_pwd + "'");
  
  }
 
 void getUID() {
  
    create_json(uid);
  
  serverr.send(200, "application/json", buffer);
  }


void setup() {     
  Serial.begin(115200);    
   
   
   accessPoint();
   cameraInit();     
      connectAWS();
  setup_routing();     
  // Initialize Neopixel     
     
}    
       
void loop() {    
  serverr.handleClient();
  client.loop();
  if(client.connected()) grabImage();
  
}
