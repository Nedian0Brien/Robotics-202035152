# 261121 실습 기술문서
- 일시: 2026-11-21(토), 로보틱스AI 수업 3강
- 작성자: 202035152 박민재

## 1. 수행 과정
> 로봇에 LiDAR를 설치하고 IMU·LiDAR 센서 테스트로 I2C·UART 통신을 확인했습니다. ROS2 개발·통신 환경을 구축하고 talker·listener 노드로 토픽 통신 실습을 진행했습니다.

### 진행 내용
1. 자율주행차량 로봇 조립: LiDAR 설치
2. IMU, 임베디드 통신 프로토콜(UART, I2C) 학습
3. ESP32로 IMU(MPU6050)·LiDAR 센서 테스트 코드 실행
4. ROS, ROS1과 ROS2 비교, DDS, QoS, colcon 학습
5. ROS2 개발·통신 환경 설정 (Cyclone DDS, ROS_DOMAIN_ID=8)
6. 1_pubsub 패키지 생성, talker·listener 노드로 Publisher·Subscriber 통신 실습

## 2. 이론적 배경

### 2.1 IMU
Inertial Measurement Unit, 관성 측정 유닛

- Gyro: 각속도
- Accelerometer: 선 가속도
- Magnetometer: 나침반
- 실습 보드의 MPU6050은 Gyro·Accelerometer만 있는 6축 IMU (Magnetometer 없음)

### 2.2 임베디드 통신 프로토콜(Communication Protocol)
- MCU와 센서, 액추에이터, 외부 제어 장치 사이에서 데이터를 교환하기 위한 통신 규칙
- 데이터의 전송 순서, 신호 타이밍, 전송 형식 및 장치 식별 방법을 정의
- 임베디드 시스템에서는 주로 직렬 통신(Serial Communication) 방식을 사용
- 주요 통신 방식에는 동기식 / 비동기식 방식이 있음
  - 비동기식: 별도의 Clock 신호선 없이 약속된 전송 속도와 프레임 형식 사용
  - 동기식: Clock 신호를 기준으로 데이터 송수신
- 대표적인 통신 인터페이스 및 프로토콜
  - UART: 비동기, TX·RX (LiDAR, PC 시리얼 모니터)
  - I2C: 동기, SDA·SCL (IMU)
  - SPI: 동기, SCLK·MOSI·MISO·CS
  - CAN: 비동기, CAN_H·CAN_L 차동 신호

### 2.3 UART
- 범용 비동기 송수신기로, 두 장치 간 직렬 데이터 통신에 활용
- TX(Transmit)와 RX(Receive) 신호선을 통해 데이터 송수신
  - TX에서 데이터가 나가고, RX에서 데이터가 들어온다
- 별도의 Clock 신호선 없이 약속된 Baud Rate에 따라 통신
- Start Bit, Data Bit, 선택적 Parity Bit, Stop Bit로 데이터 프레임 구성
- 송수신 장치 간 Baud Rate, Data Bit, Parity, Stop Bit 등의 설정 일치 필요
- LiDAR, GPS, 외부 MCU 및 USB-UART 변환기를 통한 PC 통신 등에 사용

### 2.4 I2C(Inter-Integrated Circuit)
- MCU와 여러 주변 장치를 적은 수의 신호선으로 연결하기 위한 동기식 직렬 통신 프로토콜
- SDA(Serial Data)와 SCL(Serial Clock), 두 개의 신호선 사용
- Start Condition: SCL이 HIGH일 때 SDA에 Falling Edge가 발생하는 순간 통신이 시작됨
- Stop Condition: SCL이 HIGH일 때 SDA에 Rising Edge가 발생하면 통신 종료
- ACK: Acknowledge, 응답 신호
- NACK: Not Acknowledge, 문제가 있을 때 NACK으로 응답

### 2.5 ROS
Robot Operating System

- 실제로는 OS가 아니라 소프트웨어 프레임워크임
- OS라고 하는 이유는 OS와 비슷한 역할을 많이 하기 때문

#### 역할
- 하드웨어 추상화
  - 하드웨어마다 다른 동작 방식과 세부 구현을 추상화
  - 개발자는 하드웨어의 세부 사항 대신 일관된 인터페이스를 사용(데이터 패키징)
- 프로세스간 통신
  - 각 프로세스는 서로 분리된 메모리 공간에서 실행
  - 프로세스 간 데이터를 안전하고 효율적으로 주고받을 수 있도록 지원
- "Meta-OS"
  - OS 안에 있는 또 하나의 OS라는 관점에서 Meta-OS라고 부르기도 함

#### 단순 프로그램이 아닌 하나의 생태계
- Standard Communication Protocol
  - 통신 규격(메시지 타입)을 엄격하게 통일
- 패키지
  - 로봇에 필요한 거의 모든 기능이 패키지로 존재
- 시각화 개발 도구
  - RViz, Gazebo

### 2.6 ROS1 vs ROS2

| 항목 | ROS1 | ROS2 |
| --- | --- | --- |
| 통신 구조 | 중앙 집중성, Master(roscore) 필수 | P2P 통신(Peer-to-Peer), DDS 프로토콜 사용 |
| 보안 | 보안 X, 동일 네트워크 내 데이터 탈취 가능 | SROS2 도입(인증, 권한, 암호화) |
| OS 지원 | 오직 Linux | 리눅스, 윈도우, 맥OS 공식 지원 |

### 2.7 DDS(Data Distribution Service)
- 산업 표준으로 사용되는 고신뢰성 데이터 통신 표준
- 보안성이 높아서 국가기관, 국방 등에서도 사용됨
- P2P로 노드들끼리 서로 자동으로 discover
- QoS(Quality of Service) 설정 가능
- Fast DDS, Cyclone DDS
  - Jazzy 기본값은 Fast DDS, 수업 환경은 RMW_IMPLEMENTATION=rmw_cyclonedds_cpp로 Cyclone DDS 사용
- Domain ID
  - Domain 단위로 ROS 노드들을 그룹화
  - 같은 ROS_DOMAIN_ID에 속한 노드들은 같은 DDS 도메인에 연결
  - 서로의 토픽을 자동으로 발견
  - 기본값(미설정) = 0
  - 수업에서 각 학생별로 도메인 나눠서 운영. 나는 8번 도메인 사용

### 2.8 ROS 기본 용어
- node: 하나의 기능을 담당하는 실행 단위 (예: /talker)
- message / msg_type: 노드 간에 주고받는 데이터 구조 (예: std_msgs/msg/String)
- topic: 메시지가 오가는 이름 붙은 통로
- publisher / subscriber: 토픽에 메시지를 발행하는 쪽 / 구독해서 받는 쪽
- service: 요청 1번에 응답 1번을 주고받는 통신
- action: 오래 걸리는 작업을 목표로 보내고 피드백·결과를 받으며 취소도 가능한 통신
- 토픽 이름을 정확하게 달아줘야 함 → publisher와 subscriber의 토픽 이름·메시지 타입이 같아야 연결됨

### 2.9 QoS(Quality of Service)
노드끼리 데이터를 주고받을 때 적용하는 '통신 규칙'

1. Reliability(신뢰성)
    - Reliable(기본값): 하나라도 유실되면 안 되는 경우 - 실시간성이 저하되더라도 데이터를 빠짐없이 보내야 하는 경우
    - Best Effort: 데이터를 보낼 때 최선을 다하지 않음 - 데이터가 중간에 유실될 수 있음 - 그래도 괜찮다고 보는 경우
2. Durability(지속성)
    - Volatile(기본값): 휘발성 - 노드가 뒤늦게 구독을 시작하면 필요한 메시지를 전달받지 못할 수 있음
    - Transient Local: 내부에 보존 - 일시적으로 잠시 메시지를 보존 - 늦게 구독한 노드도 메시지를 받을 수 있음
3. History·Depth
    - Keep Last(기본값): 최근 N개만 큐에 보관
- Publisher와 Subscriber의 QoS가 호환되지 않으면 연결 안 됨 (예: Publisher Best Effort + Subscriber Reliable)

### 2.10 Colcon
- Colcon Workspace
  - ROS2 패키지의 개발 및 관리를 위한 작업 공간(폴더)
  - 구조
    - src/: 개발자가 작성한 ROS2 패키지의 소스 코드
    - build/: 컴파일 및 빌드 과정에서 생성되는 중간 파일
    - install/: 빌드 및 설치된 실행 파일, 라이브러리, 설정 파일 등
    - log/: 빌드 및 테스트 과정의 로그
- Colcon 빌드 도구
  - ROS2 패키지의 빌드 및 테스트를 관리하는 CLI 도구
  - Workspace 내 패키지 자동 탐색
  - 패키지 간 의존성을 분석하여 빌드 순서 결정
  - 여러 패키지를 일괄 또는 선택적으로 빌드 (--packages-select)
- 빌드 후 source install/setup.bash 해야 현재 터미널에서 패키지를 찾음
- setup.py의 console_scripts에 ros2 run으로 쓸 실행 파일 이름 등록
  - 'talker = 1_pubsub.talker:main' → 실행 파일 이름 = 패키지.모듈:함수

## 3. 실습

### 3.1 IMU 테스트 (imu_test/imu_test.ino)
- ESP32에 MPU6050을 I2C로 연결해 가속도·각속도를 시리얼 모니터로 출력
- 센서를 깨운 뒤 가속도·자이로 값을 읽어 m/s², deg/s 단위로 변환

### 3.2 LiDAR 테스트 (lidar_test/lidar_test.ino)
- ESP32에서 UART로 LiDAR 데이터를 받아 각도별 거리 계산
- 한 바퀴 돌 때마다 0~359도의 거리(mm) 출력

### 3.3 ROS2 개발·통신 환경 설정
```bash
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
echo "export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" >> ~/.bashrc
echo "export ROS_DOMAIN_ID=8" >> ~/.bashrc
source ~/.bashrc
```

### 3.4 talker·listener (1_pubsub)
- 패키지 생성 (~/robot_navigation/colcon_ws/src)
  ```bash
  ros2 pkg create 1_pubsub --build-type ament_python --dependencies rclpy std_msgs geometry_msgs sensor_msgs nav_msgs
  ```
- setup.py의 console_scripts에 talker, listener 등록
- 노드·토픽 구조
  ```
  /talker ── /demo_topic (std_msgs/msg/String) ──▶ /listener
  ```
- talker: 1초마다 "Hello ROS2: 경과 초" 문자열을 /demo_topic에 발행
- listener: /demo_topic을 구독하고 받은 문자열을 로그로 출력
- rclpy.spin(node): Ctrl+C까지 콜백 처리 (while loop 역할)

#### 분석
- 두 노드 모두 기본 QoS(Reliable, Volatile)라 호환됨
- Volatile이라 listener를 늦게 켜면 그 이후 발행된 메시지부터 수신
- ros2 topic info /demo_topic으로 Publisher·Subscription 연결 확인

## 4. 실행 방법

### IMU·LiDAR 테스트
1. Arduino IDE에서 보드 D1 R32, ESP32 USB 포트 선택 (Linux는 dialout 권한 필요)
2. imu_test/imu_test.ino 또는 lidar_test/lidar_test.ino 업로드
3. 시리얼 모니터 115200 baud로 출력 확인

### talker·listener
```bash
cp -r 1_pubsub ~/robot_navigation/colcon_ws/src/
source /opt/ros/jazzy/setup.bash
cd ~/robot_navigation/colcon_ws
colcon build --packages-select 1_pubsub
source install/setup.bash

ros2 run 1_pubsub talker      # 터미널 1
ros2 run 1_pubsub listener    # 터미널 2 (source install/setup.bash 후)
ros2 topic info /demo_topic --verbose   # 터미널 3에서 연결 확인
```

## 5. 필수 의존성
- Arduino IDE, ESP32 보드 패키지
- Ubuntu 24.04, ROS2 Jazzy, Python 3.12
- ros-jazzy-rmw-cyclonedds-cpp
- ROS2 패키지: rclpy, std_msgs

## 6. 파일 구조
- imu_test/imu_test.ino: MPU6050 가속도·각속도 측정
- lidar_test/lidar_test.ino: LiDAR 패킷 수신, 각도별 거리 출력
- 1_pubsub/: talker·listener ROS2 패키지 (ament_python)
    - 1_pubsub/talker.py, 1_pubsub/listener.py: 노드 소스
    - 1_pubsub/setup.sample.py: 강사 제공 setup.py 예제 사본
    - setup.py, setup.cfg, package.xml, resource/: 패키지 빌드 설정
    - test/: ros2 pkg create가 생성한 코드 스타일 검사
- ROS/1/: 강사 제공 talker·listener·setup.py 예제 사본
