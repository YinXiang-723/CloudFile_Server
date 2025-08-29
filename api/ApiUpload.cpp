#include "ApiUpload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "fdfs_client.h"
#include "ApiCommon.h"

/* -------------------------------------------*/
/**
 * @brief  将一个本地文件上传到 后台分布式文件系统中
 * 对应 fdfs_upload_file /etc/fdfs/client.conf  完整文件路径
 *
 * @param file_path  (in) 本地文件的路径
 * @param fileid    (out)得到上传之后的文件ID路径
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int uploadFileToFastDfs(char *file_path, char *fileid)
{
    int ret = 0;

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0) // fd[0] → r； fd[1] → w  获取上传后返回的信息 fileid
    {
        LOG_ERROR << "pipe error";
        ret = -1;
        goto END;
    }

    //创建进程
    pid = fork(); //
    if (pid < 0)  //进程创建失败
    {
        LOG_ERROR << "fork error";
        ret = -1;
        goto END;
    }
    else if (pid == 0) //子进程
    {
        close(fd[0]); //关闭读
        dup2(fd[1], STDOUT_FILENO); //标准输出重定向到管道
        execlp("fdfs_upload_file", "fdfs_upload_file", "/etc/fdfs/client.conf", file_path, NULL);
        LOG_ERROR << "execlp error";
        exit(1);
    }
    else //父进程
    {
        close(fd[1]); //关闭写
        wait(NULL); //等待子进程结束
        char buf[1024] = {0};
        read(fd[0], buf, sizeof(buf));
        close(fd[0]);
        strcpy(fileid, buf);
        LOG_INFO << "fileid: " << fileid;
    }

END:
    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  根据文件id获取完整的http地址
 *
 * @param fileid    (in) 文件ID
 * @param fdfs_file_url  (out) 完整的http地址
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int getFullurlByFileid(char *fileid, char *fdfs_file_url)
{
    int ret = 0;

    char *p = NULL;
    char *q = NULL;
    char *k = NULL;

    char fdfs_file_stat_buf[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_host_name[HOST_NAME_LEN] = {0}; // storage所在服务器ip地址

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0)
    {
        LOG_ERROR << "pipe error";
        ret = -1;
        goto END;
    }

    //创建进程
    pid = fork();
    if (pid < 0) //进程创建失败
    {
        LOG_ERROR << "fork error";
        ret = -1;
        goto END;
    }

    if (pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);

        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO); // dup2(fd[1], 1);

        execlp("fdfs_file_info", "fdfs_file_info", s_dfs_path_client.c_str(), fileid, NULL);

        //执行失败
        LOG_ERROR << "execlp fdfs_file_info error";

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fdfs_file_stat_buf, TEMP_BUF_MAX_LEN);
        ;

        wait(NULL); //等待子进程结束，回收其资源
        close(fd[0]);
        LOG_INFO << "fdfs_file_stat_buf: " << fdfs_file_stat_buf;
        //拼接上传文件的完整url地址--->http://host_name/group1/M00/00/00/D12313123232312.png
        p = strstr(fdfs_file_stat_buf, "source ip address: ");

        q = p + strlen("source ip address: ");
        k = strstr(q, "\n");

        strncpy(fdfs_file_host_name, q, k - q);
        fdfs_file_host_name[k - q] = '\0';      // 这里这个获取回来只是局域网的ip地址，在讲fastdfs原理的时候再继续讲这个问题

        LOG_INFO << "host_name:" << s_storage_web_server_ip << ", fdfs_file_host_name: " << fdfs_file_host_name;

        // storage_web_server服务器的端口

        strcat(fdfs_file_url, "http://");
        strcat(fdfs_file_url, s_storage_web_server_ip.c_str());
        strcat(fdfs_file_url, ":");
        strcat(fdfs_file_url, s_storage_web_server_port.c_str());
        strcat(fdfs_file_url, "/");
        strcat(fdfs_file_url, fileid);

        LOG_INFO << "fdfs_file_url:" << fdfs_file_url;
    }

END:
    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  存储文件信息到数据库
 *
 * @param pDBConn 数据库连接
 * @param pCacheConn 缓存连接
 * @param user 用户名
 * @param file_name 文件名
 * @param file_md5 文件md5
 * @param file_size 文件大小
 * @param fileid 文件ID
 * @param fdfs_file_url 文件URL
 * @param folder_id 文件夹ID
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int storeFileinfo(CDBConn *pDBConn, CacheConn *pCacheConn, const char *user, const char *file_name, const char *file_md5, long file_size, const char *fileid, const char *fdfs_file_url, int folder_id)
{
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    char create_time[TIME_STRING_LEN] = {0};
    time_t now;
    struct tm *tm_now;
    int file_info_id = 0;
    int user_id = 0;
    CResultSet *pResultSet = NULL;

    //获取当前时间
    now = time(NULL);
    tm_now = localtime(&now);
    strftime(create_time, TIME_STRING_LEN, "%Y-%m-%d %H:%M:%S", tm_now);

    //获取用户ID
    sprintf(sql_cmd, "SELECT id FROM user_info WHERE user_name='%s'", user);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "get user id failed: " << sql_cmd;
        ret = -1;
        goto END;
    }
    user_id = pResultSet->GetInt("id");
    delete pResultSet;

    //插入文件信息到file_info表
    sprintf(sql_cmd, "INSERT INTO file_info (user_id, folder_id, md5, file_id, url, file_name, size, type, count, create_time) VALUES (%d, %d, '%s', '%s', '%s', '%s', %ld, '%s', 1, '%s')",
            user_id, folder_id, file_md5, fileid, fdfs_file_url, file_name, file_size, get_file_type(file_name), create_time);
    if (!pDBConn->ExecuteCreate(sql_cmd))
    {
        LOG_ERROR << "insert file_info failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    //获取文件ID
    sprintf(sql_cmd, "SELECT id FROM file_info WHERE md5='%s'", file_md5);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "get file id failed: " << sql_cmd;
        ret = -1;
        goto END;
    }
    file_info_id = pResultSet->GetInt("id");
    delete pResultSet;

    //更新用户文件数量
    sprintf(sql_cmd, "INSERT INTO user_file_count (user_id, file_count, folder_count) VALUES (%d, 1, 0) ON DUPLICATE KEY UPDATE file_count = file_count + 1", user_id);
    if (!pDBConn->ExecuteCreate(sql_cmd))
    {
        LOG_ERROR << "update user file count failed: " << sql_cmd;
        // 不影响主要功能，仅记录错误
    }

END:
    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  处理文件上传
 *
 * @param url URL
 * @param post_data POST数据
 * @param str_json 返回的JSON数据
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int ApiUpload(string &url, string &post_data, string &str_json)
{
    UNUSED(url);

    char suffix[SUFFIX_LEN] = {0};
    char fileid[TEMP_BUF_MAX_LEN] = {0};    //文件上传到fastDFS后的文件id
    char fdfs_file_url[FILE_URL_LEN] = {0}; //文件所存放storage的host_name
    int ret = 0;
    char boundary[TEMP_BUF_MAX_LEN] = {0}; //分界线信息
    char file_name[128] = {0};
    char file_content_type[128] = {0};
    char file_path[128] = {0};
    char new_file_path[128] = {0};
    char file_md5[128] = {0};
    char file_size[32] = {0};
    long long_file_size = 0;
    char user[32] = {0};
    char *begin = (char *)post_data.c_str();
    char *p1, *p2;
    char *dot = NULL; // 文件扩展名指针

    Json::Value value;

    // 解析文件夹ID，如果没有指定则为0（根文件夹）
    int folder_id = 0;
    const char *folder_id_str = strstr(post_data.c_str(), "name=\"folder_id\"");
    if (folder_id_str)
    {
        const char *p1 = strstr(folder_id_str, "\n");
        if (p1)
        {
            p1 += 4;
            const char *p2 = strstr(p1, "\n");
            if (p2)
            {
                char folder_id_value[32] = {0};
                strncpy(folder_id_value, p1, p2 - p1);
                folder_id = atoi(folder_id_value);
                LOG_INFO << "folder_id: " << folder_id;
            }
        }
    }

    // 获取数据库连接
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);

    LOG_INFO << "post_data: " << post_data;

    // 1. 解析boundary
    // Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryjWE3qXXORSg2hZiB
    // 找到起始位置
    p1 = strstr(begin, "\n"); // 作用是返回字符串中首次出现子串的地址
    if (p1 == NULL)
    {
        LOG_ERROR << "wrong no boundary!";
        ret = -1;
        goto END;
    }
    //拷贝分界线
    strncpy(boundary, begin, p1 - begin); // 缓存分界线, 比如：WebKitFormBoundary88asdgewtgewx
    boundary[p1 - begin] = ' ';          //字符串结束符
    LOG_INFO << "boundary: " << boundary;

    // 查找文件名file_name
    begin = p1 + 2;
    p2 = strstr(begin, "name=\"file_name\""); //找到file_name字段
    if (!p2)
    {
        LOG_ERROR << "wrong no file_name!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n"); // 找到file_name下一行
    p2 += 4;
    begin = p2; // 文件名开始
    p2 = strstr(begin, "\r\n");
    // 确保正确复制文件名，包括空格
    strncpy(file_name, begin, p2 - begin);
    file_name[p2 - begin] = '\0'; // 确保字符串正确终止
    LOG_INFO << "file_name: " << file_name;

    // 查找文件类型file_content_type
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_content_type\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_content_type!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_content_type, begin, p2 - begin);
    LOG_INFO << "file_content_type: " << file_content_type;

    // 查找文件file_path
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_path\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_path!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_path, begin, p2 - begin);
    LOG_INFO << "file_path: " << file_path;

    // 查找文件file_md5
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_md5\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_md5!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_md5, begin, p2 - begin);
    LOG_INFO << "file_md5: " << file_md5;

    // 查找文件file_size
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"file_size\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no file_size!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(file_size, begin, p2 - begin);
    LOG_INFO << "file_size: " << file_size;
    long_file_size = strtol(file_size, NULL, 10); //字符串转long

    // 查找user
    begin = p2 + 2;
    p2 = strstr(begin, "name=\"user\""); //
    if (!p2)
    {
        LOG_ERROR << "wrong no user!";
        ret = -1;
        goto END;
    }
    p2 = strstr(p2, "\r\n");
    p2 += 4;
    begin = p2;
    p2 = strstr(begin, "\r\n");
    strncpy(user, begin, p2 - begin);
    user[p2 - begin] = '\0'; // 确保字符串正确终止
    LOG_INFO << "user: " << user;
    
    // 检查用户字段是否为空
    if (strlen(user) == 0)
    {
        LOG_ERROR << "user field is empty!";
        ret = -1;
        goto END;
    }

    // 检查文件路径是否已经有扩展名
    dot = strrchr(file_path, '.');
    
    // 获取文件名后缀
    GetFileSuffix(file_name, suffix); // mp4, jpg, png
    
    // 根据文件路径是否已经有扩展名来决定是否添加扩展名
    if (!dot) {
        // 如果没有扩展名，添加扩展名
        strcpy(new_file_path, file_path);
        strcat(new_file_path, ".");
        strcat(new_file_path, suffix);
        LOG_INFO << "添加扩展名: " << new_file_path;
    } else {
        // 如果已经有扩展名，直接使用原路径
        strcpy(new_file_path, file_path);
        LOG_INFO << "使用原路径: " << new_file_path;
    }
    
    // 重命名 修改文件名
    LOG_INFO << "尝试重命名: " << file_path << " to " << new_file_path;
    ret = rename(file_path, new_file_path);
    if (ret < 0)
    {
        LOG_ERROR << "rename " << file_path << " to " << new_file_path << "failed";
        ret = -1;
        goto END;
    }
    //===============> 将该文件存入fastDFS中,并得到文件的file_id <============
    LOG_INFO << "uploadFileToFastDfs, file_name:" << file_name << ", new_file_path:" << new_file_path;
    if (uploadFileToFastDfs(new_file_path, fileid) < 0)
    {
        LOG_ERROR << "uploadFileToFastDfs failed ";
        ret = -1;
        goto END;
    }
    //================> 删除本地临时存放的上传文件 <===============
    LOG_INFO << "unlink: " << new_file_path;
    ret = unlink(new_file_path);
    if (ret != 0)
    {
       LOG_WARN << "unlink: " << new_file_path <<   " failed";      // 删除失败则需要有个监控重新清除过期的临时文件，比如过期两天的都删除
    }

    //================> 得到文件所存放storage的host_name <=================
    // 拼接出完整的http地址
    LOG_INFO << "getFullurlByFileid, fileid: " << fileid;
    if (getFullurlByFileid(fileid, fdfs_file_url) < 0)
    {
        LOG_ERROR << "getFullurlByFileid failed ";
        ret = -1;
        goto END;
    }

    //===============> 将该文件的FastDFS相关信息存入mysql中 <======
    LOG_INFO << "storeFileinfo, url: " << fdfs_file_url;

    // 把文件写入file_info
    if (storeFileinfo(pDBConn, pCacheConn, user, file_name, file_md5, long_file_size, fileid, fdfs_file_url, folder_id) < 0)
    {
        LOG_ERROR << "storeFileinfo failed ";
        ret = -1;
        // 严谨而言，这里需要删除 已经上传的文件
        goto END;
    }


    value["code"] = 0;
    str_json = value.toStyledString(); // json序列化, 直接用writer是紧凑方式，这里toStyledString是格式化更可读方式

    return 0;
END:
    value["code"] = 1;
    str_json = value.toStyledString(); // json序列化

    return -1;
}

int ApiUploadInit(char *dfs_path_client, char *web_server_ip, char *web_server_port, char *storage_web_server_ip, char *storage_web_server_port)
{
    s_dfs_path_client = dfs_path_client;
    s_web_server_ip = web_server_ip;
    s_web_server_port = web_server_port;
    s_storage_web_server_ip = storage_web_server_ip;
    s_storage_web_server_port = storage_web_server_port;
    return 0;
}
