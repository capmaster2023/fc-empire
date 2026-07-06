# FC Empire — intégration native `.dat` Android NDK

Ce build conserve l’application Capacitor existante : `www/index.html`, `www/css`, `www/js`, design violet, sidebar, cartes, boutons arrondis et écrans carrière.

## Ce qui a été ajouté

- Projet Android complet dans `android/`
- Plugin Capacitor natif `DatDbPlugin`
- Couche C++ Android NDK dans `android/app/src/main/cpp/`
- Lecture des assets via `AAssetManager`
- Cache SQLite natif `fc_empire_native_dat.db`
- Fichiers `.dat` et `.lng` placés dans `android/app/src/main/assets/db/`
- Workflow GitHub Actions avec NDK `26.1.10909125` et CMake `3.22.1`

## API JavaScript exposée par le plugin

Le plugin Capacitor s’appelle `DatDb` et expose :

- `loadDatabase()`
- `getContinents()`
- `getCountriesByContinent({ continentId })`
- `getCompetitionsByCountry({ countryId })`
- `getClubsByCompetition({ competitionId })`
- `getPlayersByClub({ clubId, offset, limit })`
- `searchPlayers({ query, offset, limit })`
- `getPlayer({ playerId })`
- `getClub({ clubId })`
- `getCompetition({ competitionId })`
- `getNation({ nationId })`
- `inspectFile({ file, maxStrings })` pour debugger les structures `.dat`

## Important sur les `.dat`

Le parseur est volontairement tolérant. Il lit les fichiers, extrait les chaînes structurées, remplit les tables faciles (`continents`, `nations`, `competitions`, `clubs`) et journalise les parties non comprises dans `debug_dat_files` / `debug_strings`.

Les relations binaires exactes de certains fichiers Football Manager / FC database (`players.dat`, `people.dat`, contrats, historique) ne sont pas inventées. Quand un offset n’est pas confirmé, le parseur n’écrase pas les données avec n’importe quoi. Il garde les données inspectables, ce qui est moins spectaculaire qu’un mensonge, mais étrangement plus utile.

## Interface corrigée

- Les boutons sont forcés en `type="button"`
- Protection contre `href="#"`
- `preventDefault()` et `stopPropagation()` ajoutés sur les boutons
- Retour par historique interne
- Conservation du scroll sur les écrans rechargés
- Écran environnement du jeu : continent → pays → championnat → club
- Propriétaire normal / riche avec injections jusqu’à 2 Md€
- Centre de formation séparé : finance, scouting, développement

## Build GitHub Actions

Le workflow principal est :

`.github/workflows/build-apk.yml`

Il installe Node, Java 21, Android SDK, NDK, CMake, synchronise Capacitor, compile `assembleDebug` et publie l’APK en artifact.
