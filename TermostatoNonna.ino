//in funzione per la prima volta su basetta ramata ore 18.02 giorno 08/06/18. Godi popolo.
//versione definitiva 19/07/18, ore 

#define DEBAGG_MODE 0 //a 1 abilita seriale e comunica dati. altrimenti niente. 

//DEFINIZIONE PIN I/O
#define PINSENS A1
#define PINPOT A0
#define PININT 3
#define PINRELE 5
#define PINDISP 13
#define PINSAFETY 4
#define PINWTCHDOG 2

//DEFINIZIONE COSTANTI
#define ISTERESI 0.5 //scarto tra soglie di ON e di OFF, da inserire diviso a metà
#define TMIN 16
#define TMAX 24

//VARIABILI GLOBALI
float  temperatura;
float  setPoint;
float  setpointPrec;
unsigned int timebaseCiclo;
unsigned int timebaseWDT;
unsigned int timebaseDisp;
bool GUASTO;
bool statoInt;

//PROTOTIPI FUNZIONI
void ingressi();
void uscite();
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max);
void blight();
void check();

//LIBRERIE
#include <LiquidCrystal.h>

//DICHIARAZIONE OGGETTI
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;  //up to date con orcad e pcb
LiquidCrystal Schermo(rs, en, d4, d5, d6, d7);

/* ------------ SETUP -----------*/
void setup() {
  if (DEBAGG_MODE != 0)
    Serial.begin(9600);

  pinMode(PININT, INPUT_PULLUP);
  
  GUASTO = false;
  timebaseCiclo = 0;
  timebaseWDT = 0;
  temperatura = 0;
  setPoint = 0;

  pinMode(PINWTCHDOG, INPUT); //pin in alta impedenza per non scaricare C

  analogReference(INTERNAL);//AREF DAC. Vedere sotto.

  Schermo.begin(16, 2);
  
  //Scritta perditempo per notare avvenuto intervento watchdog
  Schermo.setCursor(4, 0);
  Schermo.print("AVVIO");
  delay(1000);
  Schermo.setCursor(9, 0);
  Schermo.print(".");
  delay(1000);
  Schermo.setCursor(10, 0);
  Schermo.print(".");
  delay(1000);
  Schermo.setCursor(11, 0);
  Schermo.print(".");
  delay(1000);
  Schermo.clear();

  //Scritte normali
  Schermo.setCursor(1, 0);
  Schermo.print("T voluta: ");
  Schermo.setCursor(1, 1);
  Schermo.print("Caldaia ");
  
  check();
}

/* ------------ LOOP ------------*/
void loop() {

  if (GUASTO == false)
  {
    blight();
    uscite();
    //scrivo valore setpoint a Schermo
    Schermo.setCursor(11, 0);
    Schermo.print(setPoint, 1);
  }

  if (DEBAGG_MODE == 1)
  {
    //PASSAGGIO VALORI PER DEBAGG

    Serial.print("Temp: ");
    Serial.println(temperatura, 1);

    Serial.print("Sp: ");
    Serial.println(setPoint, 1);
  }

  check();
  timebaseCiclo = timebaseCiclo + 1;
  timebaseWDT = timebaseWDT + 1;
  //digitalWrite(PINDISP, HIGH);
  delay(20); //50Hz is the way
}

//Lettura ed elaborazione ingressi
void ingressi()
{
  int raw = analogRead(PINSENS);
  int rawSP = analogRead(PINPOT);
  statoInt = digitalRead(PININT);

  temperatura = mapfloat(raw, 0, 1024, 0, 100); //OKAY

  setPoint = mapfloat(rawSP, 0, 1023, TMIN, TMAX);  //OKAY
}

//Elaborazione dati e setaggio uscite
void uscite()
{
    if (timebaseCiclo == 25) //1 ogni 500ms
    {
      if (digitalRead(PININT) == LOW) //pullup...
      {
        if (temperatura < setPoint - ISTERESI)
        {
          digitalWrite(PINRELE, HIGH);
          Schermo.setCursor(9, 1);
          Schermo.print("ACCESA");
        }
        else
        {
          if (temperatura >= setPoint + ISTERESI)
          {
            digitalWrite(PINRELE, LOW);
            Schermo.setCursor(9, 1);
            Schermo.print("SPENTA");
          }
        }
      }
      else
      {
        digitalWrite(PINRELE, LOW);
        Schermo.setCursor(9, 1);
        Schermo.print("SPENTA");
      }
      timebaseCiclo = 0; //overflow fantastici e come evitarli
    }
}

//funzione MAP ma con ritorno in float (non standard)
float mapfloat(int x, float in_min, float in_max, float out_min, float out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
  x = 0;
}

//controllo illuminazione schermo.
void blight()
{
  float TEMPSP;
  TEMPSP = round(setPoint * 10) / 10;
  setpointPrec = round(setpointPrec * 10) / 10;
  if (TEMPSP != setpointPrec)
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
    Serial.println(TEMPSP);

    Serial.print("TBD: ");
    Serial.println(timebaseDisp, 1);
  }
}

//controllo pin safety e reset watchdog. altrimenti messaggio allarme.
void check()
{
  if ((digitalRead(PINSAFETY) == digitalRead(PINRELE)) && (GUASTO = false))
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
    Schermo.clear();
    Schermo.setCursor(0, 0);
    Schermo.print("ASSISTENZA  TEL.");
    Schermo.setCursor(0, 1);
    Schermo.print("  348 651 7961");
  }
}
