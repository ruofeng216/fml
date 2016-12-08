#include "roomfilewebview.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QDateTime>
#include <QDesktopServices>

#include "dataType/datatype.h"
#include "controller_manager.h"
#include "qutil.h"
#include "json.h"
#include "app_info.h"
#include "view_signal_manager.h"
#include "message_box_widget.h"
#include "sync_url_request.h"
#include "GlobalConstants.h"
#include "webchannel.h"

roomfilewebview::roomfilewebview(QWidget *parent)
	: webview(parent, VIEW_ROOMFILE)
{
}

roomfilewebview::~roomfilewebview()
{

}

// 刷新文件列表
void roomfilewebview::appendRoomFileList(const CRoomFileInfoList &roomfiles)
{
	QVariantMap val;
	QList<QVariant> roomfilelst;
	for (size_t i = 0; i < roomfiles.size(); i++)
	{
		QVariantMap rf;
		rf["fileid"] = qutil::toQString(roomfiles[i].GetFileUUID());
		rf["resDownload"] = roomfiles[i].GetStatus();
		rf["isSharePerson"] = getFileDeleteRight(roomfiles[i].GetOwnerId());
		rf["roomfileicon"] = qutil::toBase64(getFileIcon(roomfiles[i].GetFileExt()));
		rf["filename"] = qutil::toBase64(qutil::cutShowText(qutil::toQString(roomfiles[i].GetFileName()), 270, 15, Qt::ElideMiddle));
		rf["filedetailname"] = qutil::toBase64(qutil::toQString(roomfiles[i].GetFileName()));
		rf["filetime"] = QDateTime::fromMSecsSinceEpoch(qutil::toQString(roomfiles[i].GetCreatTime()).toULongLong()).toString("yyyy-MM-dd");
		rf["shareperson"] = qutil::toBase64(qutil::toQString(roomfiles[i].GetOwnerDisplayName()));
		rf["filesize"] = roomfiles[i].GetFileSize();
		rf["filesizedone"] = roomfiles[i].GetFileSizeDone();
		roomfilelst.append(rf);
	}
	val["roomfiles"] = roomfilelst;
	QString str = QString("rfManager.appendRoomFiles('%1')").arg(json::toString(val));
	page()->runJavaScript(str);
}

// 清空页面
void roomfilewebview::ClearFileList()
{
	QString str = QString("rfManager.ClearMsg()");
	page()->runJavaScript(str);
}

void roomfilewebview::finishInit()
{
	QVariantMap textmap;
	textmap["by"] = "by";// by  
	textmap["downloadfile"] = tr("roomdownloadfile");// 下载
	textmap["openfilepath"] = tr("openfilepath");// 打开文件夹
	textmap["deletefile"] = tr("deletefile");// 删除
	QString content = QString("rfManager.initTxtSrc('%1')").arg(json::toString(textmap));
	page()->runJavaScript(content);
}

// 处理
void roomfilewebview::slotHandle(const QString &caller, const QString &val)
{
	// 操作返回值
	QVariant res;
	QVariantMap mp = json::toMap(QByteArray::fromBase64(val.toUtf8()));
	if (mp["type"].toString() == "openfilepath") 
	{
		TRACK(HISTORY_ROOM_FILE_TAB_OPEN_FOLDER);
		bool result = openFilePath(mp["value"].toString());
		mp.clear();
		mp["result"] = result;
		emit channel()->sigCppResult(caller, json::toString(mp));
	}
	else if (mp["type"].toString() == "openfile")
	{
		bool result = openFile(mp["value"].toString());
		mp.clear();
		mp["result"] = result;
		emit channel()->sigCppResult(caller, json::toString(mp));
	}
	else if (mp["type"].toString() == "deletefile")
	{
		deleteFile(mp["value"].toString());
	}
	else if (mp["type"].toString() == "downloadfile")
	{
		TRACK(HISTORY_ROOM_FILE_TAB_DOWNLOAD);
		downloadfile(mp["value"].toString());
	}
	else if (mp["type"].toString() == "searchmore")
	{
		searchmore(mp["value"].toString());
	}
}

// 下载成功 bResult: 0 success, or fail
void roomfilewebview::slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult)
{
	QVariantMap val;
	val["fileid"] = qutil::toQString(fuuid);
	val["res"] = nResult;
	QString content = QString("rfManager.UpdateDownloadResult('%1')").arg(json::toString(val));
	page()->runJavaScript(content);
	CRoomFileInfo rinfo;
	if (ControllerManager::instance()->getCrowdInstance()->GetRoomFile(getUserid(), fuuid, rinfo))
	{
		if (nResult==0)
		{
			if (rinfo.GetStatus() != CRoomFileInfo::e_Success)
			{
				rinfo.SetStatus(CRoomFileInfo::e_Success);
				rinfo.SetFileFullName(localPath);
				rinfo.SetFileSizeDone(rinfo.GetFileSize());
				ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), fuuid, rinfo);
			}
		}
		else
		{
			if (rinfo.GetStatus() != CRoomFileInfo::e_Default)
			{
				rinfo.SetStatus(CRoomFileInfo::e_Default);
				rinfo.SetFileFullName("");
				rinfo.SetFileSizeDone(0);
				ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), fuuid, rinfo);
			}
		}

		if (nResult != 0)
		{
			ShowErrorMessage("", tr("file download faile, please again."));
		}
	}
}
// 下载进度
void roomfilewebview::slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize)
{
	QVariantMap val;
	val["fileid"] = qutil::toQString(fuuid);
	val["donesize"] = donesize;
	val["totalsize"] = totalsize;
	QString content = QString("rfManager.UpdateDownloadProgress('%1')").arg(json::toString(val));
	page()->runJavaScript(content);
	CRoomFileInfo rinfo;
	if (ControllerManager::instance()->getCrowdInstance()->GetRoomFile(getUserid(), fuuid, rinfo))
	{
		rinfo.SetStatus(CRoomFileInfo::e_Downloading);
		rinfo.SetFileSizeDone(donesize);
		ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), fuuid, rinfo);
	}
}

// 下载
void roomfilewebview::downloadfile(const QString &fileid)
{
	CRoomFileInfo rinfo;
	if (ControllerManager::instance()->getCrowdInstance()->GetRoomFile(getUserid(), qutil::toString(fileid), rinfo))
	{
		if (rinfo.GetStatus() == CRoomFileInfo::e_Default)
		{
			QString showSuffix;
			QString fext = qutil::toQString(rinfo.GetFileExt());
			if (!rinfo.GetFileExt().empty())
				showSuffix =QString(tr("File (*.%1)")).arg(qutil::toQString(rinfo.GetFileExt()));
			else
				showSuffix = tr("All File (*.*)");

			// 打开存储窗口
			QString fpath = QFileDialog::getSaveFileName(this, tr("filesaveas"), qutil::toQString(AppInfo::getFileRecvPath(false)+"\\"+rinfo.GetFileName()),showSuffix);
			if (!fpath.isEmpty())
			{
				std::string sPath = qutil::toString(fpath);
				SyncUrlRequest::instance()->addFile(rinfo.GetFileUUID(), sPath);
				rinfo.SetFileFullName(sPath);
				rinfo.SetStatus(CRoomFileInfo::e_Downloading);
				rinfo.SetFileSizeDone(0);
				ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), rinfo.GetFileUUID(), rinfo);

				QString tPath = fpath.left(fpath.lastIndexOf("/")==-1?fpath.lastIndexOf("\\"):fpath.lastIndexOf("/"));
				CSystemConfig cg = ControllerManager::instance()->getPersonInstance()->GetSystemConfig();
				if (cg.GetRecDir() != qutil::toString(tPath))
				{
					cg.SetRecDir(qutil::toString(tPath));
					ControllerManager::instance()->getPersonInstance()->SetSystemConfig(cg);
				}
			}
		}
		else if (rinfo.GetStatus() == CRoomFileInfo::e_Success)
		{
			QString fPath = qutil::toQString(rinfo.GetFileFullName());
			if (QFile::exists(fPath))
			{
				QVariantMap val;
				val["fileid"] = qutil::toQString(rinfo.GetFileUUID());
				val["res"] = 0;
				QString content = QString("rfManager.UpdateDownloadResult('%1')").arg(json::toString(val));
				page()->runJavaScript(content);
			}
		}
	}
}
// 删除
void roomfilewebview::deleteFile(const QString &fileid)
{
	bool res = ControllerManager::instance()->getCrowdInstance()->DeleteRoomFile(getUserid(), qutil::toString(fileid));
	if (!res)
	{
		emit s_OperateTimeout();
		return;
	}
	QString content = QString("rfManager.DelFile('%1')").arg(fileid);
	page()->runJavaScript(content);
}
// 打开目录
bool roomfilewebview::openFilePath(const QString &fileid)
{
	// 获取文件全路径
	CRoomFileInfo rinfo;
	if (ControllerManager::instance()->getCrowdInstance()->GetRoomFile(getUserid(), qutil::toString(fileid), rinfo))
	{
		if (rinfo.GetStatus() == CRoomFileInfo::e_Success)
		{
			QString fp = qutil::toQString(rinfo.GetFileFullName());
			if (QFile::exists(fp))
			{
				qutil::locateFile(fp.replace("/","\\"));
				return true;
			}
			else
			{
				rinfo.SetStatus(CRoomFileInfo::e_Default);
				rinfo.SetFileFullName("");
				rinfo.SetFileSizeDone(0);
				ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), rinfo.GetFileUUID(), rinfo);
				ShowErrorMessage(tr("fileoperate"), tr("file no exist"));
				return false;
			}
		}
	}
	return true;
}
// 打开文件
bool roomfilewebview::openFile(const QString &fileid)
{
	// 获取文件全路径
	CRoomFileInfo rinfo;
	if (ControllerManager::instance()->getCrowdInstance()->GetRoomFile(getUserid(), qutil::toString(fileid), rinfo))
	{
		if (rinfo.GetStatus() == CRoomFileInfo::e_Success)
		{
			QString fp = qutil::toQString(rinfo.GetFileFullName());
			if (QFile::exists(fp))
			{
				QString strPath="file:///";
				strPath.append(fp);
				QDesktopServices::openUrl(QUrl(strPath, QUrl::TolerantMode));
				return true;
			}
			else
			{// 提示错误
				rinfo.SetStatus(CRoomFileInfo::e_Default);
				rinfo.SetFileFullName("");
				rinfo.SetFileSizeDone(0);
				ControllerManager::instance()->getCrowdInstance()->UpdateRoomFile(getUserid(), rinfo.GetFileUUID(), rinfo);
				ShowErrorMessage(tr("fileoperate"), tr("file no exist"));
				return false;
			}
		}
	}
	return true;
}
// 查看更多
void roomfilewebview::searchmore(const QString &fileid)
{
	CRoomFileDisplayInfo roomfiles;
	ControllerManager::instance()->getCrowdInstance()->GetRoomFileList(getUserid(), g_roomfileshowmore, qutil::toString(fileid), roomfiles);
	appendRoomFileList(roomfiles.GetRoomFileList());
}

// 获取文件Icon
QString roomfilewebview::getFileIcon(const string &fileext)
{
	QString fileicon;
	QString fext = qutil::toQString(fileext);
	if (fext.compare(".DOC", Qt::CaseInsensitive)==0 ||
		fext.compare(".DOCX", Qt::CaseInsensitive)==0 ||
		fext.compare(".DOCM", Qt::CaseInsensitive)==0 ||
		fext.compare(".DOTX", Qt::CaseInsensitive)==0 ||
		fext.compare(".DOTM", Qt::CaseInsensitive)==0 ||
		fext.compare(".DOT", Qt::CaseInsensitive)==0 ||
		fext.compare(".WTF", Qt::CaseInsensitive)==0 ||
		fext.compare(".WPS", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/icon_DOC.png");
	} else if (fext.compare(".PDF", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/icon_PDF.png");
	} else if (fext.compare(".PPT", Qt::CaseInsensitive)==0 ||
		       fext.compare(".PPTX", Qt::CaseInsensitive)==0 ||
			   fext.compare(".PPTM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".POTX", Qt::CaseInsensitive)==0 ||
			   fext.compare(".POTM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".POT", Qt::CaseInsensitive)==0 ||
			   fext.compare(".PPSX", Qt::CaseInsensitive)==0 ||
			   fext.compare(".PPSM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".PPAM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".PPA", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/icon_PPT.png");
	} else if (fext.compare(".XLS", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLSX", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLSM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLSB", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLTX", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLTM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLT", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLAM", Qt::CaseInsensitive)==0 ||
			   fext.compare(".XLA", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/icon_XLS.png");
	} else if (fext.compare(".JPG", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/JPG.png");
	} else if (fext.compare(".PNG", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/PNG.png");
	} else if (fext.compare(".RAR", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/RAR.png");
	} else if (fext.compare(".TXT", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/TXT.png");
	} else if (fext.compare(".ZIP", Qt::CaseInsensitive)==0) {
		fileicon = "file:///" + qutil::skin("fileicon/ZIP.png");
	} else {
		fileicon = "file:///" + qutil::skin("fileicon/icon_unknown.png");
	}
	return fileicon;
}

// 获取文件删除权限
bool roomfilewebview::getFileDeleteRight(const ID& id)
{
	bool bRight = false;
	if (id == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
		bRight = true;
	CRoomMember roommember;
	ControllerManager::instance()->getCrowdInstance()->GetRoomMemberInfo(getUserid(), ControllerManager::instance()->getPersonInstance()->GetCurrentUserID(), roommember);
	if (roommember.GetRoomRole() == RoomRole_Admin || roommember.GetRoomRole() == RoomRole_Owner)
		bRight = true;
	return bRight;
}