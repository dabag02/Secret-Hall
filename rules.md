# Regole generali del progetto — La Sala Segreta

> **v3** — integra l'*addendum roguelike* (sezioni K–Q) e il *contenuto della sala* (sezione R): nicchie dietro i quadri, laghetto, muschio, alchimia, pane e farina, un enigma per ogni quadro e ogni porta.

Elenco **vincolante** dei requisiti. Ogni regola ha un ID, uno stato e il passo di `architecture.md` che la realizza.
Quando si sviluppa una funzione: citare l'ID nel `changelog.md`. Quando si cambia una regola: aggiornare qui prima di toccare il codice.

Stati: **Fatta** · **Parziale** (esiste una versione limitata) · **Nuova** (da realizzare).
Passi: `M0–M12` = migrazione/architettura (`architecture.md`, sezione 10) · `F1–F19` = funzioni (sezione 12).

---

## A. Architettura e moddabilità

| ID | Regola | Stato | Passo |
|---|---|---|---|
| A1 | **Logica e grafica sono separate.** I moduli di gameplay (`game/`, `systems/`) non includono OpenGL né SDL. La grafica legge solo una vista in sola lettura dello stato (`RenderScene`, `UiModel`) e non lo modifica mai. | Nuova | M7 |
| A2 | Le **proprietà di gameplay** (oggetti, materiali, reazioni, ricette, enigmi) stanno in file di dati leggibili e modificabili senza ricompilare. | Nuova | M11, F10 |
| A3 | Gli **asset visivi** (modelli, texture, shader, luci, tema dell'interfaccia) sono sostituibili per ID senza toccare la logica; se un asset manca si usa il wireframe predefinito. | Nuova | M7, F9 |
| A4 | Chi modifica la grafica non deve leggere codice di gameplay, e chi modifica logica/enigmi non deve toccare la grafica. | Nuova | M7 |
| A5 | Il prototipo di base deve essere **completo e giocabile** con il solo backend wireframe: la grafica avanzata è un'estensione, non un requisito. | Parziale | tutti |
| A6 | Ogni sviluppo è annotato in `changelog.md` (cronologia + stato attuale) citando gli ID delle regole. | Fatta | — |

## B. Oggetti e proprietà fisiche

| ID | Regola | Stato | Passo |
|---|---|---|---|
| B1 | **Ogni oggetto della scena** ha peso e proprietà fisiche: massa, dimensioni, materiale, restituzione elastica, attrito. | Parziale (oggetti sciolti; gli arredi sono entità con materiale, ma massa di tavolo/sedia/rastrelliera/ganci da decidere) | M4, F1 |
| B2 | Ogni oggetto reagisce a gravità, urti e lancio in base alle sue proprietà. | Parziale (gravità e rimbalzo sì) | F1, F3 |
| B3 | Anche **tavolo, scrigno, secchio, sedia e scheletro** si spostano (E tenuto) e si lanciano (T), nei limiti del peso e della Forza. | Parziale (scrigno e secchio spostabili) | F1 |
| B4 | **Macerie e ceneri** sono oggetti con peso: si raccolgono, spostano, usano nel crafting. | Parziale (ceneri non raccoglibili) | F4 |
| B5 | Il **calore** è una proprietà degli oggetti (temperatura che decade nel tempo): sostituisce i timer "rovente 10 s". | Parziale | F2 |
| B6 | I contenuti di un contenitore (scrigno, secchio) sono **oggetti veri**, non flag. | Fatta (scrigno, secchio, rastrelliera e ganci: M4c) | M4, F1 |

## C. Reazioni agli agenti

| ID | Regola | Stato | Passo |
|---|---|---|---|
| C1 | **Fuoco:** ogni oggetto infiammabile brucia e si incenerisce (tavolo, scrigno, secchio, sedia, libro, rastrelliera…). Il prodotto sono ceneri/macerie con massa. | Parziale (tavolo, rastrelliera, libro) | F2 |
| C2 | **Acqua:** spegne torce e fuochi, bagna gli oggetti (non bruciano finché asciutti), può essere contenuta (secchio). | Nuova | F2 |
| C3 | **Gravità e urti:** caduta, rimbalzo, rottura di materiali fragili oltre una soglia di energia. | Parziale | F1, F3 |
| C4 | Le reazioni sono **dati** (materiale × agente → effetto), non codice per oggetto. | Nuova | M11, F2 |

## D. Interazione

| ID | Regola | Stato | Passo |
|---|---|---|---|
| D1 | E breve = interagisci; E tenuto = trasporta; rilascio = cade; Maiusc+E = lascia cadere ciò che hai in mano. | Fatta | — |
| D2 | **T = lancio.** Tenendo premuto T compare la **traiettoria prevista** (arco in wireframe con punto d'impatto) in base alla direzione di mira del mouse, al peso dell'oggetto e alla Forza effettiva; rilasciando T l'oggetto parte. Esc annulla. | Nuova | F3 |
| D3 | La traiettoria mostrata e quella reale usano **lo stesso calcolo** (nessuna divergenza). | Nuova | F3 |

## E. Personaggio

| ID | Regola | Stato | Passo |
|---|---|---|---|
| E1 | Statistiche da dadi: migliore di 3 lanci di 2d6 per ciascuna. Armatura: +2 Forza/Vita, −1 Destrezza/Magia. Peso massimo 20 kg. | Fatta | — |
| E2 | Le **soglie** dei punteggi dei dadi danno **bonus alle prestazioni di lancio** (vedi specifiche). | Nuova | F3 |
| E3 | Il nome del personaggio è modificabile (dal libro). | Nuova | F7 |

## F. Scheletro e sedia

| ID | Regola | Stato | Passo |
|---|---|---|---|
| F1 | Lo scheletro ha **parametri propri** (come il personaggio), **non visibili**. Alla primissima partita derivano dai **punteggi scartati** (i più bassi) dei lanci di dadi; dalle morti successive **ereditano quelli del personaggio morto** (K2). Nessun effetto finché non si anima. | Nuova | F6, F11 |
| F2 | Scheletro e sedia **si spostano** dalla loro posizione. | Nuova | F1 |
| F3 | La sedia reagisce agli agenti come gli altri oggetti (brucia, cade, si lancia). | Nuova | F2 |

## G. Crafting e metalli

| ID | Regola | Stato | Passo |
|---|---|---|---|
| G1 | Il **crafting** esiste dove previsto, definito da ricette (dati): ingredienti + strumento/stazione → risultato. | Nuova | F4, M11 |
| G2 | **Fusione dei metalli** (oro, bronzo, acciaio, …) con gli strumenti giusti (fonte di calore adeguata, crogiolo, stampo) per creare nuovi oggetti. | Nuova | F5 |

## H. Diario (il libro)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| H1 | Il libro permette di **scrivere**: modificare il nome del personaggio, inserire brevi note. | Nuova | F7 |
| H2 | Il libro mantiene la **cronologia di tutte le scoperte** del personaggio (voci generate dagli eventi di gioco). La cronologia è **solo in aggiunta**: non si cancella. | Nuova | F7 |
| H3 | Nome e note ("la storia") restano modificabili in ogni vita; la cronologia no (K3). | Nuova | F7, F11 |

## I. Salvataggio

| ID | Regola | Stato | Passo |
|---|---|---|---|
| I1 | Un **menu** (Esc) con Riprendi / Salva / Carica / Esci. La partita non si ricomincia da zero a ogni avvio. | Nuova | M10, F8 |
| I2 | Il salvataggio comprende **tutto lo stato** (oggetti, posizioni, temperature, statistiche, parametri nascosti dello scheletro, diario, tempo di gioco, seme casuale), ha una **versione** e resta in locale. | Nuova | M10, F8 |
| I3 | Il salvataggio distingue tre livelli: **Meta** (sopravvive alla morte: libro, stats dello scheletro, numero di vite), **Run** (la vita in corso) e **Istanza** (vedi Q). | Nuova | M5, M10 |

## J. Qualità

| ID | Regola | Stato | Passo |
|---|---|---|---|
| J1 | Le regole di gioco hanno **test headless** (senza SDL/OpenGL). | Nuova | M12 |
| J2 | **Regressione visiva** con screenshot deterministici prima e dopo ogni passo. | Nuova | M0 |

---

## K. Roguelike e ciclo di vita (addendum 1, 4)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| K1 | La **morte fa parte del gioco** (scoperta per tentativi ed errori). Alla morte **non si ricomincia da zero**: la sala torna alla **configurazione originale** e il giocatore rinasce con **nuove statistiche** (nuovo lancio dei dadi). | Nuova | F11 |
| K2 | Lo scheletro rappresenta sempre **i resti del giocatore precedente**: eredita le statistiche del personaggio morto (nella primissima partita ha quelle scartate dai dadi). | Nuova | F11, F6 |
| K3 | Il libro conserva **nome, note e cronologia** del personaggio morto. Nella nuova vita nome e storia (note) si possono modificare; la **cronologia** di ciò che si è appreso **non si cancella**. | Nuova | F11, F7 |
| K4 | **Vittoria** solo trovando il modo di aprire la **giusta porta di uscita**. | Nuova | F10 |
| K5 | **Game over definitivo** (si ricomincia davvero da zero, senza meta-stato) **solo quando non c'è più alcun fuoco che illumina la sala** (M4). | Nuova | F13, F11 |
| K6 | Le **abilità** (crafting, incantesimi) non sono ereditate dallo scheletro, anche se registrate nel libro (O6). | Nuova | F16, F20 |

## L. Sopravvivenza (addendum 2)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| L1 | Il personaggio deve **bere e mangiare**; altrimenti perde punti vita e muore (tempistiche tarabili, vedi specifiche). | Nuova | F12 |
| L2 | Nella sala devono esistere **cibo, acqua e una fonte d'acqua**: definiti dalle regole R2–R4 (pane e farina in una nicchia, laghetto dietro la porta nord). | Nuova | F12, F21 |

## M. Fuoco, luce e oggetti roventi (addendum 3, 4, 5)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| M1 | Le **torce si consumano** (durata finita, luce che cala, poi si spengono). | Nuova | F13 |
| M2 | Il **fuoco del camino si consuma** (ha combustibile: legna e oggetti infiammabili che vi si gettano). | Nuova | F13 |
| M3 | Gli oggetti **non inceneribili** finiti nel fuoco (chiavi, gemme, monete, corazze, armi…) diventano **roventi** per un certo tempo: finché non si raffreddano non si possono raccogliere, craftare, spostare o mettere nell'inventario (tentare scotta: −1 vita). | Nuova | F14 |
| M4 | Se **nessun fuoco** illumina la sala parte un **conto alla rovescia** (clessidra); allo scadere → game over definitivo (K5). | Nuova | F13 |

## N. Contenitori (addendum 6)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| N1 | Lo **scrigno è un secondo inventario**: **6 slot**, si può prelevare **e riporre**, purché l'oggetto stia nelle dimensioni dello scrigno (spada e scudo non entrano). | Nuova | F15 |

## O. Attori, scheletro, combattimento e movimento (addendum 7, 8, 9, 10)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| O1 | **Giocatore e scheletro sono attori** con le stesse capacità: prendere armi, indossare armature, lanciare e trasportare oggetti, stesso move set. | Nuova | M4–M6, F16 |
| O2 | Se il **libro brucia**: veleno per 10 s nel raggio di 3 m (già presente) e **risveglio dello scheletro, che attacca** il personaggio. | Parziale (veleno sì) | F16 |
| O3 | Dopo il rogo del libro **non si annota più nulla**: se il giocatore muore, nella vita successiva il libro non contiene note né cronologia di ciò che è accaduto **dopo** il rogo. | Nuova | F7, F11 |
| O4 | **Move set:** tasto sx del mouse breve = fendente · sx tenuto = affondo · tasto dx breve = parata · dx tenuto = spinta. Con lo **scudo** parata e spinta avvengono con lo scudo. Con oggetto + arma, l'oggetto si lascia cadere (Maiusc+E), si lancia (T) o si ripone nell'inventario. **Maiusc+W** sale su scale, tavoli o sedie. **Ctrl** accovaccia. Lo scheletro **eredita il move set**. | Nuova | F17, F18 |
| O5 | Combattimento **in tempo reale**; l'efficacia di attacco e difesa dipende da un **confronto in background** delle statistiche tra i contendenti, con bonus/malus sui danni (da tarare). | Nuova | F17 |
| O6 | Lo scheletro **non** eredita crafting né incantesimi, anche se il personaggio li aveva appresi o sono scritti nel libro. | Nuova | F16, F20 |

## P. Interfaccia (addendum 11)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| P1 | La **clessidra** è il simbolo visivo **globale** del conto alla rovescia di ogni evento limitato nel tempo (veleno, buio, oggetti roventi, torcia in esaurimento…). Il suo aspetto è sostituibile dai mod grafici. | Parziale (solo il veleno) | M3, M7 |

## Q. Preparazione online (addendum 12)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| Q1 | Il modello dati prevede in futuro **istanze condivise**: un giocatore può iniziare dentro un'istanza già giocata da altri, con **note e cronologia** già scritte nel libro; in quel caso lo scheletro **non** usa i minimi della prima partita ma le statistiche dell'**ultimo giocatore morto in quell'istanza**. | Nuova | F19 |
| Q2 | La funzione online sarà **opt-in**; oggi nessuna rete e nessuna telemetria; i testi scritti da altri vanno **sanificati**. | Nuova | F19 |

---

## R. Contenuto della sala: nicchie, laghetto, muschio, alchimia (addendum 3)

| ID | Regola | Stato | Passo |
|---|---|---|---|
| R1 | **Dietro ogni quadro** sopra le porte c'è uno **spazio nascosto** (nicchia). Si scopre risolvendo un **enigma** (da progettare in seguito). | Nuova | F10, F21 |
| R2 | In **una nicchia** c'è l'**attrezzatura per l'alchimia** e un **crogiolo** per forgiare piccoli oggetti metallici. In **un'altra** ci sono **pane** e un **sacco di farina**. Le altre due: loot da definire. | Nuova | F21, F5 |
| R3 | La **farina** si trasforma con **acqua** in impasto, che **cotto** sul fuoco dà **altro pane** (crafting). | Nuova | F4, F21 |
| R4 | Dietro la **porta nord** c'è un **laghetto**, scoperto solo trovando il modo di aprirla (enigma): fornisce **acqua illimitata**. | Nuova | F10, F21 |
| R5 | Sulla parete **nord** della sala c'è del **muschio**: si raccoglie e si può craftare con altri ingredienti per creare **ricette/incantesimi**. | Nuova | F21, F20 |
| R6 | **Un enigma per ogni quadro e per ogni porta** (8 enigmi), ciascuno con il proprio **loot** dietro. **Una sola porta è l'uscita** (vittoria, K4). | Nuova | F10, F21 |

---

## Specifiche di gioco (valori iniziali, tutti ritoccabili come dati)

### Lancio (D2, E2)
- **Energia:** `E = 6 J × Forza_eff × (1 + bonus_soglia)`. **Velocità iniziale:** `v0 = min(18 m/s, √(2E / m))`.
  Esempi con Forza 10: torcia (1,2 kg) ≈ 10 m/s · spada (1,6 kg) ≈ 8,7 m/s · scudo/secchio (3 kg) ≈ 6,3 m/s.
- **Massa massima lanciabile:** `0,8 kg × Forza_eff` (Forza 10 → 8 kg). Oltre, il messaggio dice "troppo pesante da lanciare" (si può comunque trasportare entro i 20 kg).
- **Soglie di Forza (punteggio dei dadi, prima dei modificatori):** ≥ 10 → +10% energia · ≥ 12 → +25% energia e mira assistita (la traiettoria prevista mostra anche il primo rimbalzo).
- **Dispersione:** `max(0, 12 − Destrezza_eff) × 0,5°` attorno alla direzione di mira.
- **Mira:** direzione del mirino (il mouse muove la vista); l'oggetto parte dalla mano destra (o sinistra se la destra è vuota; Maiusc+T forza la sinistra).
- **Traiettoria:** integrazione identica a quella della fisica, fino al primo impatto; punto d'impatto segnato.
- **Urto:** fragile oltre soglia → si rompe in macerie; torcia accesa su oggetto infiammabile → lo accende.

### Materiali (esempi di dati)
| Materiale | Densità | Infiammabile | Fusione (livello di calore) | Prodotto del fuoco |
|---|---|---|---|---|
| legno | 0,6 | sì | — | cenere (5% della massa) + carbone |
| carta/pergamena | 0,8 | sì | — | cenere tossica (se inchiostro/pigmenti: regola attuale del libro) |
| ferro | 7,8 | no | 2 | — |
| acciaio | 7,85 | no | 2 | — |
| bronzo | 8,8 | no | 1 | — |
| oro | 19,3 | no | 1 | — |
| osso | 1,9 | no | — | — |
| tessuto/stoppa | 0,3 | sì | — | cenere |
Livello di calore: 1 = fuoco libero/camino acceso · 2 = forgia con mantice.

### Fusione e crafting (G1, G2) — struttura
- **Ricetta:** ingredienti (per materiale e massa) + stazione/strumento + livello di calore + tempo → oggetti risultanti; resa 90% (il resto = scorie).
- **Strumenti:** crogiolo (contenitore), stampo, pinze (permettono di maneggiare oggetti caldi senza scottarsi), mantice (alza il calore al livello 2).
- **Esempi da confermare:** monete d'oro → lingotto d'oro · chiave di bronzo → lingotto di bronzo · lingotti + stampo → nuovo oggetto.

### Scheletro (F1)
- Statistiche = **minimo dei tre lanci** di ciascuna caratteristica (registrati al primo avvio, salvati, mai mostrati). Restano inerti finché un evento non anima lo scheletro.

### Diario (H1, H2)
- **Voci automatiche** generate dagli eventi ("trovata la chiave di bronzo", "scrigno aperto", "il tavolo è crollato", "primo veleno", …) con il tempo di gioco.
- **Nome e note** con inserimento testo UTF-8, limiti di lunghezza.
- **Libro bruciato:** vale **O3** (addendum): da quel momento non si annota più nulla e le note successive non esistono nella vita seguente. *(Sostituisce la decisione precedente "cronologia conservata").*
- **Cronologia:** voci in sola aggiunta con `autore`, `vita`, `tempo di gioco`, `tipo`, `testo`; nome e note sono campi modificabili separati.

### Salvataggio (I1, I2)
- Menu Esc: Riprendi · Salva · Carica · Esci. Slot 1–5 + salvataggio automatico.
- Formato JSON con campo `version`; cartella `~/.local/share/sala_segreta/saves/`. Nessuna rete.

---

### Ciclo roguelike (K1–K6, O3)
- **Morte →** schermata "Sei morto: <causa>" → la sala torna alla **configurazione originale** (descritta come dato) → **nuovo lancio dei dadi** → nuova vita.
- **Ereditarietà:** lo scheletro riceve le statistiche **di base** (quelle lanciate, senza bonus/malus dell'armatura) del personaggio morto. Alla primissima partita: i minimi dei tre lanci di ciascuna caratteristica.
- **Meta (persistente):** nome, note, cronologia, statistiche dello scheletro, numero di vite, flag vittoria. **Run (si azzera alla morte):** sala, oggetti, giocatore, tempo, bisogni.
- **Game over definitivo:** si cancella anche il Meta (nuova partita da zero) quando il conto alla rovescia del buio scade.
- **Abilità:** non sono ereditate dallo scheletro e, per il giocatore, **vanno riapprese** a ogni vita (resta la conoscenza nella cronologia del libro).

### Sopravvivenza (L1–L2)
| Parametro | Valore iniziale |
|---|---|
| Sete | 100 → 0 in **15 min**; a 0: −1 vita ogni **20 s** |
| Fame | 100 → 0 in **30 min**; a 0: −1 vita ogni **40 s** |
| Acqua (0,5 L) | +40 sete; pane +35 fame; mela +15 fame |
| Uso di un oggetto in mano | tasto **U** (da confermare) |
Servono nella sala: una **fonte d'acqua** (cisterna/barile), uno o più **contenitori** d'acqua (il secchio può contenerne), **cibo** (pane, mele) in luoghi da scoprire.

### Combustibile, luce e buio (M1–M4, K5)
| Parametro | Valore iniziale |
|---|---|
| Durata di una torcia | **12 min**; luce piena fino a 60 s dalla fine, poi cala; ultimi 60 s con clessidra |
| Fuoco del camino | **3 ceppi** iniziali da **4 min** ciascuno; si alimenta con oggetti di legno: **60 s per kg** di legno |
| Buio (nessuna fiamma accesa: torce, camino, oggetti in fiamme) | conto alla rovescia di **15 s** (clessidra); se compare una fiamma si annulla; a 0 → game over definitivo |
| Oggetto non inceneribile nel fuoco | raggiunge la temperatura del fuoco; sicuro dopo **20 s + 4 s per kg** (gemme 10 s); toccarlo prima: −1 vita per tentativo |

### Scrigno come inventario (N1)
- **6 slot**; ogni oggetto ha una *classe di dimensione*: 1 minuscolo (chiave, gemma, monete) · 2 piccolo (libro, torcia, elmo, spallina, bracciale, stivale) · 3 medio (corazza, gambale) · 4 grande (secchio, spada, scudo). Lo scrigno accetta **fino alla classe 3** e un **volume totale di 12 punti** (classe 1=1, 2=2, 3=3). Spada, scudo, secchio non entrano.
- Il peso del contenuto **non** conta nel limite di 20 kg del giocatore, ma si somma al peso dello scrigno quando lo si trasporta.

### Combattimento e movimento (O1, O4, O5)
- **Mouse:** sx breve (< 0,35 s) fendente · sx tenuto poi rilascio = affondo · dx breve = parata (guardia ~0,4 s) · dx tenuto poi rilascio = spinta. Con scudo: parata e spinta con lo scudo.
- **Mano senza arma:** Maiusc+E, T e riporre agiscono sulla mano **senza arma/scudo**.
- **Tempi (da tarare):** fendente 0,15 s caricamento + 0,15 s attivo + 0,3 s recupero · affondo più lento e più potente · parata perfetta nei primi 0,15 s.
- **Efficacia (in background, dati modificabili):** probabilità di colpire `p = clamp(0,6 + 0,05 × (Destrezza_att − Destrezza_dif), 0,15, 0,95)` · danno `= base_arma × clamp(1 + 0,10 × (Forza_att − Forza_dif), 0,5, 2,0)` (fendente 2, affondo 3) · parata riuscita `clamp(0,5 + 0,05 × (Destrezza_dif − Destrezza_att), 0,1, 0,9)`; con scudo +0,2 e −70% al danno · spinta: distanza e stordimento ∝ (Forza_att − Forza_dif) e inversamente alla massa.
- **Salita (Maiusc+W):** su superfici "scalabili" (tavolo, sedia, scale) fino a **1,0 m** sopra il livello dei piedi. **Ctrl:** altezza occhi da 1,65 a 1,15 m.
- **Scheletro attore:** stesse azioni tramite lo stesso insieme di comandi; si risveglia con O2 e attacca; non crafta e non lancia incantesimi.

### Istanza e online (Q1–Q2) — solo modello dati
```
Istanza { id, versioneRegole, hashScena, libro { nome, note, cronologia[ {autore, vita, tempo, tipo, testo} ] },
          scheletro { statistiche }, vite, vittoria }
InstanceStore { carica(id) ; applica(delta) }      // oggi: file locale · domani: server (opt-in)
```
Cronologia in sola aggiunta con identificatori univoci (si fonde senza conflitti); identità del giocatore anonima e locale; nessun invio di dati senza consenso esplicito.

---

### Contenuto della sala (R1–R6) — assegnazione predefinita (tutta nei dati della stanza)
| Slot | Elemento | Contenuto / effetto predefinito |
|---|---|---|
| Quadro nord (Ottone, corona) | nicchia | da definire (riserva) |
| Quadro sud (Ser Guelfo, elmo) | nicchia | **attrezzatura alchemica + crogiolo** |
| Quadro est (Baldovino, barba) | nicchia | **pane + sacco di farina** |
| Quadro ovest (Rodrigo, cappello) | nicchia | da definire |
| Porta nord | **laghetto** | acqua illimitata (riempie secchio e fiale, si beve) |
| Porte sud, est, ovest | loot dietro la porta | da definire; **una è l'uscita** (provvisoria: **ovest**, `exit: true` nei dati) |
| Parete nord | **muschio** (5 porzioni, si ripristina a ogni vita) | ingrediente per ricette/incantesimi |
Assegnazione arbitraria, da cambiare nei dati senza toccare il codice. Finché gli enigmi non sono progettati, ogni slot ha un **enigma provvisorio sempre bloccato** con scorciatoia di debug (`--unlock <id>`) per provare il loot.

### Alchimia, pane e acqua (R2–R5)
- **Attrezzatura alchemica** (oggetti): mortaio con pestello (1,5 kg), alambicco (2 kg), 3 fiale (0,1 kg), bruciatore. **Crogiolo** (1 kg, contenitore da fusione, livello di calore 1 sul camino acceso).
- **Pane:** +35 fame, 0,5 kg, infiammabile. **Sacco di farina:** 2 kg.
- **Ricetta pane:** `0,3 kg farina + 0,25 L acqua → impasto (0,55 kg)`; `impasto + fuoco acceso (camino) per 60 s → pane`. Un sacco ≈ 6 pani.
- **Acqua:** secchio contiene fino a 5 L; fiala 0,1 L; bere al laghetto +40 sete per 0,5 L. Acqua illimitata solo dopo aver aperto la porta nord.
- **Muschio:** 0,05 kg per porzione; usi (ricette, incantesimi) definiti con F20.

## Decisioni aperte (valori predefiniti che userò salvo diversa indicazione)
1. **Formato dei dati:** JSON (libreria header-only `nlohmann-json`).
2. **Acqua e cibo:** *risolta* dalle regole R2–R4 (laghetto dietro la porta nord; pane e farina in una nicchia). L'acqua serve anche a spegnere fuochi e torce.
3. **Ereditarietà delle statistiche:** si ereditano quelle **di base** lanciate dal personaggio morto, non quelle con i modificatori dell'armatura.
4. **Nuove statistiche alla rinascita:** nuovo lancio dei dadi (minigioco attuale).
5. **Abilità:** da riapprendere a ogni vita; incantesimi e crafting non esistono ancora come sistema (F20).
6. **Buio:** 15 s di conto alla rovescia prima del game over definitivo.
7. **Lotta senza armi:** solo la spinta (tasto dx tenuto); nessun pugno, a meno che tu non lo voglia.
8. **Sconfitta dello scheletro:** si accascia in un mucchio d'ossa e resta nella sala fino alla morte successiva (poi torna seduto, come da K1).
9. **Porta di uscita:** provvisoriamente la **ovest** (`exit: true` nei dati, modificabile); come si apre è un enigma ancora da progettare (F10).
10. **Fusione:** il crogiolo è in una nicchia (R2); lo stampo è ancora da collocare; carico massimo fisso a 20 kg.
11. **Accesso ai quadri:** i quadri stanno a 2,95–3,88 m, oltre la salita massima (1,0 m, O4): predefinito = **risolto l'enigma, il quadro si apre e il loot scende** su una mensola raggiungibile o cade a terra (niente scalata). Alternativa: aggiungere una scala.
12. **Loot delle nicchie e delle porte non ancora assegnato:** valori provvisori nei dati, da decidere con gli enigmi.
13. **Libro/crafting:** alchimia e incantesimi (F20) usano muschio e attrezzatura; le ricette sono da definire.
