# 封头 AutoCAD 自动绘图插件（HeadsPlugin）

基于 **ObjectARX 2026** 的 AutoCAD 压力容器封头参数化绘图插件。一条主命令 `DRAW_HEAD` 即可完成「选型 → 填写参数 → 自动出图」的全流程，直接生成符合制图规范的封头全剖视图。

---

## 功能特性

支持 **6 种封头**，均按**全剖视图**（中心线左右对称）绘制：

| 类型 | 说明 | 主要输入参数 |
|---|---|---|
| 碟形封头 | 直边 + 折边圆弧 `ri` + 球冠 `Ri` | `Di, Ri, ri, T, L` |
| 1:2 椭圆封头 | 标准 2:1 椭圆，深度 = `Di/4` | `Di, T, L` |
| CNA | 无折边锥壳（无翻边、无圆角，底部尖角） | `Di, Dis, α, δn` |
| CSA | 带折边锥壳（顶部折边 `ri` + 直边 `h`，底部尖角） | 再加 `ri, h` |
| CHD | 简单锥形（底部圆角 `rs` + 直边 `hs`，顶部平切） | 再加 `rs, hs` |
| CDA | 带顶部折边 + 底部圆角锥壳 | 再加 `ri, rs, h, hs` |

**每次出图自动包含：**

- 内、外轮廓（全剖，左右镜像）
- 中心线
- 壁厚区剖面线（关联填充，随边界联动）
- 自动尺寸标注（含直径、半径、角度、高度、壁厚）

---

## 环境要求

| 项目 | 要求 |
|---|---|
| 操作系统 | Windows 10 / 11 (x64) |
| AutoCAD | 2026 (x64) |
| SDK | ObjectARX 2026 SDK |
| 编译器 | Visual Studio 2022（需安装「使用 C++ 的桌面开发」+ MFC） |
| 字符集 | Unicode |

---

## 编译方法

1. 用 Visual Studio 2022 打开 `HeadsPlugin/HeadsPlugin.sln`。
2. 选择配置 **Debug | x64**（或 Release | x64）。
3. 生成解决方案。

> **路径配置**：`HeadsPlugin/Autodesk.arx-2026.props` 中的 ObjectARX SDK 与 AutoCAD 安装路径需按本机实际位置修改（属性 `ArxSdkDir` / `AcadDir`）。

编译产物：`HeadsPlugin/x64/HeadsPlugin.arx`

也可用命令行编译：

```bat
MSBuild.exe HeadsPlugin.vcxproj -p:Configuration=Debug -p:Platform=x64
```

---

## 安装与使用

1. 启动 AutoCAD 2026。
2. 输入命令 `APPLOAD`，加载 `HeadsPlugin.arx`。
3. 输入主命令 **`DRAW_HEAD`**。
4. 在弹出的对话框中选择封头类型 → 填写参数 → 确定。
5. 在绘图区拾取**插入点**（中心线与封头底面交点）。
6. 插件自动完成绘制与标注。

---

## 几何约定

### 封头通用

- 坐标：中心线 `x = 0`，底端面 `y = 0`，向上为 `+y`；几何只算右半边，绘图时镜像到左侧拼成完整剖面。
- 单位：**mm**。
- 插入点：中心线与底面的交点。

### 锥体类（CNA / CSA / CHD / CDA）

- `Di` / `Dis`：**内**径（大端 / 小端），`α`：**内**斜壁半顶角，`ri` / `rs`：**内**半径。
- 内轮廓由参数直接确定；**外轮廓 = 内轮廓向外偏移壁厚 `δn`**。
- `H₀`（总高）**自动计算**（各段相加），仅作标注，不需输入。
- `hs`（底部直边高）仅 CHD / CDA 输入；**CNA / CSA 无底部直边**（斜壁直接到底面）。
- **端面规则**：有翻边（法兰）的端面为水平面；无翻边的「裸端」端面**垂直于内壁斜线**。
- 圆角相切方向：
  - 底部（斜壁 → 直边）：圆心在材料外侧；
  - 顶部（斜壁 → 翻边）：圆心在材料内侧。

### 碟形 / 椭圆

- 碟形：直边、折边圆弧、球冠圆弧三段相切；外轮廓 = 内轮廓向外等距偏移 `T`。
- 椭圆：外轮廓采用工程制图惯例——**半轴各 + `T` 的新椭圆**（非等距偏移曲线）。

---

## 图层规范

| 用途 | 图层名 | 颜色 | 线型 | 线宽 |
|---|---|---|---|---|
| 轮廓 | `HEAD_CONTOUR` | 白 ACI 7 | Continuous | 0.50 mm |
| 中心线 | `HEAD_CENTER` | 红 ACI 1 | CENTER | 默认 |
| 剖面线 | `HEAD_HATCH` | 绿 ACI 3 | Continuous | 默认 |
| 尺寸标注 | `HEAD_DIM` | 青 ACI 4 | Continuous | 默认 |

> 图层在首次绘图时自动创建。若图层已存在，线宽等属性以图形中已有设置为准（可手动调整或删除后重画）。

### 标注格式

- 直径：`ØDi = 1000`
- 半径（引线 + 箭头指向圆弧，无尺寸界线）：`Ri = 1000`、`ri = 100`、`rs = 60`
- 角度（一端中心线、一端内壁，弧线位于封头内部空白区）：`α = 30°`
- 高度 / 厚度：`T = 10`、`L = 40`、`H = 500`、`H₀ = 480`、`δn = 10`、`hs = 25`、`h = 40`

---

## 项目结构

```
plugin for head/
├─ HeadsPlugin/                     # Visual Studio 工程
│  ├─ acrxEntryPoint.cpp            # 插件入口 & 命令注册（DRAW_HEAD）
│  ├─ HeadsPlugin.cpp / .rc         # MFC 应用与资源
│  ├─ HeadTypeDlg.*                 # 封头选型对话框
│  ├─ DishedHead.* / DishedHeadDlg.*        # 碟形封头（几何 / 对话框）
│  ├─ EllipticalHead.* / EllipticalHeadDlg.*# 椭圆封头（几何 / 对话框）
│  ├─ ConeGeometry.* / ConeDlg.*    # 锥体几何（CNA/CSA/CHD/CDA）/ 参数对话框
│  ├─ HeadDrawer.*                  # 绘图核心：轮廓 / 中心线 / 剖面线 / 标注
│  ├─ LayerManager.*                # 图层管理创建
│  ├─ HeadTypes.h                   # 公共几何数据类型
│  ├─ DocData.*                     # 文档数据
│  └─ HeadsPlugin.vcxproj / .sln
├─ 封头插件规格说明.md               # 详细规格说明
├─ 封头AutoCAD自动绘图二次开发.docx   # 需求文档
└─ README.md
```

---

## 技术要点

- **语言/框架**：C++ / ObjectARX 2026 / MFC（动态库）。
- **几何表示**：轮廓用「顶点 + 凸度(bulge)」多段线表示，圆弧通过凸度携带，便于 `AcDbPolyline` 直接绘制。
- **剖面线**：采用**关联填充**（`AcDbHatch` + 隐藏闭合多段线边界），避免填充线「粘边」问题。
- **含中文字符串的源文件需保存为 UTF-8 with BOM**（SDK 属性将 C4819 视为错误）。

---

## 已知说明

- 未配置 GitHub CLI 时，远程仓库需手动关联或在 GitHub Desktop 中完成。
- 若 AutoCAD 已加载旧版 `HeadsPlugin.arx`，重新编译前需先在 AutoCAD 中卸载（否则链接报 `LNK1104`）。

---

## 许可

本项目仅供学习与内部使用。ObjectARX 相关版权归 Autodesk 所有。
