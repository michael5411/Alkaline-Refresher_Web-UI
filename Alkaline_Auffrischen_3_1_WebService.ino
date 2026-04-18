//Auffrischgerät für Alkaline-Batterien
//Michael Klein 1-2026 - Betriebsanleitung bitte beachten.
//Erweiterung: WiFi-Webserver mit mDNS (alkafresh_1.local)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// *** WiFi-Zugangsdaten hier eintragen ***
const char* ssid     = "DemandFlow";
const char* password = "8SWRXBzkW0mukLfN4ul6Nl0Cues46Ie";

// mDNS-Hostname: erreichbar als http://alkafresh_1.local
const char* mdnsName = "alkafresh_1";

WebServer server(80);

float battU[4]    = {0.0, 0.0, 0.0, 0.0}; // Batteriespannungen 1–4
int   battrawU[4] = {0, 0, 0, 0};          // Rohwerte der Batteriespannungen
int   battload[4] = {10, 10, 10, 10};      // Zähler für Ulimit-Erreichen
float Ulimit      = 1.6;                   // Zielspannung [V]

int pwrPins[]  = {21, 22, 25, 27};         // Pins für Batterieladeimpulse
int messPins[] = {36, 34, 39, 35};         // ADC-Pins für Spannungsmessung

// ---------------------------------------------------------------------------
// Timeout-Konfiguration
// ---------------------------------------------------------------------------
int           timeoutMinutes   = 120;                    // Vorwählbare Laufzeit [min], Default 120
unsigned long chargeStartMs[4] = {0, 0, 0, 0};          // Zeitstempel Ladestart je Kanal
bool          timeoutFlag[4]   = {false, false, false, false}; // true = durch Timeout beendet

// ---------------------------------------------------------------------------
// Post-Charge-Monitoring (60 Minuten nach Prozessende)
// ---------------------------------------------------------------------------
#define MONITOR_DAUER_MS  (60UL * 60UL * 1000UL)  // 60 Minuten in Millisekunden

bool          monitorAktiv[4]   = {false, false, false, false}; // Monitoring läuft
float         monitorStartU[4]  = {0.0, 0.0, 0.0, 0.0};        // Spannung bei Prozessende
unsigned long monitorStartMs[4] = {0, 0, 0, 0};                 // Zeitstempel Prozessende

// ---------------------------------------------------------------------------
// Prozessende (regulär oder Timeout) für Kanal idx abschließen
// ---------------------------------------------------------------------------
void beendeAuffrischung(int idx, bool byTimeout) {
  battload[idx]      = 0;
  timeoutFlag[idx]   = byTimeout;
  monitorAktiv[idx]  = true;
  monitorStartU[idx] = battU[idx];   // Spannung bei Prozessende festhalten
  monitorStartMs[idx]= millis();
}

// ---------------------------------------------------------------------------
// Hilfsfunktion: Float als "X.YY"-String formatieren
// ---------------------------------------------------------------------------
String fmtVolt(float v) {
  bool neg = (v < 0);
  float av = neg ? -v : v;
  int whole = (int)av;
  int frac  = (int)round((av - whole) * 100.0);
  if (frac == 100) { whole++; frac = 0; }
  String s = neg ? "-" : "";
  s += String(whole) + ".";
  if (frac < 10) s += "0";
  s += String(frac);
  return s;
}

// ---------------------------------------------------------------------------
// Statustext und CSS-Klasse für eine Batterie
// ---------------------------------------------------------------------------
String battStatus(int i) {
  if (battU[i] < 0.1)     return "leer / nicht eingelegt";
  if (timeoutFlag[i])      return "time out";  // Timeout-Text (orange via CSS)
  if (battload[i] <= 0)    return "aufgefrischt &#10003;";
  if (battU[i] >= Ulimit)  return "wird &#252;berpr&#252;ft &#8230;";
  return "wird aufgefrischt &#8230;";
}

String battClass(int i) {
  if (battU[i] < 0.2)      return "leer";
  if (timeoutFlag[i])       return "tout";   // orange
  if (battload[i] <= 0)     return "ok";
  return "lad";
}

// ---------------------------------------------------------------------------
// HTML-Block: Post-Charge-Monitoring-Tabelle
// Gibt leeren String zurück, wenn gerade keine Batterie überwacht wird.
// ---------------------------------------------------------------------------
String monitorHtml() {
  bool anyActive = false;
  for (int i = 0; i < 4; i++) if (monitorAktiv[i]) { anyActive = true; break; }
  if (!anyActive) return "";

  String h = "<div class='monbox'>";
  h += "<h2>Spannungen nach Prozessende</h2>";
  h += "<table>";
  h += "<tr><th></th><th>Ende&nbsp;[V]</th><th>Aktuell&nbsp;[V]</th>"
       "<th>Differenz</th><th>Zeit</th></tr>";

  for (int i = 0; i < 4; i++) {
    if (!monitorAktiv[i]) continue;
    unsigned long elapsedMin = (millis() - monitorStartMs[i]) / 60000UL;
    float diff = battU[i] - monitorStartU[i];   // negativ = Spannungsabfall

    h += "<tr>";
    h += "<td><b>Nr.&nbsp;" + String(i + 1) + "</b></td>";
    h += "<td class='volt'>" + fmtVolt(monitorStartU[i]) + "</td>";
    h += "<td class='volt'>" + fmtVolt(battU[i])         + "</td>";
    h += "<td class='diff'>";
    if (diff >= 0) h += "+";
    h += fmtVolt(diff) + "</td>";
    h += "<td class='zeit'>" + String(elapsedMin) + "&prime;</td>";
    h += "</tr>";
  }

  h += "</table>";
  h += "<p class='hint'>"
       "<b>Ende&nbsp;[V]</b> Spannung bei Auffrischungsende &nbsp;&bull;&nbsp;"
       "<b>Aktuell&nbsp;[V]</b> Ruhespannung jetzt &nbsp;&bull;&nbsp;"
       "<b>Differenz</b> Spannungsabfall seit Prozessende &nbsp;&bull;&nbsp;"
       "<b>Zeit</b> Minuten seit Prozessende (Monitoring 60&prime;)"
       "</p>";
  h += "</div>";
  return h;
}

// ---------------------------------------------------------------------------
// HTML-Seite für den Webserver
// ---------------------------------------------------------------------------
void handleRoot() {
  messungen();

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="10">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>AlkaFresh</title>
  <style>
    body   { font-family: Arial, sans-serif; background:#1e1e2e; color:#cdd6f4;
             display:flex; flex-direction:column; align-items:center; padding:20px; }
    h1     { color:#89b4fa; margin-bottom:4px; }
    h2     { color:#89dceb; font-size:1em; margin-bottom:8px; }
    p.sub  { color:#6c7086; font-size:0.85em; margin-top:0; }
    table  { border-collapse:collapse; width:420px; margin-top:20px; }
    th     { background:#313244; color:#89dceb; padding:10px 16px; text-align:center; }
    td     { padding:10px 16px; text-align:center; border-bottom:1px solid #313244; }
    tr:last-child td { border-bottom:none; }
    .ok    { color:#a6e3a1; font-weight:bold; }
    .lad   { color:#f9e2af; }
    .leer  { color:#f38ba8; }
    .tout  { color:#FF8C00; font-weight:bold; }
    .volt  { font-size:1.2em; font-weight:bold; color:#cba6f7; }
    /* Monitoring-Box */
    .monbox{ margin-top:30px; width:500px; background:#181825;
             border:1px solid #313244; border-radius:8px; padding:16px 20px; }
    .monbox table { width:100%; margin-top:0; }
    .diff  { color:#f38ba8; font-weight:bold; }
    .zeit  { color:#94e2d5; }
    .hint  { font-size:0.75em; color:#6c7086; margin-top:10px; line-height:1.5em; }
    footer { margin-top:30px; font-size:0.75em; color:#6c7086; }
  </style>
</head>
<body>
  <h1>&#9889; AlkaFresh</h1>
  <p class="sub">Seite aktualisiert sich automatisch alle 10 Sekunden</p>
  <table>
    <tr><th>Batterie</th><th>Spannung (V)</th><th>Status</th></tr>
)rawliteral";

  for (int i = 0; i < 4; i++) {
    html += "<tr><td><b>Nr. " + String(i + 1) + "</b></td>";
    html += "<td class='volt'>" + fmtVolt(battU[i]) + "</td>";
    html += "<td class='" + battClass(i) + "'>" + battStatus(i) + "</td></tr>\n";
  }

  html += "  </table>\n";

  // Monitoring-Tabelle einfügen (erscheint nur wenn aktiv)
  html += monitorHtml();

  html += R"rawliteral(
  <footer>Michael Klein &bull; AlkaFresh &bull; alkafresh_1.local</footer>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(pwrPins[i], OUTPUT);
    digitalWrite(pwrPins[i], HIGH); // Ladestrom aus
    chargeStartMs[i] = millis();    // Startzeit vorbelegen
  }

  Serial.print("Verbinde mit ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(mdnsName)) {
    Serial.println("mDNS gestartet: http://alkafresh_1.local");
  } else {
    Serial.println("mDNS-Fehler – Gerät nur über IP erreichbar.");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Webserver gestartet.");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {

  server.handleClient();

  messungen();

  unsigned long now = millis();

  for (int j = 0; j < 4; j++) {

    // ── Batterie neu eingelegt oder entnommen? ───────────────────────────────
    if (battU[j] < 0.1) {
      if (battload[j] != 10) {          // Zustand zurücksetzen nur wenn nötig
        battload[j]      = 10;
        timeoutFlag[j]   = false;
        monitorAktiv[j]  = false;
        chargeStartMs[j] = now;
      }
      continue;                          // Kein Ladeimpuls für leere Steckplätze
    }

    // ── Timeout prüfen (nur wenn noch aktiv geladen wird) ───────────────────
    if (battload[j] > 0 && !timeoutFlag[j]) {
      if ((now - chargeStartMs[j]) >= (unsigned long)timeoutMinutes * 60000UL) {
        beendeAuffrischung(j, true);     // Timeout-Ende
        Serial.print("Timeout Batterie "); Serial.println(j + 1);
      }
    }

    // ── Ladeimpuls senden (wenn noch nicht fertig und kein Timeout) ─────────
    if (battload[j] > 0 && battU[j] < Ulimit && battU[j] > 0.2) {
      digitalWrite(pwrPins[j], LOW);    // Ladestrom ein
      delay(250);
    }
    digitalWrite(pwrPins[j], HIGH);     // Ladestrom aus

    // ── Ulimit erreicht? ─────────────────────────────────────────────────────
    if (battU[j] >= Ulimit && battload[j] > 0) {
      battload[j]--;
    }

    Serial.printf("Batt %d: battload=%d timeout=%d monitorAktiv=%d\n",
    j+1, battload[j], timeoutFlag[j], monitorAktiv[j]);

    if (battload[j] == 0 && !timeoutFlag[j] && !monitorAktiv[j]) {
      Serial.printf(">>> beendeAuffrischung wird aufgerufen fuer Batt %d\n", j+1);
      beendeAuffrischung(j, false);
    }

    // ── Reguläres Ende: battload auf 0 gezählt ───────────────────────────────
    // Bewusst GETRENNT von der Ulimit-Prüfung, damit beendeAuffrischung()
    // auch dann aufgerufen wird, wenn die Spannung beim nächsten Messzyklus
    // bereits wieder unter Ulimit gefallen ist
  } // Ende for-Schleife

  // ── Post-Charge-Monitoring: nach 60 Min deaktivieren ────────────────────
  unsigned long nowMonitor = millis();   // frischer Zeitstempel!
  for (int j = 0; j < 4; j++) {
    if (monitorAktiv[j] && (nowMonitor - monitorStartMs[j]) >=MONITOR_DAUER_MS) {
      monitorAktiv[j] = false;
  }
}

  if (battU[0] >= Ulimit || battU[1] >= Ulimit ||
      battU[2] >= Ulimit || battU[3] >= Ulimit) {
    printTable();
  }

  delay(1000);
}
