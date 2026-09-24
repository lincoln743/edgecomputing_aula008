/*
  FIAP - Edge Computing - Aula 08
  Interrupcoes e o botao push

  ESP32 + push button (interrupcao) + LED RGB (catodo comum) + buzzer

  Ligacoes:
    Botao   -> GPIO 15 e GND          (INPUT_PULLUP, borda FALLING)
    LED RGB -> R: GPIO 25 / G: GPIO 26 / B: GPIO 27, cada um com 220 ohm
               COM (catodo) -> GND
    Buzzer  -> (+) GPIO 23 / (-) GND

  Comandos no Serial Monitor (115200 bps, digite e tecle Enter):
    R  G  B  Y  C  M  W   -> acende a cor
    OFF                   -> apaga
    BLINK                 -> pisca branco (sem travar o loop)
    RESET                 -> zera o contador de cliques
    STATUS                -> mostra contador e modo atual

  No Wokwi do VS Code a simulacao pausa se a aba dela nao estiver visivel, e o
  Serial Monitor fica mudo (nao recebe nem envia). Se acontecer: F1 ->
  "Wokwi: Restart Simulation" (ou "Wokwi: Resume Simulation") e digite os
  comandos no painel que fica dentro da propria aba da simulacao.

  Nenhuma biblioteca externa e necessaria.
*/

#include <Arduino.h>

// ------------------------------------------------------------------ pinos
const int BTN = 15;               // botao (INPUT_PULLUP)
const int R   = 25;               // canal vermelho do LED RGB
const int G   = 26;               // canal verde
const int B   = 27;               // canal azul
const int BUZ = 23;               // buzzer (opcional)
const int CANAL_BUZ = 0;          // canal LEDC do buzzer (o tone() usa o 0)

// ------------------------------------------- variaveis compartilhadas
// "volatile" avisa o compilador que estas variaveis mudam fora do fluxo
// normal do programa (dentro da ISR). Sem isso o compilador pode guardar
// o valor num registrador e o loop nunca enxergar a atualizacao.
volatile unsigned long contador = 0;    // total de cliques validos
volatile unsigned long ultimo   = 0;    // instante do ultimo clique aceito
volatile bool          clique   = false; // flag: "houve um clique novo"

// ----------------------------------------------------------- parametros
const unsigned long DEBOUNCE_MS = 200;  // janela que ignora o repique do contato
const unsigned long BLINK_MS    = 250;  // meio periodo do piscar

// --------------------------------------------------------------- estado
String modo = "OFF";                    // modo atual do LED
unsigned long ultimoBlink = 0;          // controle de tempo do BLINK
bool blinkAceso = false;                // fase atual do piscar

// ====================================================================
// ISR - Interrupt Service Routine
// IRAM_ATTR mantem a funcao na RAM interna do ESP32: ela precisa estar
// sempre acessivel, mesmo quando a flash esta ocupada.
// Regra de ouro: a ISR e CURTA. Ela so faz o debounce, conta e levanta
// a flag. Imprimir na serial ou tocar o buzzer fica para o loop.
// ====================================================================
void IRAM_ATTR isrBotao() {
  unsigned long agora = millis();
  if (agora - ultimo > DEBOUNCE_MS) {   // ignora repiques
    contador++;
    clique = true;
    ultimo = agora;
  }
}

// --------------------------------------------------------------- LED RGB
// LED RGB de CATODO comum: o comum vai ao GND, entao nivel ALTO acende.
// (Se o seu LED for de ANODO comum, o comum vai ao 3V3 e a logica inverte:
//  troque por digitalWrite(R, !r); e assim por diante.)
void setRGB(bool r, bool g, bool b) {
  digitalWrite(R, r);
  digitalWrite(G, g);
  digitalWrite(B, b);
}

// Traduz o texto do comando em uma cor. Devolve false se nao reconhecer.
bool aplicarCor(const String& c) {
  if      (c == "R")   setRGB(1, 0, 0);   // vermelho
  else if (c == "G")   setRGB(0, 1, 0);   // verde
  else if (c == "B")   setRGB(0, 0, 1);   // azul
  else if (c == "Y")   setRGB(1, 1, 0);   // amarelo  = R + G
  else if (c == "M")   setRGB(1, 0, 1);   // magenta  = R + B
  else if (c == "C")   setRGB(0, 1, 1);   // ciano    = G + B
  else if (c == "W")   setRGB(1, 1, 1);   // branco   = R + G + B
  else if (c == "OFF") setRGB(0, 0, 0);   // apagado
  else return false;
  return true;
}

void mostrarAjuda() {
  Serial.println(F("Comandos: R G B Y C M W | OFF | BLINK | RESET | STATUS"));
}

// ==================================================================== setup
void setup() {
  Serial.begin(115200);
  delay(1000);                          // da tempo do Serial Monitor conectar

  pinMode(BTN, INPUT_PULLUP);           // resistor interno: solto = HIGH
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(BUZ, OUTPUT);

  setRGB(0, 0, 0);                      // comeca apagado

  // O tone() do core Arduino-ESP32 2.0.x chama ledcAttachPin() ANTES de
  // configurar o canal LEDC. ledcAttachPin() le o duty de um canal que ainda
  // nao existe, e o driver do ESP-IDF registra no Serial Monitor:
  //     E (12377) ledc: ledc_get_duty(745): LEDC is not initialized
  // Criar o canal aqui - antes de qualquer tone() - elimina esse log.
  ledcSetup(CANAL_BUZ, 2000, 10);       // canal LEDC do buzzer: 2 kHz, 10 bits
  ledcAttachPin(BUZ, CANAL_BUZ);        // liga o pino ao canal (duty 0 = mudo)

  // Liga a interrupcao: quando o pino do botao for de HIGH para LOW
  // (FALLING = borda de descida), o hardware chama isrBotao().
  attachInterrupt(digitalPinToInterrupt(BTN), isrBotao, FALLING);

  Serial.println();
  Serial.println(F("=== Aula 08 - Interrupcoes, botao e LED RGB ==="));
  mostrarAjuda();
}

// ===================================================================== loop
void loop() {

  // ---------------- 1) evento do botao, registrado pela interrupcao -------
  if (clique) {
    clique = false;                     // consome o evento

    // Copia o contador com as interrupcoes desligadas: ele tem 32 bits e
    // poderia ser alterado pela ISR no meio da leitura.
    noInterrupts();
    unsigned long total = contador;
    interrupts();

    Serial.print(F("Cliques: "));
    Serial.println(total);

    tone(BUZ, 1500, 80);                // bip curto de 80 ms (nao trava o loop)
  }

  // ---------------- 2) comandos digitados no Serial Monitor ---------------
  if (Serial.available()) {
    String c = Serial.readStringUntil('\n');
    c.trim();                           // remove espacos e o \r do Enter
    c.toUpperCase();                    // aceita "off", "Off", "OFF"

    if (c.length() == 0) {
      // linha vazia: ignora

    } else if (c == "RESET") {
      noInterrupts();                   // acesso seguro a variavel da ISR
      contador = 0;
      ultimo   = 0;
      interrupts();
      Serial.println(F("contador zerado"));

    } else if (c == "BLINK") {
      modo = "BLINK";
      ultimoBlink = millis();
      blinkAceso  = false;
      Serial.println(F("modo BLINK"));

    } else if (c == "STATUS") {
      noInterrupts();
      unsigned long total = contador;
      interrupts();
      Serial.print(F("Cliques: "));
      Serial.print(total);
      Serial.print(F(" | modo: "));
      Serial.println(modo);

    } else if (aplicarCor(c)) {
      modo = c;                         // sai do BLINK ao escolher uma cor
      Serial.println("modo " + c);

    } else {
      Serial.println("comando desconhecido: " + c);
      mostrarAjuda();
    }
  }

  // ---------------- 3) modo BLINK sem travar o loop -----------------------
  // Com delay(250) o programa ficaria cego por meio segundo a cada volta e
  // os comandos da serial demorariam a ser lidos. Com millis() o loop
  // continua girando e so troca o estado do LED quando o tempo vence.
  if (modo == "BLINK" && millis() - ultimoBlink >= BLINK_MS) {
    ultimoBlink = millis();
    blinkAceso  = !blinkAceso;
    setRGB(blinkAceso, blinkAceso, blinkAceso);   // branco piscando
  }
}
