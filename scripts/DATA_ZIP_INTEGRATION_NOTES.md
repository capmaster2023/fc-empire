# Intégration db_archive_2603 — reconstruction mondiale v2

Reconstruction complète du monde à partir du décodage binaire réel des .dat
(competition.dat, club.dat, nation.dat), au lieu des ligues synthétiques 102xxx.

## Résultat
- Championnats: 503 (51 FC d'origine + 452 issus de la base décodée)
- Clubs: 6 641 (vrais noms de clubs partout)
- Joueurs: 149 943 (18 405 vrais joueurs FC conservés + effectifs générés calibrés)
- Pays couverts: 84

## Décodage du format .dat
- Header `03 01 74 61 64 2e`, records = u32 id + strings préfixées u32 terminées 0xff.
- competition.dat: type, nation_idx, réputation, level (100 = coupe). L'index d'ordre
  de fichier sert de clé de division.
- club.dat: division = u16 à l'offset 158 du blob (validé sur PL/Championship 2025-26).
- nation.dat: continent u16 en tête de blob (0=CAF,1=AFC,2=UEFA,3=CONCACAF,4=OFC,5=CONMEBOL).

## Règles de construction
- Les 51 ligues FC d'origine, leurs 662 clubs et 18 316 joueurs réels sont intacts.
- Contenu synthétique 102xxx entièrement supprimé.
- Ligues db en doublon d'une originale (même pays + niveau): utilisées pour compléter
  l'originale (77 clubs ajoutés, ex. Super League grecque 4 → 14 clubs).
- Ligue 65 « Premier Division » recatégorisée Republic of Ireland (clubs irlandais).
- Ligues féminines exclues (WSL, Frauen-Bundesliga, Elitettan, WE League, etc.).
- Ligues > 24 clubs découpées en groupes « Grp. N » (limite du calendrier round-robin).
- OVR moyen des nouvelles ligues calibré par régression réputation→avg (39 paires),
  −0,6 par niveau, clamp 46–76.
- Effectifs 22 joueurs/club (plan GK3 CB4 LB RB CDM2 CM3 CAM LM RM LW RW ST3),
  noms tirés des pools réels par nationalité (72 % locaux, fallback pays voisins).
- Chaînes montée/descente recalculées par pays (ex. Angleterre lvl 1→5 chaînée).
- Ligues CONMEBOL + UAE absentes de la db complétées avec les vrais clubs 2025-26
  (Colombie 20, Uruguay 16, Équateur 16, Bolivie 16, Venezuela 14, Paraguay 12, UAE 14).

## Ids
- Nouvelles ligues: 200001+. Nouveaux clubs: 1 000 000 + id db. Clubs de complément
  CONMEBOL/UAE: max(id)+1000. Joueurs générés: ≥ 20 000 000.

## Compatibilité
- Anciennes sauvegardes IndexedDB incompatibles: commencer une nouvelle carrière.
- db.js inchangé: le fallback conf→continent couvre les nouveaux pays.
- Script de génération: /home/claude/build_world.py (session Claude), source
  db_archive_2603.zip + données FC d'origine.

## v3 — Patch Afrique (fc_empire_africa_leagues_patch.zip)
- 5 ligues ajoutées avec vrais clubs 2025-2026 : Premier League égyptienne (21),
  Botola Pro (16), Ligue 1 algérienne (16), Ligue 1 tunisienne (16),
  Primus Ligue Burundi (16). La Betway Premiership du patch est ignorée
  (Afrique du Sud déjà couverte par la db à 2 divisions + SAFA).
- Force club proportionnelle à la réputation du patch (x0,6 autour de la moyenne) :
  Al Ahly/Zamalek ~68, Vital'O/Aigle Noir en tête au Burundi.
- Pools de noms authentiques injectés (kirundi pour le Burundi, arabes égyptiens),
  étrangers de ligue tirés d'un vivier CAF élargi (14 nationalités).
- Ids ligues 210001-210005, clubs à la suite du padding.
- Totaux v3 : 508 ligues, 6 726 clubs, 151 813 joueurs, 89 pays.
