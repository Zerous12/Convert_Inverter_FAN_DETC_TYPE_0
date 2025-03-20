    /*  
     * FAN-DETC-Sensor-TYPE-CERO  
     * Conversion logic for inverter refrigerators  
     * Version: 1.2.4  
     * Last Update: 20/03/2025 20:32  
     *  
     * Autor: Richard Mequert (Zerous)       
     * Licencia: Apache 2.0  
     */


#define VOLTAJE_PIN D5         // Pin de detección de 12V (HIGH = voltaje presente)
#define RELAY_PIN D6           // Salida del relé
#define LED_ONBOARD D4         // LED integrado (lógica invertida)
#define RETARDO_ACTIVACION 5000    // 5 segundos después de confirmación
#define DESCANSO_COMPRESOR 300000  // 5 minutos de descanso
#define TIEMPO_CONFIRMACION 60000  // 90 segundos de confirmación inicial

// Variables de control de tiempo
unsigned long lastTriggerTime = 0;
unsigned long lastOffTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long voltageStableStartTime = 0;

// Estados del sistema
bool relayOn = false;
bool enEspera = false;
bool voltageStable = false;
bool ledState = HIGH;

// Control de mensajes serial
bool mensajeStandbyImpreso = false;
bool mensajeEspera2Impreso = false;
bool mensajeActivoImpreso = false;
bool mensajeApagadoImpreso = false;
bool mensajeEsperaFinalizadaImpreso = false;
bool mensajePruebaImpreso = false;

void setup() {
    Serial.begin(115200);
    pinMode(VOLTAJE_PIN, INPUT);  // Usamos INPUT, ya que el sensor entrega 3.3V
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_ONBOARD, OUTPUT);
    
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_ONBOARD, HIGH);
    
    Serial.println("\nSistema iniciado - Esperando confirmación de voltaje...");
}

void loop() {
    int estadoVoltaje = digitalRead(VOLTAJE_PIN);
    
    // ==============================================
    // Modo Confirmación Inicial (90 segundos estables)
    // ==============================================
    if (!voltageStable) {
        if (estadoVoltaje == HIGH) {  // Voltaje presente
            if (voltageStableStartTime == 0) {
                voltageStableStartTime = millis();
                Serial.println("⚡ Voltaje detectado - Iniciando conteo de 90 segundos...");
            }
            
            // Mostrar mensaje único de modo prueba
            if (!mensajePruebaImpreso) {
                Serial.println("🔧 Modo prueba: Confirmando estabilidad del voltaje...");
                mensajePruebaImpreso = true;
            }
            
            // Verificar si se completaron los 90 segundos
            if (millis() - voltageStableStartTime >= TIEMPO_CONFIRMACION) {
                voltageStable = true;
                Serial.println("✅ Confirmación exitosa - Sistema operativo");
                digitalWrite(LED_ONBOARD, LOW); // LED fijo (encendido)
                delay(1000);
                digitalWrite(LED_ONBOARD, HIGH);
            }
            
            parpadearLED(300); // Parpadeo rápido durante la prueba
        } 
        else {  // Si no hay voltaje (LOW)
            if (voltageStableStartTime != 0) {
                Serial.println("❌ Voltaje interrumpido - Reiniciando conteo...");
                voltageStableStartTime = 0;
                mensajePruebaImpreso = false;
            }
            parpadearLED(1000); // Parpadeo lento en standby
        }
        digitalWrite(RELAY_PIN, LOW); // Asegura que el relé permanezca apagado
        return; // No se ejecuta el resto del loop hasta confirmar voltaje estable
    }

    // ==============================================
    // Operación Normal (post confirmación)
    // ==============================================
    
    // Estados del LED según la señal del sensor
    if (estadoVoltaje == LOW && !enEspera && !relayOn) {
        if (!mensajeStandbyImpreso) {
            Serial.println("🟢 Standby: Esperando señal (voltaje ausente)");
            mensajeStandbyImpreso = true;
            resetMensajes();
        }
        parpadearLED(500);  // Parpadeo a 1 Hz en standby
    }

    if (estadoVoltaje == HIGH && !relayOn) {
        if (!mensajeEspera2Impreso) {
            Serial.println("⏳ Retardo activación: 5 segundos");
            mensajeEspera2Impreso = true;
            resetMensajes();
        }
        parpadearLED(100);  // Parpadeo rápido mientras se espera retardo
    }

    if (relayOn) {
        if (!mensajeActivoImpreso) {
            Serial.println("🚀 Relé activado - Compresor funcionando");
            mensajeActivoImpreso = true;
            resetMensajes();
        }
        digitalWrite(LED_ONBOARD, LOW);  // LED encendido (lógica inversa)
    }

    // Lógica de control del relé
    if (estadoVoltaje == HIGH) {  // Se detecta voltaje (sensor activo)
        if (!relayOn && !enEspera) {
            if (millis() - lastTriggerTime >= RETARDO_ACTIVACION) {
                activarRele();
            }
        }
    } else {  // Voltaje ausente
        if (relayOn) {
            desactivarRele();
        }
    }

    // Control del tiempo de descanso del compresor
    if (enEspera && (millis() - lastOffTime >= DESCANSO_COMPRESOR)) {
        enEspera = false;
        Serial.println("🔄 Descanso completado - Listo para nuevo ciclo");
    }
}

// ==============================================
// Funciones auxiliares
// ==============================================
void activarRele() {
    digitalWrite(RELAY_PIN, HIGH);
    relayOn = true;
    lastTriggerTime = millis();
    Serial.println("⚡ Relé activado");
}

void desactivarRele() {
    digitalWrite(RELAY_PIN, LOW);
    relayOn = false;
    enEspera = true;
    lastOffTime = millis();
    Serial.println("💤 Relé desactivado - Iniciando descanso");
    resetMensajes();
}

void parpadearLED(int intervalo) {
    if (millis() - lastBlinkTime >= intervalo) {
        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(LED_ONBOARD, ledState);
    }
}

void resetMensajes() {
    mensajeStandbyImpreso = false;
    mensajeEspera2Impreso = false;
    mensajeActivoImpreso = false;
    mensajeApagadoImpreso = false;
    mensajeEsperaFinalizadaImpreso = false;
}
