// ============ BASE DE DONNÉES (données réelles FC26) ============
const DB = {
  players: [],        // objets joueurs (mutables)
  byId: new Map(),
  clubs: [],
  clubById: new Map(),
  leagues: [],
  leagueById: new Map(),
  clubsOfLeague: new Map(),
  squad: new Map(),   // clubId -> [players]
  freeAgents: [],
  academyPool: [],
  nextRegenId: 90000000,
  loaded: false,
  native: null,
  nativeMeta: null,
  nativeReady: false,

  COUNTRY_CONTINENT: {
    "Albania": "Europe",
    "Argentina": "South America",
    "Australia": "Asia",
    "Austria": "Europe",
    "Azerbaijan": "Europe",
    "Belarus": "Europe",
    "Belgium": "Europe",
    "Bolivia": "South America",
    "Bosnia and Herzegovina": "Europe",
    "Brazil": "South America",
    "Bulgaria": "Europe",
    "Canada": "North America",
    "Chile": "South America",
    "China": "Asia",
    "Colombia": "South America",
    "Croatia": "Europe",
    "Cyprus": "Europe",
    "Czechia": "Europe",
    "Denmark": "Europe",
    "Ecuador": "South America",
    "England": "Europe",
    "Estonia": "Europe",
    "Faroe Islands": "Europe",
    "Finland": "Europe",
    "France": "Europe",
    "Germany": "Europe",
    "Greece": "Europe",
    "Hungary": "Europe",
    "Iceland": "Europe",
    "India": "Asia",
    "Israel": "Europe",
    "Italy": "Europe",
    "Japan": "Asia",
    "Latvia": "Europe",
    "Luxembourg": "Europe",
    "Malta": "Europe",
    "Mexico": "North America",
    "Montenegro": "Europe",
    "Netherlands": "Europe",
    "Non classé": "Other",
    "North Macedonia": "Europe",
    "Northern Ireland": "Europe",
    "Norway": "Europe",
    "Paraguay": "South America",
    "Peru": "South America",
    "Poland": "Europe",
    "Portugal": "Europe",
    "Republic of Ireland": "Europe",
    "Romania": "Europe",
    "Russia": "Europe",
    "Saudi Arabia": "Asia",
    "Scotland": "Europe",
    "Serbia": "Europe",
    "Slovakia": "Europe",
    "Slovenia": "Europe",
    "South Africa": "Africa",
    "South Korea": "Asia",
    "Spain": "Europe",
    "Sweden": "Europe",
    "Switzerland": "Europe",
    "Turkey": "Europe",
    "Ukraine": "Europe",
    "United Arab Emirates": "Asia",
    "United States": "North America",
    "Uruguay": "South America",
    "Venezuela": "South America",
    "Wales": "Europe",
    "Burundi": "Africa",
    "Rwanda": "Africa",
    "DR Congo": "Africa",
    "Democratic Republic of Congo": "Africa",
    "Congo": "Africa",
    "Morocco": "Africa",
    "Algeria": "Africa",
    "Tunisia": "Africa",
    "Egypt": "Africa",
    "Senegal": "Africa",
    "Cameroon": "Africa",
    "Nigeria": "Africa",
    "Ghana": "Africa",
    "Ivory Coast": "Africa",
    "Côte d'Ivoire": "Africa",
    "Kenya": "Africa",
    "Tanzania": "Africa",
    "Uganda": "Africa",
    "Angola": "Africa",
    "Mali": "Africa",
    "Zambia": "Africa",
    "Zimbabwe": "Africa",
    "Qatar": "Asia",
    "Iran": "Asia",
    "Iraq": "Asia",
    "Thailand": "Asia",
    "Vietnam": "Asia",
    "Indonesia": "Asia",
    "Malaysia": "Asia",
    "New Zealand": "Oceania",
    "Fiji": "Oceania",
    "Haiti": "North America",
    "Jamaica": "North America",
    "Costa Rica": "North America",
    "Panama": "North America"
},
  CONF_CONTINENT: { UEFA: 'Europe', CAF: 'Africa', AFC: 'Asia', CONMEBOL: 'South America', CONCACAF: 'North America', OFC: 'Oceania', OTHER: 'Other' },

  continentForCountry(country, conf) {
    const c = String(country || '').trim();
    return this.COUNTRY_CONTINENT[c] || this.CONF_CONTINENT[conf] || 'Other';
  },

  worldHierarchy() {
    const out = {};
    for (const L of this.leagues || []) {
      const continent = L.continent || this.continentForCountry(L.country, L.conf);
      const country = L.country || 'Non classé';
      if (!out[continent]) out[continent] = {};
      if (!out[continent][country]) out[continent][country] = [];
      out[continent][country].push(L);
    }
    for (const continent of Object.keys(out)) {
      for (const country of Object.keys(out[continent])) {
        out[continent][country].sort((a, b) => (a.level - b.level) || ((b.avg || 0) - (a.avg || 0)) || a.name.localeCompare(b.name, 'fr'));
      }
    }
    return out;
  },

  async initNativeDatLayer() {
    const cap = window.Capacitor;
    const plugin = cap && cap.Plugins && (cap.Plugins.DatDb || cap.Plugins.DatDbPlugin);
    if (!plugin || typeof plugin.loadDatabase !== 'function') return null;
    this.native = plugin;
    try {
      const meta = await plugin.loadDatabase();
      this.nativeMeta = meta || null;
      this.nativeReady = true;
      return meta;
    } catch (e) {
      console.warn('[DatDb] couche native indisponible, fallback JSON conservé', e);
      this.nativeReady = false;
      return null;
    }
  },


  async load() {
    if (this.loaded) return;
    const nativeImport = this.initNativeDatLayer();
    const [pj, cj, lj, aj] = await Promise.all([
      fetch('data/players.json').then(r => r.json()),
      fetch('data/clubs.json').then(r => r.json()),
      fetch('data/leagues.json').then(r => r.json()),
      fetch('data/academy_pool.json').then(r => r.ok ? r.json() : []).catch(() => [])
    ]);
    // On conserve les données de jeu déjà structurées pour ne pas casser la carrière existante.
    // La couche native NDK lit les .dat et prépare le cache SQLite en parallèle pour les grosses bases.
    await nativeImport.catch(() => null);
    const F = pj.fields;
    this.players = pj.players.map(row => {
      const o = {};
      F.forEach((f, i) => o[f] = row[i]);
      o.mainPos = String(o.pos).split(',')[0].trim();
      o.group = U.posGroup(o.pos);
      o._base = [o.ovr, o.pot, o.age, o.value, o.wage, o.club, o.contract, o.pace, o.retired];
      return o;
    });
    this.clubs = cj;
    this.leagues = lj.map(L => ({ ...L, continent: this.continentForCountry(L.country, L.conf) }));
    this.academyPool = Array.isArray(aj) ? aj : [];
    this.reindex();
    this.loaded = true;
  },

  reindex() {
    this.byId = new Map(this.players.map(p => [p.id, p]));
    this.clubById = new Map(this.clubs.map(c => [c.id, c]));
    this.leagueById = new Map(this.leagues.map(l => [l.id, l]));
    this.clubsOfLeague = new Map();
    for (const c of this.clubs) {
      if (!this.clubsOfLeague.has(c.league)) this.clubsOfLeague.set(c.league, []);
      this.clubsOfLeague.get(c.league).push(c);
    }
    this.rebuildSquads();
  },

  rebuildSquads() {
    this.squad = new Map();
    this.freeAgents = [];
    for (const p of this.players) {
      if (p.retired) continue;
      if (!p.club) { this.freeAgents.push(p); continue; }
      if (!this.squad.has(p.club)) this.squad.set(p.club, []);
      this.squad.get(p.club).push(p);
    }
  },

  squadOf(clubId) { return this.squad.get(clubId) || []; },

  movePlayer(p, newClubId) {
    if (p.club && this.squad.has(p.club)) {
      const arr = this.squad.get(p.club);
      const i = arr.indexOf(p);
      if (i >= 0) arr.splice(i, 1);
    } else if (!p.club) {
      const i = this.freeAgents.indexOf(p);
      if (i >= 0) this.freeAgents.splice(i, 1);
    }
    p.club = newClubId || null;
    if (newClubId) {
      if (!this.squad.has(newClubId)) this.squad.set(newClubId, []);
      this.squad.get(newClubId).push(p);
    } else {
      this.freeAgents.push(p);
    }
  },

  // Force d'un club (basée sur le meilleur XI réel)
  clubStrength(clubId) {
    const sq = this.squadOf(clubId);
    if (sq.length === 0) return { att: 50, mid: 50, def: 50, gk: 50, ovr: 50 };
    const eff = p => {
      const st = GAME.pstate(p.id);
      let e = p.ovr * (0.85 + 0.15 * (st.fit / 100)) * (0.92 + 0.16 * (st.form / 10));
      if (st.inj > 0) e = 0;
      return e;
    };
    const gks = sq.filter(p => p.group === 'GK').sort((a, b) => eff(b) - eff(a));
    const dfs = sq.filter(p => p.group === 'DF').sort((a, b) => eff(b) - eff(a));
    const mfs = sq.filter(p => p.group === 'MF').sort((a, b) => eff(b) - eff(a));
    const ats = sq.filter(p => p.group === 'AT').sort((a, b) => eff(b) - eff(a));
    const avg = (arr, n) => {
      const s = arr.slice(0, n);
      if (!s.length) return 45;
      return s.reduce((a, p) => a + eff(p), 0) / s.length;
    };
    const gk = avg(gks, 1), def = avg(dfs, 4), mid = avg(mfs, 3), att = avg(ats, 3);
    return { gk, def, mid, att, ovr: (gk + def * 4 + mid * 3 + att * 3) / 11 };
  },

  bestXI(clubId, formation = [4, 3, 3]) {
    const sq = this.squadOf(clubId).filter(p => GAME.pstate(p.id).inj === 0);
    const sortE = arr => arr.sort((a, b) => b.ovr - a.ovr);
    const gks = sortE(sq.filter(p => p.group === 'GK'));
    const dfs = sortE(sq.filter(p => p.group === 'DF'));
    const mfs = sortE(sq.filter(p => p.group === 'MF'));
    const ats = sortE(sq.filter(p => p.group === 'AT'));
    const xi = [];
    if (gks[0]) xi.push(gks[0]);
    xi.push(...dfs.slice(0, formation[0]));
    xi.push(...mfs.slice(0, formation[1]));
    xi.push(...ats.slice(0, formation[2]));
    // compléter si effectif incomplet
    const rest = sq.filter(p => !xi.includes(p)).sort((a, b) => b.ovr - a.ovr);
    while (xi.length < 11 && rest.length) xi.push(rest.shift());
    return xi;
  }
};
