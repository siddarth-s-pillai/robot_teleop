#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32, Bool
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint
import threading

class RotaryTeleop(Node):

    def __init__(self):
        super().__init__('rotary_teleop')

        # ----------------------------------
        # Robot joints (Updated to 7 joints)
        # ----------------------------------
        self.joint_names = [
            'joint_1',
            'joint_2',
            'joint_3',
            'joint_4',
            'joint_5',
            'joint_6',
            'joint_7'
        ]

        self.num_joints = len(self.joint_names)
        self.current_joint = 0

        # Initial positions all set to 0.0
        self.joint_positions = [0.0] * self.num_joints

        # ----------------------------------
        # Encoder state
        # ----------------------------------
        self.rotation_sum = 0
        self.encoder_direction = 0
        self.encoder_steps = 0

        self.step_scale = 0.002  # radians per encoder step
        self.last_button_state = False

        self.lock = threading.Lock()

        # ----------------------------------
        # Subscribers
        # ----------------------------------
        self.create_subscription(
            Int32,
            '/telemetry/encoder_steps',
            self.steps_cb,
            10
        )

        self.create_subscription(
            Int32,
            '/telemetry/encoder_direction',
            self.direction_cb,
            10
        )

        self.create_subscription(
            Bool,
            '/telemetry/encoder_button',
            self.button_cb,
            10
        )

        # ----------------------------------
        # Trajectory publisher
        # ----------------------------------
        self.traj_pub = self.create_publisher(
            JointTrajectory,
            '/joint_trajectory_controller/joint_trajectory',
            10
        )

        # ----------------------------------
        # Timers
        # ----------------------------------
        self.create_timer(0.01, self.fast_loop)     # 10 ms
        self.create_timer(0.06, self.control_loop)  # 60 ms

        self.get_logger().info('Rotary teleop node started with 7 joints')

    # ==========================================================
    # Telemetry callbacks
    # ==========================================================
    def steps_cb(self, msg: Int32):
        with self.lock:
            self.encoder_steps = msg.data

    def direction_cb(self, msg: Int32):
        with self.lock:
            self.encoder_direction = msg.data

    def button_cb(self, msg: Bool):
        with self.lock:
            # Detect rising edge (button press)
            if msg.data and not self.last_button_state:
                self.current_joint = (self.current_joint + 1) % self.num_joints
                self.get_logger().info(
                    f"Selected joint: {self.joint_names[self.current_joint]}"
                )
            self.last_button_state = msg.data

    # ==========================================================
    # 10 ms loop — accumulate encoder motion
    # ==========================================================
    def fast_loop(self):
        with self.lock:
            if self.encoder_steps != 0:
                self.rotation_sum += self.encoder_steps * self.encoder_direction
                self.encoder_steps = 0  # consume steps

    # ==========================================================
    # 60 ms loop — move robot
    # ==========================================================
    def control_loop(self):
        with self.lock:
            if self.rotation_sum == 0:
                return

            delta = self.rotation_sum * self.step_scale
            self.rotation_sum = 0

        # Update the position of the currently selected joint
        self.joint_positions[self.current_joint] += delta

        traj = JointTrajectory()
        traj.joint_names = self.joint_names

        point = JointTrajectoryPoint()
        point.positions = self.joint_positions
        point.time_from_start.sec = 1
        point.time_from_start.nanosec = 0

        traj.points.append(point)
        self.traj_pub.publish(traj)

def main():
    rclpy.init()
    node = RotaryTeleop()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
