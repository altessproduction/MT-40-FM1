# MT-40X — changelog

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
