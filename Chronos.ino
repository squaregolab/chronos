#include <Wire.h>
#include <SPI.h>
#include "LiquidCrystal.h"
#include <Adafruit_PN532.h>
#include <avr/wdt.h>
#include <Dhcp.h>
#include <Dns.h>
#include <ethernet_comp.h>
#include <UIPClient.h>
#include <UIPEthernet.h>
#include <UIPServer.h>
#include <UIPUdp.h>

//creation du lecteur rfid
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
//creation du lcd
LiquidCrystal lcd(8, 9, 0, 1, 6, 7);

int relais = A0; //pin pour le relais
int lecture = 0; //nombre de lecture effectuee
unsigned long temps = 0; //temps depuis l'allumage de la machine
unsigned long minutes = 0; //temps en minute

//shield ethernet
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xA5, 0x7E };
IPAddress ip(192, 168, 0, 143);
EthernetClient client;
char serveur[] = "armellka.esy.es";
char erreur = 0;

void setup(void) {
  erreur = Ethernet.begin(mac);
  delay(1000);

  pinMode(relais, OUTPUT);
  lcd.begin(16, 2);
  nfc.begin();
  lcd.write(" Bienvenue chez");
  lcd.setCursor(0, 1);
  lcd.write("  SquaregoLab");
  delay(1500);

  nfc.SAMConfig();
  lcd.clear();
}


void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength;

  lcd.setCursor(0, 0);
  lcd.write("Presentez votre");
  lcd.setCursor(0, 1);
  lcd.write("     carte");

  while (lecture < 2) {
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);//lecture de la carte rfid

    if (success) {
      nfc.PrintHex(uid, uidLength);
      //si on arrive a lire une carte rfid, on active le relais et le calcul du temps d'utilisation
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write(" Utilisation en");
      lcd.setCursor(0, 1);
      lcd.write("     cours");
      lecture += 1;
      digitalWrite(relais, HIGH);
      temps = millis();
      delay(400);

      if (lecture == 2) {
        //si on lit une deuxieme fois la carte, on desactive le relais et on convertit le temps d'utilisation en minute
        lcd.clear();
        minutes = (temps / 60000) + 1;
        lcd.print(minutes);
        lcd.setCursor(9, 0);
        lcd.write("minutes");
        digitalWrite(relais, LOW);
        if (client.connect(serveur, 80)) {
          //si on arrive a se connecter au serveur, on envoie l'id de la carte et le temps d'utilisation en minutes
          client.print("GET /test.php?c=");
          client.print(uid[0]);
          client.print(uid[1]);
          client.print(uid[2]);
          client.print(uid[3]);
          client.print("&t=");
          client.print(minutes);
          client.println(" HTTP/1.1");
          client.println("Host: armellka.esy.es");
          client.println("Connection: close");
          client.println();
        }
        delay(3000);
        if (client.connected()) {
          client.stop();
          delay(1000);
          softwareReset( WDTO_60MS);
        }
      }
    }
  }
}

//fonction de comparaison de tableaux
int cmp_tab(uint8_t *tab1, uint8_t *tab2, int taille) { //ou ce que tu veux à la place de "int"
  for (int i = 0; i < taille; i++) {
    if (tab1[i] != tab2[i]) return 0;
  }
  return 1;
}

//fonction d'auto reset
void softwareReset( uint8_t prescaller) {
  wdt_enable( prescaller);
  while (1) {}
}
