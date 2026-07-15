/*
 * Teacher Servo Calibrator — Arduino IDE (ESP32)
 *
 * Opens a Wi‑Fi hotspot + simple webpage to live-tune all teacher servos
 * and copy calibration into teacher_servo_config.h
 *
 * Board: ESP32 Dev Module (same as bread-compact-esp32)
 * Library: ESP32Servo (Library Manager → search "ESP32Servo")
 *
 * Power: servos on external 5V, common GND with ESP32. Do NOT use 3.3V.
 *
 * After flash:
 *   1. Phone/PC join Wi‑Fi: TeacherCalib  (no password)
 *   2. Open browser: http://192.168.4.1
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// -------- Wi‑Fi SoftAP (open — no password) --------
const char* AP_SSID = "TeacherCalib";

WebServer server(80);

// -------- Servo pins (match teacher_servo_config.h) --------
struct ServoCal {
  const char* name;
  int pin;
  int angle;      // live test angle
  int home_deg;
  int min_deg;
  int max_deg;
  float amp;      // speak amp (can be negative)
  float hz;
  float phase;
  bool blink;
  String note;    // your notes from the webpage
};

ServoCal cal[] = {
  // name         pin  angle home min max  amp    hz    phase    blink  note
  { "mouth",      13,  80,   80,  50, 120,  20.0, 4.5,  0.0,     false, "" },
  { "eye_l",      21,  90,   90,  40, 120,  12.0, 0.35, 0.0,     true,  "" },
  { "eye_r",      22,  90,   90,  40, 120,  12.0, 0.35, 0.4,     true,  "" },
  { "shoulder_l", 23,  90,   90,   0,  90, -45.0, 0.6,  0.0,     false, "" },
  { "elbow_l",    17,   0,    0,   0,  90,  90.0, 0.9,  1.5708,  false, "" },
  { "shoulder_r", 16, 135,  135,  90, 160,  25.0, 0.6,  3.14159, false, "" },
  { "elbow_r",    12,  90,   90,  60, 120,  18.0, 0.9,  4.71239, false, "" },
};

const int N = sizeof(cal) / sizeof(cal[0]);
Servo servos[7];

bool testSpeak = false;
unsigned long speakStartMs = 0;

int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void writeServo(int i, int deg) {
  cal[i].angle = clampi(deg, 0, 180);
  servos[i].write(cal[i].angle);
}

void goHomeAll() {
  testSpeak = false;
  for (int i = 0; i < N; i++) {
    writeServo(i, cal[i].home_deg);
  }
}

void attachAll() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  for (int i = 0; i < N; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(cal[i].pin, 500, 2500);
    writeServo(i, cal[i].home_deg);
  }
}

String exportConfig() {
  String out;
  out += "// Paste into teacher_servo_config.h kTeacherServos[]\n";
  for (int i = 0; i < N; i++) {
    out += "{ \"";
    out += cal[i].name;
    out += "\", GPIO_NUM_";
    out += String(cal[i].pin);
    out += ", /*ledc set in firmware*/ 0, LEDC_LOW_SPEED_MODE, ";
    out += String(cal[i].home_deg);
    out += ", ";
    out += String(cal[i].min_deg);
    out += ", ";
    out += String(cal[i].max_deg);
    out += ", ";
    out += String(cal[i].amp, 2);
    out += "f, ";
    out += String(cal[i].hz, 2);
    out += "f, ";
    out += String(cal[i].phase, 5);
    out += "f, ";
    out += cal[i].blink ? "true" : "false";
    out += " },";
    if (cal[i].note.length()) {
      out += "  // ";
      out += cal[i].note;
    }
    out += "\n";
  }
  return out;
}

String page() {
  String h;
  h.reserve(16000);
  h += "<!DOCTYPE html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>";
  h += "<title>Teacher Servo Calib</title><style>";
  h += "body{font-family:system-ui,sans-serif;margin:12px;background:#f4f4f4;color:#222}";
  h += "h1{font-size:1.3rem} .card{background:#fff;border-radius:10px;padding:12px;margin:10px 0;box-shadow:0 1px 4px #0002}";
  h += "label{display:block;font-size:12px;margin-top:6px;color:#555}";
  h += "input[type=range]{width:100%} input[type=number],input[type=text]{width:100%;box-sizing:border-box;padding:6px}";
  h += ".row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}";
  h += "button{padding:10px 14px;margin:4px 4px 4px 0;border:0;border-radius:8px;background:#1a73e8;color:#fff;font-weight:600}";
  h += "button.sec{background:#555} button.ok{background:#0a7} .ang{font-weight:700;color:#1a73e8}";
  h += "pre{background:#111;color:#9f9;padding:10px;overflow:auto;font-size:11px;border-radius:8px;white-space:pre-wrap}";
  h += "</style></head><body>";
  h += "<h1>Teacher Servo Calibrator</h1>";
  h += "<p>Live-tune servos, note calibration, export for <code>teacher_servo_config.h</code>.</p>";
  h += "<div class=card>";
  h += "<button onclick=\"fetch('/home').then(function(){location.reload()})\">All Home</button>";
  h += "<button class=sec onclick=\"fetch('/speak?on=1')\">Test Speak ON</button>";
  h += "<button class=sec onclick=\"fetch('/speak?on=0').then(function(){fetch('/home')})\">Test Speak OFF</button>";
  h += "<button class=ok onclick=\"location.href='/export'\">Show Export</button>";
  h += "</div>";

  for (int i = 0; i < N; i++) {
    h += "<div class=card><b>";
    h += cal[i].name;
    h += "</b> &nbsp; GPIO ";
    h += String(cal[i].pin);
    h += " &nbsp; live <span class=ang id='a";
    h += String(i);
    h += "'>";
    h += String(cal[i].angle);
    h += "</span>&deg;";

    h += "<label>Live angle<input type=range min=0 max=180 value='";
    h += String(cal[i].angle);
    h += "' oninput=\"setAng(";
    h += String(i);
    h += ",this.value)\"></label><div class=row>";

    h += "<div><label>home<input type=number id='home";
    h += String(i);
    h += "' value='";
    h += String(cal[i].home_deg);
    h += "'></label></div>";

    h += "<div><label>min<input type=number id='min";
    h += String(i);
    h += "' value='";
    h += String(cal[i].min_deg);
    h += "'></label></div>";

    h += "<div><label>max<input type=number id='max";
    h += String(i);
    h += "' value='";
    h += String(cal[i].max_deg);
    h += "'></label></div></div><div class=row>";

    h += "<div><label>amp (+/-)<input type=number step=0.1 id='amp";
    h += String(i);
    h += "' value='";
    h += String(cal[i].amp, 1);
    h += "'></label></div>";

    h += "<div><label>Hz<input type=number step=0.05 id='hz";
    h += String(i);
    h += "' value='";
    h += String(cal[i].hz, 2);
    h += "'></label></div>";

    h += "<div><label>phase<input type=number step=0.01 id='phase";
    h += String(i);
    h += "' value='";
    h += String(cal[i].phase, 5);
    h += "'></label></div></div>";

    h += "<label>Note<input type=text id='note";
    h += String(i);
    h += "' value='";
    h += cal[i].note;
    h += "' placeholder='e.g. home arm down, move toward 0'></label>";

    h += "<button onclick='save(";
    h += String(i);
    h += ")'>Save ";
    h += cal[i].name;
    h += "</button>";
    h += "<button class=sec onclick=\"goHomeOne(";
    h += String(i);
    h += ")\">Goto home</button></div>";
  }

  h += "<div class=card><b>Steps</b><ol>";
  h += "<li>Drag <b>Live angle</b> until the joint looks right</li>";
  h += "<li>Put that number into <b>home</b> (and set min/max safe limits)</li>";
  h += "<li><b>amp</b>: + moves up from home, - moves toward lower angles (90&rarr;0)</li>";
  h += "<li>Write a note &rarr; Save &rarr; Show Export &rarr; paste into firmware</li>";
  h += "</ol></div>";

  h += "<script>";
  h += "function setAng(i,v){document.getElementById('a'+i).innerText=v;fetch('/set?i='+i+'&angle='+v);}";
  h += "function goHomeOne(i){var h=document.getElementById('home'+i).value;setAng(i,h);}";
  h += "function save(i){";
  h += "var q='/save?i='+i";
  h += "+'&home='+document.getElementById('home'+i).value";
  h += "+'&min='+document.getElementById('min'+i).value";
  h += "+'&max='+document.getElementById('max'+i).value";
  h += "+'&amp='+document.getElementById('amp'+i).value";
  h += "+'&hz='+document.getElementById('hz'+i).value";
  h += "+'&phase='+document.getElementById('phase'+i).value";
  h += "+'&note='+encodeURIComponent(document.getElementById('note'+i).value);";
  h += "fetch(q).then(function(r){return r.text()}).then(function(t){alert(t)});";
  h += "}";
  h += "</script></body></html>";
  return h;
}

void handleRoot() { server.send(200, "text/html", page()); }

void handleSet() {
  int i = server.arg("i").toInt();
  int a = server.arg("angle").toInt();
  if (i >= 0 && i < N) {
    writeServo(i, a);
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "bad index");
  }
}

void handleSave() {
  int i = server.arg("i").toInt();
  if (i < 0 || i >= N) {
    server.send(400, "text/plain", "bad index");
    return;
  }
  cal[i].home_deg = server.arg("home").toInt();
  cal[i].min_deg = server.arg("min").toInt();
  cal[i].max_deg = server.arg("max").toInt();
  cal[i].amp = server.arg("amp").toFloat();
  cal[i].hz = server.arg("hz").toFloat();
  cal[i].phase = server.arg("phase").toFloat();
  cal[i].note = server.arg("note");
  writeServo(i, cal[i].home_deg);
  server.send(200, "text/plain", String(cal[i].name) + " saved. home=" + String(cal[i].home_deg));
}

void handleHome() {
  goHomeAll();
  server.send(200, "text/plain", "home");
}

void handleSpeak() {
  testSpeak = server.arg("on") == "1";
  if (testSpeak) speakStartMs = millis();
  else goHomeAll();
  server.send(200, "text/plain", testSpeak ? "speak on" : "speak off");
}

void handleExport() {
  String body = "<!DOCTYPE html><html><body style='font-family:system-ui;margin:16px'>";
  body += "<h2>Calibration export</h2><p><a href='/'>Back</a></p><pre>";
  body += exportConfig();
  body += "</pre><h3>Notes</h3><pre>";
  for (int i = 0; i < N; i++) {
    body += cal[i].name;
    body += ": home=";
    body += String(cal[i].home_deg);
    body += " min=";
    body += String(cal[i].min_deg);
    body += " max=";
    body += String(cal[i].max_deg);
    body += " amp=";
    body += String(cal[i].amp, 1);
    body += " | ";
    body += cal[i].note;
    body += "\n";
  }
  body += "</pre></body></html>";
  server.send(200, "text/html", body);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("TeacherServoCalibrator");

  attachAll();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);  // open network, no password
  Serial.print("AP: ");
  Serial.println(AP_SSID);
  Serial.print("Open http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/save", handleSave);
  server.on("/home", handleHome);
  server.on("/speak", handleSpeak);
  server.on("/export", handleExport);
  server.begin();
}

void loop() {
  server.handleClient();

  if (testSpeak) {
    float t = (millis() - speakStartMs) / 1000.0f;
    for (int i = 0; i < N; i++) {
      if (cal[i].amp == 0) continue;
      float wave = sin(2.0f * PI * cal[i].hz * t + cal[i].phase);
      float lift = 0.5f + 0.5f * wave;  // 0..1 from home
      int angle = (int)(cal[i].home_deg + cal[i].amp * lift);
      angle = clampi(angle, cal[i].min_deg, cal[i].max_deg);
      writeServo(i, angle);
    }
  }
}
