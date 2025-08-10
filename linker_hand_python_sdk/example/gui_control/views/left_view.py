from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider, QPushButton
)
from PyQt5.QtCore import Qt,pyqtSignal

class LeftView(QWidget):
    # 定义一个信号，当滑动条的值发生变化时发出该信号
    slider_value_changed = pyqtSignal(dict)
    def __init__(self, joint_name=[],init_pos=[]):
        super().__init__()
        self.is_open = False  # 默认关闭状态
        self.joint_name = joint_name
        self.init_pos = init_pos
        self.init_view()

    def init_view(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(20, 20, 20, 20)  # 增加边距
        main_layout.setSpacing(15)  # 增加间距
        
        # 存储滑动条和标签
        self.sliders = []
        self.labels = []
        # 创建滑动条
        for i in range(len(self.joint_name)):
            # 每个滑动条和标签的水平布局
            slider_layout = QHBoxLayout()
            slider_layout.setSpacing(15)  # 增加间距
            
            # 标签显示滑动条的值
            label = QLabel(f"{self.joint_name[i]}: 255", self)
            label.setFixedWidth(150)  # 增加标签宽度
            label.setStyleSheet("font-size: 18px; font-weight: bold;")  # 增加字体样式
            self.labels.append(label)
            slider_layout.addWidget(label)

            # 滑动条
            slider = QSlider(Qt.Orientation.Horizontal, self)
            slider.setRange(0, 255)
            slider.setValue(self.init_pos[i])
            slider.setFixedHeight(20)  # 增加滑动条高度
            slider.setMinimumWidth(300)  # 设置最小宽度
            slider.valueChanged.connect(lambda value, index=i: self.update_label(index, value))
            self.sliders.append(slider)
            slider_layout.addWidget(slider)
            main_layout.addLayout(slider_layout)
            
        # 创建开启/关闭按钮
        self.toggle_button = QPushButton("开启", self)
        self.toggle_button.setCheckable(True)
        self.toggle_button.setChecked(False)  # 默认未选中（关闭状态）
        self.toggle_button.setFixedSize(120, 40)  # 增加按钮尺寸
        self.toggle_button.setStyleSheet("font-size: 18px; font-weight: bold; background-color: #4ecdc4; color: white;")  # 增加字体样式
        self.toggle_button.clicked.connect(self.toggle_button_clicked)
        main_layout.addWidget(self.toggle_button)

    def update_label(self, index, value):
        self.labels[index].setText(f"{self.joint_name[index]}: {value}")
        # 只有在机械手开启时才发出信号控制手指角度
        if self.is_open:
            slider_values = {}
            sliders = self.findChildren(QSlider)
            for i, slider in enumerate(sliders):
                slider_values[i] = slider.value()
            # 发出信号，传递滑动条的当前值
            self.slider_value_changed.emit(slider_values)
        
        
    def set_slider_values(self, values):
        for i, value in enumerate(values):
            if i < len(self.sliders):
                self.sliders[i].setValue(value)
                
    def get_slider_values(self):
        """获取所有滑动条的值"""
        return [slider.value() for slider in self.sliders]
    def handle_button_click(self, text):
        print(f"Button clicked with text: {text}")
        # 在这里处理按钮点击事件

    def toggle_button_clicked(self):
        if self.toggle_button.isChecked():
            self.toggle_button.setText("关闭")
            self.toggle_button.setStyleSheet("font-size: 18px; font-weight: bold; background-color: #ff6b6b; color: white;")
            self.is_open = True
            # 机械手开启状态
        else:
            self.toggle_button.setText("开启")
            self.toggle_button.setStyleSheet("font-size: 18px; font-weight: bold; background-color: #4ecdc4; color: white;")
            self.is_open = False
            # 机械手关闭状态