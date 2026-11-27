# 261122 실습 기술문서
- 일시: 2026-11-22(일), 로보틱스AI 수업 4강
- 작성자: 202035152 박민재

## 1. 수행 과정
> Launch 파일, Service, Action, 사용자 정의 인터페이스를 학습했습니다. launch 파일로 노드 세 개를 순서대로 실행하고, custom_interfaces에 메시지·서비스를 정의해 AddTwoInts 서비스와 MoveRobot 과제를 구현했습니다.

### 진행 내용
1. Launch 파일 학습, 1_pubsub의 publisher·processor·subscriber를 실행하는 launch 파일 구성
2. Service, Action, 사용자 정의 인터페이스 학습
3. custom_interfaces 패키지에 Num, RobotPose 메시지와 AddTwoInts, MoveRobot 서비스 정의
4. 2_srvmsg 패키지에서 AddTwoInts 서버·클라이언트 실행
5. MoveRobot 과제: 현재 자세와 이동 거리로 도착 자세를 계산하는 서비스 구현

## 2. 이론적 배경

### 2.1 Launch 파일
- 여러 ROS 2 노드를 한 번의 명령어로 실행하기 위한 설정 파일
- 노드 실행에 필요한 설정을 통합적으로 관리
- 저장 위치: src/패키지/launch/ 디렉터리
- 주로 Python으로 작성
  - XML과 YAML 형식도 지원하지만, 선언적 구조로 인해 복잡한 실행 로직 구현에는 제약이 있음
- 파일명 형식: 파일명.launch.py
- Python 패키지는 setup.py의 data_files에 launch 파일을 등록해야 install 폴더에 설치되고 ros2 launch로 실행 가능

### 2.2 Service
- 노드 간 요청과 응답을 기반으로 하는 통신 방식
- Client-Server 구조로 동작
  - Client: 작업 요청 및 응답 수신
  - Server: 요청 처리 및 결과 반환
- 일회성 요청이나 짧은 작업 수행에 적합
- .srv 파일을 통해 Request와 Response 데이터 구조 정의 (--- 위가 Request, 아래가 Response)
- 예시: 센서 데이터 요청, 로봇 상태 조회, 특정 값 초기화

### 2.3 Action
- 오래 걸리는 작업을 요청하고, 진행 중 피드백을 받고, 중간에 취소할 수 있는 통신 방식
- Client는 목표(Goal)를 보내고, Server는 작업 중 진행 상황(Feedback)과 완료 후 결과(Result)를 보냄
- .action 파일에 Goal, Result, Feedback 세 부분을 ---로 나눠 정의
- 내부적으로 Topic과 Service를 조합해 동작
- 예시: 목적지까지 주행 (주행 중 남은 거리를 받고, 도착하면 결과 수신)

### 2.4 Service vs Action

| 항목 | Service | Action |
| --- | --- | --- |
| 통신 방식 | 요청 1번 → 응답 1번 | 목표 → 피드백 여러 번 → 결과 |
| 주요 목적 | 짧은 일회성 작업 | 오래 걸리는 작업 |
| 중간 진행 상태 | 없음 | 피드백으로 전달 |
| 작업 취소 | 지원 안 함 | 지원 |
| 인터페이스 확장자 | .srv | .action |

### 2.5 사용자 정의 인터페이스 (Custom Interface)
- ROS 2 노드 간 통신에 사용되는 데이터 구조를 정의
- 기본 인터페이스로 표현하기 어려운 데이터를 사용자 정의 가능
- 정의된 인터페이스는 빌드 후 여러 노드에서 재사용 가능
- 인터페이스 패키지는 ament_cmake로 만들고, CMakeLists.txt에 등록한 파일만 Python·C++ 코드로 생성됨
- 인터페이스 패키지를 먼저 빌드하고 source 해야 다른 패키지에서 import 가능

## 3. 실습

### 3.1 1_pubsub: launch 구성

```
publisher ── /demo_topic (String) ──▶ processor
processor ── /processed_delta (String 안의 JSON) ──▶ subscriber
```

- publisher: 키보드로 입력한 w/a/s/d 발행
- processor: 입력을 x/y 이동량 딕셔너리로 변환한 뒤 JSON 문자열로 발행
- subscriber: JSON을 딕셔너리로 복원해 현재 위치에 더함

#### setup.py 수정
수업가이드 틀의 data_files 항목은 요소가 세 개이고 launch 파일을 가리키지 않아서, launch 폴더의 파일을 설치하도록 고쳤습니다.

```python
# 수정 전
('share/' + package_name, '/launch', ['package.xml']),
# 수정 후
('share/' + package_name + '/launch', glob('launch/*.launch.py')),
```

#### my_location.launch.py
세 ExecuteProcess의 ros2 run pkg_name exec_name을 실제 실행 파일(publisher, processor, subscriber)로 바꾸고, publisher가 시작되면 processor, processor가 시작되면 subscriber가 실행되도록 이벤트 핸들러를 등록했습니다.

```python
return LaunchDescription([
    RegisterEventHandler(
        OnProcessStart(target_action=publisher, on_start=[processor])
    ),
    RegisterEventHandler(
        OnProcessStart(target_action=processor, on_start=[subscriber])
    ),
    publisher,
])
```

- 각 노드는 gnome-terminal로 별도 창에서 실행 (publisher가 키보드 입력을 받아야 하기 때문)
- package.xml에 launch 실행 의존성(exec_depend) 추가

#### 결과
- 빌드 성공, launch 실행 후 /publisher, /processor, /subscriber 노드 확인
- /demo_topic, /processed_delta 모두 발행자 1개·구독자 1개 연결 확인

| 입력 | processor 발행 이동량 | subscriber 누적 위치 |
| --- | --- | --- |
| w | {"x": 0, "y": 1} | (0, 1) |
| a | {"x": -1, "y": 0} | (-1, 1) |
| s | {"x": 0, "y": -1} | (-1, 0) |
| d | {"x": 1, "y": 0} | (0, 0) |

#### 분석
- OnProcessStart가 감지하는 것은 gnome-terminal 프로세스의 시작이라, 노드 준비나 토픽 연결까지 보장하지 않음
  - 실제로 gnome-terminal 프로세스가 종료된 뒤에도 창 안의 노드는 계속 실행 중이었음 → 노드 상태는 ros2 node list, ros2 topic info로 확인
- publisher의 input()이 입력을 기다리는 동안 콜백이 멈추므로, 타이머가 있어도 메시지가 1초마다 자동 발행되지 않음

### 3.2 custom_interfaces

```
# msg/Num.msg
int64 my_num

# srv/AddTwoInts.srv
custom_interfaces/Num a
custom_interfaces/Num b
---
custom_interfaces/Num sum

# msg/RobotPose.msg
float64 x
float64 y
float64 theta

# srv/MoveRobot.srv
RobotPose current_pose
float64 distance
---
RobotPose result_pose
bool success
```

- AddTwoInts의 a, b, sum은 Num 메시지 객체라서 request.a.my_num처럼 내부 필드까지 접근해야 값을 읽고 씀
- CMakeLists.txt: 수업가이드 14절 틀의 rosidl_generate_interfaces()에 위 네 파일 등록
- package.xml: 수업가이드 15절의 rosidl 관련 세 줄 추가, 2_srvmsg의 package.xml에는 custom_interfaces 의존성 추가

### 3.3 AddTwoInts 서비스

```
client: 요청 객체 작성 → /add_two_ints → server: 덧셈 처리
client: 응답 결과 출력 ← /add_two_ints ← server: 응답 객체 반환
```

- 서버: /add_two_ints 서비스를 열고, 요청이 오면 a와 b를 더해 응답에 담아 반환. spin()으로 다음 요청 대기
- 클라이언트: 서버를 기다린 뒤 요청을 보내고(call_async), 응답이 올 때까지 기다렸다가 결과 출력 후 종료
- 클라이언트에 남아 있던 .num을 실제 필드 이름인 .my_num으로 수정
- setup.py에 실행 파일 server, client 등록

| 요청 a, b | 클라이언트 출력 |
| --- | --- |
| 3, 5 | Sum: 8 |
| 0, 0 | Sum: 0 |
| -2, 5 | Sum: 3 |

### 3.4 MoveRobot 과제
현재 자세 (x, y, θ)와 거리 d를 받아, 현재 방향으로 직진했을 때의 도착 자세와 완료 여부를 반환합니다. θ는 라디안입니다.

```python
pose = request.current_pose
response.result_pose.x = pose.x + request.distance * math.cos(pose.theta)
response.result_pose.y = pose.y + request.distance * math.sin(pose.theta)
response.result_pose.theta = pose.theta   # 직진이므로 방향 유지
response.success = True
```

#### 분석
- 예: (1, 2, 0)에서 거리 3 → (4, 2, 0)
- θ = 0이면 이동량이 모두 x에 더해지고, θ = π/2이면 모두 y에 더해짐
- success는 항상 True → 실제 주행 없이 좌표만 계산하므로 계산 완료를 의미
- 실제 로봇에 적용하면 주행 중 진행 상황 확인과 취소가 필요하므로 Action으로 바꾸는 편이 적합

## 4. 실행 방법

```bash
source /opt/ros/jazzy/setup.bash
cd ~/robot_navigation/colcon_ws

# 인터페이스 패키지를 먼저 빌드
colcon build --packages-select custom_interfaces
source install/setup.bash
colcon build --packages-select 1_pubsub 2_srvmsg
source install/setup.bash

# launch 실습
ros2 launch 1_pubsub my_location.launch.py

# AddTwoInts (터미널 2개)
ros2 run 2_srvmsg server
ros2 run 2_srvmsg client 3 5

# MoveRobot (터미널 2개)
ros2 run 2_srvmsg move_robot_server
ros2 run 2_srvmsg move_robot_client 1 2 0 3
```

- 2_srvmsg 폴더의 파이썬 파일은 colcon_ws/src/2_srvmsg/2_srvmsg/에 넣고, setup.py에 실행 파일 네 개(server, client, move_robot_server, move_robot_client)를 등록
- 노드끼리 같은 ROS_DOMAIN_ID를 사용해야 함

## 5. 필수 의존성
- Ubuntu 24.04, ROS2 Jazzy, Python 3.12
- ROS2 패키지: rclpy, std_msgs, launch, rosidl_default_generators, rosidl_default_runtime
- custom_interfaces (직접 만드는 인터페이스 패키지)

## 6. 파일 구조
- 2_srvmsg/
    - add_ints_server.py, add_ints_client.py: AddTwoInts 서버·클라이언트
    - move_robot_server.py, move_robot_client.py: MoveRobot 서버·클라이언트
