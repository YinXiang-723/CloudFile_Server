/**
 * ApiBatchOperationHandler.h
 * 批量操作API处理
 */

#ifndef _API_BATCH_OPERATION_HANDLER_H_
#define _API_BATCH_OPERATION_HANDLER_H_

#include <string>

// 处理批量删除文件请求
int handleBatchDeleteFiles(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理批量分享文件请求
int handleBatchShareFiles(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理批量上传文件到指定文件夹请求
int handleBatchUploadFilesToFolder(const std::string &url, const std::string &post_data, std::string &str_json);

#endif // _API_BATCH_OPERATION_HANDLER_H_
