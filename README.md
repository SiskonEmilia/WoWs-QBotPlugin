# WoWs QBot Plugin

## 插件简介

本插件主要为战舰世界 CoolQ 机器人（2065104337）提供前后端接口，兼有一定的通用性功能。

### 插件功能

1. 捕获前缀关键词作为触发器
2. 在触发器生效时进行 HTTP 请求、自动回复等动作
3. 可解析 HTTP 返回的 JSON 内容并根据模板字符串自动渲染为完整字符串进行回复。
4. 计数复读
5. 自动加群、加好友
6. 拉黑群

### 开发进度

- **version 0.0.1: Midnight** 
  - 引入 Nana GUI Library
  - 完成编译环境测试
  - 确定软件设计模式（MVC）
  - 测试数据结构可用性
- **version 0.0.2: ATRI** *<-developing*
  - 重构项目逻辑，使用 Qt5 替换 Nana 作为 GUI 库
  - 测试硬编码下插件的可用性
  - 设计、实现 GUI 界面，添加数据接口
  - 引入 Tencent RapidJSON
  - 引入 cpp-httplib
  - 实现数据持久化

## 如何获取&使用？

您可以在[本项目网页](https://github.com/SiskonEmilia/WoWs-QBotPlugin)中的 release 板块找到已经编译并打包的 `cpk` 文件，将其放置在您的 `app` 目录下重载即可使用。您可以在菜单栏内进行有 GUI 的详细配置。

**注意：您还需要额外下载 Gt5 相关 dll 方可正常运行。**

## 如何编译？

### 编译环境要求

#### 基本编译要求

- **Windows**
  - Windows 10+
  - Visual Studio C++ 生成工具 2019+（MSVC 2019+）
  - CMake 3.15+
- **MacOS、Linux（未经测试）**
  - GCC with Mingw32 （至少支持C++17）
  - CMake 3.15+

#### 推荐编译环境

- **Windows Only**
  - VSCode + CMake 拓展 + CMake Tools 拓展 + C++ 拓展
  - VS2019

### 编译实践

#### 下载源代码

首先，本项目依赖于 `cqcppsdk`，因而我们在 `.gitmodules` 中将他们申明为了子模块，因此您应该使用如下命令克隆本项目来同时下载子模块依赖：

```bash
git clone --recursive https://github.com/SiskonEmilia/WoWs-QBotPlugin.git
```

如果您已经将本项目下载到本地，则使用如下命令来更新项目子模块：

```bash
git submodule update
```

另外，由于本代码引入了 `Qt5` 库作为依赖并使用 CMake 工具构建，您需要首先安装 Qt5 库（包含 MSVC 2017 开发套件）并设置 `DCMAKE_PREFIX_PATH` 为库目录方可正常编译。

#### 配置 CMake 并编译

我们假设您已经配置好您的 VSCode，并安装了所有推荐的拓展。使用 VSCode 打开项目文件夹，在 VSCode 底部边栏可见 CMake 生成变量、CMake 工具包、生成按钮以及活动变量（即可选的 CMake 生成任务）。首先，点击工具包并点击扫描工具包，选中 `Visual Studio 生成工具 2019 Release - x86`，再选中 Release 作为生成变量，选择 ALL_BUILD 作为活动变量后，点击生成按钮即可自动执行 CMake 脚本并编译本插件。编译好的 `app.dll` 存放于 `./build/Release/` 目录下。

如您想使用 CLI 工具，则可在配置好 CMake 环境后尝试以下命令：

```bash
make ALL_BUILD
```

#### 常见编译问题

- 链接时报错：值“MT_StaticRelease”不匹配值“MD_DynamicRelease”
  - 错误情况：仅发生在使用 MSVC 进行编译链接时，是因为 cqcppsdk 仅支持使用静态的第三方库，而某些第三方库在默认情况下使用了动态链接编译选项。
  - 解决方案：可根据第三方库的说明将其编译为静态库即可。
- 编译时报错：文件"split_string.cpp"内语法错误
  - 错误情况：仅发生在使用 MSVC 进行编译时，是因为 MSVC 并不完全兼容 ISO C++ 标准，默认情况下对 `and` 等关键字支持有限，不能正常识别语法导致的。
  - 解决方案：手动修改 `split_stirng.cpp` 内的 `and` 关键字为 `&&` 即可。（这也意味着我们不能使用标准的 Nana Library）（Qt5 版本无此问题）
- CMake 测试编译器时报错：CMAKE_AR-NOTFOUND
  - 错误情况：仅发生在使用 GCC 进行交叉编译时，是因为未能找到和指定 Archive 工具导致的报错。
  - 解决方案：暂无，期待各位能够反馈。
