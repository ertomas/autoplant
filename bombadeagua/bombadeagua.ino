 
const int sensorHumedad = A0;   // Pin del sensor de humedad
const int pinRele = 7;          // Pin del relé

const int umbralSeco = 600;     // Ajusta según tu sensor

int valorHumedad = 0;
bool bombaEncendida = false;    // Estado actual de la bomba

void setup() {
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, HIGH);   // Bomba apagada al inicio (relé activo en LOW)
  Serial.begin(9600);
}

void loop() {
  valorHumedad = analogRead(sensorHumedad);

  Serial.print("Valor humedad: ");
  Serial.println(valorHumedad);

  // Suelo seco y bomba apagada → encender
  if (valorHumedad > umbralSeco && !bombaEncendida) {
    digitalWrite(pinRele, LOW);     // Encender bomba
    bombaEncendida = true;
    Serial.println("🌱 Suelo seco - Bomba ENCENDIDA 💧");
  }

  // Suelo húmedo y bomba encendida → apagar
  else if (valorHumedad <= umbralSeco && bombaEncendida) {
    digitalWrite(pinRele, HIGH);    // Apagar bomba
    bombaEncendida = false;
    Serial.println("🌿 Suelo húmedo - Bomba APAGADA");
  }

  delay(2000);  // Espera 2 segundos
}


