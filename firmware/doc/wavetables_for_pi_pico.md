# Wavetable-Bibliotheken für DIY-Eurorack-Synthesizer mit Raspberry Pi Pico

Für ein Eurorack-Modul mit Raspberry Pi Pico und PCM5102 DAC stehen zahlreiche hochwertige, frei lizenzierte Wavetable-Sammlungen zur Verfügung. Die **Adventure Kid Waveforms (AKWF)** mit über 4.300 Wellenformen unter CC0-Lizenz und die **Mutable Instruments**-Bibliotheken unter MIT-Lizenz bilden das Fundament für nahezu jedes DIY-Projekt. Das 256-Sample-Format mit 16-Bit Tiefe hat sich als optimaler Kompromiss zwischen Klangqualität und Speichereffizienz für eingebettete Systeme etabliert.

---

## Die AKWF-Sammlung als universelle Grundlage

Die **Adventure Kid Waveforms** (AKWF-FREE) auf GitHub repräsentieren den De-facto-Standard für Open-Source-Wavetables. Mit **4.300+ Single-Cycle-Wellenformen** in CC0-Lizenz (Public Domain) deckt die Sammlung praktisch alle Klangkategorien ab – von akustischen Instrumenten bis zu synthetischen Texturen. Kommerzielle Produkte wie der QuBit Chord und Elektron Model:Samples nutzen diese Wellenformen bereits.

Die technische Vielseitigkeit überzeugt besonders: Die Original-WAV-Dateien mit **600 Samples bei 16-Bit/44.1kHz** existieren parallel als konvertierte Versionen für verschiedene Plattformen. Der Ordner **AKWF--Teensy** enthält bereits fertige C-Header-Arrays mit 256 Samples – ideal für Mikrocontroller-Projekte. Zusätzlich stehen JavaScript-Arrays, PNG-Visualisierungen und Formate für Reaktor, Surge und U-he Zebra bereit.

| Kategorie | AKWF-Ordner | Enthaltene Klänge |
|-----------|-------------|-------------------|
| Streicher | cello, violin, stringbox | Gestrichene Saiteninstrumente |
| Bläser | altosax, clarinet, theramin | Holz- und Blechbläser |
| Zupfinstrumente | aguitar, plucks | Akustische Gitarren, Zupf-Algorithmen |
| Piano | piano | Klaviersamples |
| Percussion | snippets, bitreduced | Transienten, 8/16-Bit-Videospielklänge |
| Synthesizer | synth_classics, blended | Klassische Synthesizer-Wellenformen |

---

## Mutable Instruments als Goldstandard für Eurorack

Das GitHub-Repository **pichenettes/eurorack** von Émilie Gillet enthält die Wavetables der legendären Braids- und Plaits-Module unter **MIT-Lizenz**. Diese Lizenz erlaubt uneingeschränkte kommerzielle und private Nutzung – ein entscheidender Vorteil für DIY-Projekte.

**Braids** bietet 20 benannte Wavetables mit insgesamt **256 Wellenformen** in 8-Bit-Format: male, female, choir, space_voice, tampura, bowed, cello, vibes, slap, piano, organ, waves, digital, drone und bell. Die Daten liegen als Raw-Binary im `/braids/data/`-Verzeichnis vor und werden durch Python-Skripte in C++-Resource-Dateien konvertiert.

**Plaits** organisiert seine Wellenformen in **vier Bänken à 8×8 Wellenformen**:
- **Bank A**: Obertonarme Wellenformen (Additive Synthese, Drawbar-Orgel)
- **Bank B**: Obertonreiche Wellenformen (Formant-Synthese, Waveshaping)  
- **Bank C**: Wavetables aus Shruthi-1/Ambika (klassische ROM-Playback-Sounds)
- **Bank D**: Semi-zufällige Permutationen der anderen Bänke

Ein Online-Wavetable-Editor unter `pichenettes.github.io` ermöglicht das Erstellen eigener Plaits-kompatiblerWavetables direkt im Browser.

---

## Technische Spezifikationen für den Raspberry Pi Pico

Der Pico mit seinen **264 KB RAM** und **2 MB Flash** erfordert durchdachte Speicherstrategien. Bei 16-Bit-Samples mit 256 Samples pro Wellenform belegt jede Welle nur **512 Bytes** – damit passen theoretisch über 500 Wellenformen ins RAM oder 4.000+ in den Flash-Speicher.

**Empfohlenes Format für Pico-Projekte:**
- **Sample-Länge**: 256 Samples (WaveEdit-Standard, optimal für Morphing)
- **Bit-Tiefe**: 16-Bit signed PCM für den PCM5102 DAC
- **Speicherung**: `const`-Arrays im Flash via PROGMEM-Äquivalent
- **Sample-Rate**: 44.1 kHz (PCM5102 unterstützt bis 384 kHz)

Die **Mozzi-Bibliothek** unterstützt den Pico nativ mit I2S-Ausgabe und bringt vorgefertigte Wavetables im Ordner `/tables/` mit – Sinus, Sägezahn, Rechteck und Dreieck in verschiedenen Größen (256-8192 Samples). Das Python-Skript `char2mozzi.py` konvertiert beliebige Audio-Dateien in Mozzi-kompatible C-Arrays.

| Speichertyp | Kapazität | Bei 256×16-Bit | Anwendung |
|-------------|-----------|----------------|-----------|
| RAM | 264 KB | ~500 Wellenformen | Aktive Morphing-Bänke |
| Flash | 2 MB | ~4.000 Wellenformen | Gesamte Bibliothek |

---

## Spezialisierte Sammlungen nach Klangkategorie

### Orchesterinstrumente und akustische Klänge

Das GitHub-Repository **wavetable_orchestra** von CLeJack bietet Single-Cycle-Wellenformen von Orchesterinstrumenten im 2048-Sample-Format. Die Ordner `string_theory_wav` und `virtual_orchestra_wav` enthalten Streicher und orchestrale Ensembles mit Key-Tracking-Informationen im Dateinamen für realistische Approximation verschiedener Tonhöhen.

### Pads und Ambient-Texturen

**Echo Sound Works Core Wavetables** – 400 kostenlose Wavetables mit dedizierter Pad/Drone-Kategorie sowie Spectral- und Analog-Sektionen (Moog, Juno, Prophet-Sounds). Die **KRC Mathwaves**-Sammlung auf Gumroad enthält 1.600+ mathematisch generierte Wellenformen mit Tendenz zu glockenartigen Klängen, verfügbar im WaveEdit-Format (64×256 Samples).

### Orgeln und Keys

Die Mutable-Instruments-Bänke enthalten explizite Orgel-Wavetables. Bank A von Plaits simuliert Drawbar-Orgeln durch additive Synthese. Die AKWF-Sammlung bietet zusätzlich dedizierte Synthesizer-Classics, die Hammond-ähnliche Klänge ermöglichen.

### Percussion und perkussive Texturen

Die AKWF-Ordner **snippets** (Transienten verschiedener Quellen) und **bitreduced/videogame** (8/16-Bit-Ästhetik) eignen sich für perkussive Anwendungen. **Toybox Audio** bietet 142 Wavetables mit FM-Tönen und Casio-CZ-ähnlicher Phasenverzerrung.

---

## WaveEdit als Editor und Format-Standard

Der kostenlose, plattformübergreifende **WaveEdit**-Editor von Synthesis Technology definiert das dominierende Eurorack-Wavetable-Format: **64 Frames × 256 Samples = 16.384 Samples pro Bank** bei 16-Bit/44.1kHz. Die Software ermöglicht Zeichnen, FFT-basierte Harmonik-Bearbeitung und Audio-Preview.

**Forks für erweiterte Formate:**
- Jeremy Bernstein Fork: 1024 Samples pro Frame
- ModWave Fork: 2048 Samples × 64 Frames für Korg Modwave
- OsirisEdit: 32×32 Frames für Modbap Osiris

Das GitHub-Archive **wavedit-online** (smpldsnds) enthält eine Bibliothek community-erstellter Wavetable-Bänke unter CC-Lizenz, darunter PPG-Vintage-Nachbildungen und Ensoniq ESQ-1-Konvertierungen.

---

## PPG-Wavetables und Vintage-Ressourcen

Für authentische PPG-Wave-Klänge existiert das Repository **Jacajack/wave-stuff** mit extrahierten EPROM-Daten: `ppg_data.c` enthält 29 Original-Wavetables (Nr. 0-28) als 8-Bit-Wellenformen mit 64 Samples pro Halbzyklus. Die Struktur spiegelt die originale Sparse-Speicherung wider, bei der lineare Interpolation zwischen „Key"-Wellenformen erfolgt.

Das **PPG Synth Net CDROM** (330 MB ISO) und die Archive auf `waldorf.electro-music.com` bieten zusätzliche Waveterm-Library-Dateien, allerdings mit unklarer Lizenzlage für DIY-Verwendung.

---

## Konvertierungstools für Embedded-Formate

| Tool | Funktion | Ausgabeformat |
|------|----------|---------------|
| **char2mozzi.py** | Mozzi-Konvertierung | int8_t C-Arrays |
| **WavToCode** | Windows-GUI | 8/16/24-Bit C-Arrays |
| **SoundFontDecoder** | SF2→Teensy | 16-Bit .cpp/.h |
| **OKWT** | Format-Konvertierung | Serum, Surge, Vital |

Der **TeensyAudio Wavetable-Synthesis**-Decoder extrahiert Wellenformen aus beliebigen SoundFont-Dateien (.sf2) – damit wird das gesamte Universum frei verfügbarer SoundFonts zur Wavetable-Quelle.

---

## Lizenzübersicht der Hauptquellen

| Sammlung | Lizenz | Kommerzielle Nutzung | Repository/URL |
|----------|--------|----------------------|----------------|
| AKWF-FREE | **CC0 1.0** | Ja, ohne Einschränkung | github.com/KristofferKarlAxelEkstrand/AKWF-FREE |
| Mutable Instruments | **MIT** | Ja, ohne Einschränkung | github.com/pichenettes/eurorack |
| Mozzi | LGPL 2.1+ | Ja, bei Linking-Beachtung | github.com/sensorium/Mozzi |
| WaveEdit | Open Source | Ja | github.com/AndrewBelt/WaveEdit |
| Echo Sound Works | Royalty-Free | Ja | echosoundworks.com |
| KRC Mathwaves Free | Frei | Ja | kcrosley.gumroad.com |
| Wavetable Orchestra | Frei | Unklar | github.com/CLeJack/wavetable_orchestra |

---

## Praktische Empfehlung für das Pico-Projekt

Für ein Eurorack-Modul mit Raspberry Pi Pico und PCM5102 DAC ergibt sich folgende optimale Strategie:

**Primäre Quellenauswahl**: AKWF-FREE als Hauptbibliothek (CC0, 4.300+ Wellenformen) kombiniert mit Mutable Instruments Plaits-Bänken (MIT, kuratierte 256 Wellenformen mit Morphing-Struktur).

**Formatkonvertierung**: Original-WAVs mittels Python-Skript in 256-Sample/16-Bit C-Arrays konvertieren. Der AKWF--Teensy-Ordner enthält bereits fertige Header-Dateien, die nach Anpassung der Datentypen direkt verwendbar sind.

**Speicherarchitektur**: Gesamte Bibliothek im Flash speichern, maximal 4-8 aktive Wavetable-Bänke für Echtzeit-Morphing im RAM halten. Bei 64×256×2 Bytes pro Bank belegt eine vollständige Morphing-Bank nur 32 KB.

**Audio-Ausgabe**: Mozzi-Bibliothek mit I2S-Konfiguration für den PCM5102 bei 44.1 kHz nutzt die Hardware optimal aus und bietet bewährte Interpolationsalgorithmen für alias-freies Wavetable-Playback.