# 故障排查

## CMake 找不到 Pico SDK

检查 `PICO_SDK_PATH` 是否指向 Pico SDK 2.1.1。修改 SDK 路径后重新运行[构建命令](build-and-flash.md)。

## 找不到 RP2350 启动盘

使用支持数据传输的 USB 线；按住 BOOT 键再连接板子。启动盘只在 BOOTSEL 模式下出现。Windows/WSL 环境下，可从 Windows 文件管理器复制 UF2。

## 屏幕没有摄像头画面

检查摄像头模组连接和屏幕排线，确认使用 RP2350-Touch-LCD-2-C 板型。再检查 USB 串口输出中的 OV5640 初始化状态。引脚对应关系见[硬件说明](hardware.md)。

## 推理没有检测结果

查看串口中的模型初始化、张量分配和推理信息，并核对模型输入尺寸、量化参数和类别设置。当前模型文件的来源和授权尚待确认，详见[模型说明](model.md)。
