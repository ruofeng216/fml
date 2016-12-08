#ifndef ROOMFILEWEBVIEW_H
#define ROOMFILEWEBVIEW_H

#include "webview.h"

class RoomFileWebChannel;
class roomfilewebview : public webview
{
	Q_OBJECT

public:
	roomfilewebview(QWidget *parent = 0);
	~roomfilewebview();

public:
	// 刷新文件列表
	void appendRoomFileList(const CRoomFileInfoList &roomfiles);
	// 清空页面
	void ClearFileList();

private slots:
	// 下载成功 bResult: 0 success, or fail
	void slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult);
	// 下载进度
	void slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize);
	void finishInit();
	// 处理web调用
	void slotHandle(const QString &caller, const QString &val);

private:
	// 下载
	void downloadfile(const QString &fileid);
	// 删除
	void deleteFile(const QString &fileid);
	// 打开目录
	bool openFilePath(const QString &fileid);
	// 打开文件
	bool openFile(const QString &fileid);
	// 查看更多
	void searchmore(const QString &fileid);

	// 获取文件Icon
	QString getFileIcon(const string &fileext);
	// 获取文件删除权限
	bool getFileDeleteRight(const ID& id);
};

#endif // ROOMFILEWEBVIEW_H
