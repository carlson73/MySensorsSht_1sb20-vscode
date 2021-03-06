// The code of a simple node SHT-20 for mysensors.
// Inspired by the works of Andrey Berk and Alexei Bogdanov 
// Working code but not yet fully tested for stability in different modes. Also, I am just start with the SrFatcat library.
// I never tire of saying thanks to Alexey for his work.
// (https://bitbucket.org/SrFatcat/mysefektalib/src/master/).

#include "main.h"


const uint32_t sleepingPeriod = 2 * 60 * 1000;  //первое число - минуты
uint8_t counterBattery = 0;              
const uint8_t counterBatterySend = 60;          // Интервал отправки батареи и RSSI. 60 это  раз в час
const float tempThreshold = 0.3;             //  Интервал изменения для отправки    
const float humThreshold = 0.5;
float temp;
float hum;
float old_temp;
float old_hum;
float temp_onewire;
float old_temp_onewire;

DFRobot_SHT20    sht20;

const u_int8_t BLUE_LED = PIN_LED1;
const u_int8_t PIN_BUTTON = PIN_BUTTON1;
// const u_int8_t PIN_OneWire = 11;

#define onewire

#ifdef onewire
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS PIN_OneWire

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;
#endif



void preHwInit() {
   // pinMode(PIN_BUTTON, INPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, HIGH);
}

void before() {
nRF_Init();
disableNfc();
turnOffAdc();       
wait(200);
#ifdef SERIAL_PRINT
    NRF_UART0->ENABLE = 1;  
#else
    NRF_UART0->ENABLE = 0;  
#endif
  happyInit();
}

void setup() {    
    happyConfig();
    sht20.initSHT20();      // Init SHT20 Sensor
    blink(1);
    addDreamPin(PIN_BUTTON, NRF_GPIO_PIN_NOPULL, CDream::NRF_PIN_HIGH_TO_LOW);  // Добавление пробуждения от PIN_BUTTON 
    interruptedSleep.init();
    SHT_read();

    #ifdef onewire
     Serial.print("Locating devices...");
     sensors.begin();
     Serial.print("Found ");
     Serial.print(sensors.getDeviceCount(), DEC);
     Serial.println(" devices.");
     if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
    
     sensors.setResolution(insideThermometer, 9);
     Serial.print("Device 0 Resolution: ");
     Serial.print(sensors.getResolution(insideThermometer), DEC); 
     Serial.println();
    #endif

    // report parasite power requirements
    // Serial.print("Parasite power is: "); 
    // if (sensors.isParasitePowerMode()) Serial.println("ON");
    // else Serial.println("OFF");
        
}

void happyPresentation() {
    happySendSketchInfo("HappyNode nRF52832 SHT&1Wire", "V1.0");
    happyPresent(CHILD_ID_TEMP, S_TEMP, "Temperature");
    happyPresent(CHILD_ID_HUM, S_HUM, "Humidity");
    happyPresent(CHILD_ID_OneWire, S_TEMP, "TempOneWire");
   // happyPresent(MY_SEND_RSSI, S_CUSTOM, "LINK_QUALITY");
}

void loop() {

    happyProcess();

    int8_t wakeupReson = dream(sleepingPeriod);
    if (wakeupReson == MY_WAKE_UP_BY_TIMER){
    SHT_read();
    OneWire_read();
    
    }

    if (PIN_BUTTON) {  // Заглушка на обработку кнопки
        blink(3);
    }

    
}


void receive(const MyMessage & message){
    if (happyCheckAck(message)) return;

}

void SHT_read() {
    hum = sht20.readHumidity();                  // Read Humidity
    temp = sht20.readTemperature();               // Read Temperature

    if ((int)hum < 0) {
    hum = 0.0;
    }
    if ((int)hum > 99) {
    hum = 99.9;
    }

    if ((int)temp < 0) {
    temp = 0.0;
    }
    if ((int)temp > 99) {
    temp = 99.9;
    }

    SHT_send();

    if (counterBattery == 0) {
    happyNode.sendBattery(MY_SEND_BATTERY);
    happyNode.sendSignalStrength(MY_SEND_RSSI);
    blink(1);
    }
    counterBattery ++;
    if (counterBattery == counterBatterySend) {
        counterBattery = 0;
    }
}

void blink (uint8_t flash) {
  for (uint8_t i = 1; i <= flash; i++) {
    digitalWrite(BLUE_LED, LOW);
    wait(50);
    digitalWrite(BLUE_LED, HIGH);
    wait(50);
  }

}

void SHT_send() {
    if (abs(temp - old_temp) >= tempThreshold) {
    old_temp = temp;
    happySend(msgTemp.set(temp, 1));
    blink(1);
    }

    if (abs(hum - old_hum) >= humThreshold) {
    old_hum = hum;
    happySend(msgHum.set(hum, 1));
    blink(1);
    }

}

void OneWire_read() {
  sensors.requestTemperatures();
  temp_onewire = sensors.getTempCByIndex(0);
//    temp_onewire = sensors.getTempC(DeviceAddress);
  if(temp_onewire == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  OneWire_send();
}

void OneWire_send() {
    if (abs(temp_onewire - old_temp_onewire) >= tempThreshold) {
      old_temp_onewire = temp_onewire;
      happySend(msgOneWire.set(temp_onewire, 1));
      blink(1);
    }

}

void nRF_Init() {
  NRF_POWER->DCDCEN = 1; // /включение режима оптимизации питания, расход снижается на 40%, но должны быть установленны емкости 
  NRF_PWM0  ->ENABLE = 0;
  NRF_PWM1  ->ENABLE = 0;
  NRF_PWM2  ->ENABLE = 0;
  NRF_TWIM1 ->ENABLE = 0;
  NRF_TWIS1 ->ENABLE = 0;
}

void disableNfc() {
  NRF_NFCT->TASKS_DISABLE = 1;  //останавливает таски, если они есть
  NRF_NVMC->CONFIG = 1;         //разрешить запись
  NRF_UICR->NFCPINS = 0;        //отключает nfc и nfc пины становятся доступными для использования
  NRF_NVMC->CONFIG = 0;         //запретить запись
}

void turnOffAdc() {
  if (NRF_SAADC->ENABLE) {
    NRF_SAADC->TASKS_STOP = 1;
    while (NRF_SAADC->EVENTS_STOPPED) {}
    NRF_SAADC->ENABLE = 0;
    while (NRF_SAADC->ENABLE) {}
  }
}
