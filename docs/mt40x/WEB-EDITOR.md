# MT-40X — l'éditeur web et le protocole

Le firmware reste celui de Felucca 1.8.2 côté protocole : l'éditeur web de Felucca (et son
MENU_JSON) fonctionne tel quel. MT-40X y ajoute **deux réglages**, à la fin de la liste des
MENU settings (append-only, comme chez Felucca).

## Les deux réglages MT-40X

| id | Nom | Genre | Valeurs | Onglet | Effet d'un SET |
| --- | --- | --- | --- | --- | --- |
| 33 | MT-40X | enum | 0 OFF, 1 ON | AUDIO | arme / désarme le clavier de basse du MT-40 (état d'exécution, rien à sauver en flash) |
| 34 | RIDDIM | enum | 0..27 : les 28 riddims | AUDIO | charge le riddim dans les quatre pistes et pose son tempo (rc 0 : rien en flash) |

- `MENU_DESC` (72) sur l'index 33 ou 34 répond comme les autres : index, id, genre (enum),
  valeur, min 0, max 1 ou 27, le nom (MT-40X / RIDDIM), la liste des valeurs (« OFF,ON » /
  les 28 noms de riddims, jusqu'à 15 caractères — « PROMISED LAND » tient), puis l'onglet
  (2 AUDIO).
- `MENU_SET` (73) avec l'id et la valeur v14 applique et répond rc + id + valeur relue.
  Pour MT-40X et RIDDIM le rc est celui d'une sauvegarde de réglages sans changement (0) :
  rien n'est écrit en flash, l'état est celui de la session.
- Un index au-delà de la liste répond toujours index + 127 (aucun réglage), comme avant.

Extrait de `menu.json` (MENU_JSON=... build/host/editor_test) :

```json
{"id":33,"kind":0,"min":0,"max":1,"value":0,"name":"MT-40X","names":["OFF","ON"],
 "unit":"","tab":2,"tabName":"AUDIO"},
{"id":34,"kind":0,"min":0,"max":27,"value":16,"name":"RIDDIM","names":["ONE DROP",
 "STEPPERS","ROCKERS","DUBWALK","MEDITATE","DUB CUT","ZION","SKA","HEAVENLESS","TONIGHT",
 "GET IN GROOVE","HOT MILK","ANSWER","PROMISED LAND","GUNMAN","HERE I COME","SLENG TENG",
 "TEMPO","PEANY PEANY","DANCEHALL","DEMBOW","JUNGLE","DUBSTEP","MT40 POPS","MT40 SLOWRK",
 "MT40 SWING","MT40 WALTZ","MT40 SAMBA"],"unit":"","tab":2,"tabName":"AUDIO"}
```

## Les 22 timbres du Casiotone, en recettes Felucca

Le MT-40/41 porte 22 timbres. Ils ne sont pas dans le firmware (ce sont les presets du
SLOOP Dub de base, pas de Felucca 1.8.2) ; voici la table de correspondance pour les
recréer à la main sur le FM-1 — moteur, réglages de départ, caractère à viser. Nombres de
l'interface Felucca (KNOB 1..4 de chaque moteur, valeurs 0..127).

| # | Timbre du MT-40 | Moteur | Point de départ | Caractère |
| --- | --- | --- | --- | --- |
| 1 | ORGAN 1 | DRAWBAR-like (ANALOG, round) | sustenu, attaque 0, sustain plein | orgue doux |
| 2 | ORGAN 2 | ANALOG | comme 1, filtre plus ouvert | orgue plus nasal |
| 3 | PIPE ORGAN | ANALOG + quinte ajoutée (CHRD) | attaque courte | tuyau |
| 4 | ROCK ORGAN | ANALOG | attaque 0, léger DIST insert | drive |
| 5 | COMB ORGAN | ANALOG | accord désaccordé léger (DETUNE) | ragtime |
| 6 | ACCORDION | ANALOG | attaque courte, sustain plein, vibrato lent | musette |
| 7 | HARMONICA | ANALOG | attaque 30, filtre moyen | souffle |
| 8 | SYNTH-LEAD | FM6 | opérateurs 1+2, feedback léger | lead |
| 9 | SYNTH-REED | FM6 | ratio 2:1, index moyen | anche synthé |
| 10 | SYNTH-BRASS 1 | ANALOG | attaque 40, filtre ouvert, saw | cuivre |
| 11 | SYNTH-BRASS 2 | ANALOG | comme 10, plus brillant | cuivre brillant |
| 12 | SYNTH-BASS | ANALOG | grave, DEC court, filtre fermé | la basse du MT-40X |
| 13 | PIANO 1 | FM6 | ratio 1:1, décroissance moyenne | piano droit |
| 14 | PIANO 2 | FM6 | comme 13, index plus fort | piano brillant |
| 15 | ELECTRIC PIANO 1 | FM6 | ratio 1:4, index doux | Rhodes |
| 16 | ELECTRIC PIANO 2 | FM6 | comme 15, attaque plus mordante | Wurlitzer |
| 17 | ELECTRIC GUITAR | FM6 | ratio 1:2, sustain court | guitare sèche |
| 18 | JAZZ GUITAR | FM6 | comme 17, plus rond | archtop |
| 19 | MANDOLIN | FM6 | très bref, trémolo (LFO AMP rapide) | pincé |
| 20 | CELESTA | FM6 | bref, haut, cloche | célesta |
| 21 | VIBES | FM6 | moyen, vibrato lent | vibraphone |
| 22 | MARIMBA | FM6 | bref, boisé | marimba |

(La table est un guide d'écoute, pas un relevé d'octets : c'est la même démarche que la
calibration — ajustez à l'oreille et gardez vos presets utilisateur.)

## Rappel : ce que l'éditeur ne voit pas

L'état MT-40X (riddim chargé, accord pressé) est d'exécution : il n'apparaît ni dans le
projet, ni dans les réglages. Après un chargement de projet, rechargez le riddim voulu
(MENU > AUDIO > RIDDIM, ou un SET du réglage 34).
