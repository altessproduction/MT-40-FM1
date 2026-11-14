# MT-40X — guide des 28 riddims (banque SLOOP Dub 1.6, l'articulation de la machine)

La banque part des vingt-huit riddims de SLOOP Dub 1.6
(<https://github.com/altessproduction/sloop-dub-fm1>, étiquette v1.6) : quatre mesures de
seize doubles-croches, **les trois parties de la MT-40** (basse, skank, batterie — la v0.5
a retiré la mélodie, qui était la voix du toaster chez SLOOP Dub ; la machine n'en a pas,
T1 du séquenceur reste la vôtre), chaque vélocité et chaque racine de mesure d'origine.
La v0.5 y ajoute l'articulation de la machine : **la basse ne tient jamais une note, elle
re-frappe** (détail plus bas). Le fichier source de la banque est `riddims.json` ; les
grilles imprimables sont dans `riddims_grids.md`.

## La banque

Familles, d'après la documentation de SLOOP Dub : le reggae / dub / roots d'abord, la ska,
puis les classiques (la famille Studio One / rocksteady, le dancehall de bande et numérique
du début des années 1980), le dancehall, la basse UK, et les cinq autres rythmes
automatiques du Casio MT-40 (le Rock de la machine est le SLENG TENG ci-dessus).

| # | Riddim | BPM | Racines (telles qu'écrites) | Famille |
| --- | --- | --- | --- | --- |
| 1 | ONE DROP | 72 | A1 A1 D2 E2 | le one drop des roots |
| 2 | STEPPERS | 76 | A2 G2 A2 G2 | quatre temps au kick |
| 3 | ROCKERS | 80 | D2 D2 A1 A1 | le rockers des Roots Radics |
| 4 | DUBWALK | 70 | A1 F2 G2 E2 | dub lent, marche de basse |
| 5 | MEDITATE | 66 | A2 G2 F2 E2 | meditation rock, très clairsemé |
| 6 | DUB CUT | 74 | A1 A1 F2 F2 | dub à trous, la basse coupée |
| 7 | ZION | 68 | D2 A1 D2 E2 | roots qui monte |
| 8 | SKA | 88 | A2 A2 F2 G2 | la marche ska |
| 9 | HEAVENLESS | 80 | A1 F2 A1 G2 | Vin Gordon, Studio One 1968 |
| 10 | TONIGHT | 84 | E2 A2 G2 E2 | Keith & Tex 1967, la basse roulante |
| 11 | GET IN GROOVE | 88 | F2 A2 G2 F2 | Studio One 1968 (Up Park Camp) |
| 12 | HOT MILK | 92 | C2 F2 G2 C2 | Jackie Mittoo 1968 |
| 13 | ANSWER | 82 | A1 A1 A1 A1 | l'accord unique, Slim Smith 1967 |
| 14 | PROMISED LAND | 78 | F2 A#2 C2 G2 | Dennis Brown 1981 (avec Aswad) |
| 15 | GUNMAN | 78 | C2 C2 G#1 A#1 | Michael Prophet 1980, la marche lourde des Roots Radics |
| 16 | HERE I COME | 95 | A1 C2 D2 E2 | Barrington Levy 1985, la pompe en octaves |
| 17 | SLENG TENG | 82 | C#2 ×4 | le Rock du MT-40 — Wayne Smith, King Jammy's 1985 |
| 18 | TEMPO | 88 | A1 C2 D2 A1 | Anthony Red Rose 1985, Firehouse |
| 19 | PEANY PEANY | 92 | A1 C2 A1 E2 | le rebond numérique (Fade Away) 1987 |
| 20 | DANCEHALL | 95 | A1 D2 A1 G2 | le clap 3+3+2 |
| 21 | DEMBOW | 100 | A1 A1 E2 E2 | boom-ch-boom-chick |
| 22 | JUNGLE | 172 | A1 A1 C2 G1 | les subs de la jungle |
| 23 | DUBSTEP | 140 | A1 A1 G1 G1 | le half-time, le wobble |
| 24 | MT40 POPS | 96 | E2 G2 A2 E2 | le Pops de la machine |
| 25 | MT40 SLOWRK | 66 | A1 A1 E2 D2 | le Slow Rock de la machine |
| 26 | MT40 SWING | 84 | A2 F2 G2 E2 | le Swing de la machine |
| 27 | MT40 WALTZ | 90 | A2 F#2 D2 E2 | la Valse de la machine (posée sur du 4/4) |
| 28 | MT40 SAMBA | 100 | A2 A2 G2 G2 | la Samba de la machine (l'agogo sur le rim) |

Le Sleng Teng est la transcription vérifiée (l'onglet de Noel Davey, le roll Ableton, 82
bpm) : la fondamentale sur le temps, trois doubles-croches un demi-ton au-dessus, la queue
une quinte au-dessus, coupée court — les racines de mesure restent C#, les accords du skank
montent au-dessus (C#, B, A, B).

## Comment un riddim joue dans MT-40X

- **Sans touche pressée**, il joue comme écrit : chaque mesure porte sa racine à elle
  (colonne « Racines »). C'est le motif de SLOOP Dub, pas un dégradé.
- **Le clavier re-fonde** : toutes les mesures portent alors la fondamentale pressée, la
  basse et les accords suivent. Voir LISEZMOI.md pour la règle des touches.
- **La basse re-frappe** (v0.5, `tools/mt40_bass.py`) : la machine pince, elle ne tient
  pas. Chaque frappe écrite est suivie d'une **re-frappe de la même note** à la
  double-croche suivante, plus légère ; une frappe forte qui tient trois pas re-frappe
  encore à la croche suivante. Les re-frappes héritent de la note : MAJ/DOM7 les déplacent
  avec elle, la dernière quinte de mesure et ses re-frappes descendent ensemble au b7.
  Les silences restants tiennent (TIE Felucca) ; les pas marqués `cut` dans les grilles
  coupent la note tenue (MT40_CUT). Le Sleng Teng re-frappait déjà chaque double-croche
  (le groove `mtstut`, la transcription vérifiée) : il passe inchangé.
- **La qualité de l'accord** choisie au clavier change la basse : MINEUR = le groove tel
  qu'écrit ; MAJEUR = la tierce mineure monte d'un demi-ton (+3 → +4) ; SEPTIÈME = comme le
  majeur, et la **dernière quinte de chaque mesure descend à la septième bémol** (+7 → +10),
  le turnaround du dominante. Ces deux variantes sont dérivées mécaniquement : comparez avec
  la vraie machine et corrigez `riddims.json` (CALIBRATION.md).

## Du son, côté FM-1

- **T3 la basse** : ANALOG, grave, bref — DEC court, un filtre fermé ; le MT-40 pince, il
  ne tient pas. Le GATE 127 posé au chargement fait tenir les TIE ; l'enveloppe fait le
  pincé, les re-frappes font le rebond.
- **T2 le skank** : un son carré ou orgue court ; l'upstroke reste noir (GATE 40).
- **T4 la batterie** : le kit STD convient ; les niveaux de pistes (P_LN0..7) s'ajustent
  comme toujours.
- **T1 libre** : le riddim n'y entre plus (v0.5) — posez-y votre ligne ; le MT-40 n'avait
  que basse et batterie, et le skank reste la voix du sound system, à prendre ou à laisser.

## Le Sleng Teng pas à pas (pour la calibration)

Riddim n° 17, 82 bpm, racines C#2 (MIDI 37) sur les quatre mesures, accords de skank
C# / B / A / B (le skank reste au registre 57..68, ici 58 = A#3 :

- mesures 1 à 4 (le groove `mtstut`, identique partout) : C# sur le temps, puis trois
  doubles-croches à C#+1 (D) qui re-frappent, trois fois ; le pas 12 à C#+6 (G), coupé au
  pas 13 ; le pas 14 à C#+5 (F#), coupé au pas 15.
- batterie (`mt_rock`) : kick sur les temps 1 et 3, caisse claire sur 2 et 4, charleys aux
  contretemps, et le fill de la mesure 4 (la caisse claire aux pas 12, 14 et 15).
- skank (`mtup`) : l'upstroke du joueur, aux pas 2, 6, 10, 14.

C'est ce dessin que la vraie machine donne avec le preset Rock ; les grilles imprimables de
`riddims_grids.md` le montrent pas par pas.

## Les six rythmes du MT-40 dans la banque

| MT-40 | Riddim MT-40X |
| --- | --- |
| Rock | SLENG TENG (n° 17) |
| Pops | MT40 POPS (n° 24) |
| Slow Rock | MT40 SLOWRK (n° 25) |
| Swing | MT40 SWING (n° 26) |
| Waltz | MT40 WALTZ (n° 27) |
| Samba | MT40 SAMBA (n° 28) |
