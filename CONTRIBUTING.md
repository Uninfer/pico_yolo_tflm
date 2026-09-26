# 贡献指南

欢迎为 RP2350 Touch LCD YOLO 固件提交改进。

## 修改前

- 阅读[运行架构](docs/architecture.md)、[硬件与引脚](docs/hardware.md)和[模型说明](docs/model.md)。
- 修改 GPIO 时，同时核对 `boards/include/board_pins.h` 与 `docs/hardware.md`。
- 保留导入代码原有版权声明、许可证和 SPDX 标记。
- 添加模型权重、照片或示例帧时，先记录来源和再分发许可，并更新 `THIRD_PARTY_NOTICES.md`。

## 构建

按[构建与烧录说明](docs/build-and-flash.md)配置环境。提交前至少构建默认固件目标，并在变更说明中写明使用的命令和结果。若有板子可用，也请记录实测的板卡版本与检查结果。

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

## 提交说明

请说明变更影响的硬件或用户行为、涉及的文件，以及如何检查。驱动变更应列出外设和相关 GPIO；模型变更应说明输入输出张量和模型来源。

## 贡献许可

提交原创代码即表示同意将该贡献按仓库的 Apache-2.0 许可证发布。请保留贡献代码涉及的第三方声明，并在 `THIRD_PARTY_NOTICES.md` 中登记新增依赖或素材。
