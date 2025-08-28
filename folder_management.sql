-- 文件夹管理功能数据库表结构

-- 创建文件夹表
DROP TABLE IF EXISTS `folders`;
CREATE TABLE `folders` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '文件夹ID',
  `user_id` int(11) NOT NULL COMMENT '用户ID',
  `parent_id` int(11) DEFAULT NULL COMMENT '父文件夹ID，根文件夹为NULL',
  `folder_name` varchar(128) NOT NULL COMMENT '文件夹名称',
  `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `update_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  `is_deleted` tinyint(1) DEFAULT 0 COMMENT '是否已删除，0-未删除，1-已删除',
  PRIMARY KEY (`id`),
  KEY `idx_user_id` (`user_id`),
  KEY `idx_parent_id` (`parent_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='文件夹表';

-- 创建文件与文件夹关联表
DROP TABLE IF EXISTS `file_folder_relations`;
CREATE TABLE `file_folder_relations` (
  `file_id` int(11) NOT NULL COMMENT '文件ID',
  `folder_id` int(11) NOT NULL COMMENT '文件夹ID',
  `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  PRIMARY KEY (`file_id`, `folder_id`),
  KEY `idx_folder_id` (`folder_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='文件与文件夹关联表';

-- 修改用户文件列表表，添加文件夹ID字段
ALTER TABLE `user_file_list` ADD COLUMN `folder_id` int(11) DEFAULT NULL COMMENT '所属文件夹ID' AFTER `file_name`;

-- 插入根文件夹记录（每个用户一个根文件夹）
INSERT INTO folders (user_id, parent_id, folder_name, is_deleted)
SELECT id, NULL, '我的文件', 0 FROM user_info;
