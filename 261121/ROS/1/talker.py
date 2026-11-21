import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import time

class Talker(Node): # 
    def __init__(self):
        super().__init__('talker') # 실제 노드 이름은 여기 적음 - 위 클래스명은 파이썬에서만 적용
        self.publisher_ = self.create_publisher(String, '/demo_topic', 10) # 퍼블리셔 생성 - 옵션에 메시지 타입, 토픽 이름, 메모리 버퍼 인수를 줌
        self.start_time = time.time()
        self.timer = self.create_timer(1.0, self.publish_message) # hz, callback function을 넘김

    def publish_message(self):
        elapsed = int(time.time() - self.start_time)
        msg = String()
        msg.data = f"Hello ROS2: {elapsed}"
        self.publisher_.publish(msg)
        self.get_logger().info(f'Publishing: "{msg.data}"')

def main():
    rclpy.init()
    node = Talker()
    rclpy.spin(node) # while loop 기능을 함
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
