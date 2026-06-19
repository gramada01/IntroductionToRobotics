# Microprocesoare — Pregătire examen (rescris, compact)

Acest document reorganizează materialele din:
- `Microprocesoare/Pregatire_Examen/Subiecte ScM.txt`
- `Microprocesoare/Pregatire_Examen/Fituica_2022.pdf`
- `Microprocesoare/Pregatire_Examen/Pregatire.pdf`
- `Microprocesoare/Pregatire_Examen/cursuri.txt`

Scop: acces rapid la ideile importante, în format de învățare + recapitulare.

---

## 1) Ce trebuie știut pentru examen (hartă rapidă)

1. **MCU 8051**: arhitectură, memorie, porturi, întreruperi, aplicații.
2. **Unitatea de control (CU)**: rol, implementare cablată vs microprogramată, FSM.
3. **Ciclul instrucțiunii + pipeline**: Fetch/Decode/Execute, throughput, conflicte.
4. **ALU**: proiectare pe biți de control, operații aritmetice și logice.
5. **Memorie și magistrale**: adresă/date/control, acces intern vs extern.
6. **Moduri de adresare 8051**: imediată, directă, indirectă.

---

## 2) Subiecte de teorie — varianta clară

### Subiect 1 — Caracteristici și aplicații MCU 8051

**Caracteristici de bază:**
- arhitectură de tip Harvard (separare memorie program/date)
- procesor pe 8 biți
- memorie internă tipică: 4KB ROM + 128B RAM (în versiunea clasică)
- până la 64KB memorie externă cod și 64KB memorie externă date
- 4 porturi I/O x 8 biți (32 pini I/O)
- timere interne (uzual 2), port serial full duplex
- suport întreruperi (externe + interne + serial)

**Porturi/semnale importante:**
- `PSEN` — citire cod din memorie externă
- `ALE` — latch adresă pentru magistrală
- `EA` — selecție cod intern/extern
- `RXD/TXD` — serial
- `XTAL1/XTAL2` — ceas
- `RST` — reset

**Aplicații tipice:**
- sisteme embedded simple
- automatizări industriale
- control echipamente/aparate
- dispozitive de monitorizare

---

### Subiect 2 — Necesitatea și implementarea Unității de Control

**De ce este necesară CU:**
- interpretează instrucțiunea curentă
- coordonează transferurile între registre, ALU și memorie
- decide secvența de pași interni pe cicluri de ceas

**Două implementări clasice:**
1. **Hardwired (cablată):**
   - logică fixă
   - rapidă
   - greu de modificat
2. **Microprogramată:**
   - secvențe de control în memorie de control
   - flexibilă, ușor de extins
   - utilă la arhitecturi complexe

**Idee-cheie:** CU se poate modela ca **mașină cu stări finite (FSM)**.

---

### Subiect 3 — Procesarea instrucțiunilor și pipelining

**Flux standard:**
1. Fetch
2. Decode
3. Execute
4. (opțional) acces memorie
5. Write-back

**Pipeline:**
- suprapune mai multe instrucțiuni pe etape diferite
- crește throughput-ul
- aduce probleme de hazard (date/control), ce trebuie gestionate

**Observație utilă:**
- pipeline mai adânc poate crește performanța, dar crește și complexitatea controlului.

---

### Subiect 4 — Proiectare ALU pe biți de control

Set operații cerut:
- `00-1` -> Adunare
- `00-0` -> Scădere
- `01-X` -> XOR
- `10-X` -> AND
- `11-X` -> OR

Ordinea controlului: `S1, S0, Bneg`.

**Structură recomandată:**
- bloc pentru condiționarea lui `B` (negare la nevoie)
- blocuri paralele pentru `ADD`, `SUB`, `XOR`, `AND`, `OR`
- multiplexor final selectat de `S1,S0`

---

## 3) 8051 — fișă de recapitulare

### 3.1 Memoria internă clasică (RAM 128B)
- `00h–1Fh`: bănci de registre
- `20h–2Fh`: zonă adresabilă pe biți
- `30h–7Fh`: variabile + stivă

### 3.2 Memorie externă
- capacitate mai mare, acces mai lent
- necesită magistrale și semnale de control dedicate

### 3.3 Moduri de adresare
1. **Imediată**: valoare constantă în instrucțiune (`MOV A,#20h`)
2. **Directă**: adresă explicită (`MOV A,30h`)
3. **Indirectă**: adresă prin registru (`MOV A,@R0`)

### 3.4 Întreruperi (idee generală)
- surse externe + interne (timere, serial)
- niveluri de prioritate
- ordonare clară când apar simultan

---

## 4) Concepte care apar des la oral/scris

- diferența între instrucțiuni **monadice** și **diadice**
- legarea memoriei la procesor: bus de adresă/date/control
- ce face CU la Fetch vs Decode
- ce face ALU la Execute
- cum justifici utilizarea FSM la control
- de ce avem priorități de întreruperi

---

## 5) Mini-ghid de învățare (rapid)

1. Învață schema mare: `Memorie -> CU -> ALU -> Registre`.
2. Fixează cele 4 subiecte principale (8051, CU, pipeline, ALU).
3. Repetă 3 exemple de adresare și 2 exemple de întreruperi.
4. Fă 2–3 exerciții pe ALU (mapare control -> operație).
5. Exersează răspunsuri scurte (30–60 sec) pe întrebări teoretice.

---

## 6) Gata pentru subiecte noi

Poți să-mi dai direct:
- subiectul exact (text/foto)
- tipul cerut (rezolvare scurtă, lungă, bullet points, schemă)
- nivelul de detaliu (5/10/15 minute de răspuns)

Și îți dau răspunsul structurat imediat.
