// ====================================================================
// =        CÓDIGO COMBINADO: Movimiento, Detección de Colisiones    =
// =                       + Control de Bomba de Agua                 =
// =                       + Seguimiento de Luz                      =
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

// --- PINES DEL SENSOR DE LUZ IR ---
const int IR_L = 11; // Sensor Luz Izquierdo
const int IR_M = 12; // Sensor Luz Central
const int IR_R = 13; // Sensor Luz Derecho

// --- CONSTANTES PARA LAS DIRECCIONES DE LA LUZ ---
const int NO_LIGHT = 0;
const int LIGHT_LEFT = 1;
const int LIGHT_CENTER = 2;
const int LIGHT_RIGHT = 3;
const int LIGHT_CENTER_LEFT = 4;
const int LIGHT_CENTER_RIGHT = 5;
const int LIGHT_FRONT = 6;

// --- PARÁMETROS DEL ROBOT ---
const int VELOCIDAD = 140;
const unsigned long TIEMPO_RETROCESO = 300;  // Tiempo de retroceso en ms
const unsigned long TIEMPO_GIRO = 700;       // Tiempo de giro en ms (para evasión de obstáculos)
const unsigned long TIEMPO_GIRO_LUZ = 200;   // Tiempo de giro corto para seguimiento de luz
const unsigned long TIEMPO_AVANCE_TRAS_GIRO_LUZ = 1000; // Tiempo de avance tras giro por luz (1 segundo)
const unsigned long TIEMPO_PAUSA_ANTES_AVANZAR = 8000; // 8 segundos de pausa
const unsigned long TIEMPO_PAUSA_ANTES_RETROCEDER = 1000; // Sin pausa antes de retroceder
const unsigned long TIEMPO_PAUSA_TRAS_GIRO = 500; // 500 ms de pausa tras cada giro
const float DISTANCIA_OBSTACULO = 5; // Umbral de detección de obstáculos en cm

// --- PARÁMETROS DEL SISTEMA DE RIEGO ---
const int umbralSeco = 300;     // Umbral para suelo seco, ajusta según tu sensor

// --- VARIABLES GLOBALES ---
int valorHumedad = 0;
bool bombaEncendida = false;    // Estado actual de la bomba
bool girarIzquierdaLuz = false; // Controla si el giro por luz o aleatorio es a la izquierda

// Variables para el control no bloqueante del robot
unsigned long tiempoInicioAccion;

// Variables para el control no bloqueante del riego
unsigned long tiempoUltimaMedicionHumedad = 0;
const unsigned long intervaloMedicionHumedad = 500; // Medir humedad cada 500 ms

// Variables para el control no bloqueante del movimiento del robot
unsigned long tiempoUltimaMedicionDistancia = 0;
const unsigned long intervaloMedicionDistancia = 20; // Medir distancia cada 20 ms

// Enum para el estado del robot (para lógica no bloqueante)
enum EstadoRobot {
  BUSCAR_LUZ, // Estado para búsqueda de luz
  GIRANDO_LUZ, // Estado para giros por luz o aleatorios
  AVANZANDO_TRAS_GIRO_LUZ, // Estado para avanzar tras giro por luz
  PAUSANDO_TRAS_GIRO, // Pausa tras giro por luz o aleatorio
  PAUSANDO_ANTES_RETROCEDER,
  RETROCEDIENDO,
  GIRANDO,
  PAUSANDO
};
EstadoRobot estadoActual = BUSCAR_LUZ;

// --- DECLARACIÓN DE FUNCIONES ---
int getLightDirection();
void avanzar(int velocidad);
void retroceder(int velocidad);
void girarIzquierda(int velocidad);
void girarDerecha(int velocidad);
void detener();
float medirDistancia();

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

  // Configuración de pines para los sensores IR con pull-up interno
  pinMode(IR_L, INPUT_PULLUP);
  pinMode(IR_M, INPUT_PULLUP);
  pinMode(IR_R, INPUT_PULLUP);

  // Configuración de pines para el sistema de riego
  pinMode(sensorHumedad, INPUT);
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, LOW); // Apagar bomba al inicio (relé activo en LOW)

  Serial.begin(9600);
  randomSeed(analogRead(A2));
}

void loop() {
  // ========================== Lógica del Sistema de Riego (No Bloqueante) ==========================
  if (millis() - tiempoUltimaMedicionHumedad >= intervaloMedicionHumedad) {
    tiempoUltimaMedicionHumedad = millis();

    valorHumedad = analogRead(sensorHumedad);
    Serial.print("Valor humedad: ");
    Serial.println(valorHumedad);

    // Si el suelo está seco y la bomba está apagada, la enciende
    if (valorHumedad > umbralSeco && !bombaEncendida) {
      digitalWrite(pinRele, LOW);
      bombaEncendida = true;
      Serial.println("🌿 Suelo seco - Bomba ENCENDIDA 💧");
    }
    // Si el suelo está húmedo y la bomba está encendida, la apaga
    else if (valorHumedad <= umbralSeco && bombaEncendida) {
      digitalWrite(pinRele, HIGH);
      bombaEncendida = false;
      Serial.println("🌱 Suelo húmedo - Bomba APAGADA");
    }
  }

  // ========================== Lógica del Robot (No Bloqueante) ==========================
  // Verificación de distancia en cada iteración del loop
  bool obstaculoDetectado = false;
  if (millis() - tiempoUltimaMedicionDistancia >= intervaloMedicionDistancia) {
    tiempoUltimaMedicionDistancia = millis();

    float distancia = medirDistancia();
    //Serial.print("Distancia: ");
    //Serial.print(distancia);
    //Serial.println(" cm");

    // Si hay un obstáculo, detener y pasar a retroceder (excepto en PAUSANDO_ANTES_RETROCEDER y GIRANDO)
    if (distancia > 0 && distancia <= DISTANCIA_OBSTACULO && estadoActual != PAUSANDO_ANTES_RETROCEDER && estadoActual != GIRANDO) {
      Serial.println("Obstáculo detectado! Retrocediendo inmediatamente...");
      detener(); // Detener el robot inmediatamente
      estadoActual = PAUSANDO_ANTES_RETROCEDER;
      detener(); // Asegurarse de que el robot se detenga
      tiempoInicioAccion = millis();
      obstaculoDetectado = true;
    }
  }

  // Si no hay obstáculo, continuar con la lógica normal
  if (!obstaculoDetectado) {
    // Obtener dirección de la luz solo en BUSCAR_LUZ
    int direccion = (estadoActual == BUSCAR_LUZ) ? getLightDirection() : NO_LIGHT;

    switch (estadoActual) {
      case BUSCAR_LUZ:
        // Controlar el movimiento según la dirección de la luz
        switch (direccion) {
          case NO_LIGHT:
            // Girar aleatoriamente si no se detecta luz
            girarIzquierdaLuz = (random(0, 2) == 0);
            if (girarIzquierdaLuz) {
              Serial.println("No hay luz, girando izquierda aleatoriamente");
            } else {
              Serial.println("No hay luz, girando derecha aleatoriamente");
            }
            estadoActual = GIRANDO_LUZ;
            tiempoInicioAccion = millis();
            break;
          case LIGHT_LEFT:
            Serial.println("Girando izquierda por luz");
            girarIzquierdaLuz = true;
            estadoActual = GIRANDO_LUZ;
            tiempoInicioAccion = millis();
            break;
          case LIGHT_CENTER:
          case LIGHT_CENTER_LEFT:
          case LIGHT_CENTER_RIGHT:
          case LIGHT_FRONT:  // Aseguramos que se detenga cuando la luz está al frente
            Serial.println("Luz detectada. Deteniendo.");
            detener();
            break;
          case LIGHT_RIGHT:
            Serial.println("Girando derecha por luz");
            girarIzquierdaLuz = false;
            estadoActual = GIRANDO_LUZ;
            tiempoInicioAccion = millis();
            break;
        }
        break;

      case GIRANDO_LUZ:
        // Ejecutar el giro (izquierda o derecha) durante TIEMPO_GIRO_LUZ
        if (!girarIzquierdaLuz) {
          girarIzquierda(VELOCIDAD);
        } else {
          girarDerecha(VELOCIDAD);
        }
        // Verificar distancia durante el giro
        if (millis() - tiempoUltimaMedicionDistancia >= intervaloMedicionDistancia) {
          tiempoUltimaMedicionDistancia = millis();
          float distancia = medirDistancia();
          if (distancia > 0 && distancia <= DISTANCIA_OBSTACULO) {
            Serial.println("Obstáculo detectado durante giro! Retrocediendo...");
            detener();
            estadoActual = PAUSANDO_ANTES_RETROCEDER;
            detener();
            tiempoInicioAccion = millis();
            break;
          }
        }
        // Continuar giro si no hay obstáculo
        if (millis() - tiempoInicioAccion >= TIEMPO_GIRO_LUZ) {
          Serial.println("Giro por luz completo. Avanzando...");
          estadoActual = AVANZANDO_TRAS_GIRO_LUZ;
          avanzar(VELOCIDAD);
          tiempoInicioAccion = millis();
        }
        break;

      case AVANZANDO_TRAS_GIRO_LUZ:
        // Mantener el avance y verificar distancia
        direccion = getLightDirection();
        switch (direccion) {
          case LIGHT_CENTER:
          case LIGHT_CENTER_LEFT:
          case LIGHT_CENTER_RIGHT:
          case LIGHT_FRONT:
            Serial.println("Luz al frente despues del giro, deteniendo.");
            detener();
            break;
          default:
            avanzar(VELOCIDAD);
            break;
        }
        if (millis() - tiempoUltimaMedicionDistancia >= intervaloMedicionDistancia) {
          tiempoUltimaMedicionDistancia = millis();
          float distancia = medirDistancia();
          if (distancia > 0 && distancia <= DISTANCIA_OBSTACULO) {
            Serial.println("Obstáculo detectado durante avance! Retrocediendo...");
            detener();
            estadoActual = RETROCEDIENDO;
            retroceder(VELOCIDAD);
            tiempoInicioAccion = millis();
            break;
          }
        }
        // Continuar avance si no hay obstáculo
        if (millis() - tiempoInicioAccion >= TIEMPO_AVANCE_TRAS_GIRO_LUZ) {
          Serial.println("Avance tras giro completo. Pausando...");
          estadoActual = PAUSANDO_TRAS_GIRO;
          detener();
          tiempoInicioAccion = millis();
        }
        break;

      case PAUSANDO_TRAS_GIRO:
        detener();
        if (millis() - tiempoInicioAccion >= TIEMPO_PAUSA_TRAS_GIRO) {
          Serial.println("Pausa tras giro completa. Volviendo a seguir luz...");
          estadoActual = PAUSANDO;
        }
        break;

      case PAUSANDO_ANTES_RETROCEDER:
        detener();
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
          Serial.println("Pausa completa. Volviendo a seguir luz...");
          estadoActual = BUSCAR_LUZ;
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

  long duracion = pulseIn(ECHO, HIGH, 10000); // Timeout 10 ms
  if (duracion == 0) return 999;

  float distancia = duracion * 0.034 / 2;
  return distancia;
}

// ========================== FUNCIÓN PARA DETECCIÓN DE LUZ ==========================
int getLightDirection() {
  // Leer los estados de los sensores
  bool luzIzquierda = digitalRead(IR_L);
  bool luzCentral = digitalRead(IR_M);
  bool luzDerecha = digitalRead(IR_R);

  // Sensores activos en alto (HIGH cuando detectan luz)
  if (luzIzquierda == HIGH && luzCentral == HIGH && luzDerecha == HIGH) {
    return LIGHT_FRONT;
  } else if (luzIzquierda == HIGH && luzCentral == HIGH) {
    return LIGHT_CENTER_LEFT;
  } else if (luzDerecha == HIGH && luzCentral == HIGH) {
    return LIGHT_CENTER_RIGHT;
  } else if (luzIzquierda == HIGH) {
    return LIGHT_LEFT;
  } else if (luzCentral == HIGH) {
    return LIGHT_CENTER;
  } else if (luzDerecha == HIGH) {
    return LIGHT_RIGHT;
  }

  return NO_LIGHT;
}