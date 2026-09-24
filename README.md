# Aula 08 — Interrupções e o botão push (ESP32)

Projeto da disciplina **Edge Computing — FIAP**. Um ESP32 conta os cliques de um
botão usando **interrupção de hardware**, controla um **LED RGB** por comandos
digitados no Serial Monitor e toca um bip no **buzzer** a cada clique.

Roda no simulador **Wokwi** (dentro do VS Code) e também em uma placa física.

---

## O que o projeto demonstra

- **Interrupção externa (`attachInterrupt`)**: o botão avisa o processador na
  hora em que é apertado, sem o `loop()` precisar ficar lendo o pino o tempo todo.
- **ISR curta com `IRAM_ATTR`**: a rotina de interrupção só faz o debounce,
  incrementa o contador e levanta uma *flag*. Imprimir e tocar o buzzer ficam no `loop()`.
- **Variáveis `volatile`**: garantem que o `loop()` enxergue o valor que a ISR alterou.
- **Leitura segura** com `noInterrupts()` / `interrupts()` ao copiar o contador de 32 bits.
- **Debounce por software** (janela de 200 ms) para ignorar o repique do contato.
- **Piscar sem `delay()`**: o modo `BLINK` usa `millis()`, então o programa
  continua respondendo aos comandos enquanto o LED pisca.

## Componentes e ligações

| Componente | Pino do ESP32 | Observação |
|---|---|---|
| Botão push | GPIO 15 e GND | `INPUT_PULLUP`, interrupção na borda de descida (`FALLING`) |
| LED RGB — vermelho | GPIO 25 | resistor de 220 Ω |
| LED RGB — verde | GPIO 26 | resistor de 220 Ω |
| LED RGB — azul | GPIO 27 | resistor de 220 Ω |
| LED RGB — comum | GND | LED de **cátodo comum** (nível ALTO acende) |
| Buzzer (+) | GPIO 23 | (−) no GND |

O circuito completo está em [`diagram.json`](diagram.json).

## Comandos do Serial Monitor

Velocidade: **115200 bps**. Digite o comando e tecle **Enter**. Maiúsculas ou
minúsculas funcionam.

| Comando | Ação |
|---|---|
| `R` | vermelho |
| `G` | verde |
| `B` | azul |
| `Y` | amarelo (R + G) |
| `C` | ciano (G + B) |
| `M` | magenta (R + B) |
| `W` | branco (R + G + B) |
| `OFF` | apaga o LED |
| `BLINK` | pisca em branco |
| `STATUS` | mostra o total de cliques e o modo atual |
| `RESET` | zera o contador de cliques |

Cada clique no botão imprime `Cliques: N` e faz um bip curto.

Exemplo de saída:

```
=== Aula 08 - Interrupcoes, botao e LED RGB ===
Comandos: R G B Y C M W | OFF | BLINK | RESET | STATUS
modo R
Cliques: 1
Cliques: 2
Cliques: 2 | modo: R
```

## Como executar

### Pré-requisitos

- [VS Code](https://code.visualstudio.com/) com as extensões
  **PlatformIO IDE** e **Wokwi Simulator** (licença gratuita do Wokwi ativada).

### Simulação no Wokwi (VS Code)

1. Compile o firmware:
   ```bash
   pio run
   ```
2. Abra o `diagram.json` e rode **F1 → "Wokwi: Start Simulator"**.
3. Abra o painel inferior (**Ctrl + `**) e use o terminal **"Wokwi Terminal"**:
   ele é o Serial Monitor. Digite os comandos ali.
4. Clique no botão verde do circuito para gerar interrupções.

> **Atenção:** o Wokwi pausa a simulação quando a aba dela fica escondida atrás
> de outra aba do editor, e o Serial Monitor para de responder. Deixe a aba
> visível; se travar, use **F1 → "Wokwi: Restart Simulation"**.

Opcional: a saída serial também fica disponível na porta TCP 4000 (RFC2217):

```bash
pio device monitor -p rfc2217://localhost:4000 -b 115200
```

### Placa física

```bash
pio run -t upload     # grava no ESP32
pio device monitor    # abre o Serial Monitor a 115200 bps
```

Se o seu LED RGB for de **ânodo comum**, ligue o comum no 3V3 e inverta a lógica
em `setRGB()` (`digitalWrite(R, !r)` e assim por diante).

## Estrutura

```
.
├── sketch.ino         # código da aula (ISR, LED RGB, comandos, buzzer)
├── diagram.json       # circuito do Wokwi
├── wokwi.toml         # diz ao Wokwi qual firmware carregar
├── platformio.ini     # build do ESP32 com PlatformIO
└── explicacao.html    # explicação do código linha a linha (abra no navegador)
```

Nenhuma biblioteca externa é necessária: o sketch usa apenas o core Arduino do ESP32.

---

Lincoln Pereira — FIAP, Edge Computing, 3º semestre.
