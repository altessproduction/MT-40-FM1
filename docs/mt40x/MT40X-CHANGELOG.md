# MT-40X — changelog

## v0.5 (mt40x_v1.8.2-0.5) — les trois parties de la machine, la basse qui re-frappe

- **La mélodie quitte la banque** : le MT-40 joue basse et batterie, le skank est la voix
  du sound system ; la mélodie était celle du toaster de SLOOP Dub, la machine n'en a pas.
  `riddims.json` (format `mt40x-riddims-2`) et la struct `mt40_riddim_t` perdent le champ
  mel (3 584 B de flash rendus) ; au chargement **T1 est vidé et vous reste** (son,
  réglages, enregistrement). T2 skank / T3 basse / T4 batterie ne changent pas.
- **La basse re-frappe comme la machine** (`tools/mt40_bass.py`, le cœur de cette version)
  : le MT-40 pince, il ne tient pas — sa Rock jouait une frappe par double-croche. Les
  grooves convertis de SLOOP Dub frappaient une note puis la tenaient (TIE) : les
  **répétitions de note manquaient**, la basse lisait tenue là où la machine rebondit.
  Maintenant chaque frappe est suivie d'une **re-frappe de la même note** à la
  double-croche suivante (−22 de vélocité), et une frappe forte qui tient trois pas
  re-frappe encore à la croche suivante (−34, plancher 64) ; les queues restent celles du
  groove (`cut`), les silences restants tiennent. Le Sleng Teng (`mtstut`, la
  transcription vérifiée Noel Davey) et la marche des Roots Radics (`mtmarch`)
  re-frappaient déjà : ils passent **inchangés** ; les autres grooves gagnent le rebond
  (932 frappes de basse dans la banque, contre 750 à l'identique SLOOP). Un degré, une
  racine, un drum, un skank : rien d'autre ne bouge.
- **Le DOM7 suit la note** : le turnaround (la dernière quinte de mesure qui descend au
  b7) s'applique à la quinte **et à ses re-frappes** — la course entière (frappe + pincés)
  descend ensemble, là où un pincé en fin de mesure aurait pu échapper à la règle
  (`mt40.c mt40_deg`).
- La chaîne de provenance suit : `verify_bank.py` vérifie drums/skank/racines/accords à
  l'identique SLOOP Dub 1.6 et la basse contre `mt40_bass.articulate` (un calage à la main
  ensuite est signalé comme dérive, pas une erreur) ; `gen_mt40_riddims.py --bootstrap`
  donne directement la forme v0.5. Les grilles imprimables (`riddims_grids.md`) montrent
  les re-frappes : la même intervalle écrite sur les pas suivants est la note re-frappée.
- Le paquet : `docs/firmware/mt40x_v1.8.2-0.5.fwsc` (610 286 B, identité `FM-1_23456`), la
  release `build/release-0.5/`. L'émulateur recompilé (wasm 1 247 075 B, `docs/dub/`
  synchronisé, la console affiche T1 libre) ; le smoke test v0.5 passe : AUDIO-PATH-OK,
  « the machine's three tracks loaded, T1 left empty », le clavier re-racine. `ui_test`
  passe sur les deux builds (défaut et FELUCCA_CPU2=0) : le chargement, la re-frappe à la
  double-croche, T1 vide, le clavier MAJ / +1 MIN / +2 DOM7.

## v0.4 (mt40x_v1.8.2-0.4) — les sons MT-40 et l'affichage du MENU

- **Les sons MT-40 posés au chargement** (`mt40.c mt40_sounds`) : chaque chargement de
  riddim pose sur T1 (mélodie), T2 (skank) et T3 (basse) des timbres ANALOG dans l'esprit
  Casiotone — T3 la pluck carrée creuse (SQR, CUT 50, un peu de drive, ENV pluck), T1
  l'impulsion fine (PWM, CUT 85, qui sonne), T2 l'attaque courte (PWM, SUS bas). Le
  chargement garde vos LEVEL, PAN, envois et INSERT ; le moteur change à la limite de bloc
  (voice.c), comme l'UI. Les approximations des 22 timbres réels restent la calibration à
  venir (`CALIBRATION.md`).
- **L'affichage du MENU suit les valeurs MT-40X** (`ui_menu.c`) : `mt40_on` et `mt40_rid`
  entrent dans la signature de redessin du MENU — sans eux, tourner OCT+ sur RIDDIM ou
  basculer MT-40X changeait le son sans changer l'écran (la ligne se rafraîchissait au
  geste suivant seulement). Même motif que « SCREEN OFF's value redraws its row » (#157).
- Le paquet : `docs/firmware/mt40x_v1.8.2-0.4.fwsc` (610 286 B), la release
  `build/release-0.4/`. L'émulateur recompilé (wasm 1 250 693 B, `docs/dub/` synchronisé) ;
  le smoke test v0.4 passe : AUDIO-PATH-OK, le clavier re-racine, un riddim charge sur un
  autre. `ui_test` passe sur les deux builds (défaut et FELUCCA_CPU2=0).

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
