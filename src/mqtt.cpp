#include <mqtt.h>
#include <basics.h>
#include <km271.h>
#include <WiFi.h>
#include <oilmeter.h>

/* D E C L A R A T I O N S ****************************************************/  
WiFiClient espClient;
AsyncMqttClient mqtt_client;
s_mqtt_cmds mqttCmd;  // exts that are used as topics for KM271 commands
muTimer mqttReconnectTimer = muTimer();           // timer for reconnect delay
int mqtt_retry = 0;
bool bootUpMsgDone = false;

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char * addTopic(const char *suffix){
  static char newTopic[256];
  strcpy(newTopic, config.mqtt.topic);
  strcat(newTopic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect(bool sessionPresent) {
  mqtt_retry = 0;
  Serial.println("MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();
  // ... and resubscribe
  mqtt_client.subscribe(addTopic("/cmd/#"), 0);
  mqtt_client.subscribe(addTopic("/setvalue/#"), 0);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets disconnected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("MQTT disconnected");
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  payload[len] = '\0';

  long intVal = atoi((char*)payload);
  float floatVal = atoff((char*)payload);

  Serial.print("topic: ");
  Serial.println(topic);

  // ESP restarten auf Kommando
  if (strcmp (topic, addTopic(mqttCmd.RESTART[config.lang])) == 0){
    mqtt_client.publish(addTopic("/message"), 0, false, "restart requested!");
    delay(1000);
    ESP.restart();
  }
  // set date and time
  else if (strcmp (topic, addTopic(mqttCmd.DATETIME[config.lang])) == 0){
    Serial.println("cmd set date time");
    km271SetDateTimeNTP();
  }
  // set oilmeter
  else if (strcmp (topic, addTopic(mqttCmd.OILCNT[config.lang])) == 0){
    Serial.println("cmd setvalue oilcounter");
    cmdSetOilmeter(intVal);
  }
  // HK1 Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.HC1_OPMODE[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, intVal);
  }
  // HK2 Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.HC2_OPMODE[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, intVal);
  }
  // HK1 Programm
  else if (strcmp (topic, addTopic(mqttCmd.HC1_PRG[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, intVal);
  }
  // HK2 Programm
  else if (strcmp (topic, addTopic(mqttCmd.HC2_PRG[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, intVal);
  }
  // HK1 Auslegung
  else if (strcmp (topic, addTopic(mqttCmd.HC1_INTERPRET[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, intVal);
  }
  // HK2 Auslegung
  else if (strcmp (topic, addTopic(mqttCmd.HC2_INTERPRET[config.lang])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_SWITCH_OFF_THRESHOLD[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_SWITCH_OFF_THRESHOLD[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, intVal);
  }  
  // HK1 Tag-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_DAY_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, floatVal);
  }  
  // HK2 Tag-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_DAY_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, floatVal);
  } 
  // HK1 Nacht-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_NIGHT_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, floatVal);
  }  
  // HK2 Nacht-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_NIGHT_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, floatVal);
  }
  // HK1 Ferien-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_HOLIDAY_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, floatVal);
  }  
  // HK2 Ferien-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_HOLIDAY_SETPOINT[config.lang])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, floatVal);
  } 
  // WW Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.WW_OPMODE[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, intVal);
  }
  // HK1 Sommer-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_SUMMER[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_SUMMER, intVal);
  }  
  // HK1 Frost-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_FROST[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_FROST, intVal);
  } 
  // HK2 Sommer-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_SUMMER[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_SUMMER, intVal);
  }  
  // HK2 Frost-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_FROST[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_FROST, intVal);
  } 
  // WW-Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.WW_SETPOINT[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, intVal);
  } 
  // HK1 Ferien Tage
  else if (strcmp (topic, addTopic(mqttCmd.HC1_HOLIDAYS[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, intVal);
  }  
  // HK2 Ferien Tage
  else if (strcmp (topic, addTopic(mqttCmd.HC2_HOLIDAYS[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, intVal);
  } 
  // WW Pump Cycles
  else if (strcmp (topic, addTopic(mqttCmd.WW_PUMP_CYCLES[config.lang])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, intVal);
  } 
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup(){
  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);
  mqtt_client.setServer(config.mqtt.server, config.mqtt.port);
  mqtt_client.setClientId(config.wifi.hostname);
  mqtt_client.setCredentials(config.mqtt.user, config.mqtt.password);
  mqtt_client.setWill(addTopic("/status"), 0, true, "offline");
  mqtt_client.setKeepAlive(10);
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void checkMqtt(){
  // automatic reconnect to mqtt broker if connection is lost - try 5 times, then reboot
  if (!mqtt_client.connected() && WiFi.isConnected()) {
    if (mqtt_retry==0){
      mqtt_retry++;
      mqtt_client.connect();
      Serial.println("MQTT - connection attempt: 1/5");
    }
    else if (mqttReconnectTimer.delayOnTrigger(true , MQTT_RECONNECT)){
      mqttReconnectTimer.delayReset();
      if (mqtt_retry < 5)
      {
        mqtt_retry++;
        mqtt_client.connect();
        Serial.print("MQTT - connection attempt: ");
        Serial.print(mqtt_retry);
        Serial.println("/5");
      }
      else {
        Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
        Serial.println("MQTT connection not possible, esp rebooting...");
        Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
        storeData(); // store Data before reboot
        delay(500);
        ESP.restart();
      }
    }
  }
  // send bootup message after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.connected()){
    bootUpMsgDone = true;
    mqttPublish(addTopic("/message"), "restarted!", false);
  }
}

/**
 * *******************************************************************
 * @brief   MQTT Publish function for external use
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttPublish(const char* sendtopic, const char* payload, boolean retained){
  uint8_t qos = 0;
  mqtt_client.publish(sendtopic, qos, retained, payload);
}

