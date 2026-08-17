#////. this will classify fist and plam. fist is stearing wheel and palm is reverse. 
# ###upload the esp code, then esp will throw a wifi, connect the laptop to that network, then run this python script and the data will be sent to your esp 32 in real time

import cv2
import math
import socket
import numpy as np
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# ==================== CONFIGURATION ====================
ESP32_IP = "192.168.4.1"   # ESP32 Access Point IP
UDP_PORT = 4210            # Match ESP32 UDP Listener Port
MODEL_PATH = "hand_landmarker.task"
WINDOW_NAME = "ESP32 Bulletproof Gesture Steering"
STEERING_THRESHOLD = 12.0  # Steering deadzone threshold in degrees
# =======================================================

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
current_cmd = "S"

def send_udp_cmd(cmd):
    """Sends immediate zero-latency UDP command to the ESP32."""
    global current_cmd
    if cmd != current_cmd:
        current_cmd = cmd
        try:
            sock.sendto(cmd.encode(), (ESP32_IP, UDP_PORT))
            print(f"[UDP SENT] -> {cmd}")
        except Exception as e:
            print(f"[UDP ERROR] -> {e}")

# MediaPipe Hand Landmarker Initializer
base_options = python.BaseOptions(model_asset_path=MODEL_PATH)
options = vision.HandLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_hands=2,
    min_hand_detection_confidence=0.4,
    min_tracking_confidence=0.4
)
landmarker = vision.HandLandmarker.create_from_options(options)

def classify_hand_shape(landmarks):
    """
    Bulletproof Hand Classification using 3D Euclidean Distances.
    Measures finger extension relative to palm size.
    """
    wrist = np.array([landmarks[0].x, landmarks[0].y, landmarks[0].z])
    mcp_middle = np.array([landmarks[9].x, landmarks[9].y, landmarks[9].z])
    
    # Palm reference size (Wrist to Middle MCP joint)
    palm_size = np.linalg.norm(mcp_middle - wrist)
    if palm_size == 0:
        return "UNKNOWN"

    # Fingertips: Index (8), Middle (12), Ring (16), Pinky (20)
    tips = [8, 12, 16, 20]
    distances = []

    for tip_idx in tips:
        tip_pt = np.array([landmarks[tip_idx].x, landmarks[tip_idx].y, landmarks[tip_idx].z])
        dist = np.linalg.norm(tip_pt - wrist)
        distances.append(dist / palm_size)  # Normalized ratio relative to palm

    avg_tip_ratio = np.mean(distances)

    # Threshold Classification:
    # Extended Palm: Ratio usually ~1.3 to 1.8+
    # Closed Fist: Ratio usually ~0.6 to 1.0 (even when tilted towards camera)
    if avg_tip_ratio > 1.25:
        return "PALM"
    elif avg_tip_ratio < 1.10:
        return "FIST"
    
    return "UNKNOWN"

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
cv2.setWindowProperty(WINDOW_NAME, cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

smoothed_angle = 0.0
alpha = 0.30  # Steering smoothing coefficient

print("Starting ESP32 Gesture Controller... Press 'q' or 'ESC' to exit.")

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.flip(frame, 1)
    h, w, _ = frame.shape

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
    results = landmarker.detect(mp_image)

    status_text = "STOP (DETECTION FAILED)"
    command_to_send = "S"
    num_hands = len(results.hand_landmarks) if results.hand_landmarks else 0

    # MUST DETECT BOTH HANDS FOR ACTION; OTHERWISE IMMEDIATE SAFETY STOP
    if num_hands == 2:
        hand1_lm = results.hand_landmarks[0]
        hand2_lm = results.hand_landmarks[1]

        # Get Wrist Screen Coordinates
        w1_x, w1_y = int(hand1_lm[0].x * w), int(hand1_lm[0].y * h)
        w2_x, w2_y = int(hand2_lm[0].x * w), int(hand2_lm[0].y * h)

        # Sort left hand vs right hand by X-position in screen frame
        if w1_x < w2_x:
            lh_lm, rh_lm = hand1_lm, hand2_lm
            lx, ly, rx, ry = w1_x, w1_y, w2_x, w2_y
        else:
            lh_lm, rh_lm = hand2_lm, hand1_lm
            lx, ly, rx, ry = w2_x, w2_y, w1_x, w1_y

        left_state = classify_hand_shape(lh_lm)
        right_state = classify_hand_shape(rh_lm)

        # 1. BOTH FISTS -> STEERING MODE
        if left_state == "FIST" and right_state == "FIST":
            dy = ry - ly
            dx = rx - lx
            raw_angle = math.degrees(math.atan2(dy, dx))

            smoothed_angle = (alpha * raw_angle) + ((1 - alpha) * smoothed_angle)

            # Visual steering line between wrists
            cv2.line(frame, (lx, ly), (rx, ry), (0, 255, 0), 4, cv2.LINE_AA)
            cv2.circle(frame, (lx, ly), 12, (255, 0, 0), -1)
            cv2.circle(frame, (rx, ry), 12, (0, 0, 255), -1)

            if smoothed_angle < -STEERING_THRESHOLD:
                command_to_send = "L"
                status_text = "STEERING LEFT"
            elif smoothed_angle > STEERING_THRESHOLD:
                command_to_send = "R"
                status_text = "STEERING RIGHT"
            else:
                command_to_send = "F"
                status_text = "DRIVING FORWARD"

        # 2. BOTH PALMS -> REVERSE MODE
        elif left_state == "PALM" and right_state == "PALM":
            command_to_send = "B"
            status_text = "BOTH PALMS (REVERSE)"

            cv2.circle(frame, (lx, ly), 15, (0, 255, 255), -1)
            cv2.circle(frame, (rx, ry), 15, (0, 255, 255), -1)

        # 3. MISMATCHED GESTURES -> SAFETY STOP
        else:
            command_to_send = "S"
            status_text = f"STOP (L: {left_state}, R: {right_state})"

        # Render Skeletal Landmarks
        for lm in [lh_lm, rh_lm]:
            for pt in lm:
                cv2.circle(frame, (int(pt.x * w), int(pt.y * h)), 3, (200, 200, 200), -1)

    else:
        command_to_send = "S"
        status_text = "STOP (NEED BOTH HANDS)"

    # Transmit UDP Command
    send_udp_cmd(command_to_send)

    # Render Visual Overlay
    color = (0, 0, 255) if command_to_send == "S" else (0, 255, 0)
    cv2.putText(frame, f"STATUS: {status_text}", (40, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 3, cv2.LINE_AA)
    cv2.putText(frame, f"CMD: {command_to_send}", (40, 110), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (255, 255, 255), 3, cv2.LINE_AA)

    cv2.imshow(WINDOW_NAME, frame)

    key = cv2.waitKey(1) & 0xFF
    if key == ord('q') or key == 27:
        send_udp_cmd("S")
        break

cap.release()
cv2.destroyAllWindows()
sock.close()