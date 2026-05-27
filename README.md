# AURORA — Sistema de Gestão Energética para Base Lunar

## 1. Descrição do Projeto

> "O código que mantém a luz acesa na noite mais longa."

AURORA é um sistema embarcado em Arduino Uno para monitorar energia, temperatura e umidade em uma base lunar simulada no Wokwi. O controlador classifica automaticamente o estado operacional em **DIA**, **NOITE** ou **EMERGÊNCIA** e aciona LEDs, buzzer, servo motor e LCD I2C conforme a situação da base. O projeto representa uma camada física de segurança para habitats lunares inspirados no contexto do programa Artemis.

Link do simulador Wokwi: [https://wokwi.com/projects/465189529004507137](https://wokwi.com/projects/465189529004507137)

## 2. Objetivo

O objetivo do AURORA é manter a operação mínima de uma base lunar mesmo durante variações de luz, bateria e temperatura. O sistema lê um LDR como simulação de painel solar, um potenciômetro como nível de bateria e um DHT22 como sensor ambiental. Quando há luz suficiente, a base opera em modo DIA; quando a luz cai, passa para NOITE se a bateria estiver segura; se a bateria ficar crítica ou a temperatura exceder o limite, entra em EMERGÊNCIA. Em emergência, o sistema alerta com buzzer, sinaliza com LED vermelho e move o servo para cortar cargas não essenciais.

## 3. Componentes Utilizados

| Componente | Função | Pino Arduino | Obrigatório |
|------------|--------|--------------|-------------|
| Arduino Uno | Controlador principal do sistema | - | Sim |
| LDR / Photoresistor | Simula luz solar/painel solar | A0 | Sim |
| Potenciômetro 10k | Simula nível da bateria em 0-100% | A1 | Sim |
| DHT22 | Mede temperatura e umidade da base | D2 | Sim |
| LED Verde | Indica estado DIA | D9 | Sim |
| LED Amarelo | Indica estado NOITE | D10 | Sim |
| LED Vermelho | Indica estado EMERGÊNCIA | D11 | Sim |
| Buzzer | Alarme sonoro em emergência | D12 | Sim |
| Servo Motor SG90 | Corte físico de carga não essencial | D6 | Sim |
| LCD I2C 16x2 | Display local de estado, bateria e sensores | A4/A5 | Sim |
| Resistor 10k | Pull-up do DHT22 | D2/5V | Sim |
| Resistores 220 ohms | Limitação de corrente dos LEDs | D9/D10/D11 | Sim |

## 4. Lógica de Estados

| Estado | Condição | LED | Buzzer | Servo | LCD/Serial |
|--------|----------|-----|--------|-------|------------|
| DIA | `luz > 600` e temperatura não crítica | Verde HIGH | OFF | 0 graus, carga conectada | Mostra `DIA` |
| NOITE | `luz <= 600`, `bateria >= 30%` e temperatura não crítica | Amarelo HIGH | OFF | 0 graus, carga conectada | Mostra `NOITE` |
| EMERGÊNCIA | `bateria < 30%` ou `temp > 40.0 C` | Vermelho HIGH | ON, 1000 Hz | 90 graus, carga cortada | Mostra `EMERGENCIA` e causa |

O código usa debounce de 3 leituras consecutivas antes de confirmar uma troca de estado, evitando oscilação visual quando os sensores ficam perto dos limites.

## 5. Como Executar no Wokwi

1. Acesse o projeto público: [AURORA no Wokwi](https://wokwi.com/projects/465189529004507137).
2. Verifique se os arquivos `AURORA.ino`, `diagram.json` e `libraries.txt` estão no simulador.
3. Confirme no Library Manager as bibliotecas `DHT sensor library`, `Adafruit Unified Sensor` e `LiquidCrystal I2C`.
4. Clique em **Start Simulation**.
5. Abra o **Serial Monitor** em `9600 baud`.
6. Para testar DIA, deixe o LDR com luz acima do limiar e observe o LED verde.
7. Para testar NOITE, reduza a luz do LDR para `<= 600` e mantenha a bateria em `>= 30%`.
8. Para testar EMERGÊNCIA, mantenha a luz baixa e reduza o potenciômetro para `< 30%`, ou eleve a temperatura do DHT22 para acima de `40.0 C`.
9. Para normalizar, retorne a bateria para `>= 30%` e a temperatura para abaixo do limite crítico.

## 6. Screenshot do Circuito Rodando

![Circuito AURORA no Wokwi](docs/screenshot-wokwi.png)

## 7. Estrutura do Circuito

```text
Arduino Uno
├── A0  <- LDR / sensor de luz
├── A1  <- Potenciômetro da bateria
├── D2  <- DHT22 DATA + resistor pull-up 10k para 5V
├── D6  -> Servo motor SG90
├── D9  -> LED verde + resistor 220 ohms
├── D10 -> LED amarelo + resistor 220 ohms
├── D11 -> LED vermelho + resistor 220 ohms
├── D12 -> Buzzer
├── A4  -> SDA do LCD I2C
├── A5  -> SCL do LCD I2C
├── 5V  -> Alimentação dos sensores, LCD e atuadores
└── GND -> Terra comum do circuito
```

Arquivos principais do repositório:

| Arquivo | Descrição |
|---------|-----------|
| `AURORA.ino` | Código Arduino/C++ do controlador |
| `diagram.json` | Circuito Wokwi com Arduino, sensores, atuadores e LCD |
| `libraries.txt` | Bibliotecas usadas pelo simulador |
| `docs/screenshot-wokwi.png` | Screenshot do circuito AURORA no Wokwi |

## 8. Integrantes

| Nome Completo | RM |
|---------------|----|
| Bruno Carreiro Dos Santos | 569423 |
| Eduardo Bechara Medeiros Craveiro | 571081 |
| Gustavo Ferreira Tavares | 569928 |
| Gustavo Moita de Lima | 569180 |
| Felipe Rabelo | 570340 |
