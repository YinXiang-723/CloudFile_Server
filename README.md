# 云文件系统后端服务

## 项目简介

云文件系统后端服务是一个基于C++11开发的现代化文件存储与分享平台后端，支持文件上传、管理、分享和下载等功能。该系统采用多线程架构，结合MySQL和Redis进行数据存储，并使用FastDFS作为分布式文件存储系统，为前端提供高性能、可扩展的API服务。

## 主要功能

- **用户认证与管理**：支持用户注册、登录和个人信息管理
- **文件上传与存储**：支持多种格式文件上传，使用FastDFS进行分布式存储
- **文件夹管理**：支持创建、重命名、删除和移动文件夹，实现文件的分类管理
- **文件管理**：用户可以查看、删除个人上传的文件，并将文件移动到不同文件夹
- **文件分享**：生成分享链接和提取码，实现文件的安全分享
- **下载统计**：记录和展示文件下载次数，提供热门文件排行榜
- **缓存管理**：使用Redis进行缓存优化，提高系统性能

## 技术架构

### 后端技术栈
- **编程语言**：C++11
- **网络库**：自定义网络库（基于epoll实现）
- **数据库**：MySQL（主从配置）
- **缓存**：Redis
- **文件存储**：FastDFS
- **HTTP解析**：http_parser
- **JSON处理**：JsonCpp
- **日志系统**：自定义异步日志系统
- **线程池**：自定义线程池实现

### 系统架构

系统采用经典的C/S架构，主要包含以下模块：
1. **网络模块**：基于epoll的I/O多路复用，处理HTTP请求
2. **业务逻辑模块**：处理各种API请求，包括用户认证、文件上传、分享等功能
3. **数据存储模块**：通过连接池与MySQL和Redis交互，实现数据持久化和缓存
4. **文件存储模块**：通过FastDFS API实现文件的分布式存储和管理

## 项目结构

```
CloudFile_Server/
├── api/                  # API处理模块
│   ├── ApiCommon.h/cpp   # 公共API函数
│   ├── ApiFolder.h/cpp    # 文件夹管理API
│   ├── ApiFolderHandler.h/cpp # 文件夹管理API处理
│   ├── ApiFileMove.h/cpp # 文件移动API
│   ├── ApiFileMoveHandler.h/cpp # 文件移动API处理
│   ├── ApiBatchOperation.h/cpp # 批量操作API
│   ├── ApiBatchOperationHandler.h/cpp # 批量操作API处理
│   ├── ApiLogin.h/cpp    # 用户登录API
│   ├── ApiRegister.h/cpp # 用户注册API
│   ├── ApiUpload.h/cpp   # 文件上传API
│   ├── ApiMyfiles.h/cpp  # 用户文件管理API
│   ├── ApiSharefiles.h/cpp # 文件分享API
│   ├── ApiDealfile.h/cpp # 文件处理API
│   └── ApiDealsharefile.h/cpp # 分享文件处理API
├── base/                 # 基础库模块
│   ├── BaseSocket.h/cpp  # 基础Socket类
│   ├── Common.h/cpp      # 公共函数和宏
│   ├── ConfigFileReader.h/cpp # 配置文件读取
│   ├── EventDispatch.h/cpp # 事件分发
│   ├── HttpConn.h/cpp    # HTTP连接处理
│   ├── HttpParserWrapper.h/cpp # HTTP解析器封装
│   ├── Lock.h/cpp        # 锁机制
│   ├── ThreadPool.h/cpp  # 线程池
│   ├── UtilPdu.h/cpp     # 工具函数
│   ├── netlib.h/cpp      # 网络库
│   └── util.h/cpp        # 通用工具函数
├── jsoncpp/              # JSON处理库
├── mysql/                # MySQL连接池
│   └── DBPool.h/cpp      # 数据库连接池
├── redis/                # Redis客户端库
│   └── CachePool.h/cpp   # Redis连接池
├── main.cpp              # 主程序入口
├── tc_http_server.conf   # 配置文件
├── nginx.conf            # Nginx配置文件
└── 0voice_tuchuang.sql   # 数据库结构文件
```

## 数据库设计

系统使用MySQL作为主数据库，包含以下主要表：

1. **user_info**：用户信息表，存储用户名、昵称、密码等基本信息
2. **file_info**：文件信息表，存储文件的MD5、ID、URL、大小、类型等信息
3. **user_file_list**：用户文件列表，记录用户拥有的文件及其所属文件夹
4. **user_file_count**：用户文件计数表，统计用户拥有的文件数量
5. **share_file_list**：共享文件列表，记录被用户分享的文件
6. **share_picture_list**：分享图片列表，记录图片分享信息，包含提取码
7. **folders**：文件夹表，存储用户创建的文件夹信息
8. **file_folder_relations**：文件与文件夹关联表，记录文件与文件夹的关联关系

## 编译与安装

### 环境要求
- Linux操作系统（推荐Ubuntu/CentOS）
- CMake 3.0+
- GCC 4.8+ 或 Clang 3.4+
- MySQL 5.6+
- Redis 3.0+
- FastDFS

### 编译步骤

1. 创建构建目录
```bash
mkdir build
cd build
```

2. 使用CMake生成Makefile
```bash
cmake ..
```

3. 编译项目
```bash
make
```

4. 编译完成后，生成可执行文件`tc_http_server`

### 配置

1. 将`tc_http_server.conf`配置文件复制到执行目录
2. 根据实际环境修改配置文件中的以下参数：
   - 数据库连接信息（主机、端口、用户名、密码等）
   - Redis连接信息
   - FastDFS配置路径
   - 服务器监听IP和端口
   - 线程数量

### 运行

```bash
./tc_http_server
```

## API接口

系统提供RESTful API接口，主要接口包括：

### 用户相关
- `POST /api/reg` - 用户注册
- `POST /api/login` - 用户登录

### 文件相关
- `POST /api/upload` - 文件上传
- `GET /api/myfiles` - 获取用户文件列表
- `POST /api/dealfile` - 文件处理（删除等）
- `POST /api/filemove?cmd=single` - 移动单个文件到文件夹
- `POST /api/filemove?cmd=batch` - 批量移动文件到文件夹
- `POST /api/batchoperation` - 批量操作（删除、分享等）

### 文件夹相关
- `POST /api/folders` - 获取用户文件夹列表
- `POST /api/folders?cmd=create` - 创建文件夹
- `POST /api/folders?cmd=update` - 更新文件夹名称
- `POST /api/folders?cmd=delete` - 删除文件夹
- `POST /api/folders?cmd=move` - 移动文件夹
- `POST /api/folderfiles` - 获取文件夹中的文件列表

### 分享相关
- `POST /api/sharefiles` - 分享文件
- `GET /api/sharefiles` - 获取分享文件列表
- `GET /api/sharefiles/{code}` - 通过提取码获取分享文件

### 批量操作
- `POST /api/batchoperation` - 批量操作（支持批量删除、分享等）

## 版本历史

- **v0.2 (tuchuang-2)**：初始版本，采用单线程设计，日志直接输出到控制台
- **v0.3 (tuchuang-3)**：
  1. 实现日志异步处理
  2. 使用Redis进行文件计数
  3. 引入C++11线程池（部分功能已实现多线程处理）
  4. 实现数据回发功能
  5. 修复若干bug

- **v0.4 (tuchuang-4)**：
  1. 添加文件夹管理功能，支持创建、重命名、删除和移动文件夹
  2. 修改文件上传逻辑，支持文件与文件夹的关联
  3. 新增文件夹相关API接口
  4. 优化数据库结构，添加文件夹相关表
  5. 添加文件移动功能，支持单个和批量移动文件到指定文件夹
  6. 优化文件夹树形结构查询，返回树形结构的文件夹数据
  7. 添加批量操作功能，支持批量删除、分享和上传文件

## 开发与贡献

欢迎提交Issue和Pull Request来改进项目。

## 许可证

本项目采用MIT许可证，详情请参见LICENSE文件。