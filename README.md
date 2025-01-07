# ES_WiSe2024

Embedded Systems im Wintersemester 2024/2025

> [!IMPORTANT]
> Letzter Abgabe Termin: **14.01.2025**

## Liste der Aufgaben

🔎 = Erledigt, aber noch nicht abgegeben

- [x] 1 - Digitale Ports
    - [x] Aufgabe 1
    - [x] Aufgabe 2
    - [x] Aufgabe 3
    - [x] Aufgabe 4
- [x] 2 - Externe Interrupts
    - [x] Aufgabe 1
    - [x] Aufgabe 2
    - [x] Aufgabe 3
    - [x] Aufgabe 4
- [x] 3 - Timer
    - [x] Aufgabe 1
    - [x] Aufgabe 2a
    - [x] Aufgabe 2b
    - [x] Aufgabe 2c
- [x] 4 - UART
    - [x] Aufgabe 2
- [x] 5 - Speicher
    - [x] Aufgabe 1
    - [x] Aufgabe 2
    - [x] Aufgabe 3
- [x] 6a - Analoge Eingänge
    - [x] Aufgabe 2
- [x] 6b - Analoge Ausgänge
    - [x] Aufgabe 2
    - [x] Aufgabe 4
- [ ] 7 - I²C
    - [ ] Aufgabe 1
- [ ] 8 - Bootloader
    - [ ] Aufgabe 1
    

## Sonstiges

### Arduino mit avrdude über USB programmieren

```bash
avrdude -v -patmega328p -carduino -P <COMx> -b115200 -D -Uflash:w:<Datei>.hex:i
```
