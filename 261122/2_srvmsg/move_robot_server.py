import math
import rclpy
from rclpy.node import Node
from custom_interfaces.srv import MoveRobot


class MoveRobotServer(Node):
    def __init__(self):
        super().__init__('move_robot_server')
        self.srv = self.create_service(MoveRobot, '/move_robot', self.srv_callback)
        self.get_logger().info('MoveRobot Server 켜짐')

    def srv_callback(self, request, response):
        pose = request.current_pose
        response.result_pose.x = pose.x + request.distance * math.cos(pose.theta)
        response.result_pose.y = pose.y + request.distance * math.sin(pose.theta)
        response.result_pose.theta = pose.theta

        response.success = True

        self.get_logger().info(f'Incoming request: x={pose.x}, y={pose.y}, theta={pose.theta} rad, distance={request.distance}')
        result = response.result_pose
        self.get_logger().info(f'Result: x={result.x}, y={result.y}, theta={result.theta} rad, success={response.success}')
        return response


def main():
    rclpy.init()
    node = MoveRobotServer()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
