# 🎲 Galton Board OLED – Simulador com Display e Histograma

Autor: **Wagner Junior**  
Curso: Residência Tecnológica em Sistemas Embarcados  
Instituição: EmbarcaTech – HBr  
Brasília, 08 de Maio de 2025

---

## 🧠 Descrição do Projeto

Este projeto implementa uma **Galton Board virtual** utilizando um microcontrolador RP2040 com um **display OLED I²C 128x64 pixels**, conectado via BitDogLab.

A simulação representa a queda de bolas em um tabuleiro triangular de pinos, desviando aleatoriamente para a esquerda ou direita. As bolas acumulam-se nas canaletas inferiores, formando uma **distribuição normal**.

O sistema possui **três telas comutáveis**:
- **Tela 1:** simulação ao vivo com as bolas descendo e contador total.
- **Tela 2:** histograma vertical com o número acumulado de bolas em cada bin (canaleta).
- **Tela 3:** visualização em texto com os valores numéricos de cada bin.

---

## 🧱 Funcionalidades

- 🌐 **Animação em tempo real** da queda de até 8 bolas simultâneas.
- 📊 **Histograma dinâmico** com 10 barras verticais.
- 🔢 **Exibição numérica por bin** com contagem total.
- 🧩 **Triângulo de pinos virtual** desenhado com base na altura e deslocamento da simulação.
- 🔁 **Alternância de tela** por botão físico (botão B).
- 🔄 **Reinício instantâneo** da simulação com botão físico (botão A).
- ⏱️ **Configuração de tempo, densidade e largura dos bins** via macros.

---

## ⚙️ Especificações Técnicas

| Item                    | Valor                                 |
|-------------------------|---------------------------------------|
| Display                 | SSD1306 OLED 128x64 I²C               |
| Microcontrolador        | RP2040 (via BitDogLab)                |
| Pinos I²C               | SDA = GP14, SCL = GP15                |
| Botão A (Restart)       | GPIO 5                                |
| Botão B (Troca de tela) | GPIO 6                                |
| N° de Bins              | 10 canaletas                          |
| Largura por bin         | 12 px                                 |
| Altura do histograma    | 64 px (dinâmico)                      |
| Biblioteca Display      | [`ssd1306.h`](include/ssd1306.h)          |

---

## 📷 Demonstração

**Tela 1 – Bolas em queda com pinos:**

```
• Triângulo de pinos simulando Galton Board
• Bolas descendo e desviando
• Contador de bolas total no canto superior
```

**Tela 2 – Histograma:**

```
• Barras verticais representando a distribuição
• Cada barra = 1 canaleta
• Contador total também visível
```

**Tela 3 – Tabela numérica de bins:**

```
• Mostra os valores absolutos de cada canaleta
• Formato: |  24|  40|  51| ...
• Contador de bolas no rodapé
```

---

## 🚀 Como usar

1. **Clone ou copie o projeto** para seu ambiente Pico SDK.
2. Compile com `cmake` e `make` ou `ninja`.
3. Grave no RP2040 via `picotool` ou outro método.
4. Conecte:
   - Display OLED aos pinos GP14 (SDA) e GP15 (SCL)
   - Botões aos pinos GP5 (BTN A) e GP6 (BTN B)
5. Pressione:
   - **BTN A** para reiniciar a simulação
   - **BTN B** para alternar entre as 3 telas

---

## 📁 Estrutura de Código

```
galton_board.c         ← Código principal da simulação
include/
 ├── ssd1306.h         ← Interface de controle do display
 ├── ssd1306_i2c.h     ← Comunicação via I²C
 └── ssd1306_font.h    ← Fontes para o display
src/
 ├── ssd1306_i2c.c     ← Driver via I²C
 └── ssd1306_font.c    ← Implementação das fontes
```

---

## 📜 Licença

MIT License – [MIT](https://opensource.org/licenses/MIT)  
GPL-3.0 – [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.html)

---

📌 *Projeto acadêmico desenvolvido como parte da Residência Tecnológica da EmbarcaTech – HBr em 2025.*