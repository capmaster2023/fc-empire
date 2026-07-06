# FC Empire — Mode simulation live natif C++ / Canvas

## Ce qui a été ajouté

### Moteur C++ Android NDK
Emplacement : `android/app/src/main/cpp/`

Nouveaux fichiers :

- `match_engine.cpp / match_engine.h` : façade du moteur appelée par Java/JNI.
- `simulation.cpp / simulation.h` : boucle de match spatiale 2D, décisions IA, passes, tirs, interceptions, pressing, fatigue et score.
- `player.cpp / player.h` : modèle joueur avec attributs techniques, physiques, mentaux et état de match.
- `team.cpp / team.h` : modèle équipe, score, possession, tactique et sens d’attaque.
- `ball.cpp / ball.h` : état du ballon : possédé, passe, tir, ballon libre.
- `tactics.cpp / tactics.h` : conversion des consignes en paramètres exploitables par la simulation.
- `NativeMatchBridge.cpp` : pont JNI entre `MatchEnginePlugin.java` et le moteur C++.

Le moteur travaille en coordonnées normalisées `x: 0..100`, `y: 0..100`.

### Plugin Capacitor natif
Emplacement : `android/app/src/main/java/com/capmaster2023/footballempire/`

- `MatchEnginePlugin.java` expose les méthodes :
  - `startMatch(matchData)`
  - `tick(deltaTime)`
  - `pause()`
  - `resume()`
  - `setSpeed(speed)`
  - `updateTactics(teamId, tactics)`
  - `getMatchState()`
  - `stopMatch()`

`MainActivity.java` enregistre maintenant `MatchEnginePlugin` en plus de `DatDbPlugin`.

### Interface JavaScript / Canvas
Emplacements :

- `www/js/engine/match_engine_bridge.js`
- `www/js/ui/ui.js`
- `www/index.html`

`MatchEngine` prépare les données existantes (`players`, `teams`, `tactics`, formations), appelle le plugin natif si disponible, puis l’interface dessine l’état reçu sur Canvas HTML5 avec `requestAnimationFrame`.

Dans un navigateur, ou si le plugin natif n’est pas disponible, le jeu revient automatiquement au moteur spatial JS existant. Parce qu’un fallback, contrairement à certains humains, sert à quelque chose.

### Correction profil propriétaire
Dans le menu latéral du profil, le libellé n’est plus codé en dur sur `Entraîneur`.

Si `GAME.G.role === 'president'`, l’interface affiche maintenant :

- `Propriétaire`
- `Profil propriétaire`
- `Style de gestion`

Sinon elle garde :

- `Entraîneur`
- `Profil entraîneur`
- `Style d'entraînement`

## Format d’état renvoyé

Le moteur renvoie un JSON du type :

```json
{
  "time": 34.50,
  "score": { "home": 1, "away": 0 },
  "ball": { "x": 52.4, "y": 44.2, "ownerPlayerId": "123", "state": "owned" },
  "players": [
    { "id": "123", "team": "home", "name": "Player", "x": 50.1, "y": 45.8, "stamina": 82, "hasBall": true, "role": "ST", "action": "carry" }
  ],
  "event": { "type": "PASS", "text": "Passe de Martin vers Alvarez.", "time": 34.5, "playerId": "123" },
  "possession": { "home": 57, "away": 43 },
  "status": { "paused": false, "over": false, "speed": 1 }
}
```

## GitHub Actions

Fichier ajouté : `.github/workflows/build-apk.yml`

Le workflow installe Node.js, Java 21, Android SDK, CMake, NDK, lance `npx cap sync android`, compile `:app:assembleDebug`, puis publie l’APK debug en artifact.

## Commande Termux pour envoyer sur GitHub

Depuis le dossier du projet :

```bash
pkg update -y && pkg install -y git gh nodejs openjdk-21 unzip rsync && \
git init && git branch -M main && \
git add . && \
git commit -m "Add native C++ live match engine" && \
OWNER="$(gh api user -q .login)" && \
REPO="fc-empire-native-match" && \
gh repo create "$REPO" --private --source=. --remote=origin --push || \
git push -u origin main
```

Si le repo existe déjà :

```bash
git remote set-url origin https://github.com/TON_USER/TON_REPO.git && \
git add . && git commit -m "Add native C++ live match engine" && git push
```
