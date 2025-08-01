
// Pin donde está conectado el sensor capacitivo de humedad de suelo
const int sensorPin = A0;

// Umbrales para niveles de humedad (ajustar según calibración)
const int nivelBajo = 300;   // Muy húmedo (valor bajo)
const int nivelMedio = 600;  // Humedad media
const int nivelalto = 900;   // Humedad baja
// Por encima de nivelMedio es seco

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
}

void loop() {
  int valorSensor = analogRead(sensorPin);

  Serial.print("Valor sensor: ");
  Serial.print(valorSensor);
  Serial.print(" - Nivel de humedad: ");

  if (valorSensor <= nivelBajo) {
    Serial.println("Alto (Muy húmedo)");
  } else if (valorSensor > nivelBajo && valorSensor <= nivelMedio) {
    Serial.println("Medio");
  } else {
    Serial.println("Bajo (Seco)");
  }

  delay(1000);  // Espera 1 segundo antes de la siguiente lectura
}
