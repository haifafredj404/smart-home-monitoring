const MAX = 120;                       // nombre de mesures gardées
const COOLDOWN = 10 * 60 * 1000;       // anti-spam des alertes : 10 min
const T_HIGH = 30, T_LOW = 10;         // seuils température (°C)
const H_HIGH = 80, H_LOW = 25;         // seuils humidité (%)

let p = msg.payload;
if (typeof p === 'string') {
    try { p = JSON.parse(p); } catch (e) { node.warn('JSON invalide'); return null; }
}
const temp = parseFloat(p.temperature);
const hum  = parseFloat(p.humidity);
if (isNaN(temp) || isNaN(hum)) { node.warn('Valeurs invalides'); return null; }
const motion = String(p.motion).toUpperCase() !== 'NONE';
const now = Date.now();

// Historique glissant
let h = flow.get('history') || [];
h.push({ t: now, temp: temp, hum: hum });
if (h.length > MAX) h = h.slice(-MAX);
flow.set('history', h);

// Statistiques
const temps = h.map(x => x.temp);
const stats = {
    min: Math.min(...temps),
    max: Math.max(...temps),
    avg: temps.reduce((a, b) => a + b, 0) / temps.length
};

// Journal des mouvements (uniquement quand ça passe de "rien" à "détecté")
let events = flow.get('events') || [];
if (motion && !flow.get('lastMotion')) {
    events.unshift({ t: now, text: 'Mouvement détecté' });
    events = events.slice(0, 5);
    flow.set('events', events);
}
flow.set('lastMotion', motion);

// Alertes
const alerts = [];
if (temp > T_HIGH) alerts.push({ level: 'danger', text: 'Température élevée : ' + temp.toFixed(1) + ' °C' });
if (temp < T_LOW)  alerts.push({ level: 'warn',   text: 'Température basse : ' + temp.toFixed(1) + ' °C' });
if (hum > H_HIGH)  alerts.push({ level: 'warn',   text: 'Humidité élevée : ' + hum.toFixed(1) + ' %' });
if (hum < H_LOW)   alerts.push({ level: 'warn',   text: 'Humidité basse : ' + hum.toFixed(1) + ' %' });

// Notification externe (sortie 2) avec cooldown
let notif = null;
const key = alerts.map(a => a.text.split(':')[0]).join('|');
const last = flow.get('lastAlert') || { key: '', t: 0 };
if (alerts.length && (key !== last.key || now - last.t > COOLDOWN)) {
    notif = { payload: '⚠️ Smart Home : ' + alerts.map(a => a.text).join(' / ') };
    flow.set('lastAlert', { key: key, t: now });
}
if (!alerts.length) flow.set('lastAlert', { key: '', t: 0 });

msg.payload = {
    temperature: temp,
    humidity: hum,
    motion: motion,
    history: h,
    stats: stats,
    events: events,
    alerts: alerts
};
return [msg, notif];