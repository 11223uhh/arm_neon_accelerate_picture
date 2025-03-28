# ARM NEON 图像加速

**本项目使用 ARM NEON 指令集对图像处理进行加速优化，主要针对图像格式转换等常见操作。通过 NEON SIMD（单指令多数据流）技术，显著提升图像处理性能。**

[English](README_EN.md) | 简体中文

## 项目特点

- 使用 ARM NEON 指令集进行图像处理加速
- 支持多种图像格式转换
- 提供性能对比数据
- 包含详细的示例代码和注释

## 支持的操作

- RGB 与 YUV 格式转换
- 图像缩放
- 图像旋转
- 亮度和对比度调整
- 其他常见图像变换

## 环境要求

- ARM 架构处理器（支持 NEON 指令集）
- C/C++ 开发环境
- 图像处理相关库（可选）

## 使用方法

1. 克隆仓库
```bash
git clone https://github.com/11223uhh/arm_neon_accelerate_picture.git
```

2. 编译项目
```bash
cd arm_neon_accelerate_picture
make
```

3. 运行示例
```bash
./example
```

## 性能优化

本项目通过以下方式实现性能优化：
- NEON 指令并行处理
- 内存对齐优化
- 循环展开
- 缓存优化

## 贡献指南

欢迎提交 Issue 和 Pull Request 来帮助改进项目。所有贡献都将受到 MIT 许可证的保护。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。您可以自由地使用、修改和分发本项目，只需在您的项目中包含原始的许可证文本即可。