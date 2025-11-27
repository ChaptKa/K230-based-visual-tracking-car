import time
import os
import sys
import gc
from media.sensor import *
from media.display import *
from media.media import *
from machine import UART, FPIOA

sensor = None
uart = None

Width = 640
Height = 480

try:
    fpioa = FPIOA()
    fpioa.set_function(11, FPIOA.UART2_TXD)
    fpioa.set_function(12, FPIOA.UART2_RXD)
    uart = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

    sensor = Sensor(width=Width, height=Height)
    sensor.reset()
    sensor.set_framesize(width=Width, height=Height, chn=CAM_CHN_ID_0)
    sensor.set_pixformat(Sensor.RGB565, chn=CAM_CHN_ID_0)

    Display.init(Display.LT9611, to_ide=True)
    MediaManager.init()
    sensor.run()

    clock = time.clock()

    # --- ROI 定义 ---
    ROI_BOTTOM = (0, 240, 640, 240)
    ROI_LEFT  = (0, 180, 100, 100)
    ROI_RIGHT = (540, 180, 100, 100)
    ROI_TOTAL_AREA = ROI_LEFT[2] * ROI_LEFT[3]

    # --- 阈值与变量 ---
    ROI_IF = 0.20
    pixel_data = []
    fps_count = 30
    pixel_base = 3000
    pixel_if_large = 4500

    # 等待启动信号
    waiting_for_start = True

    while waiting_for_start:
        img = sensor.snapshot(chn=CAM_CHN_ID_0)

        # 检查串口是否有数据
        if uart.any():
            read_data = uart.read()
            if read_data and (b'START' in read_data):
                waiting_for_start = False
                print("收到启动信号")
        if waiting_for_start:
            img.draw_string_advanced(160, 200, 40, "Waiting for Start...", color=(255, 0, 0))
            img.draw_string_advanced(180, 250, 30, "Send '1' to go", color=(255, 0, 0))

        Display.show_image(img)
        gc.collect()

    if uart.any():
        uart.read()

    # 开机校准阶段
    for i in range(fps_count):
        img = sensor.snapshot(chn=CAM_CHN_ID_0)
        img_gray = img.to_grayscale(copy=True)

        hist = img_gray.get_histogram(roi=ROI_BOTTOM)
        thresh = hist.get_threshold().value()
        blobs = img_gray.find_blobs([(0, thresh)], roi=ROI_BOTTOM, pixels_threshold=200, merge=True)

        if blobs:
            max_b = max(blobs, key=lambda b: b.pixels())
            pixel_data.append(max_b.pixels())
            img.draw_string_advanced(200, 200, 40, f"Calibrating: {int(i/fps_count*100)}%", color=(0, 255, 0))
            img.draw_rectangle(max_b.rect(), color=(0, 255, 0))

        Display.show_image(img)
        del img_gray
        gc.collect()

    if len(pixel_data) > 0:
        pixel_base = sum(pixel_data) / len(pixel_data)
        pixel_if_large = pixel_base * 1.10
        print(f"校准完成! 基准值: {pixel_base}, 触发阈值: {pixel_if_large}")
    else:
        print("校准失败！使用默认值")

    clock = time.clock()

    while True:
        clock.tick()

        # 第一阶段：图像获取
        img_original = sensor.snapshot(chn=CAM_CHN_ID_0)
        img_proc = img_original.to_grayscale(copy=True)
        # 自动阈值计算
        histogram = img_proc.get_histogram(roi=ROI_BOTTOM)
        thresh_val = histogram.get_threshold().value()
        auto_thresholds = [(0, thresh_val)]
        # 查找关键区域色块
        blobs = img_proc.find_blobs(auto_thresholds, pixels_threshold=200, area_threshold=200, merge=True, margin=10, roi=ROI_BOTTOM)
        blobs_left = img_proc.find_blobs(auto_thresholds, roi=ROI_LEFT, pixels_threshold=10, merge=True)
        blobs_right = img_proc.find_blobs(auto_thresholds, roi=ROI_RIGHT, pixels_threshold=10, merge=True)

        # 第二阶段：决策数据处理
        max_blob = None
        error = 0
        mode = 0
        current_pixels = 0
        is_blob_large = False
        # 计算主赛道参数
        if blobs:
            max_blob = max(blobs, key=lambda b: b.pixels())
            current_pixels = max_blob.pixels()

            target_x = Width / 2
            current_x = max_blob.cx()
            error = current_x - target_x
            if current_pixels > pixel_if_large:
                is_blob_large = True
        else:
            error = 0
        # 计算左右路口特征
        left_total_pixels = sum(b.pixels() for b in blobs_left) if blobs_left else 0
        right_total_pixels = sum(b.pixels() for b in blobs_right) if blobs_right else 0

        left_ratio = left_total_pixels / ROI_TOTAL_AREA
        right_ratio = right_total_pixels / ROI_TOTAL_AREA

        is_left_active = (left_ratio >= ROI_IF)
        is_right_active = (right_ratio >= ROI_IF)
        # 强行触发
        is_strong_left  = left_ratio > 0.7
        is_strong_right = right_ratio > 0.7

        # 核心判断逻辑
        # 十字路口 / T字路口
        if (is_left_active and is_right_active and is_blob_large) or (is_strong_left and is_strong_right):
            mode = 3
        # 左岔路
        elif (is_left_active and is_blob_large) or is_strong_left:
            mode = 1
        # 右岔路
        elif (is_right_active and is_blob_large) or is_strong_right:
            mode = 2
        # 直行
        elif blobs:
            mode = 0
        # 丢失目标
        else:
            mode = 4

        # 第三阶段：通信
        if max_blob:
            uart.write(f"#{int(error)},{int(mode)}!")
        else:
            # 丢失目标时 mode 发 4
            uart.write("#0,4!")

        # 第四阶段：调试显示
        # 绘制 ROI 状态框
        color_l = (255, 0, 0) if is_left_active else (255, 255, 0)
        color_r = (255, 0, 0) if is_right_active else (255, 255, 0)
        img_original.draw_rectangle(ROI_LEFT, color=color_l, thickness=3)
        img_original.draw_rectangle(ROI_RIGHT, color=color_r, thickness=3)
        # 绘制数据信息
        if max_blob:
            img_original.draw_rectangle(max_blob.rect(), color=(0, 255, 0), thickness=4)
            img_original.draw_cross(max_blob.cx(), max_blob.cy(), color=(255, 0, 0), thickness=2)

            color_debug = (255, 0, 0) if (current_pixels > pixel_if_large) else (0, 255, 0)
            img_original.draw_string_advanced(ROI_LEFT[0], ROI_LEFT[1] - 30, 30, f"{left_ratio:.2f}", color=color_l)
            img_original.draw_string_advanced(ROI_RIGHT[0], ROI_RIGHT[1] - 30, 30, f"{right_ratio:.2f}", color=color_r)

            # 显示模式
            mode_str = "Straight"
            if mode == 1: mode_str = "Left"
            elif mode == 2: mode_str = "Right"
            elif mode == 3: mode_str = "CROSS/T"

            img_original.draw_string_advanced(10, 10, 30, f"Err: {int(error)} Mode:{mode}", color=(255, 0, 0))
            img_original.draw_string_advanced(10, 50, 40, f"Type: {mode_str}", color=(0, 0, 255))
            img_original.draw_string_advanced(10, 90, 30, f"Cur:{current_pixels} Lim:{int(pixel_if_large)}", color=color_debug)
        else:
            img_original.draw_string_advanced(10, 10, 40, "LOST", color=(255, 0, 0))

        # 第五阶段：资源清理释放
        del img_proc
        print(f"FPS = {clock.fps()}")
        img_original.compressed_for_ide()
        Display.show_image(img_original)
        gc.collect()
        os.exitpoint()

except KeyboardInterrupt as e:
    print("用户停止:", e)
except BaseException as e:
    print(f"异常: {e}")
finally:
    if isinstance(sensor, Sensor):
        sensor.stop()
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()
