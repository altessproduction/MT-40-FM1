# MT-40X — calibrer la banque sur la vraie machine

La banque v0.2 vient de SLOOP Dub 1.6, dont les riddims sont « reconstruits à la main
d'après le feel des disques ». C'est excellent — et votre MT-40 reste l'étalon. Le workflow
est le même que celui de la v0.1 : comparer, corriger, régénérer, recompiler.

## Le principe

1. **Imprimez** `riddims_grids.md` : chaque riddim y est une grille de 64 pas, trois pistes
   (batterie, basse, skank — la v0.5 a retiré la mélodie, la machine n'en a pas).
2. **Jouez le même riddim** sur le MT-40 (preset correspondant) et sur le FM-1
   (MENU > AUDIO > RIDDIM), même tempo, et comparez pas à pas.
3. **Corrigez** `riddims.json` (un éditeur de texte suffit).
4. **Régénérez** : `python3 tools/gen_mt40_riddims.py` — le fichier
   `firmware/src/mt40_riddims.h` et les grilles suivent le JSON.
5. **Recompilez** : `JIELI_TOOLCHAIN=... AC79_SDK=... ./build.sh` (voir BUILDING.md).
6. Vérifiez sur l'appareil ; commitez le `riddims.json` corrigé.

## Lire les grilles

Un caractère = une double-croche ; `|` ferme le temps, `+` ferme la mesure (4 mesures de 16).

```
DRUM K...|S...|K...|S...+K...|S...|K...|S...+     (le glyphe = la lane)
BASS 1..1|1...|5..5|....+1...|....|b7..|...x+     (l'intervalle au-dessus de la racine)
SKNK ..x.|....|..x.|....+                              (x = le pas de l'accord)
```

- DRUM : **K** kick, **S** caisse claire, **P** clap, **H** charley fermé, **O** charley
  ouvert, **T** tom, **R** rim, **C** cowbell ; `.` = rien. Une seule lane par pas (c'est la
  forme de la banque SLOOP Dub).
- BASS : l'intervalle au-dessus de la racine de mesure — `1` la fondamentale, `b3` la
  tierce mineure, `4`, `5`, `b7`, `6`, `8` l'octave... La v0.5 écrit l'articulation de la
  machine (`tools/mt40_bass.py`) : la même intervalle qui revient sur les pas suivants est
  la note **re-frappée** (plus légère), `.` = un silence qui **tient** la dernière frappe
  (TIE), `x` = le pas où la note tenue **s'arrête** (MT40_CUT).
- SKNK : `x` où l'accord frappe ; sa fondamentale est celle de la mesure (le JSON la donne :
  champ `chords`).

## Ce qu'il faut vérifier en priorité

1. **Les variantes MAJ et DOM7** : MINEUR = le groove tel qu'écrit ; MAJEUR = `b3` monte à
   `3` ; SEPTIÈME = comme le majeur et la dernière `5` de chaque mesure descend à `b7`.
   Ce sont des règles dérivées, pas des relevés : jouez le même accord sur le MT-40 et
   corrigez le groove directement dans le JSON (champ `bass`) si la machine fait autrement.
2. **Les six rythmes de la machine** (Rock/SLENG TENG, POPS, SLOWRK, SWING, WALTZ, SAMBA) :
   le MT-40 joue une mesure par accord ; la banque pose quatre mesures. Comparez mesure par
   mesure, en pressant la même fondamentale.
3. **Le Sleng Teng** : la transcription de référence (Noel Davey / Ableton, 82 bpm) est déjà
   dans la banque — vérifiez-la à l'oreille contre votre machine, c'est la plus jouée.
4. **Les vélocités** : la banque SLOOP Dub en donne 44..124 ; le moteur DRUM de Felucca les
   respecte. Si la machine accentue autrement, ajustez le champ `vel` des hits.

## Le format de riddims.json

```json
{
  "name": "SLENG TENG",
  "bpm": 82,
  "roots": [37, 37, 37, 37],     // la racine de basse de chaque mesure (MIDI)
  "chords": [37, 35, 33, 35],    // la fondamentale de l'accord de skank par mesure
  "skank_qual": 1,               // 0 MAJ, 1 MIN, 2 DOM7 — tel qu'écrit
  "drums": [[step, lane, vel], ...],   // lane : 0 K, 1 S, 2 P, 3 H, 4 O, 5 T, 6 R, 7 C
  "bass":  [[step, degré, vel], ...],  // degré en demi-tons au-dessus de la racine ;
                                       // 253 = la note tenue s'arrête là ; la v0.5
                                       // re-frappe chaque tenue (l'articulation de
                                       // la machine, tools/mt40_bass.py)
  "skank": [[step, vel], ...]          // la fondamentale de l'accord est celle de la mesure
}
```

Une règle : un pas de basse qui n'est ni écrit ni `253` tient (TIE) ; deux écrits au même
pas sont refusés (le générateur valide le JSON). Après correction, `python3
tools/gen_mt40_riddims.py` sans argument régénère `.h` et grilles à partir du JSON ; avec
`--bootstrap`, il re-extrait la banque d'origine de SLOOP Dub (pour une mise à niveau).

## Le tempo

Le BPM de chaque riddim (champ `bpm`) entre au chargement. Les valeurs viennent de la
documentation de SLOOP Dub (ONE DROP 72 ... JUNGLE 172) ; le MT-40, lui, garde ses tempos
internes — si votre machine joue sensiblement autre, ajustez le champ et notez-le dans le
commit.
