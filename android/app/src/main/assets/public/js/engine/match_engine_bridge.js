// ============ PONT JS -> PLUGIN CAPACITOR -> C++ NDK ============
// Le moteur natif calcule le match. Canvas HTML5 dessine seulement l'état reçu.
const MatchEngine = {
  plugin: null,
  state: null,
  eventLog: [],
  _lastEventKey: '',
  _native: false,
  _matchData: null,

  init() {
    const cap = window.Capacitor;
    this.plugin = cap && cap.Plugins && (cap.Plugins.MatchEngine || cap.Plugins.MatchEnginePlugin);
    this._native = !!(this.plugin && typeof this.plugin.startMatch === 'function');
    return this._native;
  },

  isNativeAvailable() {
    return this._native || this.init();
  },

  normalizePlayer(p, team, slot, coord, isAway) {
    const x = isAway ? 100 - coord[0] : coord[0];
    const y = coord[1];
    return {
      id: String(p.id),
      name: p.name || p.fullName || `Joueur ${slot + 1}`,
      team,
      role: p.mainPos || String(p.pos || 'CM').split(',')[0].trim(),
      x, y,
      stamina: U.clamp((GAME && GAME.pstate ? GAME.pstate(p.id).fit : 100) || 100, 25, 100),
      pace: p.pace || 68,
      pas: p.pas || p.pass || 68,
      pass: p.pas || p.pass || 68,
      sho: p.sho || 65,
      shooting: p.sho || 65,
      finishing: p.finishing || p.sho || 65,
      longshots: p.longshots || 60,
      dri: p.dri || 68,
      dribble: p.dri || 68,
      def: p.def || 62,
      defense: p.def || 62,
      tackling: p.tackling || p.def || 62,
      vision: p.vision || p.pas || 66,
      crossing: p.crossing || p.pas || 62,
      heading: p.heading || 62,
      composure: p.composure || 66,
      aggression: p.aggression || 62,
      gk: Math.max(p.gkRef || 0, p.gkPos || 0, p.gkDiv || 0, p.gkHan || 0, 30),
      gkRef: p.gkRef || 30,
      gkPos: p.gkPos || 30
    };
  },

  teamTactics(clubId) {
    const tac = (typeof TACTICS !== 'undefined') ? TACTICS.effectiveMods(clubId) : null;
    const styleName = tac ? tac.styleName : 'Équilibré';
    return {
      formation: tac ? tac.formation : '4-3-3',
      styleName,
      styleKey: tac && tac.liveStyleKey ? tac.liveStyleKey : 'equilibre',
      mentality: /offens/i.test(styleName) ? 'offensive' : /bloc|bas|déf/i.test(styleName) ? 'defensive' : 'balanced',
      pressing: /press/i.test(styleName) ? 'high' : /bas|bloc/i.test(styleName) ? 'low' : 'medium',
      defensiveLine: /press/i.test(styleName) ? 66 : /bas|bloc/i.test(styleName) ? 34 : 50,
      width: /aile/i.test(styleName) ? 78 : 62,
      passStyle: /direct/i.test(styleName) ? 'direct' : /possession|tiki/i.test(styleName) ? 'short' : 'mixed',
      tempo: /direct|press/i.test(styleName) ? 1.18 : /possession|tiki/i.test(styleName) ? 0.88 : 1.0
    };
  },

  buildFromEntry(entry) {
    const h = DB.clubById.get(entry.m.h);
    const a = DB.clubById.get(entry.m.a);
    const ht = this.teamTactics(h.id);
    const at = this.teamTactics(a.id);
    const hf = (TACTICS && TACTICS.FORMATIONS && TACTICS.FORMATIONS[ht.formation]) || TACTICS.FORMATIONS['4-3-3'];
    const af = (TACTICS && TACTICS.FORMATIONS && TACTICS.FORMATIONS[at.formation]) || TACTICS.FORMATIONS['4-3-3'];
    const xiH = DB.bestXI(h.id, hf.arr);
    const xiA = DB.bestXI(a.id, af.arr);
    const players = [];
    xiH.forEach((p, i) => players.push(this.normalizePlayer(p, 'home', i, hf.coords[i] || [50, 50], false)));
    xiA.forEach((p, i) => players.push(this.normalizePlayer(p, 'away', i, af.coords[i] || [50, 50], true)));
    const data = {
      competition: entry.comp || '',
      home: { id: String(h.id), name: h.name, tactics: ht },
      away: { id: String(a.id), name: a.name, tactics: at },
      players,
      tactics: { home: ht, away: at }
    };
    data._xiH = xiH;
    data._xiA = xiA;
    this._matchData = data;
    return data;
  },

  _applyState(result) {
    const state = result && (result.state || result);
    this.state = typeof state === 'string' ? JSON.parse(state) : state;
    const e = this.state && this.state.event;
    if (e && e.text) {
      const key = `${e.type}|${e.time}|${e.text}`;
      if (key !== this._lastEventKey) {
        this._lastEventKey = key;
        this.eventLog.push(e);
        if (this.eventLog.length > 90) this.eventLog.shift();
      }
    }
    return this.state;
  },

  async startMatch(matchData) {
    if (!this.isNativeAvailable()) throw new Error('Plugin MatchEngine indisponible');
    this.eventLog = [];
    this._lastEventKey = '';
    const res = await this.plugin.startMatch({ matchData });
    return this._applyState(res);
  },

  async tick(deltaTime) {
    if (!this.plugin) return this.state;
    const res = await this.plugin.tick({ deltaTime });
    return this._applyState(res);
  },

  async pause() { if (!this.plugin) return this.state; return this._applyState(await this.plugin.pause({})); },
  async resume() { if (!this.plugin) return this.state; return this._applyState(await this.plugin.resume({})); },
  async setSpeed(speed) { if (!this.plugin) return this.state; return this._applyState(await this.plugin.setSpeed({ speed })); },
  async updateTactics(teamId, tactics) { if (!this.plugin) return this.state; return this._applyState(await this.plugin.updateTactics({ teamId, tactics })); },
  async getMatchState() { if (!this.plugin) return this.state; return this._applyState(await this.plugin.getMatchState({})); },
  async stopMatch() { if (!this.plugin) return this.state; return this._applyState(await this.plugin.stopMatch({})); },

  resultFor(entry) {
    const st = this.state || {};
    const data = this._matchData || this.buildFromEntry(entry);
    const xiH = data._xiH || [];
    const xiA = data._xiA || [];
    const stats = new Map();
    [...xiH, ...xiA].forEach(p => stats.set(p.id, { rating: 6.5, goals: 0, assists: 0 }));
    const nativePlayers = new Map((st.players || []).map(p => [String(p.id), p]));
    for (const p of [...xiH, ...xiA]) {
      const np = nativePlayers.get(String(p.id));
      const row = stats.get(p.id);
      if (np) {
        row.rating = U.clamp(Number(np.rating || 6.5), 3, 10);
        row.goals = Number(np.goals || 0);
        row.assists = Number(np.assists || 0);
      }
    }
    const mapType = t => {
      t = String(t || '').toLowerCase();
      if (t === 'goal') return 'goal';
      if (t === 'save') return 'save';
      if (t === 'shot' || t === 'miss') return 'chance';
      if (t === 'interception' || t === 'tackle') return 'info';
      return 'info';
    };
    const events = (this.eventLog || []).filter(e => e && e.text).map(e => ({
      min: Math.max(1, Math.round(e.time || 0)),
      type: mapType(e.type),
      pid: e.playerId || null,
      txt: `${Math.max(1, Math.round(e.time || 0))}' ${e.text}`
    }));
    if (!events.some(e => /Coup de sifflet final/i.test(e.txt))) {
      events.push({ min: 90, type: 'info', txt: `90' Coup de sifflet final.` });
    }
    return { gh: st.score ? Number(st.score.home || 0) : 0, ga: st.score ? Number(st.score.away || 0) : 0, events, xiH, xiA, stats };
  }
};

window.MatchEngine = MatchEngine;
