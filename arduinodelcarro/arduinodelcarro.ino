// Pines del L298N
const int IN1 = 8;
const int IN2 = 9;
const int IN3 = 10;
const int IN4 = 11;
const int ENA = 5;
const int ENB = 6;

// Pines del sensor ultrasónico
const int TRIGGER = 3;
const int ECHO = 4;

// Parámetros de velocidad y tiempos
const int VELOCIDAD = 200;
const unsigned long TIEMPO_RETROCESO = 600;  // ms
const unsigned long TIEMPO_GIRO = 700;       // ms

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

  Serial.begin(9600);
  randomSeed(analogRead(A0));  // Inicializa generador aleatorio
}

void loop() {
  float distancia = medirDistancia();

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  if (distancia > 0 && distancia <= 5) {
    detener();
    delay(100);

    // Retroceder
    retroceder(VELOCIDAD);
    delay(TIEMPO_RETROCESO);
    detener();
    delay(100);

    // Girar aleatoriamente a izquierda o derecha
    if (random(0, 2) == 0) {
      girarIzquierda(VELOCIDAD);
      Serial.println("Girando izquierda");
    } else {
      girarDerecha(VELOCIDAD);
      Serial.println("Girando derecha");
    }
    delay(TIEMPO_GIRO);
    detener();
    delay(100);
  } else if (distancia > 5) {
    avanzar(VELOCIDAD);
  } else {
    detener();  // En caso de lectura inválida
  }

  delay(50);
}

// Funciones de movimiento
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

// Función para medir distancia con sensor ultrasónico
float 
medirDistancia() {
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  long duracion = pulseIn(ECHO, HIGH, 20000);  // Timeout 20 ms
  if (duracion == 0) return -1;

  float distancia = duracion * 0.034 / 2;
  return distancia;
}