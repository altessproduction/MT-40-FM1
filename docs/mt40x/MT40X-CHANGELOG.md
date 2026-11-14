# MT-40X — changelog

## v0.3 (mt40x_v1.8.2-0.3) — l'identité du hub et le dossier de soumission

- **L'identité du paquet devient `FM-1_23456`** — le numéro de MT-40X au Firmware Hub
  (<https://thefirmwarehub.com/>), un numéro `FM-1_` n'appartient qu'à un firmware. Elle vit
  dans la chaîne `FELUCCA_ID` du firmware (ABOUT, la réponse du protocole OTA :
  `ota-FM-1_23456`) et dans le marqueur du `.fwsc` (ce que le hub lit). Le loader embarqué
  reprend l'identité du paquet (loader.c la reçoit du build, `ota-` préfixé) ; les builds de
  développement gardent FM-1_900.
- **Le dossier hub à la racine** : `fmhub.json` (what anglais ≤ 300 caractères, kinds
  groove, play l'émulateur, cheat l'antisèche, la carte, la couleur), la carte
  `assets/mt40x-hub-card.png` (carré 880×880, 49 Ko) et l'antisèche anglaise
  `docs/cheat/index.html` (clavier, 28 riddims et tempos, pistes, réglages 33/34) servie
  par Pages sur `/cheat/`.
- **Le README.md de la racine** refait : tout le fonctionnement (l'histoire du MT-40 et du
  Sleng Teng, le clavier de basse, les 28 riddims et leurs tempos, l'emploi, l'émulateur,
  l'installation, le hub, la compilation, la calibration, les licences).
- Le paquet : `docs/firmware/mt40x_v1.8.2-0.3.fwsc` (610 286 B), la release
  `build/release-0.3/` (paquet, `.bin` applicatif, SHA256SUMS, licences, attribution).
  Le `.fwsc` de v0.2 reste flashable — l'identité ne change que le marquage et ABOUT.
- L'émulateur recompilé (le wasm ressort identique octet pour octet : la chaîne d'identité
  n'y est pas référencée ; `docs/dub/` synchronisé) — le smoke test finit sur
  `AUDIO-PATH-OK`.

## v0.2.1 — l'émulateur web

- **Le firmware MT-40X tourne dans le navigateur** : <https://altessproduction.github.io/MT-40-FM1/dub/>
  (les sources de `web-emu/`, la copie Pages dans `docs/dub/`). Le vrai firmware — Felucca 1.8.2 +
  le MT-40X — compilé en wasm32 freestanding (`web-emu/build.sh`, clang ou `zig cc`, pas
  d'emscripten), exécuté dans un AudioWorklet : le son (mix_block à 44,1 kHz en fixe), le
  séquenceur, les écrans (le `lcd.c`/`gfx.c` du firmware dessine chaque pixel derrière un modèle
  de panneau ST7789), les LED, le clavier de basse et la banque sont le propre code du firmware.
  Aucune source du firmware n'est modifiée.
- La façade (géométrie d'après le manuel FM-1, knobs, boutons, clés, clavier
  d'ordinateur, MIDI entrée) suit la disposition de `reference_emu/` de SLOOP Dub ; la page est
  expliquée en français. La flash SPI est une image IndexedDB : projets, réglages et autosave
  survivent au rechargement.
- **La console web** sous la machine : l'interrupteur MT-40X, les 28 riddims un clic (le chargement
  de la page RIDDIM elle-même, lues dans le wasm par `emu_mt40_name`/`emu_mt40_load`) et la vue
  des quatre pistes relue dans le séquenceur (`emu_seq_*`, le playhead compris). L'état
  (riddim chargé, accord du clavier) est l'état du firmware.
- Le smoke test livré : `node web-emu/wasm_smoke_test.js dist/mt40x.wasm` — les exports, le boot,
  la banque (28, noms, tempos), le chargement des quatre pistes, PLAY + le chemin audio, le
  clavier MAJ / +1 MIN / +2 DOM7 — finit sur `AUDIO-PATH-OK`.
- Le `.fwsc` ne change pas (le matériel n'est pas touché) ; les mêmes sources donnent l'émulateur
  et l'installable.

## v0.2 (mt40x_v1.8.2-0.2)

- **La banque de riddims est exactement celle de SLOOP Dub 1.6** (altessproduction,
  <https://github.com/altessproduction/sloop-dub-fm1>, étiquette v1.6) : 28 riddims de
  64 pas, quatre pistes (basse, skank, mélodie, batterie), chaque hit, vélocité, degré et
  racine de mesure d'origine. Source de vérité : `riddims.json` (le générateur
  `tools/gen_mt40_riddims.py` le convertit en `firmware/src/mt40_riddims.h` et en grilles
  imprimables `riddims_grids.md` ; `--bootstrap` re-extrait la banque de la copie conforme
  `tools/sloop_riddims_data.py`).
- La v0.1 portait 10 riddims dessinés à la main ; ils cèdent la place à la banque de
  référence. Les six rythmes du MT-40 y sont (SLENG TENG = Rock — la transcription
  vérifiée Noel Davey / Ableton à 82 bpm — plus POPS, SLOWRK, SWING, WALTZ, SAMBA).
- **Quatre pistes chargées** (la v0.1 n'en chargeait que deux) : T1 la mélodie, T2 le skank
  en vrais accords (POLY, GATE 40), T3 la basse (MONO, GATE 127, les TIE tiennent, MT40_CUT
  coupe), T4 la batterie (moteur DRUM, vélocités d'origine).
- **Le tempo du riddim entre au chargement** (66..172, la doc de SLOOP Dub).
- **Les fills sont dans le motif** (mesure 4), comme chez SLOOP Dub : la grille de fill
  séparée de la v0.1 disparaît ; GLO + E4 (les conditions FILL de Felucca) reste disponible.
- Le clavier de basse, le synchro start et les deux lignes MENU > AUDIO (MT-40X, RIDDIM,
  réglages éditeur 33 et 34) ne changent pas.
- Les tests : `ui_test` (les deux variantes CPU) couvre le chargement, le clavier
  (MAJ / +1 MIN / +2 DOM7 / la même touche = MAJ), le synchro start et les deux lignes
  MENU ; `editor_test` couvre les réglages 33/34 ; `ui_render` valide l'écran de
  démarrage et la page MENU 8 lignes ; `regress` : les 98 rendus dorés inchangés.

## v0.1 (mt40x_v1.8.2-0.1)

- Première version : Felucca 1.8.2 + le clavier de basse du MT-40 (fondamentale = majeur,
  +1 = mineur, +2 = septième), le synchro start, 10 riddims à la main (dont le Sleng Teng
  extrait du tutoriel Ableton), les lignes MENU > AUDIO, la grille de calibration
  `riddims_grids.md` et la documentation française.
