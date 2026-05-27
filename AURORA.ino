// ============================================================
// 1. Includes e bibliotecas
// ============================================================
#include <DHT.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// 2. #define - todos os pinos e limiares
// ============================================================
#define LDR_PIN              A0
#define BAT_PIN              A1
#define DHT_PIN              2
#define DHT_TYPE             DHT22
#define LED_G                9
#define LED_Y                10
#define LED_R                11
#define BUZZER               12
#define SERVO_PIN            6
#define LUZ_DIA              600     // limiar de luz para estado DIA
#define BAT_SEGURA           30      // percentual minimo de bateria segura
#define TEMP_CRITICA         40.0    // C - acima disso e emergencia termica
#define TEMP_ALERTA          35.0    // C - zona de atencao
#define FREQ_ALARME          1000    // Hz do buzzer em emergencia
#define SERVO_OFF            0       // angulo: carga conectada
#define SERVO_CORTE          90      // angulo: carga cortada
#define DEBOUNCE_N           3       // leituras para confirmar mudanca de estado
#define INTERVALO_MS         500     // ms entre leituras
#define LCD_ADDR             0x27
#define LCD_COLS             16
#define LCD_ROWS             2
#define TEMP_CRITICA_CAUSA   "TERMICA"
#define BAT_CRITICA_CAUSA    "BATERIA"
#define CAUSA_NENHUMA        "-"
#define CAUSA_VAZIA          ""

// ============================================================
// 3. const int / const float - valores sem tipo implicito do #define
// ============================================================
const int ADC_MINIMO = 0;
const int ADC_MAXIMO = 1023;
const int PERCENTUAL_MINIMO = 0;
const int PERCENTUAL_MAXIMO = 100;
const int SERIAL_BAUD = 9600;
const int LEITURAS_INICIAIS = 0;
const int PRIMEIRA_CONFIRMACAO = 1;
const int CASAS_DECIMAIS_SENSOR = 1;
const int CASAS_DECIMAIS_UMIDADE_LCD = 0;
const int LCD_COLUNA_INICIAL = 0;
const int LCD_LINHA_ESTADO = 0;
const int LCD_LINHA_SENSORES = 1;
const int LCD_DELAY_BOOT_MS = 1500;
const int SERIAL_DELAY_BOOT_MS = 100;
const float DHT_VALOR_FALHA = 0.0;

// ============================================================
// 4. enum EstadoAurora
// ============================================================
enum EstadoAurora {
  ESTADO_DIA,
  ESTADO_NOITE,
  ESTADO_EMERGENCIA
};

// ============================================================
// 5. Variaveis globais comentadas
// ============================================================

// Guarda o estado efetivamente aplicado aos atuadores apos o debounce.
EstadoAurora estadoAtual = ESTADO_DIA;

// Guarda o estado que esta sendo confirmado pelas leituras consecutivas.
EstadoAurora estadoCandidato = ESTADO_DIA;

// Conta quantas leituras consecutivas confirmaram o estado candidato.
int leiturasConfirmadas = LEITURAS_INICIAIS;

// Indica se a primeira leitura valida ja definiu o estado inicial do sistema.
bool estadoInicializado = false;

// Marca o instante da ultima leitura para controle nao-bloqueante com millis().
unsigned long ultimaLeitura = 0;

// Conta os ciclos de leitura exibidos no Serial Monitor.
unsigned long ciclos = 0;

// Guarda a causa textual da ultima emergencia classificada.
const char* causaEmergencia = CAUSA_VAZIA;

// ============================================================
// 6. Prototipos de todas as funcoes
// ============================================================
void lerSensores(int &luz, int &bateria, float &temp, float &umidade);
EstadoAurora classificarEstadoEnum(int luz, int bateria, float temp);
String classificarEstado(int luz, int bateria, float temp);
void aplicarEstadoEnum(EstadoAurora estado);
void aplicarEstado(String estado);
void atualizarEstadoComDebounce(EstadoAurora estadoLido);
void exibirSerial(int luz, int bateria, float temp, float umidade, EstadoAurora estado);
void atualizarLCD(int bateria, float temp, float umidade, EstadoAurora estado);
String estadoParaTexto(EstadoAurora estado);
EstadoAurora textoParaEstado(String estado);

// ============================================================
// 7. Instancias: DHT, Servo e LiquidCrystal_I2C
// ============================================================
DHT dht(DHT_PIN, DHT_TYPE);
Servo servoCarga;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// ============================================================
// 8. void setup()
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(LDR_PIN, INPUT);
  pinMode(BAT_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);

  dht.begin();
  servoCarga.attach(SERVO_PIN);
  servoCarga.write(SERVO_OFF);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(LCD_COLUNA_INICIAL, LCD_LINHA_ESTADO);
  lcd.print(F("AURORA  v1.1"));
  lcd.setCursor(LCD_COLUNA_INICIAL, LCD_LINHA_SENSORES);
  lcd.print(F("Inicializando..."));

  digitalWrite(LED_G, LOW);
  digitalWrite(LED_Y, LOW);
  digitalWrite(LED_R, LOW);
  noTone(BUZZER);

  delay(SERIAL_DELAY_BOOT_MS);
  Serial.println(F(""));
  Serial.println(F("  ╔══════════════════════════════════╗"));
  Serial.println(F("  ║   AURORA · Base Lunar Artemis    ║"));
  Serial.println(F("  ║   Sistema de Gestao Energetica   ║"));
  Serial.println(F("  ║   Versao 1.1 · Inicializado      ║"));
  Serial.println(F("  ╚══════════════════════════════════╝"));
  Serial.println(F(""));

  delay(LCD_DELAY_BOOT_MS);
  lcd.clear();
}

// ============================================================
// 9. void loop()
// ============================================================
void loop() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura >= INTERVALO_MS) {
    ultimaLeitura = agora;

    int luz = ADC_MINIMO;
    int bateria = PERCENTUAL_MINIMO;
    float temp = DHT_VALOR_FALHA;
    float umidade = DHT_VALOR_FALHA;

    lerSensores(luz, bateria, temp, umidade);

    EstadoAurora estadoLido = classificarEstadoEnum(luz, bateria, temp);
    atualizarEstadoComDebounce(estadoLido);

    aplicarEstadoEnum(estadoAtual);
    atualizarLCD(bateria, temp, umidade, estadoAtual);
    exibirSerial(luz, bateria, temp, umidade, estadoAtual);
    ciclos++;
  }
}

// ============================================================
// 10. Funcoes auxiliares
// ============================================================

/*
 * Le e processa todos os sensores do sistema AURORA.
 * Parametros:
 *   luz: referencia que recebe a leitura de luminosidade em escala 0-1023.
 *   bateria: referencia que recebe a carga simulada da bateria em 0-100%.
 *   temp: referencia que recebe a temperatura em graus Celsius.
 *   umidade: referencia que recebe a umidade relativa em percentual.
 * Retorno: nenhum.
 */
void lerSensores(int &luz, int &bateria, float &temp, float &umidade) {
  int leituraBateria = analogRead(BAT_PIN);

  // No modulo LDR do Wokwi, analogRead() fica maior no escuro.
  // A inversao mantem a regra da AURORA: luz alta representa DIA.
  luz = ADC_MAXIMO - analogRead(LDR_PIN);
  bateria = map(leituraBateria, ADC_MINIMO, ADC_MAXIMO, PERCENTUAL_MINIMO, PERCENTUAL_MAXIMO);
  bateria = constrain(bateria, PERCENTUAL_MINIMO, PERCENTUAL_MAXIMO);

  temp = dht.readTemperature();
  umidade = dht.readHumidity();

  if (isnan(temp)) {
    temp = DHT_VALOR_FALHA;
  }

  if (isnan(umidade)) {
    umidade = DHT_VALOR_FALHA;
  }
}

/*
 * Classifica o estado operacional usando enum e os sensores lidos.
 * Parametros:
 *   luz: leitura de luminosidade em escala 0-1023.
 *   bateria: carga simulada da bateria em 0-100%.
 *   temp: temperatura em graus Celsius.
 * Retorno: valor EstadoAurora correspondente a DIA, NOITE ou EMERGENCIA.
 */
EstadoAurora classificarEstadoEnum(int luz, int bateria, float temp) {
  causaEmergencia = CAUSA_VAZIA;

  if (temp > TEMP_CRITICA && temp != DHT_VALOR_FALHA) {
    causaEmergencia = TEMP_CRITICA_CAUSA;
    return ESTADO_EMERGENCIA;
  }

  if (luz > LUZ_DIA) {
    return ESTADO_DIA;
  }

  if (bateria >= BAT_SEGURA) {
    return ESTADO_NOITE;
  }

  causaEmergencia = BAT_CRITICA_CAUSA;
  return ESTADO_EMERGENCIA;
}

/*
 * Wrapper publico que classifica o estado e retorna texto legivel.
 * Parametros:
 *   luz: leitura de luminosidade em escala 0-1023.
 *   bateria: carga simulada da bateria em 0-100%.
 *   temp: temperatura em graus Celsius.
 * Retorno: String com "DIA", "NOITE" ou "EMERGENCIA".
 */
String classificarEstado(int luz, int bateria, float temp) {
  return estadoParaTexto(classificarEstadoEnum(luz, bateria, temp));
}

/*
 * Desliga todas as saidas e aciona somente os atuadores do estado informado.
 * Parametros:
 *   estado: valor EstadoAurora ja confirmado pelo debounce.
 * Retorno: nenhum.
 */
void aplicarEstadoEnum(EstadoAurora estado) {
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_Y, LOW);
  digitalWrite(LED_R, LOW);
  noTone(BUZZER);

  switch (estado) {
    case ESTADO_DIA:
      digitalWrite(LED_G, HIGH);
      servoCarga.write(SERVO_OFF);
      break;

    case ESTADO_NOITE:
      digitalWrite(LED_Y, HIGH);
      servoCarga.write(SERVO_OFF);
      break;

    case ESTADO_EMERGENCIA:
    default:
      digitalWrite(LED_R, HIGH);
      tone(BUZZER, FREQ_ALARME);
      servoCarga.write(SERVO_CORTE);
      break;
  }
}

/*
 * Wrapper publico que aplica as saidas a partir de um estado em texto.
 * Parametros:
 *   estado: String com "DIA", "NOITE" ou "EMERGENCIA".
 * Retorno: nenhum.
 */
void aplicarEstado(String estado) {
  aplicarEstadoEnum(textoParaEstado(estado));
}

/*
 * Atualiza o estado global apenas depois de leituras consecutivas confirmarem a mudanca.
 * Parametros:
 *   estadoLido: estado calculado a partir dos sensores no ciclo atual.
 * Retorno: nenhum.
 */
void atualizarEstadoComDebounce(EstadoAurora estadoLido) {
  if (!estadoInicializado) {
    estadoAtual = estadoLido;
    estadoCandidato = estadoLido;
    leiturasConfirmadas = LEITURAS_INICIAIS;
    estadoInicializado = true;
    return;
  }

  if (estadoLido == estadoAtual) {
    estadoCandidato = estadoAtual;
    leiturasConfirmadas = LEITURAS_INICIAIS;
    return;
  }

  if (estadoLido != estadoCandidato) {
    estadoCandidato = estadoLido;
    leiturasConfirmadas = PRIMEIRA_CONFIRMACAO;
    return;
  }

  leiturasConfirmadas++;

  if (leiturasConfirmadas >= DEBOUNCE_N) {
    estadoAtual = estadoCandidato;
    leiturasConfirmadas = LEITURAS_INICIAIS;
  }
}

/*
 * Exibe dados no Serial Monitor em formato de console operacional.
 * Parametros:
 *   luz: leitura de luminosidade em escala 0-1023.
 *   bateria: carga simulada da bateria em 0-100%.
 *   temp: temperatura em graus Celsius.
 *   umidade: umidade relativa em percentual.
 *   estado: estado operacional confirmado pelo debounce.
 * Retorno: nenhum.
 */
void exibirSerial(int luz, int bateria, float temp, float umidade, EstadoAurora estado) {
  bool emergencia = (estado == ESTADO_EMERGENCIA);
  const char* causaAtual = CAUSA_NENHUMA;

  if (emergencia) {
    if (temp > TEMP_CRITICA && temp != DHT_VALOR_FALHA) {
      causaAtual = TEMP_CRITICA_CAUSA;
    } else if (bateria < BAT_SEGURA) {
      causaAtual = BAT_CRITICA_CAUSA;
    } else {
      causaAtual = causaEmergencia;
    }
  }

  Serial.println(F("======================================"));
  Serial.println(F(" AURORA · Console da Base Lunar"));
  Serial.print(F(" Ciclo: "));
  Serial.println(ciclos);
  Serial.println(F("======================================"));

  Serial.print(F(" Estado    : "));
  Serial.println(estadoParaTexto(estado));

  Serial.print(F(" Causa     : "));
  Serial.println(causaAtual);

  Serial.print(F(" Luz       : "));
  Serial.print(luz);
  Serial.print(F("   (limiar: "));
  Serial.print(LUZ_DIA);
  Serial.println(F(")"));

  Serial.print(F(" Bateria   : "));
  Serial.print(bateria);
  Serial.print(F("%   (min seguro: "));
  Serial.print(BAT_SEGURA);
  Serial.println(F("%)"));

  Serial.print(F(" Temp      : "));
  Serial.print(temp, CASAS_DECIMAIS_SENSOR);
  Serial.print(F("°C  (alerta: "));
  Serial.print(TEMP_ALERTA, CASAS_DECIMAIS_SENSOR);
  Serial.print(F("°C | critico: "));
  Serial.print(TEMP_CRITICA, CASAS_DECIMAIS_SENSOR);
  Serial.println(F("°C)"));

  Serial.print(F(" Umidade   : "));
  Serial.print(umidade, CASAS_DECIMAIS_SENSOR);
  Serial.println(F("%"));

  Serial.println(F("--------------------------------------"));

  Serial.print(F(" Servo     : "));
  Serial.print(emergencia ? SERVO_CORTE : SERVO_OFF);
  Serial.print(F("°    ("));
  Serial.print(emergencia ? F("carga cortada") : F("carga conectada"));
  Serial.println(F(")"));

  Serial.print(F(" Buzzer    : "));
  Serial.println(emergencia ? F("ON") : F("OFF"));

  if (leiturasConfirmadas > LEITURAS_INICIAIS) {
    Serial.print(F(" Debounce  : aguardando confirmacao ("));
    Serial.print(leiturasConfirmadas);
    Serial.print(F("/"));
    Serial.print(DEBOUNCE_N);
    Serial.println(F(")"));
  }

  Serial.println(F("======================================"));
  Serial.println(F(""));
}

/*
 * Atualiza o display LCD com estado atual e dados dos sensores.
 * Linha 1: estado operacional + bateria.
 * Linha 2: temperatura + umidade.
 * Parametros:
 *   bateria: carga simulada da bateria em 0-100%.
 *   temp: temperatura em graus Celsius.
 *   umidade: umidade relativa em percentual.
 *   estado: estado operacional confirmado pelo debounce.
 * Retorno: nenhum.
 */
void atualizarLCD(int bateria, float temp, float umidade, EstadoAurora estado) {
  lcd.clear();

  lcd.setCursor(LCD_COLUNA_INICIAL, LCD_LINHA_ESTADO);
  switch (estado) {
    case ESTADO_DIA:
      lcd.print(F("DIA        "));
      break;

    case ESTADO_NOITE:
      lcd.print(F("NOITE      "));
      break;

    case ESTADO_EMERGENCIA:
    default:
      lcd.print(F("EMERGENCIA "));
      break;
  }
  lcd.print(bateria);
  lcd.print(F("%"));

  lcd.setCursor(LCD_COLUNA_INICIAL, LCD_LINHA_SENSORES);
  lcd.print(F("T:"));
  lcd.print(temp, CASAS_DECIMAIS_SENSOR);
  lcd.print(F("C "));
  lcd.print(F("U:"));
  lcd.print(umidade, CASAS_DECIMAIS_UMIDADE_LCD);
  lcd.print(F("%"));
}

/*
 * Converte um estado enum para texto legivel no Serial Monitor.
 * Parametros:
 *   estado: valor EstadoAurora.
 * Retorno: String com o nome do estado.
 */
String estadoParaTexto(EstadoAurora estado) {
  switch (estado) {
    case ESTADO_DIA:
      return F("DIA");

    case ESTADO_NOITE:
      return F("NOITE");

    case ESTADO_EMERGENCIA:
    default:
      return F("EMERGENCIA");
  }
}

/*
 * Converte texto de estado para enum, mantendo emergencia como fallback seguro.
 * Parametros:
 *   estado: String com o nome do estado.
 * Retorno: valor EstadoAurora correspondente.
 */
EstadoAurora textoParaEstado(String estado) {
  if (estado == "DIA") {
    return ESTADO_DIA;
  }

  if (estado == "NOITE") {
    return ESTADO_NOITE;
  }

  return ESTADO_EMERGENCIA;
}
