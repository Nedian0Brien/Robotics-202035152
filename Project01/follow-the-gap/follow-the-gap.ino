// PCB-ESP32 GPIO 매핑 (수업가이드.ipynb 기준)
#define FR_IN1 26
#define FR_IN2 25
#define FL_IN1 19
#define FL_IN2 27
#define RR_IN1 23
#define RR_IN2 12
#define RL_IN1 0
#define RL_IN2 15

#define LIDAR_RX 16
#define POT_PIN 39
#define BUZZER_PIN 5

// 모터 PWM 및 현장 보정 상수
constexpr uint32_t MOTOR_PWM_FREQ = 1000;
constexpr uint8_t MOTOR_PWM_RES = 8;
constexpr int MIN_PWM = 75;   // DC 기어드 모터 데드존 보상 최소 PWM
constexpr int MAX_PWM = 160;
constexpr int MOTOR_SIGN[4] = {1, 1, 1, 1};  // {FR, FL, RR, RL}, 반대로 돌면 -1

// LiDAR 47B 패킷
constexpr int LIDAR_PKT_SIZE = 47;
constexpr uint8_t MIN_CONFIDENCE = 50;

// 주행 규칙 상수 (거리 mm, 각도 deg, 출력은 MAX_PWM 대비 비율)
constexpr int MIN_D = 180;          // 차체 가림·근거리 잡음 제외 거리
constexpr int D_FREE = 700;         // 이보다 가까운 점만 장애물로 봄
constexpr int D_STOP = 300;         // 앞 통로 점이 이 거리면 전진 0
constexpr float R_MM = 250.0f;      // 차폭/2 + 옆 여유 150mm (차폭 측정 후 조정)
constexpr int H_DEG = 15;           // 피한 쪽을 유지하는 여유각
constexpr float V_CRUISE = 0.4f;    // 순항 출력 (속도·정지거리 측정 후 조정)
constexpr float DV = 0.1f;          // 스캔당 최대 가속
constexpr float K_W = 0.01f;        // 목표 1도당 회전 출력
constexpr float W_TURN = 0.5f;      // 목표 없을 때 제자리 회전 출력
constexpr float POT_ON = 0.05f;     // 가변저항이 이 비율 미만이면 정지
constexpr unsigned long SCAN_TIMEOUT_MS = 500;
constexpr int NONE = 999;

const int motorPins[8] = {
    FR_IN1, FR_IN2,
    FL_IN1, FL_IN2,
    RR_IN1, RR_IN2,
    RL_IN1, RL_IN2,
};

uint8_t lidarBuf[LIDAR_PKT_SIZE];
int lidarIdx = 0;
uint16_t distances[360] = {0};   // 인덱스는 LiDAR 원시 각도: 0=뒤, 90=왼쪽, 180=앞, 270=오른쪽
float prevAngle = -1.0f;
unsigned long lastScanMs = 0;

bool blocked[181];   // 인덱스 a+90, a는 앞 기준 -90..90도이고 왼쪽이 +
int side = 1;        // 마지막으로 피한 쪽, 왼쪽 +1
float v = 0.0f;


uint16_t readUInt16(uint8_t low, uint8_t high) {
    return low | (high << 8);
}


uint8_t calcCrc8(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x4D) : (crc << 1);
        }
    }
    return crc;
}


void setMotorChannel(int pin1, int pin2, int pwm) {
    if (abs(pwm) < 8) {
        ledcWrite(pin1, 0);
        ledcWrite(pin2, 0);
        return;
    }
    // 0이 아닌 명령은 데드존(MIN_PWM) 이상으로 선형 매핑함
    const int mag = map(min(abs(pwm), MAX_PWM), 8, MAX_PWM, MIN_PWM, MAX_PWM);
    if (pwm > 0) {
        ledcWrite(pin1, mag);
        ledcWrite(pin2, 0);
    } else {
        ledcWrite(pin1, 0);
        ledcWrite(pin2, mag);
    }
}


void stopMotors() {
    for (int i = 0; i < 8; i += 2) {
        setMotorChannel(motorPins[i], motorPins[i + 1], 0);
    }
}


void applyVelocity(float vNorm, float wNorm) {
    // 차동 구동(4WD 스키드 스티어) 좌·우 PWM 혼합함, w가 +면 좌회전
    const int leftPwm = int((vNorm - wNorm) * MAX_PWM);
    const int rightPwm = int((vNorm + wNorm) * MAX_PWM);

    setMotorChannel(FR_IN1, FR_IN2, rightPwm * MOTOR_SIGN[0]);
    setMotorChannel(FL_IN1, FL_IN2, leftPwm * MOTOR_SIGN[1]);
    setMotorChannel(RR_IN1, RR_IN2, rightPwm * MOTOR_SIGN[2]);
    setMotorChannel(RL_IN1, RL_IN2, leftPwm * MOTOR_SIGN[3]);
}


void markBlocked() {
    for (int i = 0; i <= 180; i++) {
        blocked[i] = false;
    }
    for (int raw = 0; raw < 360; raw++) {
        const int d = distances[raw];
        if (d <= MIN_D || d >= D_FREE) {
            continue;   // 측정 없음(0)·근거리 잡음·먼 점은 장애물에서 뺌
        }
        const int a = 180 - raw;
        // 그 방향으로 직진하면 점과의 옆 간격이 R보다 좁아지는 각 범위
        const int h = (d <= R_MM) ? 90 : int(ceilf(degrees(asinf(R_MM / d))));
        for (int k = max(a - h, -90); k <= min(a + h, 90); k++) {
            blocked[k + 90] = true;
        }
    }
}


int nearestOpen(int dir) {
    // dir 쪽(+1 왼쪽, -1 오른쪽)에서 0도에 가장 가까운 열린 각
    for (int s = 0; s <= 90; s++) {
        if (!blocked[dir * s + 90]) {
            return dir * s;
        }
    }
    return NONE;
}


int chooseTarget() {
    const int left = nearestOpen(1);
    const int right = nearestOpen(-1);
    if (left == NONE || right == NONE) {
        return (left == NONE) ? right : left;
    }
    // 좌우 후보가 비슷하면 지난번에 피한 쪽을 유지해 좌우 떨림을 막음
    const int keep = (side > 0) ? left : right;
    const int other = (side > 0) ? right : left;
    return (abs(keep) <= abs(other) + H_DEG) ? keep : other;
}


float aheadDist() {
    // 차 정면 폭 2R 통로 안에서 가장 가까운 점의 앞쪽 거리
    float best = D_FREE;
    for (int raw = 91; raw < 270; raw++) {
        const int d = distances[raw];
        if (d <= MIN_D) {
            continue;
        }
        const float a = radians(180 - raw);
        const float x = d * cosf(a);
        const float y = d * sinf(a);
        if (fabsf(y) < R_MM && x < best) {
            best = x;
        }
    }
    return best;
}


void printStatus(float pot, int target, float w, float ahead) {
    static unsigned long lastPrintMs = 0;
    if (millis() - lastPrintMs < 500) {
        return;
    }
    lastPrintMs = millis();
    if (target == NONE) {
        Serial.print("target=none");
    } else {
        Serial.printf("target=%+d", target);
    }
    Serial.printf(" side=%+d pot=%.2f v=%.2f w=%.2f ahead=%.0f\n", side, pot, v, w, ahead);
}


void decide() {
    const float pot = analogRead(POT_PIN) / 4095.0f;
    markBlocked();
    const int target = chooseTarget();
    const float ahead = aheadDist();
    float vGoal = 0.0f;
    float w = 0.0f;
    if (pot < POT_ON) {
        // 가변저항을 내리면 회전까지 멈추고 판단 결과만 출력함
    } else if (target == NONE) {
        w = side * W_TURN;   // 피하던 쪽으로 제자리 회전함
    } else {
        if (target != 0) {
            side = (target > 0) ? 1 : -1;
        }
        const float slow = constrain((ahead - D_STOP) / (D_FREE - D_STOP), 0.0f, 1.0f);
        w = K_W * target;
        vGoal = V_CRUISE * pot * max(0.0f, 1.0f - abs(target) / 45.0f) * slow;
    }
    v = min(vGoal, v + DV);   // 가속만 제한하고 감속은 즉시 반영함
    applyVelocity(v, w);
    printStatus(pot, target, w, ahead);
}


void processLidarPacket() {
    // 수업 lidar_test.ino와 동일한 12포인트 각도 보간
    const uint16_t startRaw = readUInt16(lidarBuf[4], lidarBuf[5]);
    const uint16_t endRaw = readUInt16(lidarBuf[42], lidarBuf[43]);
    if (startRaw >= 36000 || endRaw >= 36000) {
        return;
    }

    float startAngle = startRaw / 100.0f;
    float endAngle = endRaw / 100.0f;
    if (endAngle < startAngle) {
        endAngle += 360.0f;
    }
    const float angleStep = (endAngle - startAngle) / 11.0f;

    for (int i = 0; i < 12; i++) {
        const int idx = 6 + i * 3;
        const uint16_t dist = readUInt16(lidarBuf[idx], lidarBuf[idx + 1]);
        const uint8_t conf = lidarBuf[idx + 2];

        float angle = startAngle + angleStep * i;
        if (angle >= 360.0f) {
            angle -= 360.0f;
        }

        // 360->0도 전환 시 한 바퀴 완성으로 보고 주행 판단 후 배열 초기화함
        if (prevAngle > 300.0f && angle < 60.0f) {
            lastScanMs = millis();
            decide();
            memset(distances, 0, sizeof(distances));
        }

        int deg = int(angle + 0.5f);
        if (deg >= 360) {
            deg = 0;
        }
        if (dist > 0 && conf >= MIN_CONFIDENCE) {
            distances[deg] = dist;
        }
        prevAngle = angle;
    }
}


void pollLidar() {
    for (int n = 0; n < LIDAR_PKT_SIZE * 4 && Serial2.available(); n++) {
        const uint8_t b = Serial2.read();
        if (lidarIdx == 0 && b != 0x54) {
            continue;
        }
        if (lidarIdx == 1 && b != 0x2C) {
            lidarIdx = (b == 0x54) ? 1 : 0;
            continue;
        }
        lidarBuf[lidarIdx++] = b;
        if (lidarIdx < LIDAR_PKT_SIZE) {
            continue;
        }
        lidarIdx = 0;
        if (calcCrc8(lidarBuf, LIDAR_PKT_SIZE - 1) == lidarBuf[LIDAR_PKT_SIZE - 1]) {
            processLidarPacket();
        }
    }
}


void setup() {
    Serial.begin(115200);

    for (int i = 0; i < 8; i++) {
        ledcAttach(motorPins[i], MOTOR_PWM_FREQ, MOTOR_PWM_RES);
        ledcWrite(motorPins[i], 0);
    }

    Serial2.setRxBufferSize(2048);
    Serial2.begin(230400, SERIAL_8N1, LIDAR_RX, -1);

    // 준비 완료 신호음(짧은 비프)
    ledcAttach(BUZZER_PIN, 2000, 8);
    ledcWrite(BUZZER_PIN, 40);
    delay(80);
    ledcWrite(BUZZER_PIN, 0);
}


void loop() {
    pollLidar();

    // 한 바퀴 스캔이 500ms 넘게 없으면 정지함
    if (millis() - lastScanMs > SCAN_TIMEOUT_MS) {
        v = 0.0f;
        stopMotors();
    }
}
