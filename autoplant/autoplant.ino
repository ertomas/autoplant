// ====================================================================
// =        CÓDIGO COMBINADO: Movimiento y Detección de Colisiones    =
// =                       + Control de Bomba de Agua                 =
// ====================================================================

// --- PINES DEL ROBOT (MOTOR L298N y SENSOR ULTRASÓNICO) ---
const int ENA = 2;       // Habilitar motor A
const int IN1 = 3;       // Control motor A
const int IN2 = 4;       // Control motor A
const int IN3 = 5;       // Control motor B
const int IN4 = 6;       // Control motor B
const int ENB = 7;       // Habilitar motor B

const int TRIGGER = 8;   // Pin Trigger del sensor ultrasónico
const int ECHO = 9;      // Pin Echo del sensor ultrasónico

// --- PINES DEL SISTEMA DE RIEGO (SENSOR DE HUMEDAD y RELÉ) ---
const int sensorHumedad = A0;   // Pin analógico A0 para el sensor de humedad
const int pinRele = 10;         // Pin digital 10 para el relé

// --- PINES DEL SENSOR DE LUZ IR
const int IR_L = 11; // Sensor Luz Izquierdo
const int IR_M = 12; // Sensor Luz Central
const int IR_R = 13; // Sensor Luz Derecho

// --- PARÁMETROS DEL ROBOT ---
const int VELOCIDAD = 140;
const unsigned long TIEMPO_RETROCESO = 300;  // Tiempo de retroceso en ms
const unsigned long TIEMPO_GIRO = 700;       // Tiempo de giro en ms
const unsigned long TIEMPO_PAUSA_ANTES_AVANZAR = 8000; // 8 segundos de pausa
const unsigned long TIEMPO_PAUSA_ANTES_RETROCEDER = 1500; // Nuevo: 1.5 segundos de pausa antes de retroceder

// --- PARÁMETROS DEL SISTEMA DE RIEGO ---
const int umbralSeco = 300;     // Umbral para suelo seco, ajusta según tu sensor

// --- VARIABLES GLOBALES ---
int valorHumedad = 0;
bool bombaEncendida = false;    // Estado actual de la bomba
bool luzIzquierda = false;
bool luzCentral = false;
bool luzDerecha = false;

// Variables para el control no bloqueante del robot
unsigned long tiempoInicioAccion;

// Variables para el control no bloqueante del riego
unsigned long tiempoUltimaMedicionHumedad = 0;
const unsigned long intervaloMedicionHumedad = 500; // Medir humedad cada 2 segundos

// Variables para el control no bloqueante del movimiento del robot
unsigned long tiempoUltimaMedicionDistancia = 0;
const unsigned long intervaloMedicionDistancia = 50; // Medir distancia cada 50 ms

// Enum para el estado del robot (para lógica no bloqueante)
enum EstadoRobot {
  AVANZANDO,
  PAUSANDO_ANTES_RETROCEDER, // Nuevo estado
  RETROCEDIENDO,
  GIRANDO,
  PAUSANDO // Estado de pausa después de girar
};
EstadoRobot estadoActual = AVANZANDO;

void setup() {
  // Configuración de pines para el robot
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(IR_L, INPUT);
  pinMode(IR_M, INPUT);
  pinMode(IR_R, INPUT);

  // Configuración de pines para el sistema de riego
  pinMode(sensorHumedad, INPUT);
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, LOW); // Apagar bomba al inicio (relé activo en LOW)

  Serial.begin(9600);
  // Se usa A2 para randomSeed para evitar el conflicto con el sensor de humedad que ahora usa A0
  randomSeed(analogRead(A2));
}

void loop() {
  // ========================== Lógica del Sistema de Riego (No Bloqueante) ==========================
  // Verifica si ha pasado el tiempo necesario para medir la humedad
  if (millis() - tiempoUltimaMedicionHumedad >= intervaloMedicionHumedad) {
    tiempoUltimaMedicionHumedad = millis(); // Actualiza el tiempo de la última medición

    valorHumedad = analogRead(sensorHumedad);

    Serial.print("Valor humedad: ");
    Serial.println(valorHumedad);

    Serial.println("Direccion de luz: ");



    // Si el suelo está seco y la bomba está apagada, la enciende
    if (valorHumedad > umbralSeco && !bombaEncendida) {
      digitalWrite(pinRele, LOW);
      bombaEncendida = true;
      Serial.println("🌿 Suelo húmedo - Bomba APAGADA");
    }
    // Si el suelo está húmedo y la bomba está encendida, la apaga
    else if (valorHumedad <= umbralSeco && bombaEncendida) {
      digitalWrite(pinRele, HIGH);
      bombaEncendida = false;
      Serial.println("🌱 Suelo seco - Bomba ENCENDIDA 💧");
    }
  }


  // ========================== Lógica del Robot (No Bloqueante) ==========================
  if (millis() - tiempoUltimaMedicionDistancia >= intervaloMedicionDistancia) {
    tiempoUltimaMedicionDistancia = millis();

    float distancia = medirDistancia();
  //  Serial.print("Distancia: ");
  //  Serial.print(distancia);
  //  Serial.println(" cm");

    switch (estadoActual) {
      case AVANZANDO:
        avanzar(VELOCIDAD);
        if (distancia > 0 && distancia <= 15) {
          Serial.println("Obstáculo detectado! Pausando antes de retroceder...");
          estadoActual = PAUSANDO_ANTES_RETROCEDER;
          detener();
          tiempoInicioAccion = millis();
        }
        break;
      
      case PAUSANDO_ANTES_RETROCEDER:
        if (millis() - tiempoInicioAccion >= TIEMPO_PAUSA_ANTES_RETROCEDER) {
          Serial.println("Pausa completa. Retrocediendo...");
          estadoActual = RETROCEDIENDO;
          retroceder(VELOCIDAD);
          tiempoInicioAccion = millis();
        }
        break;

      case RETROCEDIENDO:
        if (millis() - tiempoInicioAccion >= TIEMPO_RETROCESO) {
          Serial.println("Retroceso completo. Girando...");
          estadoActual = GIRANDO;
          detener();
          tiempoInicioAccion = millis();

          if (random(0, 2) == 0) {
            girarIzquierda(VELOCIDAD);
            Serial.println("Girando izquierda");
          } else {
            girarDerecha(VELOCIDAD);
            Serial.println("Girando derecha");
          }
        }
        break;

      case GIRANDO:
        if (millis() - tiempoInicioAccion >= TIEMPO_GIRO) {
          Serial.println("Giro completo. Pausando...");
          estadoActual = PAUSANDO;
          detener();
          tiempoInicioAccion = millis();
        }
        break;
      
      case PAUSANDO:
        if (millis() - tiempoInicioAccion >= TIEMPO_PAUSA_ANTES_AVANZAR) {
          Serial.println("Pausa completa. Avanzando de nuevo...");
          estadoActual = AVANZANDO;
        }
        break;
    }
  }
}

// ========================== FUNCIONES DE MOVIMIENTO DEL ROBOT ==========================
void avanzar(int velocidad) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void retroceder(int velocidad) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void girarIzquierda(int velocidad) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void girarDerecha(int velocidad) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void detener() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ========================== FUNCIÓN DEL SENSOR ULTRASÓNICO ==========================
float medirDistancia() {
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  // Timeout reducido para que el sensor no bloquee el loop por mucho tiempo
  long duracion = pulseIn(ECHO, HIGH, 10000); // Timeout 10 ms
  if (duracion == 0) return 999;

  float distancia = duracion * 0.034 / 2;
  return distancia;
}
