/*********
fddkiller 2024
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <Preferences.h> //EEPROM

// Replace with your network credentials
const char* ssid     = "ESP32-WD";
const char* password = "123456789";

// Set web server port number to 80
WebServer server(80);

WiFiClient client;

// Variable to store the HTTP request
String header;

//for preferens
Preferences Pref;
String router_ssid , router_password , tg_bot , chat_id;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 2;
const int output27 = 27;

const int output17 = 17; //подтяжка для кнопок
const int input18 = 18; //кнопка установки - если нажа при подаче питаний - запустить wifi точку
const int input19 = 19; //отслеживаемый пин

int in18=0;
int in19,in19_old;
int wd_mode=1; //режим установки или отслеживания

void setup() {

  pinMode(input18, INPUT);
  pinMode(input19, INPUT);

  pinMode(output17, OUTPUT);
  digitalWrite(output17, HIGH); //подтягиваем путание к кнопкам

  Serial.begin(115200);
    delay(1000);
    Serial.println("\r\n\r\n");
  Serial.println("Start ESP32 WD by fddkiller");

  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  ReadConfig();


  in18 = digitalRead(input18);
    if(in18==0){ //при старте кнопка нажата
        wd_mode=0;
        setup_mode();

    }else{

        work_mode();

    }

  in19_old = digitalRead(input19);


  // web callback
  // принимаем ajax, отдаём json
  server.on("/", WebSetup);
  server.on("/GetData", HTTP_GET, GetData);
  server.on("/Set", HTTP_GET, Set);

  server.begin();

  SendBot("online");

}

void work_mode(){

  Serial.println("Connect to WIFI:  " + router_ssid + "/" + router_password + " ");

  WiFi.begin(router_ssid, router_password);
  //WiFi.begin("Lan3-2G", "imhotep5");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Work mode OK");
  Serial.println(WiFi.localIP());

  Serial.println("tg bot " + tg_bot);
  Serial.println("chat_id " + chat_id);

}

void setup_mode(){

  // Connect to Wi-Fi network with SSID and password
  Serial.println("Setup mode, connect to wifi ssid ESP32-WD password 123456789");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print(" AP IP address http://");
  Serial.println(IP);

  

}


void ReadConfig(){

  Pref.begin("Settings", false);
    router_ssid = Pref.getString("router_ssid");
    router_password = Pref.getString("router_password");
    tg_bot = Pref.getString("tg_bot");
    chat_id = Pref.getString("chat_id");
  

    if(router_ssid.length()==0){
      router_ssid="SSID";
      Pref.putString("router_ssid", router_ssid);
    }

    if(router_password.length()==0){
      router_password="Password";
      Pref.putString("router_password", router_password);
    }

    Pref.end();

}


void SaveConfig(){

  Pref.begin("Settings", false);
    Pref.putString("router_ssid", router_ssid);
    Pref.putString("router_password", router_password);
    Pref.putString("tg_bot", tg_bot);
    Pref.putString("chat_id", chat_id);
  Pref.end();

}

void Set() { // Callback

  Serial.println("callback Set");

  if (server.hasArg("router_ssid")) {

    router_ssid = server.arg("router_ssid");
    router_password = server.arg("router_password");
    tg_bot = server.arg("tg_bot");
    chat_id = server.arg("chat_id");

    SaveConfig();

  }

  String html = "ok";
  server.send(200, "text/html", html);
}

void GetData() { // Callback

  Serial.println("callback GetData");

  String random_number = String(random(10000));
  String message = "{\"router_ssid\":\"" + router_ssid + "\"," +
                   "\"router_password\":\"" + router_password + "\"," +
                   "\"tg_bot\":\"" + tg_bot + "\","+
                   "\"chat_id\":\"" + chat_id + "\"}";

  server.send(200, "text/plain", message); // Send message back to page
}

void WebSetup() { //CallBack

  String html = R"EOF(
  
  <html>
  <head>
  <title>ESP32 WatchDog Setup</title>
    <script>

    window.onload = function(){
      getData();
    };
  
    function getData() {
      var xmlhttp;
        if (window.XMLHttpRequest) {
          xmlhttp = new XMLHttpRequest();
        }else{
          xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
        
        xmlhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            
            obj =JSON.parse(this.responseText);
            document.getElementById("router_ssid").value = obj.router_ssid;
            document.getElementById("router_password").value = obj.router_password;
            document.getElementById("tg_bot").value = obj.tg_bot;
            document.getElementById("chat_id").value = obj.chat_id;
    
          }
        };
        
      xmlhttp.open("GET", "GetData", true);
      xmlhttp.send();
    }

    function SetServer(){
          xmlhttp = new XMLHttpRequest();
          
          router_ssid = document.getElementById("router_ssid").value;
          router_password = document.getElementById("router_password").value;
          tg_bot = document.getElementById("tg_bot").value;
          chat_id = document.getElementById("chat_id").value;
          
          url="Set?router_ssid="+router_ssid+"&router_password="+router_password+"&tg_bot="+tg_bot+"&chat_id="+chat_id;

          xmlhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
               getData();
            }
          };
          
          xmlhttp.open("GET", url, true);
          xmlhttp.send();
   
    }
         
    </script>
    </head>
  <body>
    <h2>ESP32 WatchDog Setup</h2>
    
    <br>
  
      wifi ssid: <input id=router_ssid type=text value='' name=router_ssid> <br>
      wifi password : <input id=router_password type=text value='' name=router_password> <br>
      telegram bot ID: <input id=tg_bot type=text value='' name=tg_bot> <br>
      telegram chat id: <input id=chat_id type=text value='' name=chat_id> <br>
      <input type=button value=Save onclick="SetServer()" >
      
  </body>
  </html>

  )EOF";

  server.send(200, "text/html", html); //!!! Note that returning to the web page requires "text / html" !!!
}


void SendBot(String status){

  if(tg_bot.length()==0){
    Serial.println("telegram bot ID is null");
    return;
  }

  if(chat_id.length()==0){
    Serial.println("telegram chat_id is null");
    return;
  }

  

  //1961828605:AAGeZ8cNPsh1cuxLoN2pfv_XLLtqN65u1CE
  //1903303223

  String API_URL="https://api.telegram.org/bot"+tg_bot+"/sendMessage?chat_id="+chat_id+"&text=port "+status;

  //UrlEncode.h
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int http_response_code = http.GET();
  if (http_response_code == 200){
    Serial.println("Telegram message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(http_response_code);
  }
  http.end();

}

void loop(){

  server.handleClient(); // Handling requests from clients

  in19 = digitalRead(input19);

  if(in19 != in19_old){

      in19_old=in19;
      if(in19 == 0){
        SendBot("on");
  
      }else{
        SendBot("off");
      }

    delay(500);
  }



}
