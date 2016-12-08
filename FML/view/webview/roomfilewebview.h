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
	// ˢ���ļ��б�
	void appendRoomFileList(const CRoomFileInfoList &roomfiles);
	// ���ҳ��
	void ClearFileList();

private slots:
	// ���سɹ� bResult: 0 success, or fail
	void slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult);
	// ���ؽ���
	void slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize);
	void finishInit();
	// ����web����
	void slotHandle(const QString &caller, const QString &val);

private:
	// ����
	void downloadfile(const QString &fileid);
	// ɾ��
	void deleteFile(const QString &fileid);
	// ��Ŀ¼
	bool openFilePath(const QString &fileid);
	// ���ļ�
	bool openFile(const QString &fileid);
	// �鿴����
	void searchmore(const QString &fileid);

	// ��ȡ�ļ�Icon
	QString getFileIcon(const string &fileext);
	// ��ȡ�ļ�ɾ��Ȩ��
	bool getFileDeleteRight(const ID& id);
};

#endif // ROOMFILEWEBVIEW_H
