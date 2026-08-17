import cv2
import math
import os
import requests
import concurrent.futures
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# ==================== CONFIGURATION ====================
ESP32_IP = "http://192.168.4.1"  # ESP32 Access Point IP
MODEL_PATH = "gesture_recognizer.task"
WINDOW_NAME = "ESP32 Gesture Steering Controller"
# =======================================================

if not os.path.exists(MODEL_PATH):
    raise FileNotFoundError(
        f"Model file '{MODEL_PATH}' not found! Run: "
        "wget -O gesture_recognizer.task https://storage.googleapis.com/mediapipe-models/gesture_recognizer/gesture_recognizer/float16/1/gesture_recognizer.task"
    )

base_options = python.BaseOptions(model_asset_path=MODEL_PATH)
options = vision.GestureRecognizerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_hands=2,
    min_hand_detection_confidence=0.5,
    min_tracking_confidence=0.5
)
recognizer = vision.GestureRecognizer.create_from_options(options)

# Thread Pool for non-blocking Async HTTP requests
executor = concurrent.futures.ThreadPoolExecutor(max_workers=2)
current_cmd = "S"

def _async_get(url):
    try:
        requests.get(url, timeout=0.2)
    except Exception:
        pass

def send_command(cmd):
    """Dispatches HTTP requests to ESP32 without blocking the main UI loop."""
    global current_cmd
    if cmd != current_cmd:
        current_cmd = cmd
        print(f"[CMD UPDATE] -> {cmd}")
        executor.submit(_async_get, f"{ESP32_IP}/cmd?val={cmd}")

def is_fist_geometric(landmarks):
    tips = [8, 12, 16, 20]
    pips = [6, 10, 14, 18]
    folded = 0
    for tip, pip in zip(tips, pips):
        if landmarks[tip].y > landmarks[pip].y:
            folded += 1
    return folded >= 3

def is_palm_geometric(landmarks):
    tips = [8, 12, 16, 20]
    pips = [6, 10, 14, 18]
    extended = 0
    for tip, pip in zip(tips, pips):
        if landmarks[tip].y < landmarks[pip].y:
            extended += 1
    return extended >= 3

def draw_steering_wheel(frame, center, radius, angle, active=False):
    """Renders a sleek, responsive virtual steering wheel overlay."""
    color = (0, 255, 0) if active else (120, 120, 120)
    cx, cy = center

    # Outer and Inner Rim
    cv2.circle(frame, (cx, cy), radius, color, 4, cv2.LINE_AA)
    cv2.circle(frame, (cx, cy), int(radius * 0.25), color, 3, cv2.LINE_AA)

    # Rotating Spokes
    rad = math.radians(angle)
    spoke_angles = [rad, rad + math.pi * 2/3, rad - math.pi * 2/3]

    for a in spoke_angles:
        x_end = int(cx + radius * math.cos(a))
        y_end = int(cy + radius * math.sin(a))
        x_start = int(cx + (radius * 0.25) * math.cos(a))
        y_start = int(cy + (radius * 0.25) * math.sin(a))
        cv2.line(frame, (x_start, y_start), (x_end, y_end), color, 3, cv2.LINE_AA)

# Start Video Capture & Set Native HD Resolution
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

# Fullscreen Window Setup
cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
cv2.setWindowProperty(WINDOW_NAME, cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

# Smoothing Filter Memory
smoothed_angle = 0.0
alpha = 0.25  # Lower value = smoother rotation (0.1 to 0.3 recommended)

print("Starting ESP32 Gesture Controller... Press 'q' or 'ESC' to exit.")

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        print("Camera read error.")
        break

    frame = cv2.flip(frame, 1)
    h, w, _ = frame.shape

    # Default UI Steering Wheel Position
    center_x, center_y = w // 2, int(h * 0.60)
    wheel_radius = int(min(w, h) * 0.22)
    raw_angle = 0
    wheel_active = False

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
    results = recognizer.recognize(mp_image)

    status_text = "IDLE (STOP)"
    command_to_send = "S"

    if results.hand_landmarks and len(results.hand_landmarks) == 2:
        hand1_lm = results.hand_landmarks[0]
        hand2_lm = results.hand_landmarks[1]

        w1_x, w1_y = int(hand1_lm[0].x * w), int(hand1_lm[0].y * h)
        w2_x, w2_y = int(hand2_lm[0].x * w), int(hand2_lm[0].y * h)

        if w1_x < w2_x:
            lh_lm, rh_lm = hand1_lm, hand2_lm
            lx, ly, rx, ry = w1_x, w1_y, w2_x, w2_y
            g_left = results.gestures[0][0].category_name if results.gestures else "None"
            g_right = results.gestures[1][0].category_name if results.gestures else "None"
        else:
            lh_lm, rh_lm = hand2_lm, hand1_lm
            lx, ly, rx, ry = w2_x, w2_y, w1_x, w1_y
            g_left = results.gestures[1][0].category_name if results.gestures else "None"
            g_right = results.gestures[0][0].category_name if results.gestures else "None"

        left_is_fist = (g_left in ["Closed_Fist", "fist"]) or is_fist_geometric(lh_lm)
        right_is_fist = (g_right in ["Closed_Fist", "fist"]) or is_fist_geometric(rh_lm)
        left_is_palm = (g_left in ["Open_Palm", "palm"]) or is_palm_geometric(lh_lm)
        right_is_palm = (g_right in ["Open_Palm", "palm"]) or is_palm_geometric(rh_lm)

        dy = ry - ly
        dx = rx - lx
        raw_angle = math.degrees(math.atan2(dy, dx))

        # 1. BOTH FISTS -> STEERING WHEEL MODE
        if left_is_fist and right_is_fist:
            wheel_active = True
            center_x, center_y = (lx + rx) // 2, (ly + ry) // 2
            wheel_radius = int(math.hypot(dx, dy) / 2)

            if smoothed_angle < -15:
                command_to_send = "L"
                status_text = "STEERING LEFT (FORWARD)"
            elif smoothed_angle > 15:
                command_to_send = "R"
                status_text = "STEERING RIGHT (FORWARD)"
            else:
                command_to_send = "F"
                status_text = "DRIVING FORWARD"

        # 2. BOTH PALMS -> REVERSE MODE
        elif left_is_palm and right_is_palm:
            command_to_send = "B"
            status_text = "REVERSING (BACKWARD)"

        # Draw hand landmarks
        for lm in [lh_lm, rh_lm]:
            for pt in lm:
                cv2.circle(frame, (int(pt.x * w), int(pt.y * h)), 4, (0, 255, 255), -1, cv2.LINE_AA)

    # Exponential Moving Average Smoothing for Wheel Angle
    smoothed_angle = (alpha * raw_angle) + ((1 - alpha) * smoothed_angle)

    # Render Steering Wheel
    draw_steering_wheel(frame, (center_x, center_y), wheel_radius, smoothed_angle, active=wheel_active)

    # Send non-blocking command
    send_command(command_to_send)

    # Display HUD
    color = (0, 0, 255) if command_to_send == "S" else (0, 255, 0)
    cv2.putText(frame, f"STATUS: {status_text}", (40, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 1.1, color, 3, cv2.LINE_AA)
    cv2.putText(frame, f"CMD: {command_to_send}", (40, 115),
                cv2.FONT_HERSHEY_SIMPLEX, 1.1, (255, 255, 255), 3, cv2.LINE_AA)

    cv2.imshow(WINDOW_NAME, frame)

    key = cv2.waitKey(1) & 0xFF
    if key == ord('q') or key == 27:
        send_command("S")
        break

cap.release()
cv2.destroyAllWindows()
executor.shutdown(wait=False)