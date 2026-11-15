# 261115 TinkerCAD 시뮬레이터 실습 결과

> 작성자: 202035152 박민재

TinkerCAD 시뮬레이터 상에서 LED 전구를 버튼을 사용해서 점등시키는 코드 및 실행 결과 이미지입니다.


## 파일 구성
- 실습 이미지 및 동영상 파일
    - `led_press.png`
    - `led_toggle.webm`
    - `led_brightness.png`

## 실습 결과

### 1. 버튼 누르고 있을 때 LED 점등 실습

버튼을 누르고 있을 동안에만 LED가 점등되는 실습 코드입니다.

![버튼 누르고 있을때 LED 점등 실습 결과 이미지](./led_press.png)

```c
const int buttonPin = 2;


void setup()
{
  pinMode(buttonPin, INPUT);
  Serial.begin(115200);
}

void loop()
{
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == HIGH) {
   	Serial.println("Pressed"); 
  }
  else {
   	Serial.println("Not Pressed"); 
  }
  
  delay(50);
}
```

### 2. 버튼 누를 시 LED 토글 실습

버튼을 누르면 현재 켜짐-꺼짐 상태에 따라 LED가 토글되는 코드입니다.

https://github.com/user-attachments/assets/f6f3d37f-3914-4488-85d6-fa798d79d205

[원본 WebM 열기](./led_toggle.webm)

```c
const int ledPin = 7;
const int buttonPin = 2;
bool buttonFlag = false; // 버튼 토글 상태를 반영하기 위한 플래그

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Serial.begin(115200);
}

void loop()
{
  {
  int buttonState = digitalRead(buttonPin);
  
    
  if (buttonState == HIGH) { // 버튼 입력 시 플래그 전환
    Serial.println("Pressed"); 
    if (buttonFlag == true) { //True인 경우
    	buttonFlag = false;
      Serial.println("flip to false"); 
    } else { // False인 경우
     buttonFlag = true;
      Serial.println("flip to true"); 
    }
  }
  else {
   	Serial.println("Not Pressed"); 
  }
  
  Serial.println(buttonFlag);
  delay(50); // 버튼 입력이 여러번 중복 처리되는 것을 방지하기 위한 딜레이
    
    if (buttonFlag == true) { // 플래그에 따라 버튼 켜고 끄기
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
    
   delay(50);
}
}
```

### 3. 아날로그 가변저항값에 따라 LED 밝기 조절하기

가변 저항을 조절하는 다이얼의 저항값에 따라 LED의 밝기를 조절하는 실습입니다.

![](./led_brightness.png)

```c
const int potPin = A2;
const int ledPin = 9;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  int value = analogRead(potPin);
  int brightness = map(value, 0, 1023, 0, 255); // 0~255 범위로 매핑
  
  Serial.println(brightness);
  
  analogWrite(ledPin, brightness);
  
  delay(50);
  
}
```