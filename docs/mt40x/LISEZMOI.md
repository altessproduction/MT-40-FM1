# MT-40X v0.5 — le Casio MT-40/41 sur le M-VAVE FM-1

MT-40X est une fourche (fork) du firmware libre **Felucca v1.8.2** (Hügelton Instruments,
GPL-3.0) qui ajoute au FM-1 l'accompagnement automatique du **Casio MT-40 / MT-41** : la
banque de riddims, le clavier de basse, le synchro start. La banque de riddims est celle de
**SLOOP Dub 1.6** (altessproduction, <https://github.com/altessproduction/sloop-dub-fm1>,
GPL-3.0) : **les vingt-huit riddims exactement**, convertis tels quels dans le firmware.

## Nouveau dans la v0.5 (par rapport à la v0.4)

- **Les trois parties de la machine** : la piste mélodie quitte la banque — le MT-40 joue
  basse et batterie, le skank est la voix du sound system ; la mélodie était celle du
  toaster de SLOOP Dub. Au chargement, **T1 est vidé et vous reste** (votre son, vos
  réglages) ; T2 le skank, T3 la basse, T4 la batterie ne changent pas.
- **La basse re-frappe comme la machine** : le MT-40 pince, il ne tient pas — chaque
  frappe est suivie d'une re-frappe de la même note, plus légère (et une frappe forte qui
  tient re-frappe encore à la croche suivante). Le Sleng Teng re-frappait déjà chaque
  double-croche (sa transcription vérifiée) : il passe inchangé ; les 27 autres riddims
  gagnent le rebond. Les règles sont dans `tools/mt40_bass.py` ; les grilles imprimables
  (`riddims_grids.md`) montrent les re-frappes pas à pas.

## Nouveau dans la v0.4 (par rapport à la v0.3)

- **Les sons MT-40 au chargement** : chaque chargement de riddim pose sur T1 (mélodie),
  T2 (skank) et T3 (basse) des timbres ANALOG dans l'esprit Casiotone — la pluck carrée
  creuse de la basse, l'impulsion fine des mélodies, l'attaque courte du skank. Vos LEVEL,
  PAN, envois d'effets et INSERT restent ; retouchez l'OSC/FLT de la piste à votre goût.
  Ce sont des approximations : la banque fidèle des 22 timbres de la machine est la
  calibration à venir, comme les riddims l'ont été.
- **L'affichage du MENU suit** : changer RIDDIM ou basculer MT-40X redessine la ligne
  aussitôt (la signature de redessin du MENU connaissait ces deux valeurs de trop).

## Nouveau dans la v0.3 (par rapport à la v0.2.1)

- **L'identité du paquet est `FM-1_23456`** (le numéro de MT-40X au Firmware Hub) : elle
  remplace FM-1_900 dans ABOUT, la réponse OTA (`ota-FM-1_23456`) et le marqueur du
  `.fwsc`. Le README.md de la racine explique tout le fonctionnement ; `fmhub.json`, la
  carte `assets/mt40x-hub-card.png` et l'antisèche `docs/cheat/index.html` servent la
  soumission au hub. Le reste ne change pas.

## Nouveau dans la v0.2 (par rapport à la v0.1)

- **La banque exacte de SLOOP Dub 1.6** : 28 riddims de 64 pas (4 mesures de doubles-croches),
  chaque hit, chaque vélocité, chaque degré de basse et chaque racine de mesure repris du
  générateur de SLOOP Dub (copie conforme dans `tools/sloop_riddims_data.py`, conversion par
  `tools/gen_mt40_riddims.py`). La v0.1 portait 10 riddims dessinés à la main ; ils laissent
  la place à la banque de référence, qui contient déjà les six rythmes du MT-40
  (SLENG TENG = Rock, plus POPS, SLOWRK, SWING, WALTZ, SAMBA) et le Sleng Teng transcrit
  d'après l'onglet de Noel Davey et le roll Ableton, à 82 bpm.
- **Quatre pistes chargées** au lieu de deux : T1 la mélodie, T2 les accords du skank
  (vrais accords de 3 ou 4 notes, piste en POLY), T3 la basse, T4 la batterie (moteur DRUM).
- **Le tempo du riddim entre au chargement** (BPM de la doc de SLOOP Dub : 66 à 172).
- **Les fills sont dans le motif** (mesure 4), comme dans SLOOP Dub — plus de grille de fill
  séparée ; le bout de v0.1 disparaît, GLO + E4 (les conditions FILL de Felucca) reste
  disponible sur les pas que vous marquez vous-même.
- Le clavier, le synchro start et les deux lignes MENU > AUDIO (MT-40X, RIDDIM) ne changent pas.

## Installation

1. **Sauvegardez d'abord** : projets, presets utilisateur, échantillons (SAVE > PROJECT, ou
   l'éditeur web). Une installation remplace le firmware, pas vos données flash — mais une
   sauvegarde avant toute manipulation de firmware est la règle.
2. Copiez `mt40x_v1.8.2-0.5.fwsc` sur une clé USB (FAT32) ou passez par l'installateur web.
   Sur le FM-1, mode mise à jour via M-UPGRADE, choisissez le paquet, l'appareil redémarre.
   L'écran de démarrage affiche « MT-40X v1.8.2-mt40x ».
3. Le reste de Felucca 1.8.2 est intact : moteurs, presets, projets, éditeur web, MIDI.

## Emploi

**Ouvrir le MENU** : HOME maintenu (~ 1 s). Ensuite : **ALGORITHM** change d'onglet
(DISPLAY → CONTROL → AUDIO → MIDI → SYSTEM), **PRESETS** descend ligne par ligne,
**KNOB 1–4** ou **OCT+ / OCT−** change la valeur de la ligne choisie, HOME referme.
Le chemin : HOME maintenu → ALGORITHM jusqu'à AUDIO → PRESETS jusqu'à MT-40X → OCT+ = ON.

**MENU > AUDIO** a deux lignes de plus que Felucca :

- **RIDDIM** : 28 valeurs — un tour de bouton **charge le riddim aussitôt** (les trois
  pistes de la machine, T1 vous restant, le tempo). Le nom du riddim s'affiche comme
  valeur de la ligne.
- **MT-40X** : OFF / ON. ON : **les touches sont le clavier de basse du MT-40** (elles ne
  sonnent plus comme un synthé). OFF : le FM-1 redevient un FM-1.

Une fois un riddim chargé :

| Piste | Contenu | Réglages posés au chargement |
| --- | --- | --- |
| T1 | **libre** — le riddim n'a pas de mélodie (v0.5), jouez-y la vôtre | rien (votre son, vos réglages) |
| T2 | le skank : l'accord de la mesure (racine, tierce, quinte, + septième) | GATE 40, VOICE POLY |
| T3 | la basse : le groove au-dessus de la racine de mesure, re-frappé comme la machine | GATE 127, VOICE MONO |
| T4 | la batterie (un hit par pas au plus, vélocités d'origine) | LEN 64, DIV 1/16 |

Les gestes, l'esprit MT-40 :

- **Rien presser : le riddim joue comme écrit** — ses quatre racines de mesure à lui
  (ONE DROP A-A-D-E, HEAVENLESS A-F-A-G...), exactement le motif de SLOOP Dub.
- **Une touche = l'accord**. La touche pressée donne la fondamentale en MAJEUR ; la touche
  un demi-ton au-dessus donne la MINEUR de la fondamentale précédente ; deux demi-tons
  au-dessus la SEPTIÈME de la fondamentale précédente ; **la même touche deux fois rend la
  main au majeur de cette touche** (toutes les fondamentales restent atteignables).
  Toute la basse et les accords du skank suivent aussitôt ; la batterie ne transpose
  jamais.
- **SYNCHRO START** : transport arrêté et MT-40X armé, le premier accord lancé démarre la
  lecture. Comme sur la machine.
- La basse est réécrite en degrés au-dessus de la fondamentale, et **re-frappée comme la
  machine** (chaque frappe, sa re-frappe plus légère) ; un silence **tient** la note (TIE),
  un pas marqué `cut` dans les grilles **coupe** (les queues pincées du Sleng Teng). Le
  son de basse reste le vôtre : un patch ANALOG bien grave et bref sonne MT-40.

## Limites connues (v0.2)

- L'état MT-40X (riddim chargé, accord courant) **n'est pas sauvegardé dans le projet** :
  après extinction, rechargez (MENU > AUDIO > RIDDIM). Les pistes, elles, restent dans le
  projet si vous le sauvegardez — mais sans le nom du riddim.
- MT-40X armé, les touches ne sonnent pas (elles programment l'accompagnement) — pour jouer
  du synthé par-dessus, désarmez MT-40X ou jouez au MIDI.
- Le registre du skank reste celui de la banque (57..68) ; les accords suivent la
  fondamentale pressée en classe de hauteur, pas en registre.
- Les patterns font 4 mesures de 4/4 ; la VALSE du MT-40 (3/4) est posée sur cette grille,
  comme dans SLOOP Dub.
- La qualité MAJ / DOM7 des grooves est **dérivée mécaniquement** (voir CALIBRATION.md) :
  c'est le point à comparer avec la vraie machine.

## Recompiler et recalibrer

La source complète (GPL-3.0) est dans `mt40x_v1.8.2-0.5_source.zip`. Le workflow de
calibration — grilles imprimables `riddims_grids.md`, correction de `riddims.json`,
régénération, recompilation — est dans `CALIBRATION.md`.

## Licences et crédits

- Felucca par Leo Kuroshita / Hügelton Instruments — GPL-3.0-only.
- Banque de riddims : SLOOP Dub 1.6 par altessproduction — GPL-3.0-only
  (`LICENSES/GPL-3.0-only-SLOOP-Dub.txt`, détails dans `LICENSING.md`).
- MT-40X : contributions sous GPL-3.0-only. Vous redistribuez avec la source complète.
