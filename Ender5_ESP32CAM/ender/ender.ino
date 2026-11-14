#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "esp_http_server.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* ssid = "WifiAd";
const char* password = "parolunuz";

IPAddress local_IP(192, 168, 1, 185);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 254);
IPAddress secondaryDNS(8, 8, 8, 8);

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN      4

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

bool flash_state = false;
bool vflip_state = false;
bool hmirror_state = false;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="az">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#0b1120">
  <title>Ender-5 Kamera Nəzarəti</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      background-color: #0b1120;
      color: #f1f5f9;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: env(safe-area-inset-top, 12px) env(safe-area-inset-right, 12px) env(safe-area-inset-bottom, 16px) env(safe-area-inset-left, 12px);
      touch-action: manipulation;
    }
    .app-container {
      width: 100%;
      max-width: 820px;
      display: flex;
      flex-direction: column;
      gap: 12px;
      padding-top: 6px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 0 4px;
    }
    h1 {
      font-size: 1.25rem;
      font-weight: 700;
      color: #38bdf8;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      font-size: 0.75rem;
      font-weight: 600;
      background: rgba(34, 197, 94, 0.15);
      color: #4ade80;
      padding: 5px 10px;
      border-radius: 9999px;
      border: 1px solid rgba(74, 222, 128, 0.3);
    }
    .pulse-dot {
      width: 8px;
      height: 8px;
      background: #4ade80;
      border-radius: 50%;
      box-shadow: 0 0 8px #4ade80;
      animation: pulse 1.6s infinite;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.4; transform: scale(0.85); }
    }
    .video-card {
      background: #1e293b;
      border-radius: 16px;
      overflow: hidden;
      border: 1px solid #334155;
      box-shadow: 0 12px 30px rgba(0, 0, 0, 0.6);
      display: flex;
      flex-direction: column;
    }
    .video-wrapper {
      position: relative;
      background: #000;
      width: 100%;
      aspect-ratio: 4/3;
      max-height: 68vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .video-wrapper img {
      width: 100%;
      height: 100%;
      object-fit: contain;
      display: block;
    }
    .fullscreen-btn {
      position: absolute;
      top: 10px;
      right: 10px;
      background: rgba(15, 23, 42, 0.75);
      backdrop-filter: blur(8px);
      border: 1px solid rgba(255,255,255,0.2);
      color: #fff;
      padding: 8px 12px;
      border-radius: 8px;
      font-size: 0.8rem;
      cursor: pointer;
    }
    .control-section {
      padding: 14px;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .btn-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
    }
    @media (min-width: 600px) {
      .btn-grid { grid-template-columns: repeat(4, 1fr); }
      h1 { font-size: 1.5rem; }
    }
    button, .action-link {
      min-height: 48px;
      background: #334155;
      color: #f8fafc;
      border: 1px solid #475569;
      border-radius: 12px;
      font-size: 0.95rem;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.15s ease;
      text-decoration: none;
      user-select: none;
    }
    button:active, .action-link:active {
      transform: scale(0.97);
      background: #475569;
    }
    .btn-flash.active {
      background: #eab308 !important;
      color: #000 !important;
      border-color: #fde047 !important;
      box-shadow: 0 0 15px rgba(234, 179, 8, 0.5);
    }
    .btn-capture {
      background: #0284c7;
      border-color: #38bdf8;
    }
    .btn-capture:hover { background: #0369a1; }

    .settings-panel {
      background: #0f172a;
      border-radius: 12px;
      padding: 12px 14px;
      border: 1px solid #334155;
      display: grid;
      grid-template-columns: 1fr;
      gap: 12px;
    }
    @media (min-width: 600px) {
      .settings-panel { grid-template-columns: 1fr 1fr; }
    }
    .form-group {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .form-group label {
      font-size: 0.85rem;
      font-weight: 500;
      color: #94a3b8;
    }
    select, input[type=range] {
      font-size: 16px;
      background: #1e293b;
      color: #fff;
      border: 1px solid #475569;
      border-radius: 8px;
      padding: 10px 12px;
      outline: none;
      width: 100%;
    }
    select:focus { border-color: #38bdf8; }
    
    .endpoints-card {
      background: #1e293b;
      border-radius: 12px;
      padding: 12px 16px;
      border: 1px solid #334155;
      font-size: 0.82rem;
      line-height: 1.6;
      color: #94a3b8;
      word-break: break-all;
    }
    .endpoints-card strong { color: #f1f5f9; }
    .endpoints-card code {
      color: #38bdf8;
      background: rgba(56, 189, 248, 0.1);
      padding: 2px 6px;
      border-radius: 4px;
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
    }
    footer {
      text-align: center;
      font-size: 0.75rem;
      color: #64748b;
      padding: 6px 0;
    }
  </style>
</head>
<body>
  <div class="app-container">
    <header>
      <h1>🖨️ Ender-5 Cam</h1>
      <div class="badge">
        <span class="pulse-dot"></span> CANLI
      </div>
    </header>

    <div class="video-card">
      <div class="video-wrapper" id="videoBox">
        <img src="" id="video-stream" alt="Video axını yüklənir...">
        <button class="fullscreen-btn" onclick="toggleFullscreen()">⛶ Tam Ekran</button>
      </div>

      <div class="control-section">
        <div class="btn-grid">
          <button id="flash-btn" class="btn-flash" onclick="toggleFlash()">💡 Fənər</button>
          <button onclick="toggleFlip('vflip')">↕️ Alt-Üst</button>
          <button onclick="toggleFlip('hmirror')">↔️ Güzgü</button>
          <a href="/capture" target="_blank" class="action-link btn-capture">📷 Şəkil Çək</a>
        </div>

        <div class="settings-panel">
          <div class="form-group">
            <label for="resolution">📐 Çözünürlük (Ölçü):</label>
            <select id="resolution" onchange="setControl('framesize', this.value)">
              <option value="8" selected>VGA (640x480) - Standart / Sürətli</option>
              <option value="9">SVGA (800x600) - Yaxşı balans</option>
              <option value="11">HD (1280x720) - 720p Geniş ekran</option>
              <option value="12">SXGA (1280x1024) - Yüksək keyfiyyət</option>
              <option value="13">UXGA (1600x1200) - Maksimum 2MP</option>
            </select>
          </div>

          <div class="form-group">
            <label for="quality">✨ Sıxılma Keyfiyyəti:</label>
            <select id="quality" onchange="setControl('quality', this.value)">
              <option value="10">Maksimum Aydınlıq (10)</option>
              <option value="12" selected>Yüksək Keyfiyyət (12)</option>
              <option value="18">Balanslaşdırılmış (18)</option>
              <option value="25">Aşağı / Ən Yüksək FPS (25)</option>
            </select>
          </div>

          <div class="form-group" style="grid-column: 1 / -1;">
            <label for="brightness">☀️ Parlaqlıq: <span id="bright-val" style="color:#38bdf8; font-weight:bold;">0</span></label>
            <input type="range" id="brightness" min="-2" max="2" value="0" oninput="document.getElementById('bright-val').innerText = this.value; setControl('brightness', this.value)">
          </div>
        </div>
      </div>
    </div>

    <div class="endpoints-card">
      <strong>Klipper (Fluidd / Mainsail) və OctoPrint linkləri:</strong><br>
      Stream: <code id="stream-url">http://192.168.1.185:81/stream</code><br>
      Snapshot: <code id="snap-url">http://192.168.1.185/capture</code>
    </div>

    <footer>
      ESP32-CAM Dual-Server &bull; Ender-5 Monitorinq &bull; Sabit IP: 192.168.1.185
    </footer>
  </div>

  <script>
    const host = window.location.hostname;
    const streamPort = 81;
    const streamUrl = 'http://' + host + ':' + streamPort + '/stream';

    window.addEventListener('load', () => {
      document.getElementById('video-stream').src = streamUrl;
      document.getElementById('stream-url').innerText = streamUrl;
      document.getElementById('snap-url').innerText = 'http://' + host + '/capture';
    });

    function toggleFlash() {
      fetch('/toggle-flash')
        .then(res => res.text())
        .then(state => {
          const btn = document.getElementById('flash-btn');
          if (state === '1') {
            btn.classList.add('active');
            btn.innerText = '💡 Fənər: AÇIQ';
          } else {
            btn.classList.remove('active');
            btn.innerText = '💡 Fənər';
          }
        })
        .catch(err => console.error("Fənər xətası:", err));
    }

    function toggleFlip(type) {
      fetch('/' + type);
    }

    function setControl(variable, value) {
      fetch('/control?var=' + variable + '&val=' + value)
        .then(() => {
          if (variable === 'framesize') {
            const stream = document.getElementById('video-stream');
            stream.src = '';
            setTimeout(() => {
              stream.src = streamUrl + '?t=' + Date.now();
            }, 300);
          }
        })
        .catch(err => console.error("Parametr xətası:", err));
    }

    function toggleFullscreen() {
      const el = document.getElementById('videoBox');
      if (!document.fullscreenElement && !document.webkitFullscreenElement) {
        if (el.requestFullscreen) {
          el.requestFullscreen();
        } else if (el.webkitRequestFullscreen) {
          el.webkitRequestFullscreen();
        }
      } else {
        if (document.exitFullscreen) {
          document.exitFullscreen();
        } else if (document.webkitExitFullscreen) {
          document.webkitExitFullscreen();
        }
      }
    }
  </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      delay(20);
      continue;
    }

    if (fb->format != PIXFORMAT_JPEG) {
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted) {
        delay(20);
        continue;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if (res != ESP_OK) break;
  }
  return res;
}

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

static esp_err_t flash_handler(httpd_req_t *req) {
  flash_state = !flash_state;
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flash_state ? HIGH : LOW);
  
  const char* resp = flash_state ? "1" : "0";
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, resp, strlen(resp));
  return ESP_OK;
}

static esp_err_t vflip_handler(httpd_req_t *req) {
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    vflip_state = !vflip_state;
    s->set_vflip(s, vflip_state ? 1 : 0);
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t hmirror_handler(httpd_req_t *req) {
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    hmirror_state = !hmirror_state;
    s->set_hmirror(s, hmirror_state ? 1 : 0);
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t control_handler(httpd_req_t *req) {
  char* buf;
  size_t buf_len;
  char variable[32] = {0,};
  char value[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int val = atoi(value);
  sensor_t * s = esp_camera_sensor_get();
  int res = 0;

  if (!strcmp(variable, "framesize")) {
    if (s->pixformat == PIXFORMAT_JPEG) {
      res = s->set_framesize(s, (framesize_t)val);
    }
  } else if (!strcmp(variable, "quality")) {
    res = s->set_quality(s, val);
  } else if (!strcmp(variable, "brightness")) {
    res = s->set_brightness(s, val);
  } else {
    res = -1;
  }

  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, "OK", 2);
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;

  httpd_uri_t index_uri   = { .uri = "/",             .method = HTTP_GET, .handler = index_handler,   .user_ctx = NULL };
  httpd_uri_t capture_uri = { .uri = "/capture",      .method = HTTP_GET, .handler = capture_handler, .user_ctx = NULL };
  httpd_uri_t flash_uri   = { .uri = "/toggle-flash", .method = HTTP_GET, .handler = flash_handler,   .user_ctx = NULL };
  httpd_uri_t vflip_uri   = { .uri = "/vflip",        .method = HTTP_GET, .handler = vflip_handler,   .user_ctx = NULL };
  httpd_uri_t hmirror_uri = { .uri = "/hmirror",      .method = HTTP_GET, .handler = hmirror_handler, .user_ctx = NULL };
  httpd_uri_t control_uri = { .uri = "/control",      .method = HTTP_GET, .handler = control_handler, .user_ctx = NULL };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &flash_uri);
    httpd_register_uri_handler(camera_httpd, &vflip_uri);
    httpd_register_uri_handler(camera_httpd, &hmirror_uri);
    httpd_register_uri_handler(camera_httpd, &control_uri);
    Serial.println("Esas Server (Port 80) ise dusdu!");
  }

  httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
  stream_config.server_port = 81;
  stream_config.ctrl_port = 32769;

  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };

  if (httpd_start(&stream_httpd, &stream_config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("Video Stream Server (Port 81) ise dusdu!");
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println("\n--- Ender-5 ESP32-CAM Baslayir ---");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera xətası: 0x%x\n", err);
    return;
  }

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_VGA);
    s->set_quality(s, 12);
  }

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Sabit IP parametr xətası!");
  }

  Serial.printf("Wi-Fi sebekesine qosulur: %s\n", ssid);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi sebekesine qosuldu!");
  Serial.print("Veb Panel: http://");
  Serial.println(WiFi.localIP());
  Serial.print("Canli Video Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");

  startCameraServer();
}

void loop() {
  delay(10000);
}