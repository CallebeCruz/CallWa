// === ESP WAL – FIRMWARE COMPLETO =============================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <math.h>

// --- prototypes das animações de status
void tickStatusBlend(uint16_t periodMs = 1200, uint8_t vmax = 160);
void flashOk(uint8_t reps = 2, uint16_t onMs = 120, uint16_t offMs = 80);

// ====== DEFINIÇÕES WIFI WAL ======
const char* ssid     = "VIVOFIBRA-WIFI6-0D91";   // SSID WAL FamiliaCruz
const char* password = "79pXpBacHBgawyD"; // SENHA WAL 32569867@cruz

// ====== MQTT HIVEMQ ======
const char* mqtt_server = "70d4da565d9c476782999dc9303945b0.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "ESP_WALL";
const char* mqtt_pass   = "Serzinhos2025";

const char* mqtt_topic_ping = "luminaria/ping";
const char* mqtt_topic_out  = "luminaria/para_voce";
const char* client_id       = "ESP32_Wal";
const char* esp_id          = "ESP_WAL";

// ====== LED ONBOARD ======
const int ledPin = 2;
bool ledState = false;
unsigned long lastBlink = 0;
unsigned long blinkInterval = 200;

// ====== MAPEAMENTO DO TUBO ======
#define NUM_LEDS 144
#ifndef STEPS_PER_TURN
#define STEPS_PER_TURN 28   // ~27 leds por volta p/ Ø60 mm; ajuste fino 26~28
#endif
#define NUM_TURNS ((NUM_LEDS + STEPS_PER_TURN - 1) / STEPS_PER_TURN)
#define LAST_TURN_COUNT (NUM_LEDS - ((NUM_TURNS - 1) * STEPS_PER_TURN))

inline int idxFromTurnCol(int turn, int col) {
  if (turn < 0 || turn >= NUM_TURNS) return -1;
  int colsThisTurn = (turn == NUM_TURNS - 1) ? max(1, LAST_TURN_COUNT) : STEPS_PER_TURN;
  col = (col % colsThisTurn + colsThisTurn) % colsThisTurn;
  int base = turn * STEPS_PER_TURN;
  int idx  = base + col;
  return (idx >= 0 && idx < NUM_LEDS) ? idx : -1;
}

// ====== FASTLED / MQTT CLIENT ======
WiFiClientSecure espClient;
PubSubClient client(espClient);

#define LED_PIN     4
#define COLOR_ORDER GRB
#define LED_TYPE    WS2812B
CRGB leds[NUM_LEDS];

// ====== TOUCH BUTTONS ======
#define TOUCH_PENSANDO   T4   // GPIO 13
#define TOUCH_SAUDADES   T6   // GPIO 14
#define TOUCH_DESCULPAS  T7   // GPIO 27
#define TOUCH_TOAQUI     T8   // GPIO 33
#define TOUCH_SURPRESA   T9   // GPIO 32

int thresholds[] = {40, 40, 40, 40, 40};
unsigned long lastTouchTime[5] = {0, 0, 0, 0, 0};
const unsigned long debounceDelay = 1000;

// ====== PING ======
unsigned long lastPingTime = 0;
const unsigned long pingInterval = 500;

// ====== DELAY COOPERATIVO ======
inline void coopDelay(uint32_t ms){
  uint32_t end = millis() + ms;
  while ((int32_t)(end - millis()) > 0){
    client.loop();
    delay(1); // yield curto
  }
}

// "Pump" rápido pra manter rede viva
#define PUMP() do{ client.loop(); delay(0); }while(0)

// ===== REPEAT LOCAL =====
bool repeatLocal = false;
unsigned long lastRepeatAt = 0;
const unsigned long repeatIntervalMs = 30UL * 1000UL; // 30s

// último efeito rastreado
char last_efeito[24] = {0};
char last_cor[16]    = {0};   // "#rrggbb" normalizado para comparação
int  last_velocidade = -1;

const char* peer_id = "ESP_CALL"; // id do dispositivo do Callebe

const char* TRACKED_EFFECTS[] = {
  "saudades", "pensando", "desculpas", "to_aqui", "surpresinha", "bajulinho"
};
const size_t TRACKED_N = sizeof(TRACKED_EFFECTS)/sizeof(TRACKED_EFFECTS[0]);

bool isTracked(const char* ef){
  if(!ef) return false;
  for(size_t i=0;i<TRACKED_N;i++) if(strcmp(ef, TRACKED_EFFECTS[i])==0) return true;
  return false;
}

// ===== FLAG GLOBAL DE CANCELAMENTO (Botão "Desligar" via Web) =====
volatile bool gCancelEffect = false;

// Macro pra efeitos verificarem se tem cancelamento pedido
#define CHECK_CANCEL() do{ \
  if(gCancelEffect){ \
    gCancelEffect = false; \
    FastLED.clear(true); \
    return; \
  } \
}while(0)

// ===== ANIMAÇÃO DE STATUS (verde+azul) =====
void tickStatusBlend(uint16_t periodMs, uint8_t vmax) {
  const CRGB cA = CRGB(0, 200, 255);
  const CRGB cB = CRGB(0, 255, 120);
  uint16_t t = millis() % periodMs;
  uint8_t  u = (t < periodMs/2) ? map(t, 0, periodMs/2, 0, 255)
                                : map(t, periodMs/2, periodMs, 255, 0);
  CRGB col = blend(cA, cB, u);
  col.nscale8_video(vmax);
  fill_solid(leds, NUM_LEDS, col);
  FastLED.show();
}

void flashOk(uint8_t reps, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i=0;i<reps;i++){
    fill_solid(leds, NUM_LEDS, CRGB(0, 230, 180)); FastLED.show(); delay(onMs);
    FastLED.clear(true); delay(offMs);
  }
}

// ====== WIFI ======
void setup_wifi() {
  Serial.println("\nConectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);

    if (millis() - lastBlink >= blinkInterval) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
    tickStatusBlend(1100, 170);
  }
  Serial.println("WiFi conectado!");
  flashOk(2);
  FastLED.clear(true);
  blinkInterval = 1000;
  espClient.setInsecure();
}

// ====== HELPERS DE COR / EASING ======
inline uint8_t easeInOutCubic8(uint8_t x){
  float xf=x/255.0f; 
  float y=(xf<0.5f)?4*xf*xf*xf:1-powf(-2*xf+2,3)/2;
  return (uint8_t)(y*255.0f+0.5f);
}
inline uint8_t gamma8(uint8_t v){
  float f=v/255.0f; 
  f=powf(f,2.2f); 
  return (uint8_t)(f*255.0f+0.5f);
}
inline CRGB hexToRGB(String hx){
  if (hx.length()<7) return CRGB::White;
  long n=strtol(hx.substring(1,7).c_str(),NULL,16);
  return CRGB((n>>16)&0xFF,(n>>8)&0xFF,n&0xFF);
}

// ====== MODO ESTÁTICO (LUMINÁRIA) ============================================
// efeito "estatico": acende todos os LEDs na cor recebida
void efeitoEstatico(String corHex = "#ffffff") {
  gCancelEffect = false;   // nova "sessão", limpa eventual cancel anterior
  CRGB c = hexToRGB(corHex);
  fill_solid(leds, NUM_LEDS, c);
  FastLED.show();
}

// ====== EFEITOS ===============================================================

// Pensando – versão cooperativa
void efeitoPensando(int ciclos = 2, int velocidade = 30) {
  gCancelEffect = false;
  for (int c = 0; c < ciclos; c++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      CHECK_CANCEL();
      FastLED.clear();
      leds[i] = CRGB::DeepSkyBlue;
      FastLED.show();
      coopDelay(velocidade);
    }
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      CHECK_CANCEL();
      FastLED.clear();
      leds[i] = CRGB::DeepSkyBlue;
      FastLED.show();
      coopDelay(velocidade);
    }
  }
}

// Saudade – lub-dub com easing e coop
void efeitoSaudades(int batidas = 4,
                    uint8_t brilhoBase = 18,
                    uint8_t pico1 = 255,
                    uint8_t pico2 = 180) {

  gCancelEffect = false;

  const int atk1 = 130, dec1 = 190, pausa1 = 70;
  const int atk2 = 100, dec2 = 230, pausa2 = 360;

  auto easeSin = [](float x){
    return 0.5f - 0.5f * cosf(3.1415926f * x);
  };

  auto fazPulso = [&](uint8_t pico, int atk, int dec) {
    // ATAQUE
    for (int t = 0; t <= atk; t += 8) {
      CHECK_CANCEL();
      float x = (float)t / (float)max(1, atk);
      uint8_t eased = (uint8_t)(easeSin(x) * 255.0f + 0.5f);
      uint8_t br = map(eased, 0, 255, brilhoBase, pico);
      br = gamma8(br);

      uint8_t h = map(br, brilhoBase, 255, 0, 12);
      uint8_t s = map(br, brilhoBase, 255, 255, 230);

      fill_solid(leds, NUM_LEDS, CHSV(h, s, br));
      FastLED.show();
      PUMP();
      coopDelay(8);
    }
    // DECAY
    for (int t = 0; t <= dec; t += 8) {
      CHECK_CANCEL();
      float x = (float)t / (float)max(1, dec);
      uint8_t eased = (uint8_t)((1.0f - easeSin(x)) * 255.0f + 0.5f);
      uint8_t br = map(eased, 0, 255, brilhoBase, pico);
      br = gamma8(br);

      uint8_t h = map(br, brilhoBase, 255, 0, 12);
      uint8_t s = map(br, brilhoBase, 255, 255, 230);

      fill_solid(leds, NUM_LEDS, CHSV(h, s, br));
      FastLED.show();
      client.loop();
      delay(8);
    }
  };

  for (int i = 0; i < batidas; i++) {
    CHECK_CANCEL();
    int atk1v = atk1 + random(-8, 9);
    int dec1v = dec1 + random(-10, 11);
    int atk2v = atk2 + random(-8, 9);
    int dec2v = dec2 + random(-12, 13);
    int pausa1v = pausa1 + random(-10, 11);
    int pausa2v = pausa2 + random(-20, 21);

    fazPulso(pico1, atk1v, dec1v);
    CHECK_CANCEL();
    delay(pausa1v); client.loop();

    fazPulso(pico2, atk2v, dec2v);
    CHECK_CANCEL();
    delay(pausa2v); client.loop();
  }

  CHECK_CANCEL();
  fill_solid(leds, NUM_LEDS, CHSV(8, 240, brilhoBase));
  FastLED.show();
}

// Desculpas – versão “batida dupla” com glints
void efeitoDesculpas (String corHex = "#3aa7ff",
                      int velocidade = 40,
                      int batidas = 3,
                      bool brilhos = true) {
  gCancelEffect = false;

  CRGB cBase = hexToRGB(corHex);
  CRGB cPeak = CRGB::White;
  uint8_t baseV = 16;
  uint8_t pico1 = 235;
  uint8_t pico2 = 185;
  int frameDelay = constrain(velocidade, 3, 120);

  auto pulse = [&](uint8_t pico, int atk_ms, int dec_ms){
    // ATAQUE
    for (int t=0; t<=atk_ms; t+=8){
      CHECK_CANCEL();
      uint8_t e = easeInOutCubic8(map(t,0,atk_ms,0,255));
      uint8_t v = gamma8(map(e,0,255,baseV,pico));
      uint8_t mix = map(e,0,255,32,12);
      CRGB mixed = blend(cBase, cPeak, mix);
      for (int i=0;i<NUM_LEDS;i++){ leds[i]=mixed; leds[i].nscale8_video(v); }
      if (brilhos && (t%32==0)) {
        for (int s=0;s<max(1,NUM_LEDS/48);s++){
          int idx=random16(NUM_LEDS); leds[idx]+=CRGB(40,40,40);
        }
      }
      FastLED.show();
      PUMP();
      coopDelay(frameDelay);
    }
    // DECAY
    for (int t=0; t<=dec_ms; t+=8){
      CHECK_CANCEL();
      uint8_t e = easeInOutCubic8(map(t,0,dec_ms,255,0));
      uint8_t v = gamma8(map(e,0,255,baseV,pico));
      uint8_t mix = map(e,0,255,32,12);
      CRGB mixed = blend(cBase, cPeak, mix);
      for (int i=0;i<NUM_LEDS;i++){ leds[i]=mixed; leds[i].nscale8_video(v); }
      FastLED.show();
      PUMP();
      coopDelay(frameDelay);
    }
  };

  for (int i=0;i<batidas;i++){
    CHECK_CANCEL();
    pulse(pico1, 120, 180);
    CHECK_CANCEL();
    delay(70); client.loop();
    pulse(pico2, 90,  220);
    CHECK_CANCEL();
    delay(340); client.loop();
  }

  CHECK_CANCEL();
  fill_solid(leds, NUM_LEDS, cBase);
  for (int i=0;i<NUM_LEDS;i++) leds[i].nscale8_video(baseV);
  FastLED.show();
}

// To aqui – respirando (cooperativo)
void efeitoToAquiRespirando(int ciclos = 3, int delayTempo = 10) {
  gCancelEffect = false;
  for (int i = 0; i < ciclos; i++) {
    for (int b = 0; b <= 255; b += 5) {
      CHECK_CANCEL();
      fill_solid(leds, NUM_LEDS, CHSV(200, 255, b));
      FastLED.show();
      coopDelay(delayTempo);
    }
    for (int b = 255; b >= 0; b -= 5) {
      CHECK_CANCEL();
      fill_solid(leds, NUM_LEDS, CHSV(200, 255, b));
      FastLED.show();
      coopDelay(delayTempo);
    }
  }
  CHECK_CANCEL();
  FastLED.clear();
  FastLED.show();
}

// To aqui – rastro cooperativo
void efeitoToAquiRastro(uint8_t steps = 5, uint8_t delayTempo = 60) {
  gCancelEffect = false;
  FastLED.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    CHECK_CANCEL();
    for (int j = 0; j <= steps; j++) {
      int pos = i - j;
      if (pos >= 0) {
        uint8_t brilho = 255 - (j * (255 / steps));
        leds[pos] = CHSV(200, 255, brilho);
      }
    }
    FastLED.show();
    coopDelay(delayTempo);
  }
  CHECK_CANCEL();
  coopDelay(500);
  FastLED.clear();
  FastLED.show();
}

// Surpresinha – fogo com cor/vel/duração
void efeitoSurpresinhaFogo(String corHex = "#ff6a00", int velocidade = 50, int duracao_ms = 3000) {
  gCancelEffect = false;

  velocidade = constrain(velocidade, 0, 100);
  uint8_t cooling   = map(velocidade, 0, 100, 20, 55);
  uint8_t sparking  = map(velocidade, 0, 100, 60, 200);
  uint8_t bpmBase   = map(velocidade, 0, 100, 12, 38);
  uint8_t glitterTh = map(velocidade, 0, 100, 3, 14);
  uint8_t mixTint   = 100;

  CRGB tint = hexToRGB(corHex);

  static uint8_t heat[NUM_LEDS];
  memset(heat, 0, sizeof(heat));

  CRGBPalette16 firePalette(
    CRGB::Black, CRGB(90, 10, 0),
    CRGB(180, 40, 0), CRGB(255, 80, 0),
    CRGB(255, 140, 0), CRGB(255, 200, 60),
    CRGB(255, 255, 180), CRGB::White,
    CRGB::Black, CRGB(90, 10, 0),
    CRGB(180, 40, 0), CRGB(255, 80, 0),
    CRGB(255, 140, 0), CRGB(255, 200, 60),
    CRGB(255, 255, 180), CRGB::White
  );

  int frameDelay = map(velocidade, 0, 100, 34, 18);

  unsigned long start = millis();
  while ((millis() - start) < (unsigned long)duracao_ms) {
    CHECK_CANCEL();
    // resfriar
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t cooldown = random8(0, ((cooling * 10) / max(1, NUM_LEDS)) + 2);
      heat[i] = qsub8(heat[i], cooldown);
    }

    // difusão
    for (int k = NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // faíscas na base
    if (random8() < sparking) {
      uint8_t y = random8(0, min<uint16_t>(7, NUM_LEDS));
      heat[y] = qadd8(heat[y], random8(160, 255));
    }

    uint8_t bpmVar = bpmBase + random8(0, 4);
    uint8_t pulse = beatsin8(bpmVar, 150, 255);

    for (int j = 0; j < NUM_LEDS; j++) {
      uint8_t idx = scale8(heat[j], 240);
      CRGB fireCol = ColorFromPalette(firePalette, idx);

      fireCol.r = qadd8(fireCol.r, map(pulse, 150, 255, 0, 30));
      fireCol.g = qsub8(fireCol.g, map(pulse, 150, 255, 0, 10));
      fireCol.b = qsub8(fireCol.b, map(pulse, 150, 255, 0, fireCol.b));

      CRGB mixed = blend(fireCol, tint, mixTint);
      nscale8x3_video(mixed.r, mixed.g, mixed.b, pulse);
      leds[j] = mixed;
    }

    if (random8() < glitterTh) {
      int p = random16(NUM_LEDS);
      leds[p] += CRGB(80, 60, 20);
    }

    FastLED.show();
    for (int t = 0; t < frameDelay; t += 2) {
      CHECK_CANCEL();
      client.loop();
      delay(2);
    }
  }

  CHECK_CANCEL();
  FastLED.clear(true);
}

// Aspectro – versão cooperativa (usa cor + vel + tempo)
void efeitoAspectro(String corHex = "#ffffff", int velocidade = 50, int tempo_ms = 5000) {
  gCancelEffect = false;

  CRGB corBase = CRGB::White;

  if (corHex.length() == 9 || corHex.length() == 7) {
    long number = strtol(corHex.substring(1, 7).c_str(), NULL, 16);
    byte r = (number >> 16) & 0xFF;
    byte g = (number >> 8) & 0xFF;
    byte b = number & 0xFF;
    corBase = CRGB(r, g, b);
  }

  int delayFade = constrain(velocidade, 5, 200);
  unsigned long start = millis();

  while (millis() - start < tempo_ms) {
    // Fade In
    for (int b = 0; b <= 255; b += 5) {
      CHECK_CANCEL();
      CRGB stepColor = corBase;
      stepColor.fadeLightBy(255 - b);
      fill_solid(leds, NUM_LEDS, stepColor);
      FastLED.show();
      coopDelay(delayFade);
    }

    // Fade Out
    for (int b = 255; b >= 0; b -= 5) {
      CHECK_CANCEL();
      CRGB stepColor = corBase;
      stepColor.fadeLightBy(255 - b);
      fill_solid(leds, NUM_LEDS, stepColor);
      FastLED.show();
      coopDelay(delayFade);
    }
  }

  CHECK_CANCEL();
  FastLED.clear();
  FastLED.show();
}

// Bajulinho – orbita em espiral
void efeitoBajulinho(String corHex = "#ff66aa", int velocidade = 60) {
  gCancelEffect = false;

  CRGB cor = CRGB::White;
  if (corHex.length() >= 7) {
    long x = strtol(corHex.substring(1, 7).c_str(), NULL, 16);
    cor = CRGB((x>>16)&0xFF, (x>>8)&0xFF, x&0xFF);
  }

  const uint8_t trailCols  = 6;
  const uint8_t trailTurns = 3;
  const uint8_t fadeBase   = 180;
  int delayFrame = constrain(velocidade, 5, 120);

  int turn = 0;
  int col  = 0;

  long stepsBudget = (long)(NUM_TURNS - 1) * STEPS_PER_TURN;

  while (stepsBudget-- > 0) {
    CHECK_CANCEL();
    for (int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(fadeBase);

    for (uint8_t v = 0; v < trailTurns; v++) {
      int tActual = turn - (int)v;
      if (tActual < 0) break;

      int colsThisTurn = (tActual == NUM_TURNS - 1) ? max(1, LAST_TURN_COUNT) : STEPS_PER_TURN;

      for (uint8_t t = 0; t < trailCols; t++) {
        int c1 = col - (int)t;
        c1 = (c1 % colsThisTurn + colsThisTurn) % colsThisTurn;
        int idx = idxFromTurnCol(tActual, c1);
        if (idx >= 0) {
          CRGB px = cor;
          uint8_t att = map(v * trailCols + t, 0, trailCols * trailTurns - 1, 220, 20);
          px.fadeLightBy(255 - att);
          leds[idx] += px;
        }
      }
    }

    FastLED.show();
    for (int i = 0; i < delayFrame; i += 2) {
      CHECK_CANCEL();
      client.loop();
      delay(2);
    }

    int colsThisTurn = (turn == NUM_TURNS - 1) ? max(1, LAST_TURN_COUNT) : STEPS_PER_TURN;
    col++;
    if (col >= colsThisTurn) {
      col = 0;
      turn++;
      if (turn >= NUM_TURNS - 1) break;
    }
  }

  CHECK_CANCEL();
  for (int k = 0; k < 2; k++) {
    for (int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(120);
    FastLED.show(); delay(40); client.loop();
  }
  FastLED.clear(true);
}

// ====== CALLBACK MQTT =========================================================
void playEffectByName(const char* efeito, const String& cor, int velocidade);

void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];
  Serial.println(mensagem);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, mensagem);
  if (error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char* from   = doc["from"]   | "";
  const char* to     = doc["to"]     | "ALL";
  const char* efeito = doc["efeito"] | "";
  String cor         = doc["cor"]    | "#ffffff";

  // normaliza cor para comparação
  String corNorm = cor;
  if (corNorm.length() == 9) corNorm = corNorm.substring(0, 7);

  int velocidade = 50;
  if (doc["velocidade"].is<int>())              velocidade = doc["velocidade"].as<int>();
  else if (doc["velocidade"].is<const char*>()) velocidade = atoi(doc["velocidade"]);

  String toDest = to;

  // --- Botão "Desligar" vindo do Web: corta tudo e limpa fita ---
  if (efeito && strcmp(efeito, "desligar") == 0) {
    Serial.println("[CMD] Desligar recebido – cancelando efeitos e limpando fita.");
    gCancelEffect = true;
    repeatLocal = false;
    FastLED.clear(true);
    FastLED.show();
    return;
  }

  // START: se a msg foi PARA o WAL (ou ALL) e é um dos rastreados, salva
  if ((toDest == esp_id || toDest == "ALL") && isTracked(efeito)) {
    strlcpy(last_efeito, efeito, sizeof(last_efeito));
    strlcpy(last_cor, corNorm.c_str(), sizeof(last_cor));
    last_velocidade = velocidade;
    repeatLocal = true;
    lastRepeatAt = millis();
    Serial.printf("[TRACK] Salvado p/ repeat: efeito=%s cor=%s vel=%d\n",
                  last_efeito, last_cor, last_velocidade);
  }

  // STOP: se vier msg PARA o ESP_CALL com mesmo efeito/cor/vel, cancela repeat
  if (strcmp(to, peer_id) == 0 && isTracked(efeito)) {
    int velRx = -1;
    if (doc["velocidade"].is<int>()) velRx = doc["velocidade"].as<int>();
    else if (doc["velocidade"].is<const char*>()) velRx = atoi(doc["velocidade"]);

    if (repeatLocal &&
        strcmp(efeito, last_efeito) == 0 &&
        (corNorm == String(last_cor)) &&
        (velRx == last_velocidade)) {
      repeatLocal = false;
      Serial.println("[TRACK] Encontrou mesmo efeito para ESP_CALL → repeat local cancelado.");
    }
  }

  // Executa localmente se o destino for WAL/ALL
  if (toDest == esp_id || toDest == "ALL") {
    if (efeito && *efeito) {
      if (strcmp(efeito, "estatico")    == 0) efeitoEstatico(cor);
      else if (strcmp(efeito, "pensando")    == 0) efeitoPensando();
      else if (strcmp(efeito, "saudades")    == 0) efeitoSaudades();
      else if (strcmp(efeito, "desculpas")   == 0) efeitoDesculpas();
      else if (strcmp(efeito, "to_aqui")     == 0) efeitoToAquiRespirando();
      else if (strcmp(efeito, "surpresinha") == 0) efeitoSurpresinhaFogo(cor, velocidade);
      else if (strcmp(efeito, "aspectro")    == 0) efeitoAspectro(cor, velocidade);
      else if (strcmp(efeito, "bajulinho")   == 0) efeitoBajulinho(cor, velocidade);
    }
  }
}

void playEffectByName(const char* efeito, const String& cor, int velocidade){
  if(!efeito) return;
  if (strcmp(efeito, "estatico")     == 0) efeitoEstatico(cor);
  else if (strcmp(efeito, "pensando")== 0) efeitoPensando();
  else if (strcmp(efeito, "saudades")== 0) efeitoSaudades();
  else if (strcmp(efeito, "desculpas")==0) efeitoDesculpas();
  else if (strcmp(efeito, "to_aqui") == 0) efeitoToAquiRespirando();
  else if (strcmp(efeito, "surpresinha")==0) efeitoSurpresinhaFogo(cor, velocidade);
  else if (strcmp(efeito, "bajulinho")==0) efeitoBajulinho(cor, velocidade);
  else if (strcmp(efeito, "aspectro") ==0) efeitoAspectro(cor, velocidade);
}

// ====== MQTT RECONNECT ========================================================
void reconnect() {
  while (!client.connected()) {
    tickStatusBlend(800, 180);

    if (client.connect(client_id, mqtt_user, mqtt_pass)) {
      client.subscribe("luminaria/#");
      flashOk(3);
      FastLED.clear(true);
    } else {
      if (millis() - lastBlink >= 300) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
      }
      delay(0);
    }
  }
}

// ====== ENVIAR EFEITO =========================================================
void sendEfeito(const char* efeito,
                const char* cor = "#23d300ff",
                int velocidade = 50,
                const char* to = "ESP_CALL")
{
  StaticJsonDocument<256> doc;
  doc["from"] = esp_id;     // "ESP_WAL"
  doc["to"]   = to;
  doc["efeito"] = efeito;
  doc["cor"]     = cor;
  doc["velocidade"] = velocidade;

  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  client.publish(mqtt_topic_out, buf);
}

// ====== CHECAR TOQUE ==========================================================
void checkTouch() {
  int rawValues[5] = {
    touchRead(13),
    touchRead(14),
    touchRead(27),
    touchRead(33),
    touchRead(32)
  };
  const char* efeitos[] = {"pensando", "saudades", "desculpas", "to_aqui", "surpresinha"};
  for (int i = 0; i < 5; i++) {
    if (rawValues[i] < thresholds[i] && millis() - lastTouchTime[i] > debounceDelay) {
      sendEfeito(efeitos[i]);
      lastTouchTime[i] = millis();
    }
  }
}

// ====== SETUP =================================================================
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear(); FastLED.show();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(768);
  client.setKeepAlive(20);
  client.setSocketTimeout(1);
  WiFi.setSleep(false);
}

// ====== LOOP ==================================================================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // bloco de repetição local
  if (repeatLocal) {
    unsigned long now = millis();
    if ((unsigned long)(now - lastRepeatAt) >= repeatIntervalMs) {
      playEffectByName(last_efeito, String(last_cor), last_velocidade);
      lastRepeatAt = now;
      Serial.printf("[REPEAT] Reexecutado localmente: %s\n", last_efeito);
    }
  }

  checkTouch();

  if (millis() - lastBlink >= blinkInterval) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }

  if (millis() - lastPingTime >= pingInterval) {
    lastPingTime = millis();
    StaticJsonDocument<256> pingDoc;
    pingDoc["id"]        = esp_id;
    pingDoc["from"]      = "WAL";
    pingDoc["to"]        = "ALL";
    pingDoc["status"]    = "ok";
    pingDoc["timestamp"] = millis();
    char buffer[256];
    serializeJson(pingDoc, buffer);
    client.publish(mqtt_topic_ping, buffer);
  }
}
