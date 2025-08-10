#!/usr/bin/env python3
import sys,os,time,argparse
current_dir = os.path.dirname(os.path.abspath(__file__))
target_dir = os.path.abspath(os.path.join(current_dir, "../.."))
sys.path.append(target_dir)
from LinkerHand.linker_hand_api import LinkerHandApi
from LinkerHand.utils.color_msg import ColorMsg
'''
手掌握拳
'''
def main():
    parser = argparse.ArgumentParser(description='处理手势参数')
    parser.add_argument('--hand_type', choices=['left', 'right'], required=True, help='指定左手或右手')
    parser.add_argument('--hand_joint', required=True, help='指定LinkerHand型号')
    parser.add_argument('--can', default="can0", help='指定CAN编号')
    args = parser.parse_args()
    print(f"手类型: {args.hand_type}, 关节: {args.hand_joint}")

    hand_joint = args.hand_joint
    hand_type = args.hand_type
    can = args.can
    # 初始化API
    hand = LinkerHandApi(hand_joint=hand_joint,hand_type=hand_type,can=can)
    # 设置速度
    if hand_joint == "L7":
        speed = [120,250,250,250,250,250,250]
    else:
        speed = [120,250,250,250,250]
    hand.set_speed(speed=speed)
    ColorMsg(msg=f"当前为{hand_joint} {hand_type},设置速度为:{speed}", color="green")
    while True:
        matrix_touch = hand.get_matrix_touch()
        print("矩阵压感数据")
        print(matrix_touch)
        time.sleep(0.1)


if __name__ == "__main__":
    # python3 linker_hand_get_matrix_touch.py --hand_type left --hand_joint L10
    main()