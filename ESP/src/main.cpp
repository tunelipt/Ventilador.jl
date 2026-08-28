#include <Arduino.h>
#include <Wifi.h>
#include <PubSubClient.h>
#include <sMQTTBroker.h>
#include <Adafruit_MCP4725.h>
#include <MCP_DAC.h>

#define led 2          // pino 2 - LED
#define TUNEL 25       // pino 23 - TUNEL

// --------------  Configurações do DAC ----------------//

bool control = false;                                                 // booleana de controle da rotação
int rpm;                                                              // variavel que armazena a velocidade enviada
SPIClass vspi(VSPI);                                                  // Barramento VSPI esp32
MCP4921 MCP(&vspi);                                                   // Objeto direcioando ao DAC 
long map(long x,long in_min,long in_max, long out_min, long out_max); // varivavel para converter rpm(45-400) para bits(0-4095)


// ---------------- CONFIGURAÇÕES DE REDE --------------------//

IPAddress local_IP(192,168,0,47);       
IPAddress gateway(192,168,0,47);
IPAddress subnet(255,255,0,0);

const char* ssid = "rede";                 // NOME DA REDE 
const char* passw = "senharede" ;           // SENHA DA REDE

const char* mqtt_server = "192.168.0.180";  // SERVIDOR MQTT
const char* mqtt_user = "usuariomqtt";            // USER MQTT
const char* mqtt_pass = "senhamqtt";        // SENHA MQTT

String clientId = "Ventilador-";            // ID MQTT - ESP32

// VARIAVES DE LEITURA (MENSAGEM,TOPICO,STATUS)
String last_msg = "";                       
String last_topic = "";
bool new_msg = false;

// ESP como Cliente
WiFiClient espClient;           
PubSubClient client(espClient); 


void wifi(){                                                        // Configuração e conexão Wifi
WiFi.mode(WIFI_STA);                        
if(!WiFi.config(local_IP,gateway,subnet)){
}
WiFi.begin(ssid,passw);

while(WiFi.status() != WL_CONNECTED){
    delay(1000);
}
}


void reconnect(){                                                   // Conexão ao MQTT
  while (!client.connected()) {     // Loop enquanto não reconecta
   Serial.print("Attempting MQTT connection...");
    
   // Conexão do cliente no servidor, com usario e senha

   if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
     Serial.println("connected");
     digitalWrite(led,HIGH);
     client.subscribe("tunel/rpm");
     client.subscribe("tunel/status");
    } 

    // Tentar novamente em 5 segundos
    else {
     Serial.print("failed, rc=");
     Serial.print(client.state());
     Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void RPMStart(){                                                    // LIGA TUNEL
  Serial.println("TUNEL:ON");
            control = true;
  digitalWrite(TUNEL, HIGH);
}


void RPMStop(){                                                     // DESLIGA TUNEL
   Serial.println("TUNEL:OFF");
   MCP.write( 0);
             control = false;
              delay(200);
  digitalWrite(TUNEL, LOW);
  
}


void rotacao(int rpm){                                                     // CONTROLA O DAC - ALTERA ROTAÇÃO

if(rpm < 45 || rpm > 400){return;}                                          // Limitar extremos caso erre no comando

int dac_value = round(((rpm - 1) / 550.0) * 4095);

//rpm = constrain(rpm,45,550);
//int dac_value = map(rpm,45,550,0,4095);

MCP.write(dac_value);
}


void callback(char* topic, byte* message, unsigned int length){     // Função de callback (recebimento de mensagem)
  Serial.print("Mensagem recebida no tópico: ");                    // Printar topico
  Serial.println(topic);                                            // Printar topico
  
  
  String msg;                                                       // variavel da mensagem

  for (int i = 0; i < length; i++) {msg += (char)message[i];}       // ler cada caracter da mensagem 
  Serial.print("Conteúdo: ");                                       // Printar mensagem        
  Serial.println(msg);                                              // Printar mensagem   
  
  
  last_msg = msg;                                                   // ultima mensagem
  last_topic = String(topic);                                       // ultimo topico
  new_msg = true;                                                   // Sinalizador de nova mensagem
}


void process(String topico,String msg){                             // Interpretar mensagens recebidas 

    // ----------------- Interpretar mensagens no topico de status do ventilaor ----------------------//

    if(topico =="tunel/status"){               
        if(msg == "run") {RPMStart();}          // QUANDO FOR RECEBIDO A MSG 'RUN' LIGA O TUNEL
        else if(msg =="stop") {RPMStop();}      // QUANDO FOR RECEBIDO A MSG 'STOP' DESLIGA O TUNEL

}
    // ----------------- Interpretar mensagens no topico de rotação do ventilaor ----------------------//
    if(topico =="tunel/rpm"){
    if(control == true) {rotacao(msg.toInt());}  // CHAMA A FUNÇÃO DE TROCA DE ROTAÇÃO
    }
}


void setup(){                                                       // Configurações 

pinMode(led,OUTPUT);                    // Configurar pino led como saida 
pinMode(TUNEL,OUTPUT);                  // Configurar pino tunel como saida
digitalWrite(TUNEL, LOW);               // Iniciar desligado
digitalWrite(led,LOW);                  // Iniciar desligado
vspi.begin();                           // needs to be called before begin()
MCP.begin(5);                           // 5 for VSPI and 15 for HSPI
MCP.write(0);                           // Iniciar DAQ desligado

Serial.begin(115200);                   // Iniciar Serial
wifi();                                 // Inicializar Função Wifi
client.setServer(mqtt_server,1883);     // Conectando no Broker pela porta 1883
client.setCallback(callback);           // Declarando callback 

}


void loop() {                                                       // Rotina de uso
if(!client.connected()){reconnect();}   // Reconectar caso não haja conexão 
client.loop();                          // Loop como client no servidor


if(new_msg){process(last_topic,last_msg);new_msg = false;}  // Se houver alguma nova msg, processe a msg
}
