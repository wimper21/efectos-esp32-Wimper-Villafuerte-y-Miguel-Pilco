#define PIN_BTN 0         // para el boton
#define PIN_LED_PWM 2     // El led para el pwm
#define NUM_LEDS 5        
const int LEDS_RULETA[NUM_LEDS] = {4, 16, 17, 18, 19}; // Los pines de la ruleta

// Cosas del Timer
hw_timer_t* timer = NULL; // Aquí guardamos la configuración del timer
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // El seguro para no mezclar datos
volatile uint32_t tickCount = 0; // El contador que se mueve cada 10ms

// La interrupción Lo que pasa en segundo plano
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux); // Cerramos el candado 
  tickCount++; // Sumamos un tick (pasaron 10 milisegundos)
  portEXIT_CRITICAL_ISR(&timerMux);  // Abrimos el candado
}

// Función para leer el tiempo sin errores 
uint32_t leerTick() {
  uint32_t temp;
  portENTER_CRITICAL(&timerMux); // Bloqueamos un momento
  temp = tickCount; // Copiamos el valor
  tickCount = 0;    // Lo reiniciamos para la siguiente vuelta
  portEXIT_CRITICAL(&timerMux);  // Desbloqueamos
  return temp;
}

// Estado del sistema 
int estado = 1; // Empezamos en modo 1

void setup() {
  Serial.begin(115200); // 
  
  // Configuramos el botón y los LEDs
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED_PWM, OUTPUT);
  for(int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS_RULETA[i], OUTPUT);
  }

  // Arrancamos el Timer 
  timer = timerBegin(1000000); // Lo ponemos  a 1MHz
  timerAttachInterrupt(timer, &onTimer); // Le decimos qué hacer cuando suene
  timerAlarm(timer, 10000, true, 0); // Que suene cada 10ms 
}

void loop() {
  // Revisar si se toco el botón
  static bool ultimoBoton = HIGH;
  bool botonActual = digitalRead(PIN_BTN);
  
  if (botonActual == LOW && ultimoBoton == HIGH) {
    // Si lo presionaron, cambiamos de juego 1 o 2
    estado = (estado == 1) ? 2 : 1;
    
    // Apagamos todo para que no se quede nada prendido 
    digitalWrite(PIN_LED_PWM, LOW);
    for(int i = 0; i < NUM_LEDS; i++) digitalWrite(LEDS_RULETA[i], LOW);
    Serial.println("¡Cambiamos de modo!");
  }
  ultimoBoton = botonActual;

  //  Ejecutar la lógica 
  uint32_t ticksPasados = leerTick();
  
  if (ticksPasados > 0) {
    if (estado == 1) {
      // MODO: EL LED de pwm
      static int brillo = 0;
      static int paso = 2; // Qué tan rápido sube o baja
      
      brillo += (paso * (int)ticksPasados);
      if (brillo <= 0 || brillo >= 255) paso *= -1; // Si llega al tope, se regresa
      brillo = constrain(brillo, 0, 255);  //Que no se vuelva loco el valor
      
      analogWrite(PIN_LED_PWM, brillo);
      Serial.printf("Brillo: %d\n", brillo);
    } 
    else {
      // MODO: LA RULETA 
      static int tiempoAcumulado = 0;
      static int cualLed = 0;
      
      tiempoAcumulado += ticksPasados;
      
      // Si ya pasaron 50 ticks o sea, 0.5 segundos
      if (tiempoAcumulado >= 50) {
        tiempoAcumulado = 0; // Reiniciamos el conteo
        
        // Apagamos todos y prendemos solo el que sigue
        for(int i = 0; i < NUM_LEDS; i++) digitalWrite(LEDS_RULETA[i], LOW);
        digitalWrite(LEDS_RULETA[cualLed], HIGH);
        
        cualLed = (cualLed + 1) % NUM_LEDS; // Volvemos al inicio al llegar al final
      }
    }
  }
}