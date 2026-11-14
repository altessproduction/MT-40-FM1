# L'émulateur web MT-40X

**Jouer : <https://altessproduction.github.io/MT-40-FM1/dub/>** (GitHub Pages, dossier `docs/dub/`)

Un émulateur navigateur du M-VAVE FM-1 qui tourne sur le **firmware MT-40X de ce dépôt**
(GPL-3.0) compilé en WebAssembly, avec un HAL de navigateur à la place du matériel FM-1.
Aucune ré-implémentation du synthé : le son, le séquenceur, les écrans, le clavier de
basse du MT-40 et les 28 riddims de la banque SLOOP Dub 1.6 sont le propre code du
firmware. La flash SPI du FM-1 est une image locale du navigateur (IndexedDB) :
l'autosave, les projets et les réglages survivent au rechargement.

La page est la façade du FM-1, et **le menu du firmware est la seule surface de
commande** — tenez HOME, puis SELECT, KNOB 1–4, OCT+ / OCT−, exactement comme sur la
machine. Sous la façade, la console web ajoute le sucre : l'interrupteur MT-40X, les
28 riddims en un clic (le code de chargement de la page RIDDIM elle-même) et la vue
des quatre pistes, relue dans le séquenceur du firmware. Le tout expliqué en français
sous la machine.

## Comment ça marche

```
web-emu/mt40x_wasm.c     la racine d'unité web : un HAL navigateur (temps, entrées,
                         ADC, un modèle de panneau ST7789, des stubs audio) + les
                         sources du firmware dans le même ordre que sur matériel
web-emu/worklet.js       le wasm tourne DANS un AudioWorklet : process() tire
                         mix_block() pour un audio sans à-coups ; une trame UI court
                         toutes les ~17 ms et publie framebuffer + LED + état MT-40X
web-emu/emu.js           la façade : écran, knobs, boutons, clés, clavier d'ordinateur
                         + entrée MIDI, lueurs LED — et la console MT-40X
web-emu/index.html       la peau de la machine + le manuel en français
docs/dub/                la copie GitHub Pages, resynchronisée par build.sh
```

- L'écran LCD est émulé au niveau du protocole du panneau (CASET/RASET/RAMWR), si bien
  que le `lcd.c`/`gfx.c` du firmware dessine chaque pixel.
- Les entrées vont droit dans l'état debouncé `fm1_in` que lit le firmware ; les
  crans d'encodeur arrivent en pas.
- DSP 44,1 kHz en virgule fixe, comme sur la machine. Aucun flottant dans le firmware.
- Le clavier de basse est le vrai chemin du firmware (`mt40.c mt40_key_press`, l'hook
  de `seq.c keyboard_block`) : MAJ / MIN / DOM7 et le SYNCHRO START sont ceux de la
  machine.

## Compiler

Il faut `clang` + `lld` avec la cible wasm32 (pas d'emscripten — tout clang 18+, par
exemple [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases)), ou `zig cc`
(`pip install ziglang`, la cible `wasm32-freestanding`), et `python3` (+ numpy,
Pillow) pour les tables générées du firmware :

```sh
mkdir -p build/gen
python3 tools/gen_tables.py   build/gen/felucca_tables.h
python3 tools/gen_samples.py  build/gen/felucca_samples.h
python3 tools/gen_adpcm.py    build/gen/felucca_adpcm.h     # si demandé
python3 tools/gen_fm6_patches.py build/gen/felucca_fm6.h
# les tables UI :
python3 tools/gen_aa_font.py  build/gen/ui_fonts.h
python3 tools/gen_aa_icons.py build/gen/ui_icons.h
python3 tools/gen_aa_keycaps.py build/gen/ui_keycaps.h
python3 tools/gen_ui_palettes.py build/gen/ui_palettes.h

sh web-emu/build.sh           # compile dist/mt40x.wasm, resynchronise docs/dub/
node web-emu/wasm_smoke_test.js dist/mt40x.wasm
```

Le smoke test est la vérification livrée : les exports, le démarrage (le splash, la
page HOME), la banque (28 riddims, leurs noms et tempos), le chargement d'un riddim
dans les quatre pistes, le transport PLAY et le chemin audio, le clavier de basse
(MAJ, +1 MIN, +2 DOM7) — il finit sur `AUDIO-PATH-OK`.

## Commandes

- rangée du milieu du clavier d'ordinateur = clés blanches depuis C4, rangée du haut = clés noires
- `Z`/`X` OCT−/OCT+, `1`–`0` = FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ,
  `Espace` PLAY, `R` REC, `↑↓` SELECT, `←→` ALGORITHM, `[` `\` PRESETS
- knobs : glisser ou molette ; boutons/clés : clic ou toucher (multi-touch ok)
- un clavier MIDI branché joue dedans (canaux comme sur la machine : 1–3 les synthés,
  10 la batterie ; MT-40X armé, les clés sont le clavier de basse)

## Limites connues

- L'écran HARDWARE CALIBRATION (tenir OCT− + OCT+ au power-on) attend la matrice de
  clés réelle — n'y entrez pas.
- La capture audio / USB-audio, l'éditeur web SysEx et l'upload d'échantillons sont
  stubbés ; `usb_start` n'est jamais appelé (les barrières `csync` du pi32 et les
  registres USB sortent de la compilation).
- Sur téléphone la machine tourne et se met à l'échelle pour remplir l'écran (portrait).

## Licence

GPL-3.0-only, comme le firmware qu'il embarque. Le HAL web et la coquille suivent la
disposition de `reference_emu/` de SLOOP Dub (altessproduction, GPL-3.0).
