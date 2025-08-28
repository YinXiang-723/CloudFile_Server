/**
 * ApiBatchOperation.h
 * 批量操作API接口
 */

#ifndef _API_BATCH_OPERATION_H_
#define _API_BATCH_OPERATION_H_

#include <string>
#include <json/json.h>

// 批量删除文件
int batchDeleteFiles(const std::string &user, const Json::Value &file_md5s, std::string &str_json);

// 批量分享文件
int batchShareFiles(const std::string &user, const Json::Value &file_md5s, std::string &str_json);

// 批量上传文件到指定文件夹
int batchUploadFilesToFolder(const std::string &user, const Json::Value &files, int folder_id, std::string &str_json);

// 初始化批量操作API
int ApiBatchOperationInit();

#endif // _API_BATCH_OPERATION_H_
