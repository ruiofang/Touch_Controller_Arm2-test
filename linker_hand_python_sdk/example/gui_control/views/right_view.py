import sys,os,time
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QLineEdit, QPushButton, QWidget, QGridLayout, QScrollArea,
    QHBoxLayout, QLabel, QMessageBox, QDialog, QFormLayout, QSpinBox
)
from PyQt5.QtCore import Qt, pyqtSignal
current_dir = os.path.dirname(os.path.abspath(__file__))
target_dir = os.path.abspath(os.path.join(current_dir, "../../.."))
sys.path.append(target_dir)
from LinkerHand.utils.load_write_yaml import LoadWriteYaml

class EditActionDialog(QDialog):
    """编辑动作对话框"""
    def __init__(self, action_name="", parent=None):
        super().__init__(parent)
        self.setWindowTitle("编辑动作")
        self.setModal(True)
        self.setFixedSize(300, 150)
        
        layout = QFormLayout(self)
        
        # 动作名称输入框
        self.name_edit = QLineEdit(action_name)
        layout.addRow("动作名称:", self.name_edit)
        
        # 按钮布局
        button_layout = QHBoxLayout()
        self.ok_button = QPushButton("确定")
        self.cancel_button = QPushButton("取消")
        
        self.ok_button.clicked.connect(self.accept)
        self.cancel_button.clicked.connect(self.reject)
        
        button_layout.addWidget(self.ok_button)
        button_layout.addWidget(self.cancel_button)
        layout.addRow("", button_layout)
        
    def get_action_name(self):
        return self.name_edit.text().strip()

class RightView(QMainWindow):
    add_button_handle = pyqtSignal(str)  # 定义一个信号
    handle_button_click = pyqtSignal(str)  # 定义一个信号
    delete_action_signal = pyqtSignal(str)  # 删除动作信号
    edit_action_signal = pyqtSignal(str, str)  # 编辑动作信号 (旧名称, 新名称)
    
    def __init__(self,hand_joint="L20", hand_type="left"):
        super().__init__()
        self.hand_joint = hand_joint
        self.hand_type = hand_type
        self.buttons = []
        self.MAX_ACTIONS = 20  # 最大动作数量
        self.yaml = LoadWriteYaml() # 初始化配置文件
        self.all_action = None
        self.all_action = self.yaml.load_action_yaml(hand_type=self.hand_type,hand_joint=self.hand_joint)
        self.setWindowTitle("动作管理")
        self.setGeometry(100, 100, 800, 600)  # 增加窗口宽度和高度
        self.init_ui()
        self.init_buttons()

    def init_ui(self):
        # 主窗口容器
        main_widget = QWidget()
        self.setCentralWidget(main_widget)

        # 主布局
        self.main_layout = QVBoxLayout(main_widget)
        self.main_layout.setContentsMargins(10, 10, 10, 10)
        self.main_layout.setSpacing(10)

        # 标题和状态
        title_label = QLabel(f"动作管理 - {self.hand_type}手 {self.hand_joint}")
        title_label.setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;")
        self.main_layout.addWidget(title_label)
        
        # 状态信息
        self.status_label = QLabel(f"当前动作数量: {len(self.all_action) if self.all_action else 0}/{self.MAX_ACTIONS}")
        self.status_label.setStyleSheet("color: blue; margin-bottom: 10px;")
        self.main_layout.addWidget(self.status_label)

        # 输入框和添加按钮
        input_layout = QHBoxLayout()
        self.input_field = QLineEdit()
        self.input_field.setPlaceholderText("输入动作名称...")
        self.add_button = QPushButton("添加动作")
        self.add_button.clicked.connect(self.add_button_to_list)
        
        input_layout.addWidget(self.input_field)
        input_layout.addWidget(self.add_button)
        self.main_layout.addLayout(input_layout)

        # 创建一个滚动区域用于按钮列表
        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.scroll_widget = QWidget()
        self.scroll_layout = QGridLayout(self.scroll_widget)
        self.scroll_layout.setContentsMargins(10, 10, 10, 10)
        self.scroll_layout.setSpacing(10)
        self.scroll_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.scroll_area.setWidget(self.scroll_widget)

        # 添加滚动区域到主布局
        self.main_layout.addWidget(self.scroll_area)

        # 网格布局参数
        self.row = 0
        self.column = 0
        self.BUTTONS_PER_ROW = 5  # 每行5个按钮，增加显示数量

    def init_buttons(self):
        if self.all_action == None:
            return
        for item in self.all_action:
            self.add_action_button(item["ACTION_NAME"], item)

    def add_action_button(self, action_name, action_data=None):
        """添加动作按钮"""
        # 检查是否达到最大数量
        if len(self.buttons) >= self.MAX_ACTIONS:
            QMessageBox.warning(self, "警告", f"已达到最大动作数量限制 ({self.MAX_ACTIONS})")
            return False
            
        # 创建按钮容器
        button_container = QWidget()
        button_layout = QVBoxLayout(button_container)
        button_layout.setContentsMargins(5, 5, 5, 5)
        button_layout.setSpacing(5)
        
        # 主按钮
        button = QPushButton(action_name)
        button.setFixedSize(150, 50)  # 增加按钮尺寸
        button.clicked.connect(lambda checked, text=action_name: self.handle_button_click.emit(text))
        button_layout.addWidget(button)
        
        # 操作按钮布局
        op_layout = QHBoxLayout()
        
        # 编辑按钮
        edit_btn = QPushButton("编辑")
        edit_btn.setFixedSize(60, 30)  # 增加编辑按钮尺寸
        edit_btn.clicked.connect(lambda checked, name=action_name: self.edit_action(name))
        op_layout.addWidget(edit_btn)
        
        # 删除按钮
        delete_btn = QPushButton("删除")
        delete_btn.setFixedSize(60, 30)  # 增加删除按钮尺寸
        delete_btn.clicked.connect(lambda checked, name=action_name: self.delete_action(name))
        op_layout.addWidget(delete_btn)
        
        button_layout.addLayout(op_layout)
        
        # 添加到网格布局
        self.scroll_layout.addWidget(button_container, self.row, self.column, alignment=Qt.AlignmentFlag.AlignTop)

        # 更新行列位置
        self.column += 1
        if self.column >= self.BUTTONS_PER_ROW:
            self.column = 0
            self.row += 1

        self.buttons.append(button_container)
        self.update_status()
        return True

    def add_button_to_list(self):
        text = self.input_field.text().strip()
        if not text:
            QMessageBox.warning(self, "警告", "请输入动作名称")
            return
            
        # 检查是否已存在同名动作
        if self.all_action:
            for action in self.all_action:
                if action["ACTION_NAME"] == text:
                    QMessageBox.warning(self, "警告", f"动作 '{text}' 已存在")
                    return
        
        # 添加按钮
        if self.add_action_button(text):
            self.input_field.clear()
            self.add_button_handle.emit(text)

    def delete_action(self, action_name):
        """删除动作"""
        reply = QMessageBox.question(self, "确认删除", 
                                   f"确定要删除动作 '{action_name}' 吗？",
                                   QMessageBox.Yes | QMessageBox.No)
        
        if reply == QMessageBox.Yes:
            # 从按钮列表中移除
            for i, button_container in enumerate(self.buttons):
                button = button_container.findChild(QPushButton)
                if button and button.text() == action_name:
                    self.scroll_layout.removeWidget(button_container)
                    button_container.deleteLater()
                    self.buttons.pop(i)
                    break
            
            # 重新排列按钮
            self.rearrange_buttons()
            
            # 发送删除信号
            self.delete_action_signal.emit(action_name)
            
            self.update_status()

    def edit_action(self, old_name):
        """编辑动作"""
        dialog = EditActionDialog(old_name, self)
        if dialog.exec_() == QDialog.Accepted:
            new_name = dialog.get_action_name()
            if not new_name:
                QMessageBox.warning(self, "警告", "动作名称不能为空")
                return
                
            if new_name == old_name:
                return
                
            # 检查新名称是否已存在
            if self.all_action:
                for action in self.all_action:
                    if action["ACTION_NAME"] == new_name:
                        QMessageBox.warning(self, "警告", f"动作 '{new_name}' 已存在")
                        return
            
            # 更新按钮文本
            for button_container in self.buttons:
                button = button_container.findChild(QPushButton)
                if button and button.text() == old_name:
                    button.setText(new_name)
                    break
            
            # 发送编辑信号
            self.edit_action_signal.emit(old_name, new_name)

    def rearrange_buttons(self):
        """重新排列按钮"""
        # 清空布局
        while self.scroll_layout.count():
            item = self.scroll_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                self.scroll_layout.removeWidget(widget)
        
        # 重新添加按钮
        self.row = 0
        self.column = 0
        for button_container in self.buttons:
            self.scroll_layout.addWidget(button_container, self.row, self.column, alignment=Qt.AlignmentFlag.AlignTop)
            self.column += 1
            if self.column >= self.BUTTONS_PER_ROW:
                self.column = 0
                self.row += 1

    def update_status(self):
        """更新状态信息"""
        self.status_label.setText(f"当前动作数量: {len(self.buttons)}/{self.MAX_ACTIONS}")

    def clear_scroll_layout(self):
        """清空 scroll_layout 中的所有小部件"""
        while self.scroll_layout.count():
            item = self.scroll_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()
        self.buttons.clear()
        self.row = 0
        self.column = 0
        self.update_status()
