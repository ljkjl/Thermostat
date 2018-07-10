//in funzione per la prima volta su basetta ramata ore 18.02 giorno 08/06/18. Godi popolo.

#define DEBAGG_MODE 0 //a 1 abilita seriale e comunica dati. altrimenti niente. 

#define PINSENS A1
#define PINPOT A0
#define PININT 3
#define PINRELE 5
#define PINDISP 13
#define PINSAFETY 4
#define PINWTCHDOG 2

#define ISTERESI 0.5 //scarto tra soglie di ON e di OFF, da inserire già diviso a metà
#define TMIN 16
#define TMAX 24

float  temperatura;
float  setPoint;
float  setpointPrec;
unsigned int timebaseCiclo;
unsigned int timebaseWDT;
unsigned int timebaseDisp;
bool GUASTO;

void principale();
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max);
void check();
void blight();

#include <LiquidCrystal.h>

const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;//up to date con orcad e pcb
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  if (DEBAGG_MODE == 1)
    Serial.begin(9600);
  GUASTO = false;
  timebaseCiclo = 0;
  timebaseWDT = 0;
  temperatura = 0;
  setPoint = 0;

  pinMode(PINWTCHDOG, INPUT); //pin in alta impedenza per non scaricare C

  analogReference(INTERNAL);//AREF DAC. Vedere sotto.

  lcd.begin(16, 2);
  lcd.setCursor(1, 0);
  lcd.print("T voluta: ");
  lcd.setCursor(1, 1);
  lcd.print("Caldaia ");
  
  check();
}

/* ------------ LOOP ------------*/
void loop() {

  int raw = analogRead(PINSENS);
  int rawSP = analogRead(PINPOT);
  bool statoInt = digitalRead(PININT);

  temperatura = mapfloat(raw, 0, 1024, 0, 100); //OKAY

  setPoint = mapfloat(rawSP, 0, 1023, TMIN, TMAX);  //OKAY

  if (GUASTO == false)
  {
    blight();
    principale();
    //scrivo valore setpoint a schermo
    lcd.setCursor(11, 0);
    lcd.print(setPoint, 1);
  }

  if (DEBAGG_MODE == 1)
  {
    //PASSAGGIO VALORI PER DEBAGG

    Serial.print("Temp: ");
    Serial.println(temperatura, 5);

    Serial.print("Sp: ");
    Serial.println(setPoint, 1);
  }


  check();
  timebaseCiclo = timebaseCiclo + 1;
  timebaseWDT = timebaseWDT + 1;
  delay(20); //50Hz is the way
}

//CONTROLLO VALORI E USCITE
void principale()
{
    if (timebaseCiclo == 25) //1 ogni 500ms
    {
      if (digitalRead(PININT) == HIGH)
      {
        if (temperatura < setPoint - ISTERESI)
        {
          digitalWrite(PINRELE, HIGH);
          lcd.setCursor(9, 1);
          lcd.print("ACCESA");
        }
        else
        {
          if (temperatura >= setPoint + ISTERESI)
          {
            digitalWrite(PINRELE, LOW);
            lcd.setCursor(9, 1);
            lcd.print("SPENTA");
          }
        }
      }
      else
      {
        digitalWrite(PINRELE, LOW);
        lcd.setCursor(9, 1);
        lcd.print("SPENTA");
      }
      timebaseCiclo = 0; //overflow fantastici e come evitarli
    }
}

//funzione MAP ma con ritorno in float (non standard)
float mapfloat(int x, float in_min, float in_max, float out_min, float out_max) //CASTOM FANCTIONSSSS. Map usa solo formato int.
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
  x = 0;
}

//controllo pin safety e reset watchdog. altrimenti messaggio allarme.
void check()
{
  if (digitalRead(PINSAFETY) == digitalRead(PINRELE))
  {
    //il transistor e il relè fungono bene
    GUASTO = false;
    if(timebaseWDT >= 500) //1 volta ogni 10 secondi
      {
        pinMode(PINWTCHDOG, OUTPUT);
        digitalWrite(PINWTCHDOG, LOW);
        if(timebaseWDT == 550) //aka, ogni 10s + 1s
        {
          pinMode(PINWTCHDOG, INPUT);
          timebaseWDT = 0;
        }
      }
  }
  else
  {
    //houston we've had a problem here
    GUASTO = true;
    digitalWrite(PINRELE, LOW);
    digitalWrite(PINDISP, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("ASSISTENZA  TEL.");
    lcd.setCursor(0, 1);
    lcd.print("  348 651 7961");
  }
}

//controllo illuminazione schermo.
void blight()
{
  
  if (setPoint, 1 != setpointPrec, 1)
  {
    digitalWrite(PINDISP, HIGH);
  }
  
  if (digitalRead(PINDISP) == HIGH)
  {
      timebaseDisp = timebaseDisp + 1;
  }

  if (timebaseDisp == 150)
  {
    digitalWrite(PINDISP, LOW);
    timebaseDisp = 0;
  }
    
  setpointPrec = setPoint;


  if (DEBAGG_MODE == 1)
  {
    Serial.print("spp: ");
    Serial.println(setpointPrec, 1);

    Serial.print("TBD: ");
    Serial.println(timebaseDisp, 1);
  }
}


