#include <IBusBM.h>
#include <SerialRelay.h>

// =====================================================
// LIGACOES - ESP32
// =====================================================
//
// RECEPTOR FLYSKY (iBUS)
// ----------------------
// GND do receptor   -> GND do ESP32
// VCC/5V receptor  -> 5V/VIN do ESP32
// iBUS do receptor -> GPIO 16 do ESP32 (RX2)
//
// MODULO RELE SERIAL ROBOCORE
// ---------------------------
// DATA  -> GPIO 25
// CLOCK -> GPIO 26
// GND   -> GND ESP32
//
// RELES
// ---------------------------
// RELE 1 -> Esteira esquerda
// RELE 2 -> Esteira direita
// RELE 3 -> Auxiliar
// RELE 4 -> Pulso de 1 segundo
//
// =====================================================

IBusBM ibus;


// =====================================================
// PINOS
// =====================================================

#define IBUS_RX_PIN 16

#define LED_STATUS 2

#define RELAY_DATA  25
#define RELAY_CLOCK 26


// =====================================================
// MODULO RELE SERIAL ROBOCORE
// =====================================================

const byte NUM_MODULES = 1;

SerialRelay relays(
  RELAY_DATA,
  RELAY_CLOCK,
  NUM_MODULES
);


// =====================================================
// RELES
// =====================================================

#define RELAY_LEFT   1
#define RELAY_RIGHT  2
#define RELAY_AUX    3
#define RELAY_PULSE  4


// =====================================================
// CANAIS
// =====================================================
//
// readChannel(0) = CH1
// readChannel(1) = CH2
// etc.
//
// =====================================================

// CH1
// Controle auxiliar
#define CHANNEL_AUX 0

// CH2
// Reservado para failsafe
#define CHANNEL_FAILSAFE 1

// CH3
// Frente
#define CHANNEL_FORWARD 2

// CH4
// Esquerda / direita
#define CHANNEL_TURN 3

// CH5
// Potenciometro 1
#define CHANNEL_ARM_1 4

// CH6
// Potenciometro 2
#define CHANNEL_ARM_2 5


// =====================================================
// CONFIGURACOES
// =====================================================

#define JOYSTICK_MIN 1400
#define JOYSTICK_MAX 1600

// CH5 e CH6 precisam estar praticamente no maximo
#define ARM_THRESHOLD 1900

// CH2 abaixo disso = failsafe
#define FAILSAFE_THRESHOLD 1200

// Tempo do rele 4
#define PULSE_TIME 1000


// =====================================================
// ESTADOS DOS RELES
// =====================================================

bool relayLeftState = false;
bool relayRightState = false;
bool relayAuxState = false;
bool relayPulseState = false;


// =====================================================
// CONTROLE DO PULSO
// =====================================================

bool pulseActive = false;
bool triggerLocked = false;

unsigned long pulseStartTime = 0;


// =====================================================
// CONTROLE DOS RELES
// =====================================================

void setRelay(
  int relayNumber,
  bool state
) {

  relays.SetRelay(
    relayNumber,
    state
      ? SERIAL_RELAY_ON
      : SERIAL_RELAY_OFF,
    1
  );


  if (relayNumber == RELAY_LEFT) {
    relayLeftState = state;
  }


  if (relayNumber == RELAY_RIGHT) {
    relayRightState = state;
  }


  if (relayNumber == RELAY_AUX) {
    relayAuxState = state;
  }


  if (relayNumber == RELAY_PULSE) {
    relayPulseState = state;
  }
}


// =====================================================
// DESLIGA TODAS AS ESTEIRAS
// =====================================================

void stopTracks() {

  setRelay(
    RELAY_LEFT,
    false
  );

  setRelay(
    RELAY_RIGHT,
    false
  );
}


// =====================================================
// DESLIGA TODOS OS RELES
// =====================================================

void stopAll() {

  setRelay(
    RELAY_LEFT,
    false
  );

  setRelay(
    RELAY_RIGHT,
    false
  );

  setRelay(
    RELAY_AUX,
    false
  );

  setRelay(
    RELAY_PULSE,
    false
  );


  relayLeftState = false;
  relayRightState = false;
  relayAuxState = false;
  relayPulseState = false;

  pulseActive = false;
  triggerLocked = false;
}


// =====================================================
// FRENTE
// =====================================================

void forward() {

  setRelay(
    RELAY_LEFT,
    true
  );

  setRelay(
    RELAY_RIGHT,
    true
  );
}


// =====================================================
// ESQUERDA
// =====================================================

void left() {

  setRelay(
    RELAY_LEFT,
    true
  );

  setRelay(
    RELAY_RIGHT,
    false
  );
}


// =====================================================
// DIREITA
// =====================================================

void right() {

  setRelay(
    RELAY_LEFT,
    false
  );

  setRelay(
    RELAY_RIGHT,
    true
  );
}


// =====================================================
// CONTROLE DAS ESTEIRAS
// =====================================================

void controlTracks(
  int forwardChannel,
  int turnChannel
) {

  // ESQUERDA

  if (
    turnChannel <
    JOYSTICK_MIN
  ) {

    left();

    return;
  }


  // DIREITA

  if (
    turnChannel >
    JOYSTICK_MAX
  ) {

    right();

    return;
  }


  // FRENTE

  if (
    forwardChannel >
    JOYSTICK_MAX
  ) {

    forward();

    return;
  }


  // CENTRO

  stopTracks();
}


// =====================================================
// CONTROLE DO RELE 3 / GATILHO RELE 4
// =====================================================
//
// FUNCIONAMENTO NORMAL:
//
// CH1 para esquerda
// = RELE 3 ON
//
//
// MODO GATILHO:
//
// CH5 no maximo
// CH6 no maximo
// CH1 para esquerda
//
// = RELE 4 ON por 1 segundo
//
// Nesse modo o RELE 3 permanece desligado.
//
// =====================================================

void controlAux(
  int auxChannel,
  int ch5,
  int ch6
) {

  // ===================================================
  // VERIFICA SE CH5 E CH6 ESTAO NO MAXIMO
  // ===================================================

  bool armed =

    ch5 >
    ARM_THRESHOLD

    &&

    ch6 >
    ARM_THRESHOLD;


  // ===================================================
  // VERIFICA MOVIMENTO DO CH1
  // ===================================================

  bool auxPressed =

    auxChannel <
    JOYSTICK_MIN;


  // ===================================================
  // SOLTOU O CH1
  // ===================================================

  if (!auxPressed) {

    // Libera um novo disparo

    triggerLocked = false;


    // Relé 3 sempre desligado quando soltar

    setRelay(
      RELAY_AUX,
      false
    );
  }


  // ===================================================
  // MODO GATILHO
  // CH5 + CH6 NO MAXIMO
  // ===================================================

  if (armed) {

    // Garante que o relé 3 fique desligado

    setRelay(
      RELAY_AUX,
      false
    );


    // -----------------------------------------------
    // DISPARA SOMENTE UMA VEZ
    // -----------------------------------------------

    if (
      auxPressed &&
      !triggerLocked &&
      !pulseActive
    ) {

      triggerLocked = true;

      pulseActive = true;

      pulseStartTime = millis();


      setRelay(
        RELAY_PULSE,
        true
      );


      Serial.println();
      Serial.println(
        "======================================"
      );

      Serial.println(
        "GATILHO ACIONADO"
      );

      Serial.println(
        "RELE 4 ON - 1 SEGUNDO"
      );

      Serial.println(
        "======================================"
      );
    }


    return;
  }


  // ===================================================
  // FUNCIONAMENTO NORMAL DO RELE 3
  // ===================================================

  if (auxPressed) {

    setRelay(
      RELAY_AUX,
      true
    );

  } else {

    setRelay(
      RELAY_AUX,
      false
    );
  }
}


// =====================================================
// ATUALIZA PULSO DO RELE 4
// =====================================================

void updatePulse() {

  if (
    pulseActive &&
    millis() - pulseStartTime >= PULSE_TIME
  ) {

    setRelay(
      RELAY_PULSE,
      false
    );


    pulseActive = false;


    Serial.println();
    Serial.println(
      "RELE 4 OFF"
    );
  }
}


// =====================================================
// VALIDA CANAL
// =====================================================

bool validChannel(
  int value
) {

  return (
    value >= 900 &&
    value <= 2100
  );
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  // ===================================================
  // LED
  // ===================================================

  pinMode(
    LED_STATUS,
    OUTPUT
  );


  digitalWrite(
    LED_STATUS,
    LOW
  );


  // ===================================================
  // DESLIGA TODOS OS RELES
  // ===================================================

  stopAll();


  // ===================================================
  // IBUS
  // ===================================================

  Serial2.begin(
    115200,
    SERIAL_8N1,
    IBUS_RX_PIN,
    -1
  );


  ibus.begin(
    Serial2,
    IBUSBM_NOTIMER
  );


  // ===================================================
  // DEBUG
  // ===================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "RHINO INICIADO"
  );

  Serial.println(
    "ESP32 + FLYSKY + ROBOCORE"
  );

  Serial.println(
    "========================================"
  );

  Serial.println();

  Serial.println(
    "CH2 RESERVADO PARA FAILSAFE"
  );

  Serial.println();

  Serial.println(
    "GATILHO:"
  );

  Serial.println(
    "CH5 MAX + CH6 MAX + CH1 ESQUERDA"
  );

  Serial.println();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  ibus.loop();


  // ===================================================
  // LE CANAIS
  // ===================================================

  int channels[10];


  for (
    int i = 0;
    i < 10;
    i++
  ) {

    channels[i] =
      ibus.readChannel(i);
  }


  // ===================================================
  // CANAIS UTILIZADOS
  // ===================================================

  int ch1 =
    channels[
      CHANNEL_AUX
    ];


  int ch2 =
    channels[
      CHANNEL_FAILSAFE
    ];


  int ch3 =
    channels[
      CHANNEL_FORWARD
    ];


  int ch4 =
    channels[
      CHANNEL_TURN
    ];


  int ch5 =
    channels[
      CHANNEL_ARM_1
    ];


  int ch6 =
    channels[
      CHANNEL_ARM_2
    ];


  // ===================================================
  // VERIFICA SINAL
  // ===================================================

  bool basicSignalValid =

    validChannel(ch1) &&

    validChannel(ch2) &&

    validChannel(ch3) &&

    validChannel(ch4) &&

    validChannel(ch5) &&

    validChannel(ch6);


  // ===================================================
  // FAILSAFE
  // ===================================================

  bool failsafe =

    !basicSignalValid

    ||

    ch2 <
    FAILSAFE_THRESHOLD;


  // ===================================================
  // FAILSAFE ATIVO
  // ===================================================

  if (failsafe) {

    stopAll();


    digitalWrite(
      LED_STATUS,
      LOW
    );


    Serial.println(
      "FAILSAFE - TODOS OS RELES DESLIGADOS"
    );


    delay(50);

    return;
  }


  // ===================================================
  // SINAL OK
  // ===================================================

  digitalWrite(
    LED_STATUS,
    HIGH
  );


  // ===================================================
  // CONTROLE DAS ESTEIRAS
  // ===================================================

  controlTracks(
    ch3,
    ch4
  );


  // ===================================================
  // CONTROLE AUXILIAR / GATILHO
  // ===================================================

  controlAux(
    ch1,
    ch5,
    ch6
  );


  // ===================================================
  // CONTROLA TEMPO DO RELE 4
  // ===================================================

  updatePulse();


  // ===================================================
  // DEBUG DOS CANAIS
  // ===================================================

  Serial.print(
    "CH1:"
  );

  Serial.print(
    ch1
  );


  Serial.print(
    " CH2:"
  );

  Serial.print(
    ch2
  );


  Serial.print(
    " CH3:"
  );

  Serial.print(
    ch3
  );


  Serial.print(
    " CH4:"
  );

  Serial.print(
    ch4
  );


  Serial.print(
    " CH5:"
  );

  Serial.print(
    ch5
  );


  Serial.print(
    " CH6:"
  );

  Serial.print(
    ch6
  );


  // ===================================================
  // MOSTRA SE GATILHO ESTA ARMADO
  // ===================================================

  bool armed =

    ch5 >
    ARM_THRESHOLD

    &&

    ch6 >
    ARM_THRESHOLD;


  Serial.print(
    " | GATILHO:"
  );

  Serial.print(
    armed
      ? "ARMADO"
      : "NORMAL"
  );


  // ===================================================
  // DEBUG DOS RELES
  // ===================================================

  Serial.print(
    " || R1:"
  );

  Serial.print(
    relayLeftState
      ? "ON"
      : "OFF"
  );


  Serial.print(
    " R2:"
  );

  Serial.print(
    relayRightState
      ? "ON"
      : "OFF"
  );


  Serial.print(
    " R3:"
  );

  Serial.print(
    relayAuxState
      ? "ON"
      : "OFF"
  );


  Serial.print(
    " R4:"
  );

  Serial.println(
    relayPulseState
      ? "ON"
      : "OFF"
  );


  delay(50);
}
