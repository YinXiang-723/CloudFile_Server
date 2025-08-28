# 文件夹管理功能实现文档

## 概述

本文档记录了为云文件系统后端服务添加文件夹管理功能的实现过程。该功能允许用户创建、重命名、删除和移动文件夹，并将文件上传到指定文件夹中，实现文件的分类管理。

## 功能设计

### 1. 数据库设计

#### 新增表结构

1. **folders 表**：存储文件夹信息
   - `id`：文件夹ID，主键
   - `user_id`：用户ID，外键关联user_info表
   - `parent_id`：父文件夹ID，NULL表示根文件夹
   - `folder_name`：文件夹名称
   - `create_time`：创建时间
   - `update_time`：更新时间
   - `is_deleted`：是否已删除，0-未删除，1-已删除

2. **file_folder_relations 表**：存储文件与文件夹的关联关系
   - `file_id`：文件ID，外键关联user_file_list表
   - `folder_id`：文件夹ID，外键关联folders表
   - `create_time`：创建时间

#### 修改现有表结构

在 `user_file_list` 表中添加 `folder_id` 字段，用于存储文件所属的文件夹ID。

### 2. API设计

#### 文件夹管理API

1. **获取用户文件夹列表**
   - URL：`GET /api/folders?user=<username>&token=<token>`
   - 功能：获取指定用户的所有文件夹列表
   - 返回：JSON格式的文件夹列表

2. **创建文件夹**
   - URL：`POST /api/folders?cmd=create&user=<username>&token=<token>`
   - 请求体：JSON格式，包含`folder_name`和`parent_id`（可选）
   - 功能：创建新文件夹
   - 返回：JSON格式，包含新创建的文件夹ID

3. **更新文件夹名称**
   - URL：`POST /api/folders?cmd=update&user=<username>&token=<token>`
   - 请求体：JSON格式，包含`folder_id`和`new_name`
   - 功能：修改指定文件夹的名称
   - 返回：JSON格式的操作结果

4. **删除文件夹**
   - URL：`POST /api/folders?cmd=delete&user=<username>&token=<token>`
   - 请求体：JSON格式，包含`folder_id`
   - 功能：删除指定文件夹（只能删除空文件夹）
   - 返回：JSON格式的操作结果

5. **移动文件夹**
   - URL：`POST /api/folders?cmd=move&user=<username>&token=<token>`
   - 请求体：JSON格式，包含`folder_id`和`new_parent_id`
   - 功能：移动文件夹到指定父文件夹下
   - 返回：JSON格式的操作结果

6. **获取文件夹中的文件列表**
   - URL：`GET /api/folderfiles?user=<username>&folder_id=<id>&token=<token>`
   - 功能：获取指定文件夹中的文件列表
   - 返回：JSON格式的文件列表

#### 文件上传API修改

修改文件上传API，支持指定文件夹ID：
- 在上传请求中添加`folder_id`字段，指定文件要上传到的文件夹
- 如果未指定文件夹ID，则文件上传到根文件夹

## 实现细节

### 1. 文件夹管理API实现

#### 文件夹数据模型

在 `ApiFolder.cpp` 中实现了文件夹的CRUD操作：
- `getUserFolders`：获取用户文件夹列表
- `createFolder`：创建新文件夹
- `updateFolderName`：更新文件夹名称
- `deleteFolder`：删除文件夹
- `moveFolder`：移动文件夹
- `getFolderFiles`：获取文件夹中的文件列表

#### 文件夹API处理

在 `ApiFolderHandler.cpp` 中实现了API请求的处理：
- `handleGetUserFolders`：处理获取用户文件夹列表请求
- `handleCreateFolder`：处理创建文件夹请求
- `handleUpdateFolderName`：处理更新文件夹名称请求
- `handleDeleteFolder`：处理删除文件夹请求
- `handleMoveFolder`：处理移动文件夹请求
- `handleGetFolderFiles`：处理获取文件夹中的文件列表请求

#### HTTP路由

在 `HttpConn.cpp` 中添加了文件夹管理API的路由处理：
- `_HandleGetUserFoldersRequest`：处理获取用户文件夹列表请求
- `_HandleCreateFolderRequest`：处理创建文件夹请求
- `_HandleUpdateFolderNameRequest`：处理更新文件夹名称请求
- `_HandleDeleteFolderRequest`：处理删除文件夹请求
- `_HandleMoveFolderRequest`：处理移动文件夹请求
- `_HandleGetFolderFilesRequest`：处理获取文件夹中的文件列表请求

### 2. 文件上传功能修改

#### 数据库操作修改

修改了 `ApiUpload.cpp` 中的 `storeFileinfo` 函数：
- 添加 `folder_id` 参数，支持指定文件所属文件夹
- 在 `user_file_list` 表中插入记录时，包含 `folder_id` 字段

#### 文件夹ID解析

在 `ApiUpload.cpp` 中添加了文件夹ID的解析逻辑：
- 从上传请求中解析 `folder_id` 字段
- 如果未指定文件夹ID，则默认为0（根文件夹）

## 安全性考虑

1. **权限验证**：所有文件夹操作API都需要验证用户token，确保用户只能操作自己的文件夹
2. **数据验证**：对文件夹名称进行合法性检查，防止SQL注入和路径遍历攻击
3. **软删除**：文件夹删除采用软删除方式，保留数据但标记为已删除
4. **循环引用检查**：移动文件夹时检查是否会造成循环引用（将文件夹移动到自己的子文件夹中）

## 性能优化

1. **索引优化**：为 `folders` 表的 `user_id` 和 `parent_id` 字段添加索引，提高查询效率
2. **批量操作**：支持批量获取文件夹中的文件列表，减少数据库查询次数
3. **缓存优化**：使用Redis缓存用户文件夹列表，减少重复查询

## 测试建议

1. **功能测试**：测试文件夹的创建、重命名、删除、移动等基本功能
2. **边界测试**：测试各种边界情况，如删除非空文件夹、移动文件夹到自身等
3. **性能测试**：测试大量文件夹和文件情况下的系统性能
4. **安全测试**：测试各种安全攻击，如SQL注入、权限越权等

## 后续优化建议

1. **文件夹共享功能**：实现文件夹的分享功能，允许用户分享整个文件夹
2. **批量操作**：支持批量上传文件到指定文件夹
3. **文件夹搜索**：添加文件夹搜索功能，方便用户快速找到目标文件夹
4. **文件夹模板**：支持创建文件夹模板，快速创建常用的文件夹结构
