// AtomS3 Spirit Level - bubble level, tube level and inclinometer using the MPU6886.
//  click       : next mode (bubble / tube / angle)
//  hold 0.6 s  : zero the current mode here (calibrate), release to apply
//  hold 2.2 s  : reset that mode's zero, release to apply
// Serial console (115200): ver | mode N | zero | reset | sim ax ay az | sim off | shot
#include <M5Unified.h>
#include <Preferences.h>
#include <math.h>

#define FW_VERSION "1.0.0"

// Flip these if the bubble moves the wrong way (it should run to the HIGH side).
#ifndef BUBBLE_X_SIGN
#define BUBBLE_X_SIGN 1.0f
#endif
#ifndef BUBBLE_Y_SIGN
#define BUBBLE_Y_SIGN -1.0f
#endif

static const float LEVEL_DEG = 0.5f;  // "level" band (green)
static const float NEAR_DEG = 2.0f;   // "almost" band (amber)

static M5Canvas cv(&M5.Display);
static Preferences prefs;

enum Mode : uint8_t { BUBBLE, TUBE, ANGLE, NUM_MODES };
static const char* MODE_NAMES[] = {"BUBBLE", "TUBE", "ANGLE"};

static uint8_t mode = BUBBLE;
static float off[NUM_MODES][2] = {{0, 0}, {0, 0}, {0, 0}};  // zero offsets (degrees)

static float ax = 0, ay = 0, az = 1;  // filtered accel (g)
static bool simOn = false;
static float simA[3] = {0, 0, 1};
static bool shotRequested = false;
static uint32_t lastFrame = 0, lastLog = 0, toastUntil = 0, levelSince = 0;
static char toast[24] = "";

static uint16_t C_BG, C_DIM, C_RING, C_TEXT, C_BLUE, C_GOOD, C_WARN;

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float deg(float rad) { return rad * 57.29578f; }
static inline uint16_t rgb(int r, int g, int b) { return cv.color565(r, g, b); }

static void saveOffsets() {
  prefs.putBytes("off", off, sizeof(off));
  prefs.putUChar("mode", mode);
}

static void say(const char* t) {
  strlcpy(toast, t, sizeof(toast));
  toastUntil = millis() + 1200;
}

// ---- measurements ---------------------------------------------------------
// Flat on a table (screen up): tilt about x (roll) and y (pitch), degrees.
static void bubbleAngles(float& bx, float& by) {
  float z = fmaxf(az, 0.05f);
  bx = deg(atan2f(ax, z)) - off[BUBBLE][0];
  by = deg(atan2f(ay, z)) - off[BUBBLE][1];
}

// Standing up, screen facing you: rotation in the screen plane (shelf / picture level).
static float tubeAngle() {
  float a = deg(atan2f(ax, ay));
  if (a > 90) a -= 180;
  if (a < -90) a += 180;
  return a - off[TUBE][0];
}

// Slope of the screen plane against horizontal, 0..90 (ramp / roof pitch).
static float slopeAngle() {
  return deg(atan2f(sqrtf(ax * ax + ay * ay), fabsf(az))) - off[ANGLE][0];
}

static uint16_t bandColor(float absDeg) {
  if (absDeg <= LEVEL_DEG) return C_GOOD;
  if (absDeg <= NEAR_DEG) return C_WARN;
  return C_BLUE;
}

static void txt(const char* s, int x, int y, int size, uint16_t col,
                textdatum_t d = textdatum_t::middle_center) {
  cv.setTextDatum(d);
  cv.setTextColor(col);
  cv.setTextSize(size);
  cv.drawString(s, x, y);
}

// ---- screens --------------------------------------------------------------
static void drawBubble() {
  float bx, by;
  bubbleAngles(bx, by);
  float total = sqrtf(bx * bx + by * by);
  uint16_t col = bandColor(total);

  const int cx = 64, cy = 58, R = 46;
  cv.drawCircle(cx, cy, R, col == C_BLUE ? C_RING : col);
  cv.drawCircle(cx, cy, R - 1, col == C_BLUE ? C_RING : col);
  cv.drawCircle(cx, cy, 24, C_DIM);
  cv.drawCircle(cx, cy, 10, C_RING);
  cv.drawFastHLine(cx - R, cy, 2 * R, C_DIM);
  cv.drawFastVLine(cx, cy - R, 2 * R, C_DIM);

  float px = clampf(bx * BUBBLE_X_SIGN * 5.0f, -37, 37);
  float py = clampf(by * BUBBLE_Y_SIGN * 5.0f, -37, 37);
  float d = sqrtf(px * px + py * py);
  if (d > 37) { px *= 37 / d; py *= 37 / d; }
  cv.fillCircle(cx + (int)px, cy + (int)py, 10, col);
  cv.fillCircle(cx + (int)px - 3, cy + (int)py - 3, 3, rgb(255, 255, 255));

  char b[32];
  snprintf(b, sizeof b, "%+.1f  %+.1f", bx, by);
  txt(b, 64, 113, 1, C_TEXT);
  if (total <= LEVEL_DEG) txt("LEVEL", 64, 8, 1, C_GOOD);
  else txt("x    y  (deg)", 64, 8, 1, C_DIM);
}

static void drawTube() {
  float a = tubeAngle();
  uint16_t col = bandColor(fabsf(a));
  bool flat = fabsf(az) > 0.85f;

  const int x0 = 8, x1 = 120, cy = 48, h = 30;
  cv.drawRoundRect(x0, cy - h / 2, x1 - x0, h, 14, col == C_BLUE ? C_RING : col);
  cv.drawRoundRect(x0 + 1, cy - h / 2 + 1, x1 - x0 - 2, h - 2, 13, col == C_BLUE ? C_RING : col);
  cv.drawFastVLine(64 - 9, cy - h / 2 + 3, h - 6, C_DIM);
  cv.drawFastVLine(64 + 9, cy - h / 2 + 3, h - 6, C_DIM);
  float px = clampf(a * BUBBLE_X_SIGN * 5.5f, -38, 38);
  cv.fillEllipse(64 + (int)px, cy, 15, 9, col);
  cv.fillCircle(64 + (int)px - 5, cy - 3, 2, rgb(255, 255, 255));

  char b[24];
  snprintf(b, sizeof b, "%+.1f", a);
  txt(b, 58, 92, 3, col);
  txt("o", 112, 84, 2, col);
  if (flat) txt("stand it up", 64, 119, 1, C_WARN);
  else if (fabsf(a) <= LEVEL_DEG) txt("LEVEL", 64, 119, 1, C_GOOD);
  else txt("screen toward you", 64, 119, 1, C_DIM);
}

static void drawAngle() {
  float v = slopeAngle();
  float frac = clampf(fabsf(v) / 90.0f, 0, 1);
  uint16_t col = fabsf(v) <= LEVEL_DEG ? C_GOOD : C_BLUE;
  cv.fillArc(64, 60, 50, 58, 135, 405, C_DIM);
  if (frac > 0.003f) cv.fillArc(64, 60, 50, 58, 135, 135 + 270 * frac, col);

  char b[24];
  snprintf(b, sizeof b, "%.1f", fabsf(v));
  txt(b, 58, 56, 3, col);
  txt("o", 114, 46, 2, col);
  float pct = fabsf(v) < 89 ? tanf(fabsf(v) / 57.29578f) * 100.0f : 9999;
  if (pct < 1000) snprintf(b, sizeof b, "slope %.0f%%", pct);
  else snprintf(b, sizeof b, "slope --");
  txt(b, 64, 86, 1, C_TEXT);
  txt("lay it on the slope", 64, 112, 1, C_DIM);
}

static void drawChrome(uint32_t now) {
  // mode name + dots
  txt(MODE_NAMES[mode], 4, 123, 1, C_DIM, textdatum_t::middle_left);
  for (int i = 0; i < NUM_MODES; i++)
    cv.fillCircle(104 + i * 8, 123, i == mode ? 3 : 2, i == mode ? C_TEXT : C_DIM);

  if (M5.BtnA.isPressed()) {
    if (M5.BtnA.pressedFor(2200)) txt("release: RESET zero", 64, 100, 1, C_WARN);
    else if (M5.BtnA.pressedFor(600)) txt("release: ZERO here", 64, 100, 1, C_GOOD);
  }
  if (now < toastUntil) {
    cv.fillRoundRect(14, 50, 100, 26, 6, rgb(20, 24, 40));
    cv.drawRoundRect(14, 50, 100, 26, 6, C_GOOD);
    txt(toast, 64, 63, 1, C_TEXT);
  }
}

static void draw(uint32_t now) {
  cv.fillSprite(C_BG);
  switch (mode) {
    case TUBE: drawTube(); break;
    case ANGLE: drawAngle(); break;
    default: drawBubble(); break;
  }
  drawChrome(now);
  cv.pushSprite(0, 0);
}

// ---- input ----------------------------------------------------------------
static void readMotion() {
  float rx, ry, rz;
  if (simOn) {
    rx = simA[0]; ry = simA[1]; rz = simA[2];
  } else {
    if (!M5.Imu.update()) return;
    M5.Imu.getAccel(&rx, &ry, &rz);
  }
  const float k = simOn ? 1.0f : 0.25f;  // light smoothing for the real sensor
  ax += (rx - ax) * k;
  ay += (ry - ay) * k;
  az += (rz - az) * k;
}

static void zeroHere() {
  switch (mode) {
    case BUBBLE: {
      float z = fmaxf(az, 0.05f);
      off[BUBBLE][0] = deg(atan2f(ax, z));
      off[BUBBLE][1] = deg(atan2f(ay, z));
      break;
    }
    case TUBE: {
      float a = deg(atan2f(ax, ay));
      if (a > 90) a -= 180;
      if (a < -90) a += 180;
      off[TUBE][0] = a;
      break;
    }
    default: off[ANGLE][0] = deg(atan2f(sqrtf(ax * ax + ay * ay), fabsf(az))); break;
  }
  saveOffsets();
  say("ZERO SET");
}

static void resetZero() {
  off[mode][0] = off[mode][1] = 0;
  saveOffsets();
  say("ZERO RESET");
}

static void runCommand(char* cmd) {
  int v;
  float a, b, c;
  if (!strcmp(cmd, "ver")) Serial.printf("AtomS3 Spirit Level %s\n", FW_VERSION);
  else if (sscanf(cmd, "mode %d", &v) == 1) { mode = ((v % NUM_MODES) + NUM_MODES) % NUM_MODES; saveOffsets(); }
  else if (!strcmp(cmd, "zero")) zeroHere();
  else if (!strcmp(cmd, "reset")) resetZero();
  else if (!strcmp(cmd, "sim off")) simOn = false;
  else if (sscanf(cmd, "sim %f %f %f", &a, &b, &c) == 3) { simOn = true; simA[0] = a; simA[1] = b; simA[2] = c; }
  else if (!strcmp(cmd, "shot")) shotRequested = true;
  else Serial.println("commands: ver | mode 0-2 | zero | reset | sim ax ay az | sim off | shot");
}

static void pollSerial() {
  static char buf[40];
  static int n = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      buf[n] = 0;
      if (n) runCommand(buf);
      n = 0;
    } else if (n < (int)sizeof(buf) - 1) {
      buf[n++] = ch;
    }
  }
}

static void sendShot() {
  Serial.printf("SHOT %d %d\n", 128, 128);
  Serial.write((const uint8_t*)cv.getBuffer(), 128 * 128 * 2);
  Serial.println();
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(130);
  cv.setColorDepth(16);
  cv.createSprite(128, 128);
  C_BG = rgb(6, 8, 16);
  C_DIM = rgb(40, 48, 70);
  C_RING = rgb(70, 84, 120);
  C_TEXT = rgb(235, 240, 255);
  C_BLUE = rgb(80, 200, 255);
  C_GOOD = rgb(60, 220, 120);
  C_WARN = rgb(255, 190, 60);
  prefs.begin("level", false);
  if (prefs.getBytesLength("off") == sizeof(off)) prefs.getBytes("off", off, sizeof(off));
  mode = prefs.getUChar("mode", BUBBLE) % NUM_MODES;
  Serial.printf("AtomS3 Spirit Level %s, IMU type %d\n", FW_VERSION, (int)M5.Imu.getType());
}

void loop() {
  M5.update();
  uint32_t now = millis();

  if (M5.BtnA.wasReleaseFor(2200)) resetZero();
  else if (M5.BtnA.wasReleaseFor(600)) zeroHere();
  else if (M5.BtnA.wasClicked()) {
    mode = (mode + 1) % NUM_MODES;
    saveOffsets();
  }

  pollSerial();
  if (now - lastFrame < 16) return;
  lastFrame = now;
  readMotion();
  draw(now);
  if (shotRequested) {
    shotRequested = false;
    sendShot();
  }
  if (now - lastLog > 1000) {
    lastLog = now;
    Serial.printf("a=%.3f %.3f %.3f mode=%d\n", ax, ay, az, mode);
  }
}
