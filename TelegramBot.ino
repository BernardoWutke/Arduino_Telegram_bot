#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "heltec.h"
#define BAND 915E6
/////radio varaveis
char st;

//Pino onde está o Relê
#define RELAY_PIN 25

//Intervalo entre as checagens de novas mensagens
#define INTERVAL 1000

//Token do seu bot. Troque pela que o BotFather te mostrar
#define BOT_TOKEN "codigo"

//Troque pelo ssid e senha da sua rede WiFi
#define SSID "nome_da_rede"
#define PASSWORD "SENHA"

//Comandos aceitos

//Padroes
// Primeiro Dispositivo : 

const String D1_ON = "ligar";
const String D1_OFF = "desligar";

// Segundo Dispositivo
const String D2_ON = "ligar 2"
const String STATS = "status";
const String START = "/start";
// Emojis : 
const String JOINHA = "ud83dudc4d";
//Saudacoes : 
const String BOM_DIA = "bom dia";




//Estado do relê
int relayStatus = HIGH;

//Cliente para conexões seguras
WiFiClientSecure client;
//Objeto com os métodos para comunicarmos pelo Telegram
UniversalTelegramBot bot(BOT_TOKEN, client);
//Tempo em que foi feita a última checagem
uint32_t lastCheckTime = 0;

//Quantidade de usuários que podem interagir com o bot
#define SENDER_ID_COUNT 2
//Ids dos usuários que podem interagir com o bot.
//É possível verificar seu id pelo monitor serial ao enviar uma mensagem para o bot
String validSenderIds[SENDER_ID_COUNT] = {"id", "id"};

void setup()
{
  Serial.begin(115200);
  Heltec.begin(true, true, true, true, BAND);
  //Inicializa o WiFi e se conecta à rede
  setupWiFi();

  //Coloca o pino do relê como saída e enviamos o estado atual
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayStatus);


}

void setupWiFi()
{
  Serial.print("Connecting to SSID: ");
  Serial.println(SSID);

  //Inicia em modo station e se conecta à rede WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  //Enquanto não estiver conectado à rede
  while (WiFi.status() != WL_CONNECTED)
  {

    Heltec.display->setContrast(255);
    Heltec.display->clear();

    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 0, "Conectando");
    delay(100);
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 25, ".");
    delay(100);
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 25, " .");
    delay(100);
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 25, "  .");
    delay(100);
    Heltec.display->display();


  }

  //Se chegou aqui está conectado
  Serial.println();
  Serial.println("Connected");

  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_24);
  Heltec.display->drawString(0, 20, "Conectado !!");
   Heltec.display->display();

  
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++) //para cada mensagem nova
  {
    String chatId = String(bot.messages[i].chat_id); //id do chat
    String senderId = String(bot.messages[i].from_id); //id do contato

    Serial.println("senderId: " + senderId); //mostra no monitor serial o id de quem mandou a mensagem
    Serial.println("A menssagem é : " + bot.messages[i].text);
    Heltec.display->setContrast(255);
    Heltec.display->clear();

    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 0, "Mensagem :");
    if(bot.messages[i].text != "ud83dudc4d"){ 
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 25, bot.messages[i].text);
    } else {
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 25, "Joinha");
    }


    Heltec.display->display();
    boolean validSender = validateSender(senderId); //verifica se é o id de um remetente da lista de remetentes válidos

    if (!validSender) //se não for um remetente válido
    {
      bot.sendMessage(chatId, "Desculpe mas você não tem permissão", "HTML"); //envia mensagem que não possui permissão e retorna sem fazer mais nada
      continue; //continua para a próxima iteração do for (vai para próxima mensgem, não executa o código abaixo)
    }

    String text = bot.messages[i].text; //texto que chegou

    if (text.equalsIgnoreCase(START))
    {
      handleStart(chatId, bot.messages[i].from_name); //mostra as opções
    }
    else if (text.equalsIgnoreCase(D1_ON))
    {
      handleLightOn(chatId); //liga o relê
    }
    else if (text.equalsIgnoreCase(D1_OFF))
    {
      handleLightOff(chatId); //desliga o relê
    }
    else if (text.equalsIgnoreCase(STATS)) {
      handleStatus(chatId); // mostrar status
    } else if (text.equalsIgnoreCase(JOINHA)){
      emoji_joinha(chatId);
    } else if(text.equalsIgnoreCase(BOM_DIA)) {
      bom_dia(chatId);
    }
    else { 
      handleNotFound(chatId);
    }


  }//for
}

boolean validateSender(String senderId)
{
  //Para cada id de usuário que pode interagir com este bot
  for (int i = 0; i < SENDER_ID_COUNT; i++)
  {
    //Se o id do remetente faz parte do array retornamos que é válido
    if (senderId == validSenderIds[i])
    {
      return true;
    }
  }

  //Se chegou aqui significa que verificou todos os ids e não encontrou no array
  return false;
}

void handleStart(String chatId, String fromName)
{
  //Mostra Olá e o nome do contato seguido das mensagens válidas
  String message = "<b>Olá " + fromName + ".</b>\n";
  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}

String getCommands()
{
  //String com a lista de mensagens que são válidas e explicação sobre o que faz
  String message = "Os comandos disponíveis são:\n\n";
  message += "<b>" + LIGHT_ON + "</b>: Para ligar o Dispositivo\n";
  message += "<b>" + LIGHT_OFF + "</b>: Para desligar o Dispositivo\n";
  return message;
}


void handleLightOn(String chatId)
{
  //Liga o relê e envia mensagem confirmando a operação
  relayStatus = LOW; //A lógica do nosso relê é invertida
  digitalWrite(RELAY_PIN, relayStatus);
  LoRa.beginPacket();
  LoRa.print('0');
  int estado = LoRa.endPacket();
  bot.sendMessage(chatId, "A luz está <b>acesa</b>", "HTML");
}

void handleLightOff(String chatId)
{
  //Desliga o relê e envia mensagem confirmando a operação
  relayStatus = HIGH; //A lógica do nosso relê é invertida
  digitalWrite(RELAY_PIN, relayStatus);
  bot.sendMessage(chatId, "O Dispositivo está <b>desligado</b>", "HTML");
  LoRa.beginPacket();
  LoRa.print('1');
  int estado = LoRa.endPacket();
}


void handleStatus(String chatId)
{
  String message = "";

  //Verifica se o relê está ligado ou desligado e gera a mensagem de acordo
  if (relayStatus == LOW) //A lógica do nosso relê é invertida
  {
    message += "A luz está acesa\n";
  }
  else
  {
    message += "A luz está apagada\n";
  }




  bot.sendMessage(chatId, message, "");
}

void handleNotFound(String chatId)
{
  //Envia mensagem dizendo que o comando não foi encontrado e mostra opções de comando válidos
  String message = "Comando não encontrado\n";
  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}

//Zueiras

void  emoji_joinha(String chatId) {
  String message = "👍\n";
  bot.sendMessage(chatId, message, "HTML");
}

void bom_dia(String chatId) {
  String estado = "";

    if (relayStatus == LOW) //A lógica do nosso relê é invertida
  {
    estado = "Ligado";
  }
  else
  {
   estado = "Desligado";
  }
  String menssage = "Bom dia seu dispositivo está : " + estado;
  bot.sendMessage(chatId, menssage, "HTML");
}

void loop()
{
  //Tempo agora desde o boot
  uint32_t now = millis();

  //Se o tempo passado desde a última checagem for maior que o intervalo determinado
  if (now - lastCheckTime > INTERVAL)
  {
    //Coloca o tempo de útlima checagem como agora e checa por mensagens
    lastCheckTime = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }

  Serial.println(LoRa.packetRssi());
}
