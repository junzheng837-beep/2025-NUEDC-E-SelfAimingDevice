# 融合版：全屏 800x480 矩形识别 + 云台激光控制（K230端PID版）
# 架构：K230负责"看+算"，MCU只负责"转发给驱动器"
# 发送协议：AA BB X_L X_H Y_L Y_H CC（RPM，int16小端）
import time, os, gc, math, struct
from math import sqrt
from media.sensor import *
from media.display import *
from media.media import *
import cv_lite
from time import ticks_ms
from machine import UART
from machine import FPIOA
from machine import Pin

# ---------- 硬件初始化 ----------
fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)
fpioa.set_function(53, FPIOA.GPIO53)   # 单按键
fpioa.set_function(33, FPIOA.GPIO33)   # 激光

uart = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

key = Pin(53, Pin.IN, Pin.PULL_DOWN)
laser_pin = Pin(33, Pin.OUT)

# 激光引脚低电平有效：0=开启，1=关闭
LASER_ON = 0
LASER_OFF = 1
laser_pin.value(LASER_OFF)

# 单击检测参数
CLICK_TIMEOUT = 400

# 模式常量
MODE_IDLE = 0
MODE_TRACK = 4

# ========== 识别配置 ==========
DETECT_WIDTH = 320
DETECT_HEIGHT = 240
image_shape = [DETECT_HEIGHT, DETECT_WIDTH]

# 显示分辨率
DISPLAY_WIDTH = 800
DISPLAY_HEIGHT = 480

SCALE_X = DISPLAY_WIDTH / DETECT_WIDTH
SCALE_Y = DISPLAY_HEIGHT / DETECT_HEIGHT

# Canny边缘检测参数（极低阈值，提高灵敏度）
CANNY_THRESH1_BASE = 5
CANNY_THRESH2_BASE = 20
APPROX_EPSILON = 0.15
AREA_MIN_RATIO = 0.00005
MAX_ANGLE_COS = 0.9
GAUSSIAN_BLUR_SIZE = 3
S_THRESHOLD = 30
MIN_EDGE_LENGTH = 3
SIDE_TOLERANCE = 80
LENGTH_THRESHOLD = 200

# 激光/摄像头安装位置
INITIAL_LASER_X = 160
INITIAL_LASER_Y = 120

# ========== 抗干扰增强参数（融合开源代码） ==========
# 中心区域过滤（全图3/5，放宽以识别侧边目标）
CENTER_RATIO = 0.6
CENTER_W = int(DETECT_WIDTH * CENTER_RATIO)
CENTER_H = int(DETECT_HEIGHT * CENTER_RATIO)
CENTER_MIN_X = (DETECT_WIDTH - CENTER_W) // 2
CENTER_MAX_X = CENTER_MIN_X + CENTER_W
CENTER_MIN_Y = (DETECT_HEIGHT - CENTER_H) // 2
CENTER_MAX_Y = CENTER_MIN_Y + CENTER_H

# 长宽比过滤
MAX_ASPECT_RATIO_X10 = 30

# 动态ROI参数
ACQUIRE_RATIO = 0.75
ACQUIRE_W = int(DETECT_WIDTH * ACQUIRE_RATIO)
ACQUIRE_H = int(DETECT_HEIGHT * ACQUIRE_RATIO)
ACQUIRE_ROI = ((DETECT_WIDTH - ACQUIRE_W) // 2,
               (DETECT_HEIGHT - ACQUIRE_H) // 2,
               ACQUIRE_W, ACQUIRE_H)
# 动态ROI参数（每边扩展25像素）
ROI_PADDING = 25
MIN_ROI_W = 130  # 最小ROI，防止误识别
MIN_ROI_H = 130
MAX_ROI_W = 300
MAX_ROI_H = 220

# 最小矩形识别阈值（小于此尺寸不识别）
MIN_RECT_WIDTH = 65
MIN_RECT_HEIGHT = 65

# 增强识别参数（激进放宽）
MIN_AREA = 50
MAX_AREA = 80000
MAX_ASPECT_RATIO_X10 = 50
MIN_RECT_DENSITY = 0.4

# 防跳变参数
EMA_NUM = 1
EMA_DEN = 2
MAX_POSITION_JUMP = 60
HARD_POSITION_JUMP = 120
MAX_AREA_CHANGE_PCT = 100
HARD_AREA_CHANGE_PCT = 200
MAX_LOST_HOLD = 15

# ========== PID控制器 ==========
class PID:
    def __init__(self, kp, ki, kd, integral_limit=0.15, output_limit=50.0):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.integral_limit = integral_limit
        self.output_limit = output_limit
        self.integral = 0.0
        self.prev_error = 0.0
        self.prev_time = None

    def update(self, error):
        now = ticks_ms()
        if self.prev_time is None:
            self.prev_time = now
            output = self.kp * error
            return max(-self.output_limit, min(self.output_limit, output))
        dt = ((now - self.prev_time) & 0xFFFFFFFF) / 1000.0
        if dt <= 0 or dt > 1.0:
            dt = 0.02
        self.prev_time = now

        self.integral += error * dt
        max_integral = self.integral_limit * self.output_limit
        if self.integral > max_integral:
            self.integral = max_integral
        elif self.integral < -max_integral:
            self.integral = -max_integral

        derivative = (error - self.prev_error) / dt if dt > 0 else 0
        self.prev_error = error

        output = self.kp * error + self.ki * self.integral + self.kd * derivative
        if output > self.output_limit:
            output = self.output_limit
        elif output < -self.output_limit:
            output = -self.output_limit
        return output

    def reset(self):
        self.integral = 0.0
        self.prev_error = 0.0
        self.prev_time = ticks_ms()


# ==================== 协议发送 ====================
def send_speed_rpm(x_rpm, y_rpm):
    x_rpm = int(max(-200, min(200, round(x_rpm))))
    y_rpm = int(max(-200, min(200, round(y_rpm))))
    uart.write(struct.pack('<BBhhB', 0xAA, 0xBB, x_rpm, y_rpm, 0xCC))


# ==================== 辅助函数 ====================
def make_roi(cx, cy, w, h):
    w = max(8, min(int(w), DETECT_WIDTH))
    h = max(8, min(int(h), DETECT_HEIGHT))
    x = int(cx - w // 2)
    y = int(cy - h // 2)
    if x < 0:
        x = 0
    elif x + w > DETECT_WIDTH:
        x = DETECT_WIDTH - w
    if y < 0:
        y = 0
    elif y + h > DETECT_HEIGHT:
        y = DETECT_HEIGHT - h
    return (x, y, w, h)

def get_search_roi(last_cx, last_cy, prev_rect, lost_hold_count):
    # 有上一帧矩形时，ROI = 矩形大小 + 每边扩展25像素
    if last_cx is not None and prev_rect is not None:
        prev_w = prev_rect[2]
        prev_h = prev_rect[3]

        roi_w = max(MIN_ROI_W, prev_w + ROI_PADDING * 2)
        roi_h = max(MIN_ROI_H, prev_h + ROI_PADDING * 2)

        if lost_hold_count > 0:
            roi_w += lost_hold_count * 30
            roi_h += lost_hold_count * 20

        return make_roi(last_cx, last_cy, roi_w, roi_h)
    return ACQUIRE_ROI

def _norm_angle_diff(theta1, theta2):
    diff = abs(theta1 - theta2) % 180
    if diff > 90:
        diff = 180 - diff
    return diff

def are_segments_parallel(theta1, theta2, tolerance=35):
    return _norm_angle_diff(theta1, theta2) <= tolerance

def are_segments_vertical(theta1, theta2, tolerance=35):
    return abs(_norm_angle_diff(theta1, theta2) - 90) <= tolerance

def find_intersection(x1, y1, x2, y2, x3, y3, x4, y4):
    AB_x = x2 - x1
    AB_y = y2 - y1
    AC_x = x3 - x1
    AC_y = y3 - y1
    CD_x = x4 - x3
    CD_y = y4 - y3
    det = AB_x * CD_y - AB_y * CD_x
    if abs(det) < 1e-6:
        return None
    t = (AC_x * CD_y - AC_y * CD_x) / det
    intersection_x = x1 + t * AB_x
    intersection_y = y1 + t * AB_y
    return int(intersection_x), int(intersection_y)


# ==================== 摄像头初始化 ====================
def camera_init():
    global sensor
    sensor = Sensor()
    sensor.reset()
    sensor.set_framesize(chn=CAM_CHN_ID_0, width=DETECT_WIDTH, height=DETECT_HEIGHT)
    sensor.set_pixformat(Sensor.RGB888, chn=CAM_CHN_ID_0)
    sensor.set_framesize(chn=CAM_CHN_ID_1, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT)
    sensor.set_pixformat(Sensor.RGB888, chn=CAM_CHN_ID_1)
    Display.init(Display.ST7701, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, fps=120, to_ide=True)
    MediaManager.init()
    sensor.run()

def camera_deinit():
    global sensor
    sensor.stop()
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()


# ==================== 主循环 ====================
def main_loop():
    fps = time.clock()

    # PID控制器
    pid_yaw = PID(kp=0.38, ki=0.05, kd=0.0, integral_limit=0.5, output_limit=30.0)
    pid_pitch = PID(kp=0.38, ki=0.05, kd=0.0, integral_limit=0.5, output_limit=30.0)

    target_x = INITIAL_LASER_X
    target_y = INITIAL_LASER_Y

    flag = MODE_IDLE
    current_target_center = None
    last_locked_center = None

    # 追踪状态
    last_cx, last_cy = None, None
    last_area = 0
    last_w, last_h = 0, 0
    lost_hold_count = 0
    smooth_cx, smooth_cy = None, None
    prev_rect = None

    # EMA平滑
    MAX_POSITION_JUMP_SQ = MAX_POSITION_JUMP * MAX_POSITION_JUMP
    HARD_POSITION_JUMP_SQ = HARD_POSITION_JUMP * HARD_POSITION_JUMP

    # 掉帧惯性滑行
    last_yaw_rpm = 0.0
    last_pitch_rpm = 0.0
    lost_frame_cnt = 0
    LOST_DECAY = 0.85
    LOST_MAX_FRAMES = 8

    # 单击检测
    key_last = 0
    click_count = 0
    first_click_time = 0

    frame_cnt = 0

    print("系统启动（电赛E题抗干扰增强版）")
    while True:
        fps.tick()
        try:
            os.exitpoint()
            img_det = sensor.snapshot(chn=CAM_CHN_ID_0)
            img = sensor.snapshot(chn=CAM_CHN_ID_1)

            # ---------- 矩形检测 ----------
            img_np = img_det.to_numpy_ref()

            rects = cv_lite.rgb888_find_rectangles_with_corners(
                image_shape, img_np,
                CANNY_THRESH1_BASE, CANNY_THRESH2_BASE,
                APPROX_EPSILON,
                AREA_MIN_RATIO,
                MAX_ANGLE_COS,
                GAUSSIAN_BLUR_SIZE
            )

            # ---------- 动态ROI（用上一帧的矩形） ----------
            roi = get_search_roi(
                last_cx if last_cx is not None else DETECT_WIDTH // 2,
                last_cy if last_cy is not None else DETECT_HEIGHT // 2,
                prev_rect,
                lost_hold_count
            )

            # ---------- 多目标筛选 + 得分机制 ----------
            candidate_rects = []
            best_rect = None
            best_score = None
            best_cx, best_cy = 0, 0
            best_area = 0
            best_w, best_h = 0, 0

            if rects:
                for r in rects:
                    # ROI过滤
                    rx, ry = r[4], r[5]
                    if not (roi[0] <= rx <= roi[0] + roi[2] and
                            roi[1] <= ry <= roi[1] + roi[3]):
                        continue

                    area = r[2] * r[3]

                    # 面积范围过滤
                    if area < MIN_AREA or area > MAX_AREA:
                        continue

                    corners = [(r[4], r[5]), (r[6], r[7]), (r[8], r[9]), (r[10], r[11])]

                    # 简化几何验证：只检查基本条件
                    rw, rh = r[2], r[3]

                    # 小矩形过滤
                    if rw < MIN_RECT_WIDTH or rh < MIN_RECT_HEIGHT:
                        continue

                    # 长宽比过滤
                    if max(rw, rh) * 10 > min(rw, rh) * MAX_ASPECT_RATIO_X10:
                        continue

                    # 计算中心点
                    cx = (corners[0][0] + corners[1][0] + corners[2][0] + corners[3][0]) / 4
                    cy = (corners[0][1] + corners[1][1] + corners[2][1] + corners[3][1]) / 4

                    # 简化评分：基于面积
                    quality = area // 10

                    if flag == MODE_TRACK and last_cx is not None and last_area > 0:
                        dx0 = cx - last_cx
                        dy0 = cy - last_cy
                        dist_sq = dx0 * dx0 + dy0 * dy0
                        area_change_pct = abs(area - last_area) * 100 // last_area if last_area > 0 else 0
                        score = quality - dist_sq // 4 - area_change_pct * 20
                    else:
                        dx0 = cx - DETECT_WIDTH // 2
                        dy0 = cy - DETECT_HEIGHT // 2
                        center_dist_sq = dx0 * dx0 + dy0 * dy0
                        score = quality - center_dist_sq // 4

                    if best_score is None or score > best_score:
                        best_score = score
                        best_rect = r
                        best_cx, best_cy = cx, cy
                        best_area = area
                        best_w, best_h = rw, rh

            # ---------- 防跳变验证 ----------
            candidate_valid = best_rect is not None
            if candidate_valid and flag == MODE_TRACK and last_cx is not None and last_area > 0:
                dx0 = best_cx - last_cx
                dy0 = best_cy - last_cy
                dist_sq = dx0 * dx0 + dy0 * dy0
                area_change_pct = abs(best_area - last_area) * 100 // last_area if last_area > 0 else 0

                if (dist_sq > HARD_POSITION_JUMP_SQ or
                    area_change_pct > HARD_AREA_CHANGE_PCT or
                    (dist_sq > MAX_POSITION_JUMP_SQ and area_change_pct > MAX_AREA_CHANGE_PCT)):
                    candidate_valid = False

            # ---------- 状态更新 ----------
            if candidate_valid:
                if not (flag == MODE_TRACK) or smooth_cx is None:
                    smooth_cx, smooth_cy = best_cx, best_cy
                else:
                    smooth_cx = (EMA_NUM * best_cx + (EMA_DEN - EMA_NUM) * smooth_cx) // EMA_DEN
                    smooth_cy = (EMA_NUM * best_cy + (EMA_DEN - EMA_NUM) * smooth_cy) // EMA_DEN

                current_target_center = (int(smooth_cx), int(smooth_cy))
                last_locked_center = current_target_center
                lost_hold_count = 0
                last_area = best_area
                last_cx, last_cy = smooth_cx, smooth_cy
                last_w, last_h = best_w, best_h
                prev_rect = best_rect
            else:
                if flag == MODE_TRACK:
                    lost_hold_count += 1
                    if lost_hold_count >= MAX_LOST_HOLD:
                        current_target_center = None
                        smooth_cx, smooth_cy = None, None
                        last_cx, last_cy = None, None
                        last_area = 0
                        last_w, last_h = 0, 0
                        prev_rect = None
                else:
                    current_target_center = None

            # ---------- 绘制部分 ----------
            if best_rect is not None:
                dcx = int(best_cx * SCALE_X)
                dcy = int(best_cy * SCALE_Y)
                dc = [[int(best_rect[2*i+4]*SCALE_X), int(best_rect[2*i+5]*SCALE_Y)] for i in range(4)]
                for s in range(1, 5):
                    s0 = s - 1
                    s1 = s % 4
                    img.draw_line(dc[s0][0], dc[s0][1], dc[s1][0], dc[s1][1],
                                  color=(0, 255, 0), thickness=3)
                    img.draw_circle(dc[s0][0], dc[s0][1], 2,
                                    color=(0, 0, 255), fill=True, thickness=2)
                img.draw_circle(dcx, dcy, 5, color=(255, 0, 0), thickness=3)

            # 中心区域框（固定）
            img.draw_rectangle(int(CENTER_MIN_X * SCALE_X), int(CENTER_MIN_Y * SCALE_Y),
                              int(CENTER_W * SCALE_X), int(CENTER_H * SCALE_Y),
                              color=(0, 200, 0), thickness=1)

            # 动态ROI搜索区域（根据目标大小变化）
            roi_disp_x = int(roi[0] * SCALE_X)
            roi_disp_y = int(roi[1] * SCALE_Y)
            roi_disp_w = int(roi[2] * SCALE_X)
            roi_disp_h = int(roi[3] * SCALE_Y)
            img.draw_rectangle(roi_disp_x, roi_disp_y, roi_disp_w, roi_disp_h,
                              color=(200, 200, 0), thickness=2)
            img.draw_string_advanced(roi_disp_x + 5, roi_disp_y + 5, 16,
                                   f"ROI:{roi[2]}x{roi[3]}", color=(200, 200, 0))

            # 坐标显示
            if current_target_center is not None:
                img.draw_string_advanced(5, 40, 24,
                                         f"目标: ({current_target_center[0]},{current_target_center[1]})",
                                         color=(0, 255, 0))
            else:
                img.draw_string_advanced(5, 40, 24, "未锁定目标", color=(255,0,0))

            laser_disp_x = int(INITIAL_LASER_X * SCALE_X)
            laser_disp_y = int(INITIAL_LASER_Y * SCALE_Y)
            img.draw_string_advanced(5, 20, 24,
                                     f"激光: ({INITIAL_LASER_X},{INITIAL_LASER_Y})",
                                     color=(255,0,0))
            img.draw_cross(laser_disp_x, laser_disp_y, color=(255, 0, 0), size=20, thickness=2)

            img.draw_string_advanced(650, 0, 24, f"fps:{int(fps.fps())}", color=(0, 255, 0))

            # ---------- 按键检测 ----------
            key_now = key.value()
            if key_now == 1 and key_last == 0:
                now = ticks_ms()
                if click_count == 0:
                    first_click_time = now
                    click_count = 1
                elif now - first_click_time < CLICK_TIMEOUT:
                    click_count = 2
            key_last = key_now

            single_click = False
            if click_count == 1 and ticks_ms() - first_click_time > CLICK_TIMEOUT:
                single_click = True
                click_count = 0
            elif click_count == 2:
                click_count = 0

            # ---------- 模式处理 ----------
            if flag == MODE_IDLE:
                if single_click:
                    if current_target_center is not None:
                        flag = MODE_TRACK
                        laser_pin.value(LASER_ON)
                        pid_yaw.reset()
                        pid_pitch.reset()
                        send_speed_rpm(0, 0)
                        print("追踪模式启动")

            elif flag == MODE_TRACK:
                if current_target_center is not None:
                    cx, cy = current_target_center
                    err_x = cx - target_x
                    err_y = cy - target_y
                    if lost_frame_cnt > 0:
                        last_yaw_rpm = 0.0
                        last_pitch_rpm = 0.0
                    yaw_rpm = -pid_yaw.update(err_x)
                    pitch_rpm = pid_pitch.update(err_y)
                    send_speed_rpm(yaw_rpm, pitch_rpm)
                    last_yaw_rpm = yaw_rpm
                    last_pitch_rpm = pitch_rpm
                    lost_frame_cnt = 0
                else:
                    lost_frame_cnt += 1
                    if lost_frame_cnt <= LOST_MAX_FRAMES:
                        last_yaw_rpm *= LOST_DECAY
                        last_pitch_rpm *= LOST_DECAY
                        send_speed_rpm(last_yaw_rpm, last_pitch_rpm)
                    else:
                        send_speed_rpm(0, 0)

                if single_click:
                    flag = MODE_IDLE
                    laser_pin.value(LASER_OFF)
                    send_speed_rpm(0, 0)
                    last_yaw_rpm = 0
                    last_pitch_rpm = 0
                    pid_yaw.reset()
                    pid_pitch.reset()

            Display.show_image(img)

        except KeyboardInterrupt as e:
            print("user stop: ", e)
            break
        except Exception as e:
            print(f"Exception {e}")
            continue

        time.sleep_ms(1)
        frame_cnt += 1
        if frame_cnt % 30 == 0:
            gc.collect()

def main():
    os.exitpoint(os.EXITPOINT_ENABLE)
    camera_is_init = False
    try:
        print("camera init")
        camera_init()
        camera_is_init = True
        print("camera capture")
        main_loop()
    except Exception as e:
        print(f"Exception {e}")
    finally:
        if camera_is_init:
            send_speed_rpm(0, 0)
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
