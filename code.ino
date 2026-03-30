#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ------------------- CONFIG -------------------
const char *ssid = "";
const char *password = "";

#define ECG_PIN 35
#define EMG_PIN 34
#define LO_PLUS 32
#define LO_MINUS 33
#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ------------------- SHARED DATA (THREAD-SAFE) -------------------
SemaphoreHandle_t dataMutex;
struct Biometrics
{
    float bpm = 0;
    float emgPercent = 0;
    float temp = 0;
    float hum = 0;
} sharedData;

// ------------------- CONSTANTS & MATH -------------------
const int SAMPLE_RATE_HZ = 250;
const int TICK_PERIOD_MS = 1000 / SAMPLE_RATE_HZ;

// ------------------- HTML -------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32 Biometric Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
  body { background:#0a0a0a; color:white; font-family:sans-serif; text-align:center; }
  .card { background:#161616; margin:15px; padding:20px; border-radius:12px; display:inline-block; width:40%; min-width:300px; }
  h2 { color:#00e676; margin-top:0; }
  .val { font-size:2.5em; font-weight:bold; }
</style></head><body>
<h1>BIOMETRIC COMMAND CENTER</h1>
<div class="card"><h2>HEART RATE</h2><div class="val"><span id="bpm">--</span> <small>BPM</small></div><canvas id="chartBPM"></canvas></div>
<div class="card"><h2>MUSCLE LOAD</h2><div class="val"><span id="emg">--</span> <small>%</small></div><canvas id="chartEMG"></canvas></div>
<div class="card"><h2>ENV</h2><div>TEMP: <span id="temp">--</span>°C | HUM: <span id="hum">--</span>%</div></div>
<script>
  var ws = new WebSocket(`ws://${window.location.hostname}:81/`);
  const cfg = (l, c) => ({ type:'line', data:{labels:[], datasets:[{label:l,data:[],borderColor:c,tension:0.3}]}, options:{animation:false,scales:{y:{beginAtZero:false}}}});
  var cBPM = new Chart(document.getElementById('chartBPM'), cfg('BPM','#ff5252'));
  var cEMG = new Chart(document.getElementById('chartEMG'), cfg('EMG','#00e676'));
  ws.onmessage = (e) => {
    var d = JSON.parse(e.data);
    if(d.bpm) { document.getElementById("bpm").innerHTML = d.bpm.toFixed(0); update(cBPM, d.bpm); }
    if(d.emg !== undefined) { document.getElementById("emg").innerHTML = d.emg.toFixed(1); update(cEMG, d.emg); }
    if(d.temp) document.getElementById("temp").innerHTML = d.temp.toFixed(1);
    if(d.hum) document.getElementById("hum").innerHTML = d.hum.toFixed(1);
  };
  function update(c, v) {
    if(c.data.labels.length > 20) { c.data.labels.shift(); c.data.datasets[0].data.shift(); }
    c.data.labels.push(""); c.data.datasets[0].data.push(v); c.update();
  }
</script></body></html>
)rawliteral";

// ------------------- SENSING TASK (CORE 1) -------------------
void Sensing_Task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // ECG Vars
    float ecgDc = 2048, ecgFil = 0, peak = 50;
    unsigned long lastBeat = 0;
    bool beatHold = false;
    float bpmBuf[5] = {0};
    int bIdx = 0;

    // EMG Vars
    const int win = 50;
    float emgSumSq = 0;
    int emgCount = 0;
    float dynBaseline = 10; // Simple auto-adjusting noise floor

    for (;;)
    {
        // 1. ECG Sampling (250Hz)
        if (!(digitalRead(LO_PLUS) || digitalRead(LO_MINUS)))
        {
            int raw = analogRead(ECG_PIN);
            ecgDc += 0.002 * (raw - ecgDc);
            float hp = raw - ecgDc;
            ecgFil += 0.1 * (hp - ecgFil);

            peak *= 0.996;
            if (ecgFil > peak)
                peak = ecgFil;

            if (ecgFil > (peak * 0.7) && !beatHold && (millis() - lastBeat > 300))
            {
                unsigned long rr = millis() - lastBeat;
                lastBeat = millis();
                beatHold = true;
                if (rr > 350 && rr < 1500)
                {
                    bpmBuf[bIdx] = 60000.0 / rr;
                    bIdx = (bIdx + 1) % 5;
                    float avg = 0;
                    for (int i = 0; i < 5; i++)
                        avg += bpmBuf[i];
                    xSemaphoreTake(dataMutex, portMAX_DELAY);
                    sharedData.bpm = avg / 5.0;
                    xSemaphoreGive(dataMutex);
                }
            }
            if (ecgFil < (peak * 0.3))
                beatHold = false;
        }

        // 2. EMG Sampling (Running RMS)
        int emgRaw = analogRead(EMG_PIN);
        float val = emgRaw - 2048.0;
        emgSumSq += (val * val);
        emgCount++;

        if (emgCount >= win)
        {
            float rms = sqrt(emgSumSq / win);
            if (rms < dynBaseline)
                dynBaseline = (dynBaseline * 0.9) + (rms * 0.1);
            float act = (rms - dynBaseline) / (600.0 - dynBaseline); // 600 is arbitrary Max

            xSemaphoreTake(dataMutex, portMAX_DELAY);
            sharedData.emgPercent = constrain(act * 100.0, 0, 100);
            xSemaphoreGive(dataMutex);

            emgSumSq = 0;
            emgCount = 0;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TICK_PERIOD_MS));
    }
}

// ------------------- TELEMETRY TASK (CORE 0) -------------------
void Telemetry_Task(void *pvParameters)
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.println(WiFi.localIP());

    server.on("/", []()
              { server.send_P(200, "text/html", index_html); });
    server.begin();
    webSocket.begin();

    unsigned long lastDHT = 0;
    for (;;)
    {
        server.handleClient();
        webSocket.loop();

        if (millis() - lastDHT >= 2500)
        {
            lastDHT = millis();
            float t = dht.readTemperature();
            float h = dht.readHumidity();
            if (!isnan(t))
            {
                xSemaphoreTake(dataMutex, portMAX_DELAY);
                sharedData.temp = t;
                sharedData.hum = h;
                xSemaphoreGive(dataMutex);
            }
        }

        StaticJsonDocument<200> doc;
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        doc["bpm"] = sharedData.bpm;
        doc["emg"] = sharedData.emgPercent;
        doc["temp"] = sharedData.temp;
        doc["hum"] = sharedData.hum;
        xSemaphoreGive(dataMutex);

        String out;
        serializeJson(doc, out);
        webSocket.broadcastTXT(out);
        vTaskDelay(200 / portTICK_PERIOD_MS); // 5Hz Telemetry is plenty
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LO_PLUS, INPUT_PULLDOWN);
    pinMode(LO_MINUS, INPUT_PULLDOWN);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    dht.begin();
    dataMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(Sensing_Task, "Sensors", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(Telemetry_Task, "Network", 8192, NULL, 1, NULL, 0);
}

void loop() { vTaskDelete(NULL); }