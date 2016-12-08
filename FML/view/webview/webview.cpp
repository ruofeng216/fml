#include "webview.h"
#include <QDesktopServices>
#include <QUuid> 
#include <QEventLoop>
#include <QWebEngineHistory>
#include <QWebEngineSettings>
#include <QTimer>
#include <QFile>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QClipboard>
#include <QMimeData>
#include <QPointer>
#include <QWaitCondition>
#include <QApplication>
#include <QDesktopWidget>
#include <QBuffer>
#include <QNetworkReply>
#include <QDrag>
 
#include "dataType/datatype.h"
#include "controller_manager.h"
#include "qutil.h"
#include "json.h"
#include "app_info.h"
#include "view_controller.h"
#include "imageedit.h"
#include "right_click_menu.h"
#include "user_detail_widget.h"
#include "GlobalConstants.h"
#include "message_box_widget.h"
#include "in_room_search_widget.h"
#include "noticepop.h"
#include "friend_request_widget.h"
#include "view_signal_manager.h"
#include "sync_url_request.h"
#include "webpage.h"
#include "webchannel.h"

// ��˫��ʱ����
#define CLICKTIMER (450)
#define PURCHASETIMER (500)
#define MMSTATUSTIMER (500)

void id_replace(ULONG64 oldId, const std::string& newId, const std::function<void(const std::string&)> &setNewId)
{
	if (newId.empty()) {
		setNewId(ID::MakeQmID(oldId).GetID());
	}
}

webview::webview(QWidget *parent, VIEWSTYLE_e eStyle)
	: QWebEngineView(parent)
	, m_viewstyle(eStyle)
	, m_clicktimer(0)
	, m_Purchase(0)
	, m_channel(new WebViewChannel(this))
	, m_page(0)
	, m_isWebLoadFinished(false)
	, m_isWebChannelInit(false)
{
	LOG_INFO << "webview init";
	m_page = new WebPage(this);
	this->setPage(m_page);
	page()->setWebChannel(m_channel->channel());

	connect(m_channel, &WebViewChannel::sigWebChannelInit, [this]() {
		m_isWebChannelInit = true; 
		if (m_isWebLoadFinished) {
			emit s_init();
		}
	});
	connect(m_channel, &WebViewChannel::sigContextMenu, this, &webview::slotContextMenu, Qt::QueuedConnection);
	connect(m_channel, &WebChannel::sigHandle, this, &webview::slotHandle);
	connect(this->page(), SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));
	connect(this->page(), SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
	connect(m_page, SIGNAL(sigLoadUrl(const QUrl &)), this, SLOT(hyperlinkClicked(const QUrl &)));

	setMinimumSize(QSize(100,100));
	setAcceptDrops(false);
	setMouseTracking(true);

	if (QSysInfo::windowsVersion() < QSysInfo::WV_WINDOWS7) {
		page()->setBackgroundColor(Qt::transparent);
	} else {
		page()->setBackgroundColor(QColor("#0f0f10"));
	}
	
	setFocusPolicy(Qt::ClickFocus);
	// ��������
	setObjectName("webview_" + QUuid::createUuid().toString());

	// ��������ҳ����ʾ�ٷֱ�
	if (m_viewstyle == VIEW_CURPAGE)
	{
		const CSystemConfig &sysconfig = ControllerManager::instance()->getPersonInstance()->GetSystemConfig();
		this->page()->setZoomFactor((sysconfig.GetShowPercent()*1.0)/100);
	}
	
	connect(ViewController::instance(), SIGNAL(s_updateSetting()), this, SLOT(slotUpdateSetting()));
	connect(ViewController::instance(), &ViewController::s_WebviewImageReplaceNty, this, &webview::slotWebviewImageReplaceNty);
	connect(ViewSignalManager::instance(), &ViewSignalManager::s_downloadprogress, this, &webview::slotdownloadprogress, Qt::QueuedConnection);
	connect(ViewSignalManager::instance(), &ViewSignalManager::s_downloadresult, this, &webview::slotdownloadresult, Qt::QueuedConnection);
    connect(ViewSignalManager::instance(), &ViewSignalManager::sigCPicDownLoadNty, this, &webview::updateGifBriefImage, Qt::QueuedConnection);

	initLables();
	LOG_INFO << "webview end";
}

webview::~webview()
{
}

WebViewChannel* webview::channel()
{
	return m_channel;
}

void webview::runjs(const QString &js, const JsResponseCb &cb)
{
	if (m_isWebLoadFinished) {
		if (cb) {
			page()->runJavaScript(js, cb);
		} else {
			page()->runJavaScript(js);
		}
	} else {
		bool isNextRepeat = false;
		if (!m_loadFinishRunList.isEmpty()) {
			const QPair<QString, JsResponseCb> &last = m_loadFinishRunList.last();
			isNextRepeat = last.first == js;
		}
		// ��һ���������ظ���
		if (isNextRepeat) {
			qDebug() << "runjs filter:" << js;
		} else {
			qDebug() << "runjs after load finished:" << js;
			m_loadFinishRunList.append(qMakePair(js, cb));
		}
	}
}

// ������ҳ
void webview::loadUrl(const QString &url)
{

	if (url.isEmpty())
	{
		// ���ر��ص���ҳ
		switch (m_viewstyle) 
		{
		case VIEW_CURPAGE: //��ǰ��Ϣչʾ
			{
				// �϶��ļ�����
				setAcceptDrops(true);
				page()->load(QUrl("file:///"+qutil::skin("js/msg.html")));
			}
			break;
		case VIEW_RECORD: // ������Ϣ��¼չʾ ����
		case VIEW_GRECORD: // ��Ϣ��¼ ȫ�ֲ���ҳ��
			page()->load(QUrl("file:///"+qutil::skin("js/history.html")));
			break;
		case VIEW_INTERACTION: // �ظ����۵� ��Ƕ���ؽ���ҳ��
			page()->load(QUrl("file:///"+qutil::skin("js/bond_reply.html")));
			break;
		case VIEW_SYS_TIPS:
			page()->load(QUrl("file:///"+qutil::skin("js/systips.html")));
			break;
		case VIEW_ROOMFILE:
			page()->load(QUrl("file:///"+qutil::skin("js/roomfile.html")));
			break;
		}
	}
	else
	{
		// ������Ƕ��ҳ
		if (url.isEmpty())
		{
			page()->load(QUrl("about:blank"));
		} 
		else
		{
			page()->load(QUrl(url));
		}
	}
	LOG_INFO << "webview load url:" << url.toStdString();
}

// �����û�id
void webview::setUserid(const ID& id)
{
	m_Chattoid = id;
}
ID webview::getUserid()
{
	return m_Chattoid;
}

void webview::finish(bool bsuccess)
{
	LOG_INFO << "webview finish start, bSuccess:" << bsuccess;
	if (bsuccess)
	{
		m_isWebLoadFinished = true;
		if (m_viewstyle != VIEW_DEFAULT)
		{
			finishInit();
			if (m_isWebChannelInit) {
				emit s_init();
			}
			for (int i = 0, count = m_loadFinishRunList.size(); i < count; i++) {
				runjs(m_loadFinishRunList[i].first, m_loadFinishRunList[i].second);
			}
			m_loadFinishRunList.clear();
		}
	}
	LOG_INFO << "webview finish end";
}

void webview::slotLoadProgress(int progress)
{
	LOG_DEBUG << "slotLoadProgress, url:" << qutil::toString(this->url().toString()) << " [" << progress << "]";
}

// ������Ӧ
void webview::hyperlinkClicked(const QUrl &links)
{
	if (!links.isEmpty())
	{
		QString str = links.toString();
		if (str.startsWith(qutil::skin("js/")))
		{
			str = str.replace(qutil::skin("js/"), "");
		}
		// ��Ҳ������
		QDesktopServices::openUrl(QUrl(str));
	}
}

// ����ͼƬ
void webview::slotWebviewImageReplaceNty(const QString &briefSrc, const QString &oldSrc)
{
	if (briefSrc.startsWith("http:")||
		!QFile::exists(briefSrc))
	{
		QString bPath = briefSrc;
		bPath = bPath.replace("\\","/");
		QStringList bList = bPath.split("/");
		QString bName = bList.back();
		QString briefP = qutil::toQString(AppInfo::getPicturesPath())+bName;
		QPixmap p(oldSrc);
		// �ߴ�ѹ��
		qutil::makePicSizeBrief(p);
		p.save(briefP, qutil::getImageFileFormat(oldSrc));
		// ˢ��ͼƬ
		QVariantMap v;
		v["oldurl"] = qutil::toBase64(briefSrc);
		v["newurl"] = qutil::toBase64("file:///" + briefP);
		v["briefwidth"] = QPixmap(briefP).width();
		v["briefheight"] = QPixmap(briefP).height();
		QString str = QString("chatMessageManager.updateImage('%1')").arg(json::toString(v));
		runjs(str);
	} 
	else if (!QFile(briefSrc).exists())
	{
		QPixmap p(oldSrc);
		// �ߴ�ѹ��
		qutil::makePicSizeBrief(p);
		p.save(briefSrc, qutil::getImageFileFormat(oldSrc));
	}
}

void webview::slotContextMenu(const QVariant &val)
{
	static bool s_isPopup = false;
	if (s_isPopup) {
		return;
	}

	s_isPopup = true;
	QVariantMap vm = json::toMap(val.toByteArray());
	QString nodeName = vm["nodeName"].toString();

	auto connectChatClear = [this](RightClickMenu &menu) {
		connect(&menu, &RightClickMenu::sigChatClear, [this](const QVariant &data){
			emit s_ClearHtml();
		});
	};

	auto connectChatProportion = [this](RightClickMenu &menu) {
		connect(&menu, &RightClickMenu::sigChatProportion, [this](const QVariant &data){
			this->page()->setZoomFactor(data.toDouble());
			setShowPercent(data.toDouble() * 100);
			emit s_ChangeInputFontWeight(data.toDouble() * 100);
		});
	};

	if (hasSelection()) {
		QVariantMap data;
		data["selected"] = "text";
		data["percent"] = this->page()->zoomFactor();
		RightClickMenu menu(getMenuType(), QVariant(data));
		connectChatClear(menu);
		connectChatProportion(menu);
		connect(&menu, &RightClickMenu::sigCopy, [this](QString &data){
			triggerPageAction(QWebEnginePage::Copy);
			fromSumscope();
		});
		menu.exec(QCursor::pos());
	} else {
		if (nodeName == "img") {
			QVariantMap data;
			data["selected"] = "image";
			// ����ͼƬ
			QString filepold = vm["imgpath"].toString();
			if (filepold.isEmpty()) {
				return;
			}
			
			filepold = filepold.replace("file:///", "");
			if (!qutil::isSystemFace(filepold)) {
				QVariantMap mp = json::toMap(vm["imgvalue"].toByteArray());
                QString imageSrc = qutil::fromBase64(mp["imagepath"].toByteArray());
                data["imageSrc"] = imageSrc;
                data["imageLnkPath"] = qutil::fromBase64(mp["imageLnkPath"].toByteArray());
                if (QFile::exists(imageSrc))
                {
                    filepold = imageSrc;
				}
			}

			QFileInfo oldfile(filepold);
			data["content"] = oldfile.filePath();
			data["percent"] = this->page()->zoomFactor();
			RightClickMenu menu(getMenuType(), QVariant(data));
			connectChatClear(menu);
			connectChatProportion(menu);
			menu.exec(QCursor::pos());
		} else {
			QVariantMap data;
			data["selected"] = "null";
			data["content"] = "";
			data["percent"] = this->page()->zoomFactor();
			RightClickMenu menu(getMenuType(), QVariant(data));
			connectChatClear(menu);
			connectChatProportion(menu);
			menu.exec(QCursor::pos());
		}
	}
	s_isPopup = false;
}

void webview::finishInit()
{
	initTextSrc();
	initErrorCode();
	SetSettingMgr();
}

// ������
void webview::slotopenurl(const QString &val)
{
	QString strUrl = qutil::fromBase64(val.toUtf8());
	if (!strUrl.isEmpty())
	{
		// ��Ҳ������
		QDesktopServices::openUrl(QUrl(strUrl));
	}
}

// ��ȡ��Ϣģ��
QString webview::slotgetHtmlContent(const QString &val)
{
	QString str;
	QFile a(val);
	if (a.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		str = a.readAll();
	}
	return str;
}

// �鿴����
void webview::slotsearchmore()
{
	emit s_searchmore();
	TRACK(OUTPUTBOX_MORE);	
}

// �鿴���࣬����Ϣ��¼
void webview::slotopenRecordtoSearchMore()
{
	emit s_openRecordtoSearchMore();
	TRACK(OUTPUTBOX_HISTORY);	
}

// �����ļ�
void webview::slotfileaccept(const QString &caller, const QString &msgid)
{
	QVariantMap mapfile;
	mapfile["msgid"] = msgid;
	mapfile["click"] = "recieve";
	mapfile["res"] = false;
	mapfile["filepath"] = "";
	ULONG64 messageid = msgid.toULongLong();
	CMessage msg;
	if (messageid>0 &&
		ControllerManager::instance()->getMessageInstance()->GetMsgFromId(m_Chattoid, messageid, msg) &&
		msg.GetBodyList().bodylist_size()==1)
	{
		QMClient::MessageBody msgbody = msg.GetBodyList().bodylist(0);
		if (msgbody.type() == QMClient::MSG_Body_Type_File)
		{
			CFileSendParam fp;
			if (ControllerManager::instance()->getMessageInstance()->FileSendParamParse(msgbody.msg(), fp))
			{
				std::string filesavep = qutil::receiveFilePath(fp.GetFileName());
				LOG_INFO << "file accept1:" << filesavep;
				mapfile["res"] = ControllerManager::instance()->getMessageInstance()->AcceptFile(m_Chattoid, messageid, filesavep);
				QFileInfo tmpf(qutil::toQString(filesavep));
				mapfile["filepath"] = qutil::toBase64(tmpf.filePath());
				emit s_FileSavepath(messageid, tmpf.filePath(), mapfile["res"].toBool());
			}
		}
	}
	emit m_channel->sigCppResult(caller, json::toString(mapfile));
}
// ����ļ�
void webview::slotfileSave(const QString &caller, const QString &msgid)
{
	QVariantMap mapfile;
	mapfile["msgid"] = msgid;
	mapfile["click"] = "saveas";
	mapfile["res"] = false;
	mapfile["filepath"] = "";
	ULONG64 messageid = msgid.toULongLong();
	CMessage msg;
	LOG_INFO << "slotfileSave,msgid:" << messageid << ",msgbodysize:" << msg.GetBodyList().bodylist_size();
	if (messageid>0 &&
		ControllerManager::instance()->getMessageInstance()->GetMsgFromId(m_Chattoid, messageid, msg) &&
		msg.GetBodyList().bodylist_size()==1)
	{
		QMClient::MessageBody msgbody = msg.GetBodyList().bodylist(0);
		if (msgbody.type() == QMClient::MSG_Body_Type_File)
		{
			CFileSendParam fp;
			if (ControllerManager::instance()->getMessageInstance()->FileSendParamParse(msgbody.msg(), fp))
			{
				// ��ȡ�ļ���׺
				QFileInfo srcInfo(qutil::toQString(fp.GetFileName()));
				QString showSuffix;
				QString filename = qutil::toQString(fp.GetFileName());
				if (!srcInfo.suffix().isEmpty()) {
					showSuffix =QString(tr("File (*.%1)")).arg(srcInfo.suffix());
				} else {
					showSuffix = tr("All File (*.*)");
				}
				// �򿪴洢����
				QString fpath = QFileDialog::getSaveFileName(this, tr("filesaveas"), qutil::toQString(AppInfo::getFileRecvPath(false)+"\\")+filename,showSuffix);
				if (!fpath.isEmpty())
				{
					LOG_INFO << "file accept2:" << fpath.toLocal8Bit().data();
					mapfile["res"] = ControllerManager::instance()->getMessageInstance()->AcceptFile(m_Chattoid, messageid, qutil::toString(fpath));
					mapfile["filepath"] = qutil::toBase64(fpath);
					emit s_FileSavepath(messageid, fpath, mapfile["res"].toBool());
					if (mapfile["res"].toBool())
					{
						QString tPath = fpath.left(fpath.lastIndexOf("/")==-1?fpath.lastIndexOf("\\"):fpath.lastIndexOf("/"));
						CSystemConfig cg = ControllerManager::instance()->getPersonInstance()->GetSystemConfig();
						if (cg.GetRecDir() != qutil::toString(tPath))
						{
							cg.SetRecDir(qutil::toString(tPath));
							ControllerManager::instance()->getPersonInstance()->SetSystemConfig(cg);
						}
					}
				}
			}
			else {
				LOG_ERROR << "slotfileSave FileSendParamParse failed";
			}
		}
		else {
			LOG_ERROR << "slotfileSave type != MSG_Body_Type_File";
		}
	}
	emit m_channel->sigCppResult(caller, json::toString(mapfile));
}
// �ܾ������ļ�
void webview::slotfilereject(const QString &caller, const QString &msgid)
{
	QVariantMap mapfile;
	mapfile["msgid"] = msgid;
	bool b = ControllerManager::instance()->getMessageInstance()->RefuseFile(m_Chattoid, msgid.toULongLong());
	emit s_FileRejectCancel(msgid.toULongLong(), b);
	mapfile["result"] = b;
	emit m_channel->sigCppResult(caller, json::toString(mapfile));
}

// ����ת���߷���
void webview::slotOfflineSend(const QString &caller, const QString &msgid)
{
	QVariantMap mapfile;
	mapfile["msgid"] = msgid;
	bool b = ControllerManager::instance()->getMessageInstance()->OfflineSend(m_Chattoid, msgid.toULongLong());
	if (!b) {
		emit s_OperateTimeout();
	}
	mapfile["result"] = b;
	emit m_channel->sigCppResult(caller, json::toString(mapfile));
}

// ��ͼƬ  ͼƬ�Ŵ���ʾ
void webview::slotimageDbclickShow(const QString &val)
{
	QVariantMap mp = json::toMap(val.toUtf8());
	QString filepold = qutil::fromBase64(mp["imagepath"].toByteArray());
	filepold = filepold.replace(QRegExp("file:/+"), "");
	filepold = filepold.startsWith("qrc:/")?filepold.replace("qrc:/", ":/"):filepold;
	QFileInfo oldfile(filepold);
	QString filepbrief = qutil::fromBase64(mp["imagebrief"].toByteArray());
	if (filepbrief.startsWith("file://"))
	{
		filepbrief = filepbrief.replace(QRegExp("file:/+"), "");
	    filepbrief = filepbrief.startsWith("qrc:/")?filepbrief.replace("qrc:/", ":/"):filepbrief;
	}
	
	//if (QFile::exists(filepbrief))
	{
		QVariantMap fp;
		fp["imagepath"] = filepold;
        fp["imageLnkPath"] = qutil::fromBase64(mp["imageLnkPath"].toByteArray());
		fp["imagebrief"] = filepbrief;
		fp["chatid"] = mp["chatid"];
		fp["msgid"] = mp["msgid"];
		fp["width"] = mp["imagebriefwidth"];
		fp["height"] = mp["imagebriefheight"];
		
		if (ViewController::instance()->IsExist(IMAGE_EDIT_WINDOW_ID))
		{
			ViewController::instance()->closewnd(IMAGE_EDIT_WINDOW_ID);
			ViewController::instance()->openwnd(IMAGE_EDIT_WINDOW_ID, "", false, fp);
		}
		else
		{
			ViewController::instance()->openwnd(IMAGE_EDIT_WINDOW_ID, "", false, fp);
		}
	}
}


// ���·���
void webview::slotresendmsg(const QString &val)
{
	emit s_resendmsg(val);
}

// ������
void webview::slotOpenInfoWnd(const QString &val)
{
	ID otherid = ID::MakeID(val.toStdString());
	if (otherid.IsQqDomain()) {
		ID qmOtherId;
		if (!ControllerManager::instance()->getQQInstance()->GetQQBindQM(otherid, qmOtherId)) {
			LOG_INFO << "qqmsg, slotOpenInfoWnd, GetQQBindQM failed, ID:" << otherid.GetID();
			return;
		}
	}

	if (ControllerManager::instance()->getPersonInstance()->GetCurrentUserID() == otherid) {
		return;
	}

	// fixme QM����ID
	if ((otherid.IsQmUserId() && m_Chattoid.IsQmRoomId()) || otherid.IsQqDomain()) // ���ǵ�ǰ��ϵ����
	{
		if (m_openuserinfo != otherid)
		{
			if (m_clicktimer > 0)
			{
				killTimer(m_clicktimer);
				m_clicktimer = 0;
			}
			m_openuserinfo = otherid;
			m_infop = QCursor::pos();
			m_clicktimer = startTimer(CLICKTIMER);
		}
		else
		{
			if (m_clicktimer > 0)
			{
				killTimer(m_clicktimer);
				m_clicktimer = 0;
				m_openuserinfo = ID::NULL_ID;
			}
		}
	}
}

// ������˵����촰��
void webview::slotOpenChatto(const QString &val)
{
	ID otherid = ID::MakeID(val.toStdString());
	if (ControllerManager::instance()->getPersonInstance()->GetCurrentUserID() == otherid) {
		return;
	}

	if (otherid.IsQmUserId() || otherid.IsQqUserId()) {
		ViewController::instance()->openChat(otherid);
		UserDetailWidget::instance()->hide();
	}
}

// ���ļ���
void webview::slotOpenFilepath(const QString &val)
{
	QString decodestr = QString::fromUtf8(QByteArray::fromBase64(val.toUtf8()));
	if (QFile::exists(decodestr))
	{
		qutil::locateFile(decodestr.replace("/","\\"));
	}
	else
	{// ��ʾ����
		ShowErrorMessage(tr("fileoperate"), tr("file no exist"));
	}
}
// ���ļ�
void webview::slotOpenFile(const QString &val)
{
	TRACK(OUTPUTBOX_FILE_OPEN);
	QString decodestr = QString::fromUtf8(QByteArray::fromBase64(val.toUtf8()));
	if (QFile::exists(decodestr))
	{
 		QString strPath="file:///";
 		strPath.append(decodestr);
 		QDesktopServices::openUrl(QUrl(strPath, QUrl::TolerantMode));
	}
	else
	{// ��ʾ����
		ShowErrorMessage(tr("fileoperate"), tr("file no exist"));
	}
}

// ��ʷ��Ϣ��¼��ҳ
void webview::slothistoryPage(const QString &val)
{
	bool bturn = (bool)val.toInt();// false ���ϣ�true ����
	emit s_historyPage(bturn);
}

// ���´��ı���Ϣ����
void webview::slotUpdateBigTxtMsgShow(const QString &val)
{
	if (!val.isEmpty())
	{
		if (m_viewstyle == VIEW_CURPAGE)
		{
			emit s_getBigTxtmsg(val);
		}
		else if (m_viewstyle == VIEW_RECORD)
		{
			ULONG64 msgid = val.toULongLong();
			CMessage cmsg;
			if (msgid>0 &&
				ControllerManager::instance()->getMessageInstance()->GetMsgFromId(m_Chattoid, msgid, cmsg))
			{
				updateBigTxtmsg(cmsg);
			}
		}
	}
}

// Ⱥ����Աչʾ
void webview::slotShowGroupMassReceivers(const QString &val)
{
	QVariantMap mp = json::toMap(val.toUtf8());
	CContactInfoSnap cinfo;
	std::vector<ID> ids;
	std::vector<STRINGUTF8> namelst;           
	std::vector<STRINGUTF8> companylst;
	for (int i = 0; i < mp["ids"].toList().size(); i++)
	{
		ids.push_back(ID::MakeQmID(mp["ids"].toList()[i].toString().toStdString()));
	}
	for (int i = 0; i < mp["names"].toList().size(); i++)
	{
		namelst.push_back(qutil::toString(mp["names"].toList()[i].toString()));
	}
	for (int i = 0; i < mp["companys"].toList().size(); i++)
	{
		companylst.push_back(qutil::toString(mp["companys"].toList()[i].toString()));
	}
	cinfo.SetIdList(ids);
	cinfo.SetNameList(namelst);
	cinfo.SetCompanyNameList(companylst);

	QPoint pos = QCursor::pos();
	
	InRoomSearchWidget *inRoomSearchWidget = new InRoomSearchWidget(cinfo, InRoomSearchWidget::Top, this);
	if (pos.x()+inRoomSearchWidget->width() > QApplication::desktop()->width())
	{
		pos.setX(pos.x() - inRoomSearchWidget->width());
	}
	if (pos.y()+inRoomSearchWidget->height() > QApplication::desktop()->height())
	{
		pos.setY(pos.y() - inRoomSearchWidget->height());
	}
	inRoomSearchWidget->show();
	inRoomSearchWidget->move(pos);
}

void webview::slotStartDrag(const QString &type, const QString &val)
{
	if (type.isEmpty() || val.isEmpty()) {
		return;
	}

	if (type == "html") {
		QDrag *drag = new QDrag(this);
		QMimeData *mimeData = new QMimeData();
		mimeData->setHtml(val);
		drag->setMimeData(mimeData);
		Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
	} else if (type == "img") {
		QString imagePath = val;
		if (imagePath.startsWith("file://")) {
			imagePath = imagePath.replace(QRegExp("file:/+"), "");
			imagePath = imagePath.startsWith("qrc:/") ? imagePath.replace("qrc:/", ":/") : imagePath;
		}
		QPixmap pix(imagePath);
		if (!pix.isNull()) {
			QDrag *drag = new QDrag(this);
			QMimeData *mimeData = new QMimeData();
			QString html = QString("<img src='%1'></img>").arg(imagePath);
			mimeData->setHtml(html);
			drag->setMimeData(mimeData);
			drag->setPixmap(pix);
			drag->setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));
			Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
		}
	}
}


// ���ծȯ����  QB ��ϵ���۷�
void webview::slotCreditoright(const QString &val)
{
	TRACK(OUTPUTBOX_PRICE_REPLY);
	QVariantMap mp = json::toMap(val.toUtf8());
	if (ID::MakeQmID(mp["fromid"].toString().toStdString()) == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
	{
		return;
	}

	mp["chatId"] = m_Chattoid.GetID_C_Str();
	QByteArray cachedata = QByteArray::fromBase64(mp["msgvalue"].toByteArray());
	STRINGUTF8 strCache = STRINGUTF8(cachedata.constData(), cachedata.length());
	switch (mp["type"].toInt())
	{
	case QMClient::MSG_Body_Type_QB_QuoteMoney: // �ʽ�
		{
			QMClient::QuotationMoneyInfo quoteInfo;
			quoteInfo.ParseFromString(strCache);
			
			ID uid = m_Chattoid.IsQmUserId() ? ControllerManager::instance()->getPersonInstance()->GetCurrentUserID() : m_Chattoid;
			if (ControllerManager::instance()->getQbInstance()->IsBondValid(QMClient::MSG_Body_Type_QB_QuoteMoney, CQuoteState(quoteInfo.quotationid(), uid)))
			{//�ʽ����Ƿ���Ч
				QString wndid = CONTACT_QUOTED_WINDOW_ID+QString("_MM%1").arg(qutil::toQString(quoteInfo.quotationid()));
				if (ViewController::instance()->AppendConnectWithQB(wndid))
				{
					ViewController::instance()->openwndNonblocking(wndid, mp["fromid"].toString(), true, mp);
				}
			}
			else
			{
				NoticePop *pNoticePop = new NoticePop(NULL, tr("quotation invalid"), QCursor::pos());
			}
			break;
		}
	case QMClient::MSG_Body_Type_QB_QuoteConditions: // ����
		{
			break;
		}
	case QMClient::MSG_Body_Type_QB_QuoteBond: // ծȯ
		{
			QMClient::QuotationBondInfo bondmsginfo;
			bondmsginfo.ParseFromString(strCache);

			ID uid = m_Chattoid.IsQmUserId() ? ControllerManager::instance()->getPersonInstance()->GetCurrentUserID() : m_Chattoid;

			if (ControllerManager::instance()->getQbInstance()->IsBondValid(QMClient::MSG_Body_Type_QB_QuoteBond, CQuoteState(bondmsginfo.quotationid(), uid)))
			{// ծȯ
				QString wndid = CONTACT_QUOTED_WINDOW_ID+QString("_Bond%1").arg(qutil::toQString(bondmsginfo.quotationid()));
				if (ViewController::instance()->AppendConnectWithQB(wndid))
				{
					ViewController::instance()->openwndNonblocking(wndid, mp["fromid"].toString(), true, mp);
				}
			} 
			else
			{
				NoticePop *pNoticePop = new NoticePop(NULL, tr("quotation invalid"), QCursor::pos());
			}
			
			break;
		}
	default:
		break;
	}
}

// ���Ⱥ��Ƭ
void webview::slotRoomcard(const QString &caller, const QString &val)
{
	QVariantMap mp = json::toMap(val.toUtf8());
	ID myid = ControllerManager::instance()->getPersonInstance()->GetCurrentUserID();
	ID roomid = ID::MakeQmID(mp["roomid"].toString().toStdString());
	bool bstatus = mp["status"].toBool();
	mp["chatId"] = m_Chattoid.GetID_C_Str();
	if (bstatus)
	{// ������Ⱥ
		TRACK(OUTPUTBOX_SHARE_ROOMCARD_INROOM);
		bool isin = ControllerManager::instance()->getCrowdInstance()->IsUserInRoom(myid, roomid);
		if (isin)
		{
			if (MessageBoxWidget::Yes == ShowQuestionMessage(tr("smart tips"), tr("you are now in the room, whether to chat in?"), ViewController::instance()->getPopupWidget(myid)))
			{
				ViewController::instance()->openChat(roomid);
			}
			
			bstatus = false;
		}
		else
		{
			ViewController::instance()->openwnd(APPLYFORROOM_WINDOW_ID, tr("apply for room"), false, mp);
		}
	} 
	else
	{// ������Ϣ
		TRACK(OUTPUTBOX_SHARE_ROOMCARD_SENDMSG);
		bool isin = ControllerManager::instance()->getCrowdInstance()->IsUserInRoom(myid, roomid);
		if (isin)
		{
			ViewController::instance()->openChat(roomid);
		}
		else
		{
			if (MessageBoxWidget::Yes == ShowQuestionMessage(tr("smart tips"), tr("you are now not in the room, whether to apply in?"), ViewController::instance()->getPopupWidget(myid)))
			{
				ViewController::instance()->openwnd(APPLYFORROOM_WINDOW_ID, tr("apply for room"), false, mp);
			}
			bstatus = true;
		}
	}
	mp.clear();
	mp["result"] = bstatus;
	emit m_channel->sigCppResult(caller, json::toString(mp));
}

// ���ծȯ���룬����
void webview::slotBondOpt(const QString &val)
{
	TRACK(OUTPUTBOX_BOND_LINK_CLICK);
	ControllerManager::instance()->getQbInstance()->ShowBondDetail(qutil::toString(val));
}

// ����
void webview::slotSubscribe(const QString &caller, const QString &val)
{
	QVariantMap mp = json::toMap(val.toUtf8());
	QVariantMap SubscribeMap = json::toMap(QByteArray::fromBase64(mp["Subscribevalue"].toByteArray()));
	ID otherid = ID::MakeQmID(mp["fromid"].toString().toStdString());
	QString bondkey = SubscribeMap["bondkey"].toString();
	QString companyid = SubscribeMap["companyid"].toString();
	bool result = ControllerManager::instance()->getPersonInstance()->IsSubscribeGoods(otherid, qutil::toString(bondkey), qutil::toString(companyid));
	if (!result) {
		result = ControllerManager::instance()->getPersonInstance()->SubscribeGoods(ID::MakeQmID(mp["fromid"].toString().toStdString()), qutil::toString(companyid), qutil::toString(bondkey));
	}
	mp.clear();
	mp["result"] = result;
	emit m_channel->sigCppResult(caller, json::toString(mp));
}

// �ض�div ����ΪͼƬ
void webview::slotGrabPixmap(const QString &val)
{
	QStringList strList = val.split(",");
	if (strList.size() > 1) {
		QPixmap pix;
		pix.loadFromData(QByteArray::fromBase64(strList[1].toUtf8()));
		if (!pix.isNull()) {
			CLinkAddressInfo linkinfo;
			ControllerManager::instance()->getGlobalInstance()->GetLinkAddressInfo(linkinfo);
			QPixmap logo = QPixmap(qutil::skin("js/img_qb_logo"));
			qutil::setQuoteToClipboard(pix, logo, qutil::toQString(linkinfo.GetQuotation()), this->page()->zoomFactor());
			NoticePop *pNoticePop = new NoticePop(NULL, tr("image in clipbord"), QCursor::pos());
		}
	}
}

void webview::slotHandle(const QString &caller, const QString &val)
{
	// ��������ֵ
	QVariant res;
	QVariantMap mp = json::toMap(QByteArray::fromBase64(val.toUtf8()));
	if (mp["type"].toString() == "OpenBriefNews") {
		auto openBriefNews = [this](const QVariantMap &mp) {
			QVariantMap vm;
			QStringList urlTemp = mp["value"].toString().split(";");
			QString webUrl;
			if (urlTemp.size() > 2) {
				vm["UserId"] = urlTemp[0];
				vm["MsgId"] = urlTemp[1].toULongLong();
				webUrl = urlTemp[2];
			} else if (urlTemp.size() > 0) {
				webUrl = urlTemp[0];
			} else {
				webUrl = mp["value"].toString();
			}
			vm["WebUrl"] = webUrl;
			vm["ParamType"] = mp["paramtype"];
			vm["ShareType"] = 1;
			QString title = tr("QM NEWS");
			if (vm["ParamType"].toInt() == EUrlParamTypeNamePwd) { //�Ƿ��ǲ²²»
				title = mp["title"].toString();
			}
			vm["Title"] = title;

			int linkType = mp["linkType"].toInt();
			if (linkType > 0) {
				if (ControllerManager::instance()->getQbInstance()->IsOnline()) {
					// ͨ��QB��PDF����
					bool bDisablePdf = (linkType == 2); // ��������, 0:��ͨ���ӣ�1:pdf���ӿ����أ�2:pdf���Ӳ�������
					std::string newsTitle = qutil::toString(mp["newsTitle"].toString());
					std::string newsUrl = qutil::toString(webUrl);
					std::string newsImg = qutil::toString(mp["newsImg"].toString());
					std::string picUUID = ControllerManager::instance()->getGlobalInstance()->GetPicUUID(newsImg);
					LOG_INFO << "ReqOpenNewsDetail,linkType:" << linkType << ",title:" << newsTitle 
						<< ",url:" << newsUrl << ",imgPath:" << newsImg << ",imgUUID:" << picUUID;
					CQBShareNewsDetail detail;
					detail.SetIsDisablePDF(bDisablePdf);
					detail.SetIsPdf(true);
					detail.SetPicUUID(picUUID);
					detail.SetTitle(newsTitle);
					detail.SetURL(newsUrl);
					ControllerManager::instance()->getQbInstance()->ReqOpenNewsDetail(detail);
				} else {
					ShowWarnMessage(DefaultTitle, tr("qb offline,please login!"), this);
				}
			} else {
				ViewController::instance()->openwnd(DEFAULTWEB_WINDOW_ID + webUrl, title, false, vm);
			}
		};
		openBriefNews(mp);
	} else if (mp["type"].toString() == "ShowSessionInfo") {
		// �򿪻Ự
		ID id = ID::MakeQmID(mp["value"].toString().toStdString());
		if (!id.IsEmpty() &&
			id != ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
		{
			ViewController::instance()->openChat(id);
		}
	} else if (mp["type"].toString()=="opencontext") {
		// ��ʷ��¼�鿴������
		QString str=mp["value"].toString();
		QStringList list=str.split(";");
		TRACK(HISTORY_CONTEXT);
		if (list.size()>1)
		{
			ULONG64 msgid=list[0].toULongLong();
			ID targetid = ID::MakeQmID(list[1].toStdString());
			emit s_OpenContext(targetid,msgid);
		}
	} else if (mp["type"].toString() == "AddFriend_Again") {
		// ����Է����Լ�Ϊ����
		ID id = ID::MakeQmID(mp["value"].toString().toStdString());
		if (id.IsQmUserId() && id != ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
		{
			QVariantHash hash;
			hash.insert("reqType", FriendRequestWidget::SEND);
			hash.insert("userId", id.GetID_C_Str());
			hash.insert("addAgain", ControllerManager::instance()->getUserInstance()->IsFriend(id));
			ShowFriendRequestWidget(hash);
		}
	} else if (mp["type"].toString() == "PurchaseConfirm") {
		// һ���깺ȷ��
		QString id = mp["value"].toString();
		QStringList lst = id.split("_");
		QString purchaseid = lst[1];
		ULONG64 timeout = lst[2].toULongLong();
		ULONG64 msgid = lst[3].toULongLong();

		if (msgid > 0)
		{
			CPurchaseUkey pKey;
			pKey.SetFromId(m_Chattoid);
			pKey.SetToId(ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
			pKey.SetMsgid(msgid);
			pKey.SetPurchaseID(qutil::toString(purchaseid));
			pKey.SetModifyTime(timeout);

			if (ControllerManager::instance()->getQbInstance()->ConfirmPurchase(pKey))
			{
				emit s_PurchaseConfirm(msgid);
			}
		}
	} else if (mp["type"].toString() == "OpenQBHelpCenterLink") {
		// QB������Ϣ����
		QString weburl = mp["weburl"].toString();
		ControllerManager::instance()->getQbInstance()->ReqOpenHelpPage(weburl.toStdString());
	} else if (mp["type"].toString() == "resendMassMessage") { // Ⱥ����Ա���·���
		ULONG64 id = mp["value"].toULongLong();
		CMessage oldMessage;
		ControllerManager::instance()->getMessageInstance()->GetMsgFromId(ID::QmMassAssistantID(), id, oldMessage);

		QMClient::MessageBodyList bodylist;
		CMsgSendReq massMsg;
		vector<ID> ids;
		bool bres = false; // �����Ƿ�ƥ��
		for (int i = 0; i < oldMessage.GetBodyList().bodylist_size(); i++)
		{
			if (oldMessage.GetBodyList().bodylist(i).type() != QMClient::MSG_Body_Type_ContactInfoSnap)
			{
				QMClient::MessageBody *newM = bodylist.add_bodylist();
				*newM = oldMessage.GetBodyList().bodylist(i);
			}
			else
			{
				QMClient::ContactInfoSnap contactinfo;
				contactinfo.ParseFromString(oldMessage.GetBodyList().bodylist(i).msg());
				CProtoComp::ContactInfoSnap(contactinfo);

				if (contactinfo.ntype() == QMClient::ContactInfoSnap::e_sendfail)
				{
					bres = true;

					for (int ii = 0; ii < contactinfo.strid_size(); ii++)
					{
						ids.push_back(ID::MakeQmID(contactinfo.strid(ii)));
					}
					contactinfo.set_bsend(true);
					// ������Ϣ - �ѷ���
					oldMessage.BodyList().mutable_bodylist(i)->set_msg(contactinfo.SerializeAsString());
					ControllerManager::instance()->getMessageInstance()->UpdateLocalMsg(oldMessage);
					emit s_UpdateMessage(oldMessage);
				}
			}
		}
		if (bres)
		{
			// Ⱥ����Ϣ
			massMsg.SetBodyList(bodylist);
			massMsg.SetToIdList(ids);
			massMsg.AddToId(ID::QmMassAssistantID());
			emit ViewSignalManager::instance()->sigGroupSendMessage(massMsg);
		}
	} else if (mp["type"].toString() == "AsktoBeaddedFriends") { // Ⱥ����Ա��Ҫ��֤�Ӻ���
		ULONG64 id = mp["value"].toULongLong();
		CMessage oldMessage;
		ControllerManager::instance()->getMessageInstance()->GetMsgFromId(ID::QmMassAssistantID(), id, oldMessage);

		QMClient::MessageBodyList bodylist;
		CMsgSendReq massMsg;
		vector<ID> ids;
		bool bres = false; // �����Ƿ�ƥ��
		QMClient::ContactInfoSnap contactinfo;
		for (int i = 0; i < oldMessage.GetBodyList().bodylist_size(); i++)
		{
			if (oldMessage.GetBodyList().bodylist(i).type() != QMClient::MSG_Body_Type_ContactInfoSnap)
			{
				QMClient::MessageBody *newM = bodylist.add_bodylist();
				*newM = oldMessage.GetBodyList().bodylist(i);
			}
			else
			{
				contactinfo.ParseFromString(oldMessage.GetBodyList().bodylist(i).msg());
				if (contactinfo.ntype() == QMClient::ContactInfoSnap::e_addcheck)
				{
					bres = true;

					for (int ii = 0; ii < contactinfo.strid_size(); ii++)
					{
						ids.push_back(ID::MakeQmID(contactinfo.strid(ii)));
					}

					contactinfo.set_bsend(true);
					// ������Ϣ - �ѷ���
					oldMessage.BodyList().mutable_bodylist(i)->set_msg(contactinfo.SerializeAsString());
				}
			}
		}
		if (bres)
		{
			// ����������֤
			res = ControllerManager::instance()->getUserInstance()->AskAddFriendBatch(ids);
			if (res.toBool())
			{
				ControllerManager::instance()->getMessageInstance()->UpdateLocalMsg(oldMessage);
				emit s_UpdateMessage(oldMessage);
			}
		}
		mp.clear();
		mp["result"] = bres;
		emit m_channel->sigCppResult(caller, mp);
	} else if (mp["type"].toString() == "gotoRoomfile") { // ��Ⱥ�ļ��б�
		TRACK(OUTPUTBOX_ROOM_UPLOAD);
		emit ViewSignalManager::instance()->sigGotoRoomFile(m_Chattoid);
	} else if (mp["type"].toString() == "RoomfileRefuseSend") { // Ⱥ�ļ�ȡ���ϴ�
		QString msguuid = mp["value"].toString();
		ControllerManager::instance()->getCrowdInstance()->CancelUploadRoomFile(qutil::toString(msguuid));
	}
}

// ���سɹ� bResult: 0 success, or fail
void webview::slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult)
{
	std::string url = ControllerManager::instance()->getGlobalInstance()->GetPicURL(fuuid);
	if (nResult == 0)
	{
		QString fPath = qutil::toQString(localPath);
		// ˢ��ͼƬ
		QVariantMap v;
		v["oldurl"] = qutil::toBase64(qutil::toQString(url));
		v["newurl"] = qutil::toBase64("file:///" + fPath);
		v["briefwidth"] = QPixmap(fPath).width();
		v["briefheight"] = QPixmap(fPath).height();
		QString str = QString("chatMessageManager.updateImage('%1')").arg(json::toString(v));
		runjs(str);
	}
	else
		LOG_ERROR << "reply error, url:" << url << "(" << nResult << ")";
}
	
// ���ؽ���
void webview::slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize)
{

}

// ͼƬ(GIF)������ɣ�����Ϊ��ͼ
void webview::updateGifBriefImage(const CPicDownLoadNty &notify)
{
    STRINGUTF8 thumbnailPicUrl;
    if (!notify.GetBriefUUID().empty() && notify.GetBriefUUID() != "null")
    {
        thumbnailPicUrl = std::string("file:///") + AppInfo::getPicturesPath()
            + notify.GetBriefUUID().substr(0, notify.GetBriefUUID().find('_'));
    }

    m_updateGifBriefImage(
        notify.GetId(),
        qutil::toQString(thumbnailPicUrl),
        qutil::toQString(notify.GetFilePath()),
        (notify.GetErrorCode() == 0));

    return;
}

// ͼƬ(GIF)������ɣ�����Ϊ��ͼ
void webview::m_updateGifBriefImage(
    const ID &chatId,
    const QString &thumbnailPicUrl,
    const QString &origImageLocalPath,
    bool isDownloadOkay)
{
    if (!isDownloadOkay
        || chatId != m_Chattoid
        || QString(qutil::getImageFileFormat(origImageLocalPath)) != "GIF")
    {
        return;
    }

    if (!thumbnailPicUrl.isEmpty())
    {
        // ˢ��ͼƬ
        QVariantMap v;
        v["oldurl"] = qutil::toBase64(thumbnailPicUrl);
        v["newurl"] = qutil::toBase64("file:///" + origImageLocalPath);

        QSize origImageSize = QImage(origImageLocalPath).size();
        if (origImageSize.width() <= g_imagewidth
            && origImageSize.height() <= g_imageheight)
        {
            v["briefwidth"] = origImageSize.width();
            v["briefheight"] = origImageSize.height();
        }
        else
        {
            QSize preferredThumbnailPicSize
                = origImageSize.scaled(g_imagewidth, g_imageheight, Qt::KeepAspectRatio);
            v["imagebriefwidth"] = preferredThumbnailPicSize.width();
            v["imagebriefheight"] = preferredThumbnailPicSize.height();
        }
        QString str = QString("chatMessageManager.updateImage('%1')").arg(json::toString(v));
        runjs(str);
    }

    return;
}

// �����Ϣ
void webview::appendMessage(const CMessage &val /* ��Ϣ���� */)
{
	QString msg = analyzeMessage(val);
	QString str = QString("chatMessageManager.appendMessage('%1')").arg(msg);
    runjs(str);
	LOG_INFO << "rmsg appendMessage,chatid:" << m_Chattoid.GetID() << ",msgid:" << val.GetMsgId();
}

// �����������Ϣ  �鿴����
void webview::prependMessage(const CMessageList &vallst)
{
	if (vallst.empty())
	{
		return;
	}
	QVariantMap msglst;
	QList<QVariant> lst;
	foreach (const CMessage &val, vallst)
	{
		lst.append(analyzeMessagetoVmap(val));
	}
	msglst["message"] = lst;
	QString str = QString("chatMessageManager.prependMessage('%1')").arg(json::toString(msglst));
	runjs(str);
}

// �����������Ϣ
void webview::appendMessage(const CMessageList &vallst, int nScrollPos)
{
	if (vallst.empty())
	{
		return;
	}
	LOG_INFO << "rmsg appendMessage start,chatid:" << m_Chattoid.GetID() << ",msgsize:" << vallst.size();
	QVariantMap msglst;
	QList<QVariant> lst;
	QStringList strList;
	foreach (const CMessage &val, vallst)
	{
		lst.append(analyzeMessagetoVmap(val));
		strList << QString::number(val.GetMsgId());
	}
	const int PRINT_COUNT = 16;
	if (strList.size() > PRINT_COUNT) {
		strList = strList.mid(strList.size() - PRINT_COUNT, PRINT_COUNT);
	}
	LOG_INFO << "rmsg appendMessage end,msgid," << qutil::toString(strList.join(","));
	msglst["message"] = lst;
	msglst["scrollpos"] = nScrollPos;
	QString str = QString("chatMessageManager.appendMessageList('%1')").arg(json::toString(msglst));
	runjs(str);

    /* ��QQ���ܡ� */
    const CMessage &firstMessage = vallst.front();
    if (ControllerManager::instance()->getQQInstance()->IsQQMsg(firstMessage))
    {
        if (m_Chattoid.IsQmVirtualId())
        {
            ID qqId = ControllerManager::instance()->getQQInstance()->GetQQIdFromVQMId(m_Chattoid);

            if (firstMessage.GetTo() == m_Chattoid)
            {
                LOG_TRACE << "��QQ���ܡ�QM ������Ϣ���� qq" << qqId.GetLastID() << "�����";
            }
            if (firstMessage.GetFrom() == m_Chattoid)
            {
                LOG_TRACE << "��QQ���ܡ�QM ������Ϣ���� qq" << qqId.GetLastID() << "�����";
            }
        }
        else
        {
            if (firstMessage.GetTo() == m_Chattoid)
            {
                LOG_TRACE << "��QQ���ܡ�QM ������Ϣ�����Ѱ� QQ�����";
            }
            if (firstMessage.GetFrom() == m_Chattoid)
            {
                LOG_TRACE << "��QQ���ܡ�QM ������Ϣ�����Ѱ� QQ�����";
            }
        }
    }
}

// �����Ϣ
void webview::appendHistoryMessage(const CMessageList &vallst /* ��Ϣ���� */,
								   const QString &hilight/* �����ֶ� */,
								   const ULONG64& locateId /* ��λid*/)
{
	if (vallst.empty())
	{
		return;
	}
	QVariantMap msglst;
	QList<QVariant> lst;
	foreach (const CMessage &val, vallst)
	{
		QVariantMap msgmap = analyzeMessagetoVmap(val);
		msgmap["hilight"] = hilight;
		m_hightlight=hilight;
		lst.append(msgmap);
	}
	msglst["message"] = lst;
	msglst["locateid"] = locateId;
	QString str = QString("chatMessageManager.appendHistoryMessage('%1')").arg(json::toString(msglst));
	runjs(str);
}
// ���ҳ��
void webview::clearhtml()
{
	runjs("chatMessageManager.clearAll()");
	SetSettingMgr();
	// ǿ���������Ļ���
	//QWebSettings::clearMemoryCaches();
}

// ɾ��ҳ��ǰn����Ϣ
void webview::delHtmlmsgFromMsgid(const CMessage &val)
{
	QString str = QString("chatMessageManager.removeTop50Messages('%1')").arg(analyzeMessage(val));
	runjs(str);
	// ǿ���������Ļ���
	//QWebSettings::clearMemoryCaches();
}

// ���ù�����λ��
void webview::setScrollPosition(int nIndex)
{
	QString str = QString("chatMessageManager.setScrollbarPos('%1')").arg(nIndex);
	runjs(str);
}
// �������ײ�
void webview::ScrollToButtom()
{
	runjs("chatMessageManager.ScrollToButtom()");
}

// ��ȡ������λ��
int webview::getScrollPosition()
{
	return m_channel->slotScrollbarPos();
}

// ��ʾ�鿴����
void webview::showSearchMore()
{
	runjs("chatMessageManager.prependMessageSeeMoreDivider()");
}

// ��ʾ�鿴���࣬���Ҳ������¼
void webview::openRecordtoSearchMore(bool b)
{
	runjs(QString("chatMessageManager.prependMessageSeeHistoryDivider(%1)").arg(b));
}

// ��ʾ�ָ��� 
void webview::showSpilitline(const QString &locateId)
{
	if (locateId.isEmpty()) {
		runjs(QString("chatMessageManager.insertMessageDivider()"));
	} else {
		runjs(QString("chatMessageManager.insertMessageDivider('%1')").arg(locateId));
	}
}

// ���µ�ǰҳ�����Ϣid
void webview::updateCurPageMsgid(ULONG64 msgid, const STRINGUTF8 &uuid, int errorCode)
{
	QVariantMap mapv;
	mapv["oldmsgid"] = qutil::toQString(uuid);
	mapv["newmsgid"] = msgid;
	mapv["errorcode"] = errorCode;
	QString str = QString("chatMessageManager.updateMessageStatus('%1')").arg(json::toString(mapv));
	runjs(str);
}

// �ļ��������֪ͨ
void webview::UpdateFileSendProgress(const CFileSendProgress &fp)
{
	QVariantMap mapf;
	mapf["msgid"] = fp.GetMsgId();
	mapf["errorcode"] = fp.GetErrorCode()>0 ? 1 : 0;  // 0�ɹ���1����ʧ�ܣ�2�ܾ������ļ�
	mapf["donesize"] = fp.GetDoneSize();
	mapf["fullsize"] = fp.GetTotalSize();
	QString str = QString("chatMessageManager.refreshTransferProgress('%1')").arg(json::toString(mapf));
	runjs(str);
}

// �ļ���Ϣ״̬���֪ͨ
void webview::FileSendStatusChangeJS(const CFileSendStatusChange &fnty, bool bSend)
{
	QVariantMap mapf;
	mapf["msgid"] = fnty.GetMsgId();
	if (bSend)
	{// �Լ�����
		mapf["filestatus"] = getFileSendStatus(fnty.GetSendStatus());
	} 
	else
	{// ����
		mapf["filestatus"] = getFileRecvStatus(fnty.GetRecvStatus());
	}
	QString str = QString("chatMessageManager.refreshTransferResult('%1')").arg(json::toString(mapf));
	runjs(str);
}

// Ⱥ�ļ��ϴ��ɹ�
void webview::RoomFileUploadNotify(const CRoomFileUploadNotify &p)
{
	QVariantMap mapf;
	mapf["msgid"] = qutil::toQString(p.GetMsgUUID());
	if (p.GetErrorCode() == 0)
	{
		mapf["filestatus"] = getFileSendStatus(FileSend_Status_Done);
	}
	else if (p.GetErrorCode() == FileSend_Status_Cancel)
	{
		mapf["filestatus"] = getFileSendStatus(FileSend_Status_Cancel);
	}
	else
	{
		mapf["filestatus"] = getFileSendStatus(FileSend_Status_Failed);
	}
	QString str = QString("chatMessageManager.RoomfileUpdateResult('%1')").arg(json::toString(mapf));
	runjs(str);
}
// Ⱥ�ļ��ϴ�����
void webview::RoomFileSendProgress(const CRoomFileSendProgress &p)
{
	QVariantMap mapf;
	mapf["msgid"] = qutil::toQString(p.GetMsgUUID());
	mapf["donesize"] = p.GetDoneSize();
	mapf["fullsize"] = p.GetTotalSize();
	QString str = QString("chatMessageManager.RoomfileUpdateProgress('%1')").arg(json::toString(mapf));
	runjs(str);
}

// ϵͳ��ʾ
void webview::systemtips(Systip_e etype, const QVariantMap &valtip)
{
	QVariantMap systip;
	switch (etype)
	{
	case Tip_GroupInvite:                 //Ⱥ���������Ⱥ��ʾ  xxx ������ xxx ������Ⱥ�ġ�
		{
			systip["type"] = "Tip_GroupInvite";
			ID firstid = ID::MakeQmID(valtip["firstid"].toString().toStdString());
			ID secondid = ID::MakeQmID(valtip["secondid"].toString().toStdString());
			QString fname, sname;
			if (firstid == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
			{//�Լ�
				fname = tr("you");
			}
			else
			{
				CUserInfo fuinfo;
				ControllerManager::instance()->getUserInstance()->GetUserInfo(firstid, fuinfo);
				fname = qutil::toQString(fuinfo.GetShowName());
			}
			if (secondid == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
			{//�Լ�
				fname = tr("you");
			}
			else
			{
				CUserInfo suinfo;
				ControllerManager::instance()->getUserInstance()->GetUserInfo(secondid, suinfo);
				sname = qutil::toQString(suinfo.GetShowName());
			}
			systip["firstname"] = qutil::webShowString(fname);
			systip["content"] = "";
			systip["secondname"] = qutil::webShowString(sname);
			break;
		}
	case Tip_AddGroup:                    //Ⱥ�ı��˼���Ⱥ��ʾ  xxx ������Ⱥ�ġ�
		{
			systip["type"] = "Tip_AddGroup";
			ID firstid = ID::MakeQmID(valtip["firstid"].toString().toStdString());
			systip["firstid"] = valtip["firstid"];
			CUserInfo fuinfo;
			ControllerManager::instance()->getUserInstance()->GetUserInfo(firstid, fuinfo);
			CCompanyInfo companyinfo;
			ControllerManager::instance()->getUserInstance()->GetCompanyInfo(fuinfo.GetCompanyId(),companyinfo);
			systip["firstname"] = qutil::webShowString(qutil::toQString(fuinfo.GetShowName())+"-"+qutil::toQString(companyinfo.GetCompanyShortName()));
			systip["content"] = "";
			systip["secondname"] = "";
			break;
		}
	case Tip_MeAddGroup:                  //Ⱥ���Լ�����Ⱥ��ʾ  �� �Ѿ������˱�Ⱥ����������к��ɡ�
		{
			systip["type"] = "Tip_MeAddGroup";
			systip["firstname"] = tr("you");
			systip["content"] = "";
			systip["secondname"] = "";
			break;
		}
	case Tip_ExitGroup:                   //Ⱥ�ı����˳�Ⱥ��ʾ  xxx �˳���Ⱥ�ġ�
		{
			systip["type"] = "Tip_ExitGroup";
			ID firstid = ID::MakeQmID(valtip["firstid"].toString().toStdString());
			CUserInfo fuinfo;
			ControllerManager::instance()->getUserInstance()->GetUserInfo(firstid, fuinfo);
			systip["firstname"] = qutil::webShowString(qutil::toQString(fuinfo.GetShowName()));
			systip["content"] = "";
			systip["secondname"] = "";
			break;
		}
	case Tip_RecieveFile_OtherOffline:    //�����ļ����Է�����(���ͷ�)�����շ�չʾ  �Է������жϣ��ļ�����ʧ�ܡ�
		{
			systip["type"] = "Tip_RecieveFile_OtherOffline";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("netoff filetrans fail").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_SendFile_OtherCancel:        //�����ļ����ͷ�ȡ�������շ���ʾ   �Է�ȡ�����ļ����ļ���.��ʽ���ķ��͡�
		{
			systip["type"] = "Tip_SendFile_OtherCancel";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("other cancel filesend").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_SendFile_OtherRefuse:        //�����ļ����շ��ܾ������ͷ���ʾ   �Է��ܾ����ļ����ļ���.��ʽ���Ľ��ա�
		{
			systip["type"] = "Tip_SendFile_OtherRefuse";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("other reject filerecv").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_SendFile_Success_My:         //�����ļ��ɹ����ͷ�������ʾ   �ļ����ļ���.��ʽ������ɹ���
		{
			systip["type"] = "Tip_SendFile_Success_My";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("success send").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_SendFile_Success_Other:      //�����ļ��ɹ����շ���ʾ   �ļ����ļ���.��ʽ���ѽ��������ļ������ơ�
		{
			systip["type"] = "Tip_SendFile_Success_Other";
			QString filename = valtip["filename"].toString();
			QString filesave = valtip["filesave"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("success recv").arg(filename) + QStringLiteral("��") + filesave + QStringLiteral("����"));
			systip["secondname"] = "";
			break;
		}
	case Tip_RoomFile_UploadSuccess:      //Ⱥ�ļ��ϴ��ɹ�
		{
			systip["type"] = "Tip_RoomFile_UploadSuccess";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("roomfile upload success").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_RoomFile_UploadFailed:       //Ⱥ�ļ��ϴ�ʧ��
		{
			systip["type"] = "Tip_RoomFile_UploadFailed";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("roomfile upload failed").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_RoomFile_UploadTimeOut: // �������粻�ȶ���Ⱥ�ļ��ϴ�ʧ��
		{
			systip["type"] = "Tip_RoomFile_UploadTimeOut";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("roomfile upload failed because of your rubbish net").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_RoomFile_NoEnoughSpace: // Ⱥ�ռ䲻�㣬Ⱥ�ļ��ϴ�ʧ��
		{
			systip["type"] = "Tip_RoomFile_UploadTimeOut";
			QString filename = valtip["filename"].toString();
			systip["firstname"] = "";
			systip["content"] = qutil::webShowString(tr("no enough space, roomfile upload failed").arg(filename));
			systip["secondname"] = "";
			break;
		}
	case Tip_YouAddGroup: // ���Ѽ���Ⱥ��
		{
			systip["type"] = "Tip_YouAddGroup";
			break;
		}
	}
	QString str = QString("chatMessageManager.appendReminderMessage('%1')").arg(json::toString(systip));
	runjs(str);
}

// ���ػظ����۵�ҳ��
void webview::loadInteractionPage(const QVariantMap &val)
{
	// ͨ����Ϣ���� �� �������� ȷ��չʾģ�� 
	QString str = QString("showBondReply('%1')").arg(json::toString(val));
	runjs(str);
}

// ���´��ı���Ϣ
void webview::updateBigTxtmsg(const CMessage &val)
{
	QString msg = analyzeMessage(val, m_hightlight, true);
	QString str = QString("chatMessageManager.extendMessageContent('%1')").arg(msg);
	runjs(str);
}


// ϵͳ���� - ģ��չʾ
void webview::SetSettingMgr()
{
	slotUpdateSetting();
}

// ��λ��Ϣ
void webview::LocateMessage(const QVariant& msgid)
{
	QString str = QString("chatMessageManager.LocateMessage('%1')").arg(msgid.toString());
	runjs(str);
}

// ������ѯ�깺��״̬����
// ȷ���깺�����깺��״̬
// �깺״̬�ı�����
void webview::PurchaseStatus(const CPurchaseUkey &purchaseKey, int nStatus)
{
	QVariantMap mapPurcahse;
	mapPurcahse["purchaseid"] = QString("%1_%2_%3").arg("QuoteBondPurchase").arg(qutil::toQString(purchaseKey.GetPurchaseID())).arg(purchaseKey.GetMoidfyTime());
	mapPurcahse["modifytime"] = purchaseKey.GetMoidfyTime();
	mapPurcahse["from"] = purchaseKey.GetFromId().GetID_C_Str();
	mapPurcahse["to"] = purchaseKey.GetToId().GetID_C_Str();
	mapPurcahse["msgid"] = purchaseKey.GetMsgid();
	QString errMsg = qutil::toQString(ControllerManager::instance()->getQbInstance()->GetPurchaseErrorStatusMsg(nStatus));
	mapPurcahse["result"] = (!errMsg.isEmpty()) ? -1 : nStatus;
	mapPurcahse["errorStr"] = errMsg;
	QString str = QString("chatMessageManager.PurchaseUpdate('%1')").arg(json::toString(mapPurcahse));
	runjs(str);
}

// ������Ϣչʾ
void webview::UpdateMessage(const CMessage &msg)
{
	QString msgContent = analyzeMessage(msg);
	QString str = QString("chatMessageManager.UpdateMessage('%1')").arg(msgContent);
	runjs(str);
}

void webview::keyPressEvent(QKeyEvent * ev)
{
	if (ev->modifiers()==Qt::ControlModifier && ev->key()==Qt::Key_C)
	{
		if (!selectedText().isEmpty())
		{
			// ����
			triggerPageAction(QWebEnginePage::Copy);
			fromSumscope();
			return;
		}
	} else if (ev->key() == Qt::Key_PageUp) { // �Ϸ�ҳ
		runjs(QString("ScrollManager.scrollToOffset(%1)").arg(-300));
		return;
	} else if (ev->key() == Qt::Key_PageDown) { // �·�ҳ
		runjs(QString("ScrollManager.scrollToOffset(%1)").arg(300));
	} else if (ev->key() == Qt::Key_Home) { // �ö�
		runjs("ScrollManager.scrollToTop()");
		return;
	} else if (ev->key() == Qt::Key_End) { // �õ�
		runjs("ScrollManager.scrollToBottom()");
		return;
	}

	QWebEngineView::keyPressEvent(ev);
}

void webview::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_clicktimer)
	{// ���������������ӻ���������������˫���¼��ڵĵ�������
		// ��ϵ��
		if (!m_openuserinfo.IsEmpty())
		{
			TRACK(OUTPUTBOX_ROOM_USERNAME_CLICK);
			killTimer(m_clicktimer);
			m_clicktimer = 0;
			UserDetailWidget::instance()->setUserInfo(m_openuserinfo);
			UserDetailWidget::instance()->popup(m_infop);
			m_openuserinfo = ID::NULL_ID;
		}
	} 
	else if (event->timerId() == m_Purchase) 
	{// һ���깺��ѯ
		killTimer(m_Purchase);
		m_Purchase = 0;
		ControllerManager::instance()->getQbInstance()->QueryPurchaseStatus(m_Purchaselst);
		m_Purchaselst.clear();
	}
	else if (event->timerId() == m_MMStatus)
	{// mm ����
		killTimer(m_MMStatus);
		m_MMStatus = 0;
		ControllerManager::instance()->getQbInstance()->BatchGetBondValid(QMClient::MSG_Body_Type_QB_QuoteMoney, m_MMStatusList);
		m_MMStatusList.clear();
	}
	else if (event->timerId() == m_QuoteBond)
	{// ծȯ
		killTimer(m_QuoteBond);
		m_QuoteBond = 0;
		ControllerManager::instance()->getQbInstance()->BatchGetBondValid(QMClient::MSG_Body_Type_QB_QuoteBond, m_QuoteBondList);
		m_QuoteBondList.clear();
	}
}

void webview::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}

void webview::dropEvent(QDropEvent * event)
{
	event->acceptProposedAction();

	QList<QUrl> urls = event->mimeData()->urls();
	QList<QString> files;
	foreach(const QUrl &url, urls) {
		QString path = url.toString();
		if (qutil::containLocalFile(path)) {
			files.append(path);
		}
	}

	if (files.isEmpty()) {
		return;
	}
	if (files.size()>g_fileCount)
	{
		emit ViewSignalManager::instance()->sigMaxFileCount();
		return;
	}
	// �ļ���Ϣ --- һ���ļ�һ����Ϣ���ϲ�
	foreach (const QString &file, files) {
		QMClient::MessageBodyList bodyList;
		QMClient::MessageBody* body = bodyList.add_bodylist();
		body->set_type(QMClient::MSG_Body_Type_File);
		body->set_msg(qutil::toString(file));
		emit ViewSignalManager::instance()->sigSendMessage(bodyList, m_Chattoid);
	}
}

void webview::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_viewstyle == VIEW_CURPAGE || m_viewstyle == VIEW_CURPAGE || m_viewstyle == VIEW_GRECORD) {
		emit m_channel->sigContextMenu("");
	}
}

// ��Ϣ����
QString webview::analyzeMessage(const CMessage &val, const QString &hilight, bool btxtfull)
{
#if 0
	QVariantMap msgmap = testRobotMsg(val, btxtfull)/*analyzeMessagetoVmap(val, btxtfull)*/;
#else
	QVariantMap msgmap = /*testRobotMsg(val, btxtfull)*/analyzeMessagetoVmap(val, btxtfull,hilight);
#endif
	
	msgmap["hilight"] = hilight;
	return json::toString(msgmap);
}

QVariantMap webview::analyzeMessagetoVmap(const CMessage &val, bool btxtfull, const QString highlight)
{
	m_Chattoid = ViewController::instance()->getChatOtherid(val);

	QVariantMap msgmap;

	// ��ϢID
	msgmap["msgid"] = val.GetMsgId() == 0 ? qutil::toQString(val.GetUUID()) : QString::number(val.GetMsgId());
	// ��Ϣ����
	msgmap["msgtype"] = (m_Chattoid.IsQmMassGroupId() || m_Chattoid == ID::QmMassAssistantID()) ? getMessageType(MSG_Type_Group_Broadcast) : getMessageType(val.GetMsgType());
	// ��Ϣʱ�䣬����ת��������ҳ����������ʾ
	msgmap["msgtime"] = val.GetDate();
	// ������ID
	msgmap["fromid"] = val.GetFrom().GetID_C_Str();
	// ������ID
	msgmap["toid"] = val.GetTo().GetID_C_Str();
	msgmap["hilight"] = highlight;
	if (val.GetFrom() == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
		msgmap["showplace"] = false;  // �ұ���ʾ �Լ�����Ϣ
	else
		msgmap["showplace"] = true;   // �����ʾ ���˵���Ϣ
	// �����Ⱥ���鲻��ʾİ���˷���ʧ����ʾ����Ⱥ����������ʾ
	if (val.GetTo().IsQmMassGroupId() && val.GetErrorCode() == ERR_CLIENT_REJECT_STRANGER_MSG) {
		msgmap["errorcode"] = 0;
	}
	else {
		msgmap["errorcode"] = val.GetErrorCode();
	}
	// ��������ʾ��
	QString showname;
	CUserInfo uinfo;
	if (!val.GetFrom().IsQmVirtualId()) {
		ControllerManager::instance()->getUserInstance()->GetUserInfo(val.GetFrom(), uinfo);
	}
	showname = qutil::toQString(uinfo.GetShowName());
	if (m_Chattoid.IsQmRoomId()) {// Ⱥid
		if (!val.GetFrom().IsQmRobotId())
		{
			CCompanyInfo companyinfo;
			ControllerManager::instance()->getUserInstance()->GetCompanyInfo(uinfo.GetCompanyId(), companyinfo);
			showname = companyinfo.GetCompanyShortName().empty() == true ? showname : (showname + " - " + qutil::toQString(companyinfo.GetCompanyShortName()));
		}
	}
	else if (ID::QmMassAssistantID() == m_Chattoid) {
		showname = tr("massGroup assistant");
	}

	if (val.GetFrom().IsQmVirtualId()) {
		QMClient::ExtendQQMessage qqExtend;
		if (ControllerManager::instance()->getQQInstance()->GetQQExtendInfo(val, qqExtend)) {
			ControllerManager::instance()->getQQInstance()->UpdateQQRoomMsgShowName(qqExtend);
			if (qqExtend.chattype() == EQQType_Room) {
				ID qqFromId = ID::MakeID(qqExtend.fromuin());
				ID qqToId = ID::MakeID(qqExtend.touin());
				ID qmFromId;
				msgmap["toid"] = val.GetFrom().GetID_C_Str();
				if (ControllerManager::instance()->getQQInstance()->GetQQBindQM(qqFromId, qmFromId)) {
					msgmap["fromid"] = qqFromId.GetID_C_Str();
				} else {
					msgmap["fromid"] = qqFromId.GetID_C_Str();
				}
			}
			showname = qutil::toQString(qqExtend.showname());
		}
	}
	
	msgmap["fromname"] = qutil::webShowString(showname);
	//msgmap["shortname"] = cutShowText(qutil::toQString(uinfo.GetShowName()), g_nameshowlength).toUtf8().qutil::toBase64();
	// ͷ��
	QFileInfo tmp(qutil::getHeads(uinfo.GetAvatar(), uinfo.GetAvatarId(), true));
	msgmap["head"] = tmp.filePath().startsWith("qrc:/") ? tmp.filePath() : "file:///" + tmp.filePath();
	// ������Ϣ����
	if (val.GetBodyList().bodylist_size() == 0 && val.GetBodyList().extendcontent().size() == 0)
	{
		return QVariantMap();
	}
	else
	{
		msgmap["msgbodylisttype"] = val.GetBodyList().bodylisttype();
		switch (val.GetBodyList().bodylisttype())
		{
		case QMClient::MSG_Basic: // ������Ϣ����
		{
			msgmap["msgbodylist"] = analyzeMessageBodyList(val, msgmap, btxtfull);
			break;
		}
		case QMClient::MSG_MassGroup: // Ⱥ��������Ϣ����  --> ContactInfoSnap
		{
			// �ݴ���
			if (m_Chattoid != ID::QmMassAssistantID())
				msgmap["msgbodylisttype"] = QMClient::MSG_Basic;

			msgmap["msgbodylist"] = analyzeMessageBodyList(val, msgmap, btxtfull);
			// ����������Ϣ ������Ϣ
			if (m_Chattoid.IsQmMassGroupId() || m_Chattoid == ID::QmMassAssistantID())
			{// Ⱥ��
				QVariantMap mapMass;
				getMassUsers(val.GetBodyList().extendcontent(), mapMass);
				msgmap["extendcontent"] = qutil::toBase64(json::toString(mapMass));
			}
			break;
		}

		case QMClient::MSG_Robot: // ����С����Ϣ����
		{
			msgmap["msgbodylist"] = analyzeMessageBodyList(val, msgmap, btxtfull);
			// ����������Ϣ ������Ϣ
			QMClient::RobotMessageInfo robotInfo;
			robotInfo.ParseFromString(val.GetBodyList().extendcontent());
			CProtoComp::RobotMessageInfo(robotInfo);

			QVariantMap mapRoobt;
			mapRoobt["id"] = robotInfo.strid().c_str();

			QString sName = qutil::toQString(robotInfo.name());
			QString cName = qutil::toQString(robotInfo.orgname());
			if (sName.isEmpty())
			{
				CUserInfo cuinfo;
				ControllerManager::instance()->getUserInstance()->GetUserInfo(ID::MakeQmID(robotInfo.strid()), cuinfo);
				sName = qutil::toQString(cuinfo.GetShowName());
			}
			if (cName.isEmpty())
			{
				CUserInfo cuinfo;
				ControllerManager::instance()->getUserInstance()->GetUserInfo(ID::MakeQmID(robotInfo.strid()), cuinfo);
				CCompanyInfo companyinfo;
				ControllerManager::instance()->getUserInstance()->GetCompanyInfo(cuinfo.GetCompanyId(), companyinfo);
				cName = qutil::toQString(companyinfo.GetCompanyShortName());
			}
			if (sName.isEmpty() && cName.isEmpty())
			{
				mapRoobt["name"] = QString("%1(%2)").arg(tr("anonymous")).arg(tr("Anonymous agency"));
			}
			else if (sName.isEmpty())
			{
				mapRoobt["name"] = QString("%1(%2)").arg(tr("anonymous")).arg(cName);
			}
			else if (cName.isEmpty())
			{
				mapRoobt["name"] = QString("%1(%2)").arg(sName).arg(tr("Anonymous agency"));
			}
			else
				mapRoobt["name"] = QString("%1(%2)").arg(sName).arg(cName);
			mapRoobt["sayextend"] = qutil::toQString(robotInfo.sayextend());
			mapRoobt["autoextend"] = qutil::toQString(robotInfo.autoextend());
			if (robotInfo.bodylist_size() > 0)
			{
				CMessage cmsg = val;
				QMClient::MessageBodyList tmpbodylist;
				tmpbodylist.set_bodylisttype(QMClient::MSG_Basic);
				*tmpbodylist.mutable_bodylist() = robotInfo.bodylist();
				cmsg.SetBodyList(tmpbodylist);
				mapRoobt["autoReply"] = analyzeMessageBodyList(cmsg, msgmap, btxtfull);
			}
			msgmap["extendcontent"] = qutil::toBase64(json::toString(mapRoobt));

			break;
		}
		case QMClient::MSG_QQMsg: // QQ ��Ϣ
		{
			CMessage qqMsg = val;
			qqMsg.BodyList().set_bodylisttype(QMClient::MSG_Basic);
			qqMsg.BodyList().clear_bodylist();

			QMClient::ExtendQQMessage qqExtend;
			qqExtend.ParseFromString(qqMsg.GetBodyList().extendcontent());
			for (int i = 0; i < qqExtend.msg_size(); i++) {
				QMClient::MessageBody *qmMsg = qqMsg.BodyList().add_bodylist();
				*qmMsg = qqExtend.msg(i);
			}
			msgmap["msgbodylist"] = analyzeMessageBodyList(qqMsg, msgmap, btxtfull);
			break;
		}
		case QMClient::MSG_MassGroupNew:// Ⱥ��������Ϣ����  --> �ɹ� + ��Ϣ + ʧ�� + ��֤
		{
			msgmap["msgbodylist"] = analyzeMessageBodyList(val, msgmap, true);
			break;
		}
		default:
			break;
		}
	}

	return msgmap;
}

QVariant webview::analyzeMessageBodyList(const CMessage &val, QVariantMap &msgmap, bool btxtfull)
{
	ID curId = ViewController::instance()->getChatOtherid(val);
	ULONG64 txtCount = 0;
	bool bBigText = false;
	QList<QVariant> msglist;

	// ��ǰ�����ظ��Ĳ����ı���Ʋ��������鳬�ı��ڲ����ֵ���������֪ͨ��Ϣ�ڡ�Ϊ�������ϰ汾��ֻ��������һ��Э�顣
	bool bOldTxt = false;
	bool bNewOrg = false;

	for (int i = 0; i < val.GetBodyList().bodylist_size(); i++)
	{
		QVariantMap msgcontent;
		msgcontent["type"] = val.GetBodyList().bodylist(i).type();
		if (bBigText && // ���ı�
			(((curId.IsQmMassGroupId() || curId.IsQmMassAssistantID()) && // Ⱥ��id
			val.GetBodyList().bodylist(i).type() != QMClient::MSG_Body_Type_QB_Contactsdiscard/*Ⱥ����Ա����Ϣ*/) ||
			(m_Chattoid.IsQmUserId() || m_Chattoid.IsQmRoomId() || m_Chattoid.IsQmOrgId())))
		{
			continue;
		}
		switch (val.GetBodyList().bodylist(i).type())
		{
		case QMClient::MSG_Body_Type_TEXT:       // �ı���Ϣ
			{
				if (btxtfull)
				{
					msgcontent["msg"] = analyzeMessageContent(val, i);
					break;
				}
				ID chatid = ViewController::instance()->getChatOtherid(val);
				QString msgstr = qutil::toQString(val.GetBodyList().bodylist(i).msg());
				int ntxt = msgstr.count();
				if (ntxt > g_cutstrcount-txtCount)
				{
					msgstr = msgAnalyse::msgContentAnalyze(chatid, qutil::getShortContent(msgstr), val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
					QString seemore = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_SEEMORE));
					seemore.replace("{msgid}", val.GetMsgId()==0?qutil::toQString(val.GetUUID()):QString::number(val.GetMsgId()));
					seemore.replace("{seemore}", tr("searchmore"));
					msgstr = qutil::toBase64(msgstr + " " + seemore);
					msgcontent["msg"] = msgstr;
					bBigText = true;
				}
				else
				{
					txtCount = txtCount + ntxt;
					msgcontent["msg"] = qutil::toBase64(msgAnalyse::msgContentAnalyze(chatid, msgstr, val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID()));
				}
				
				break;
			}
		case QMClient::MSG_Body_Type_EnhancedTEXT:       // �ı���Ϣ
			{
				if (btxtfull)
				{
					QVariant msgtmp = analyzeMessageContent(val, i);
					if (msgtmp.isNull()) continue;
					msgcontent["msg"] = msgtmp;
					break;
				}
				ID chatid = ViewController::instance()->getChatOtherid(val);
				QMClient::TxtContent txt;
				txt.ParseFromString(val.GetBodyList().bodylist(i).msg());
				QString msgstr = "";
				QVariantMap txtExtend;
				txtExtend["type"] = txt.type();
				switch (txt.type())
				{
				case QMClient::TxtType_Basic:
					{
						msgstr = qutil::toQString(txt.content());
						int ntxt = msgstr.count();
						if (ntxt > g_cutstrcount-txtCount)
						{
							msgstr = msgAnalyse::msgContentAnalyze(chatid, qutil::getShortContent(msgstr), val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
							QString seemore = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_SEEMORE));
							seemore.replace("{msgid}", val.GetMsgId()==0?qutil::toQString(val.GetUUID()):QString::number(val.GetMsgId()));
							seemore.replace("{seemore}", tr("searchmore"));
							msgstr = msgstr + " " + seemore;
							txtExtend["content"] = msgstr;
							msgcontent["msg"] = qutil::toBase64(json::toString(txtExtend));
							bBigText = true;
						}
						else
						{
							txtCount = txtCount + ntxt;
							txtExtend["content"] = msgAnalyse::msgContentAnalyze(chatid, msgstr, val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
							msgcontent["msg"] = qutil::toBase64(json::toString(txtExtend));
						}
						break;
					}
				case QMClient::TxtType_Organization:
					{
						if (bNewOrg) continue;
						bOldTxt = true;
						QVariant msgtmp = analyzeMessageContent(val, i);
						if (msgtmp.isNull()) continue;
						msgcontent["msg"] = msgtmp;
						break;
					}
				}
				
				break;
			}
		case QMClient::MSG_Body_Type_Emoticon:    // ϵͳ����
		case QMClient::MSG_Body_Type_EnhancedEmoticon:    // ϵͳ����
		case QMClient::MSG_Body_Type_PIC:         // ͼƬ
		case QMClient::MSG_Body_Type_File:       // �ļ�����
		case QMClient::MSG_body_Type_RoomFile:   // Ⱥ�ļ�
		case QMClient::MSG_Body_Type_Shake:       // ����
		case QMClient::MSG_Body_Type_EnhancedShake:       // ����
		case QMClient::MSG_Body_Type_RoomCard:    // Ⱥ��Ƭ
		case QMClient::MSG_Body_Type_QB_QuoteMoney:    // ��������
		case QMClient::MSG_Body_Type_QB_QuoteConditions: // ��������
		case QMClient::MSG_Body_Type_QB_QuoteBond:       // ծȯ����
		case QMClient::MSG_Body_Type_FinancialNews:       // ������Ѷ-�ƾ�ͷ��
		case QMClient::MSG_Body_Type_NEWBONDPUSH: // ��ȯ����
		case QMClient::MSG_Body_Type_BONDTXT: // ������Ϣ
		case QMClient::MSG_Body_Type_News: // ����/���� ������
		case QMClient::MSG_Body_Type_Bond: // ծȯ --> BondInfo 
		case QMClient::MSG_Body_Type_SessionInfo: // �Ự���� --> SessionInfo
		case QMClient::MSG_Body_Type_Link: // ���� --> LinkInfo
		case QMClient::MSG_Body_Type_Purchase: // һ���깺������Ϣ --> PurchaseInfo
		case QMClient::MSG_Body_Type_ContactInfoSnap: // һ���깺������Ϣ --> ContactInfoSnap
		case QMClient::MSG_Body_Type_ShareBond: //ծȯ���� --> ShareBondInfo
		case QMClient::MSG_Body_Type_MarginalGuidance: // �߼�ָ�� --> MarginalGuidance
		case QMClient::MSG_Body_Type_BondsDelay: // ծȯ�Ƴٷ��� --> BondsDelay
		case QMClient::MSG_Body_Type_Quoted_Alert: // �������� --> QuotedAlertInfo
        case QMClient::MSG_Body_Type_QB_HelpCenterLink: // QB������Ϣ���� --> QBHelpCenterLink
		case QMClient::MSG_Body_Type_MWNotice:
			{
				QVariant msgtmp = analyzeMessageContent(val, i);
				if (msgtmp.isNull()) continue;
				msgcontent["msg"] = msgtmp;
				break;
			}
		case QMClient::MSG_Body_Type_OrganizationNotice:       // ����֪ͨ
			{
				if (bOldTxt) continue;
				bNewOrg = true;
				QVariant msgtmp = analyzeMessageContent(val, i);
				if (msgtmp.isNull()) continue;
				msgcontent["msg"] = msgtmp;
				break;
			}
		case QMClient::MSG_Body_Type_QB_Contactsdiscard: // Ⱥ������ - ���۶���
			{
				if (m_Chattoid.IsQmMassGroupId() || m_Chattoid == ID::QmMassAssistantID())
				{// Ⱥ��
					QVariantMap massMap;
					getMassUsers(val.GetBodyList().bodylist(i).msg(), massMap);
					QString massGstr = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_GROUPMASSTITLE));
					massGstr.replace("{tip}", massMap["tip"].toString());
					massGstr.replace("{receiverList}", massMap["receiverList"].toString());
					massGstr.replace("{receiverids}", massMap["receiverids"].toString());
					msgmap["massgroup"] = qutil::toBase64(massGstr);
					continue;
				}
				break;
			}
		default:
			if (!val.GetBodyList().bodylist(i).basiccontent().empty())
			{
				msgcontent["type"] = QMClient::MSG_Body_Type_EnhancedTEXT;
				QVariantMap txtExtend;
				txtExtend["type"] = QMClient::TxtType_Basic;
				QString msgstr = qutil::toQString(val.GetBodyList().bodylist(i).basiccontent());
				txtExtend["content"] = msgAnalyse::msgContentAnalyze(curId, msgstr, val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
				msgcontent["msg"] = qutil::toBase64(json::toString(txtExtend));
			} 
			else
			{
				msgcontent["type"] = QMClient::MSG_Body_Type_EnhancedTEXT;
				QVariantMap txtExtend;
				txtExtend["type"] = QMClient::TxtType_Basic;
				txtExtend["content"] = tr("unkownmessage");
				msgcontent["msg"] = qutil::toBase64(json::toString(txtExtend));
			}
		}
		msglist.append(msgcontent);
	}

	return msglist;
}

QVariant webview::analyzeMessageContent(const CMessage &val, int nIndex)
{
	ID curId = ViewController::instance()->getChatOtherid(val);
	QVariant msgcontent;
	switch (val.GetBodyList().bodylist(nIndex).type())
	{
	case QMClient::MSG_Body_Type_TEXT:       // �ı���Ϣ
		{
			ID chatid = ViewController::instance()->getChatOtherid(val);
			QString msgstr = qutil::toQString(val.GetBodyList().bodylist(nIndex).msg());
			msgcontent = qutil::toBase64(msgAnalyse::msgContentAnalyze(chatid, msgstr, val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID()));
			break;
		}
	case QMClient::MSG_Body_Type_EnhancedTEXT:// �ı���Ϣ
		{
			ID chatid = ViewController::instance()->getChatOtherid(val);
			QMClient::TxtContent txt;
			txt.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString msgstr = "";
			QVariantMap txtExtend;
			txtExtend["type"] = txt.type();
			switch (txt.type())
			{
			case QMClient::TxtType_Basic:
				{
					msgstr = qutil::toQString(txt.content());
					txtExtend["content"] = msgAnalyse::msgContentAnalyze(chatid, msgstr, val.GetFrom()!=ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
					msgcontent = qutil::toBase64(json::toString(txtExtend));
					break;
				}
			case QMClient::TxtType_Organization:
				{
					QMClient::TxtOrganization txtOrg;
					txtOrg.ParseFromString(txt.content());
					QVariantMap orgTxt;
					orgTxt["type"] = txtOrg.type();
					switch (txtOrg.type())
					{
					case QMClient::OrgType_Basic:
						{
							msgstr = qutil::toQString(txtOrg.content());
							orgTxt["content"] = msgstr;
							txtExtend["content"] = orgTxt;
							msgcontent = qutil::toBase64(json::toString(txtExtend));
							break;
						}
					case QMClient::OrgType_Subscribe:
						{
							QMClient::QuotationBondBrief sub;
							sub.ParseFromString(txtOrg.content());
							QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
							bondMsg.replace("{bondkey}", qutil::toQString(sub.combbondkey()));
							bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondtype()));
							bondMsg.replace("{bondval}", qutil::toQString(sub.bondshowname()));
							msgstr = tr("no price of this code at current, you can subscribe it.").arg(bondMsg);
							orgTxt["content"] = msgstr;
							// �Ƿ���Ҫ����
							if (val.GetFrom().IsQmOrgId())
							{
								orgTxt["subscribe"] = !ControllerManager::instance()->getPersonInstance()->IsSubscribeGoods(val.GetFrom(), sub.companyid(), sub.combbondkey());
							}
							else
							{
								orgTxt["subscribe"] = false;
							}
							QVariantMap bondValue;
							QVariantMap SubscribeMap;
							SubscribeMap["bondkey"] = qutil::toQString(sub.combbondkey());
							SubscribeMap["companyid"] = qutil::toQString(sub.companyid());
							bondValue["Subscribevalue"] = qutil::toBase64(json::toString(SubscribeMap));
							bondValue["fromid"] = val.GetFrom().GetID_C_Str();
							orgTxt["bondvalue"] = bondValue;
							txtExtend["content"] = orgTxt;
							msgcontent = qutil::toBase64(json::toString(txtExtend));
							break;
						}
					case QMClient::OrgType_Price: // �����ı�
						{
							QMClient::QuotationBondBrief sub;
							sub.ParseFromString(txtOrg.content());
							QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
							bondMsg.replace("{bondkey}", qutil::toQString(sub.combbondkey()));
							bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondtype()));
							bondMsg.replace("{bondval}", qutil::toQString(sub.bondshowname()));
							msgstr = QString("%1 %2/%3 %4/%5").arg(bondMsg) \
															  .arg(qutil::toQString(sub.bid())) \
															  .arg(qutil::toQString(sub.ofr())) \
															  .arg(qutil::toQString(sub.volbid())) \
															  .arg(qutil::toQString(sub.volofr()));
							orgTxt["content"] = msgstr;
							txtExtend["content"] = orgTxt;
							msgcontent = qutil::toBase64(json::toString(txtExtend));
							break;
						}
					case QMClient::OrgType_NoPrice: // ���ޱ����ı�
						{
							QMClient::QuotationBondBriefList sub;
							sub.ParseFromString(txtOrg.content());
							QStringList bondpriTxt;
							for (int i = 0; i < sub.bondbriefs_size(); i++)
							{
								QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
								bondMsg.replace("{bondkey}", qutil::toQString(sub.bondbriefs(i).combbondkey()));
								bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondbriefs(i).bondtype()));
								bondMsg.replace("{bondval}", qutil::toQString(sub.bondbriefs(i).bondshowname()));
								bondpriTxt.append(bondMsg);
							}
							msgstr = tr("no price of this code at current, you can subscribe it.").arg(bondpriTxt.join(tr("chinese dunhao")));
							orgTxt["content"] = msgstr;
							txtExtend["content"] = orgTxt;
							msgcontent = qutil::toBase64(json::toString(txtExtend));
							break;
						}
					case QMClient::OrgType_PriceRange: // һ������ȯ����䶯����Ϣ
						{
							QMClient::BondReminderInfo reminderinfo;
							reminderinfo.ParseFromString(txtOrg.content());
							QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
							bondMsg.replace("{bondkey}", qutil::toQString(reminderinfo.bondbrief().combbondkey()));
							bondMsg.replace("{linkColor}", qutil::getBondColor(reminderinfo.bondbrief().bondtype()));
							bondMsg.replace("{bondval}", qutil::toQString(reminderinfo.bondbrief().bondshowname()));
							msgstr = tr("bond reminder").arg(qutil::toQString(reminderinfo.distributors()))
								.arg(bondMsg)
								.arg(qutil::toQString(reminderinfo.remindertxt()));
							orgTxt["content"] = msgstr;
							txtExtend["content"] = orgTxt;
							msgcontent = qutil::toBase64(json::toString(txtExtend));

							break;
						}
					}
					break;
				}
			}
			
			break;
		}
	case QMClient::MSG_Body_Type_Emoticon:    // ϵͳ����
		{
			std::string eid = val.GetBodyList().bodylist(nIndex).msg();
			QStringList tmplst = qutil::toQString(eid).split("/");
			QString epkgid = tmplst.size()==3?tmplst[1]:"face";
			CEmoticon eicon(qutil::toString(epkgid));
			IResourcesController::ErrorEmoticon err = ControllerManager::instance()->getResourcesInstance()->GetEmoticon(eicon.GetEmoticonPackageId(), eid, eicon);
			QVariantMap imgjson;
			CEmoticonPackage pkg;
			IResourcesController::ErrorEmoticon errpkg = ControllerManager::instance()->getResourcesInstance()->GetEmoticonPackage(qutil::toString(epkgid), pkg);
			QString imgPath = qutil::toQString(AppInfo::getEmoticonPath()+pkg.GetEmoticonPackagePath()+std::string("\\")+eicon.GetEmoticonPath());
			imgjson["imagepath"] = err==IResourcesController::Err_NoError ? qutil::toBase64("file:///"+imgPath.replace("\\","/")) : "";
			QPixmap pix(imgPath);
			imgjson["imagewidth"] = pix.width();
			imgjson["imageheight"] = pix.height();
			imgjson["isUpdate"] = err!=IResourcesController::Err_NoError;
			imgjson["imagedesc"] = qutil::toQString(eicon.GetEmoticonDescribe());
			msgcontent = imgjson;
			
			break;
		}
	case QMClient::MSG_Body_Type_EnhancedEmoticon:    // ϵͳ����
		{
			QMClient::SysEmotionInfo sysEmotionInfo;
			sysEmotionInfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());

			CEmoticon eicon(sysEmotionInfo.typeid_());
			eicon.SetEmoticonDescribe(sysEmotionInfo.describe());
			IResourcesController::ErrorEmoticon err = ControllerManager::instance()->getResourcesInstance()->GetEmoticon(
				eicon.GetEmoticonPackageId(), sysEmotionInfo.emotion(), eicon);
			if (err != IResourcesController::Err_NoError) {
				// �����ϰ汾
				err = ControllerManager::instance()->getResourcesInstance()->GetEmoticonFromName(sysEmotionInfo.emotion(), eicon);
			}
			QString imgPath = qutil::toQString(AppInfo::getEmoticonPath()+eicon.GetEmoticonPackageId()+std::string("\\")+eicon.GetEmoticonPath());
			QVariantMap imgjson;
			imgjson["imagepath"] = err==IResourcesController::Err_NoError ? qutil::toBase64("file:///"+imgPath.replace("\\","/")) : "";
			QPixmap pix(imgPath);
			imgjson["imagewidth"] = pix.width();
			imgjson["imageheight"] = pix.height();
			imgjson["isUpdate"] = pix.isNull();
			QString desc = qutil::toQString(eicon.GetEmoticonDescribe()).trimmed();
			if (desc.isEmpty()) {
				desc = QString(tr("[face]"));
			} else if (desc[0] != '[' && desc[desc.size() - 1] != ']') {
				desc = QString("[%1]").arg(desc);
			}
			imgjson["imagedesc"] = qutil::toBase64(desc);
			msgcontent = imgjson;
			break;
		}
	case QMClient::MSG_Body_Type_PIC:         // ͼƬ
		{
			CPicSendParam picParam;
			ControllerManager::instance()->getMessageInstance()->PicSendParamParse (
                val.GetBodyList().bodylist(nIndex).msg(),
                picParam);

            // ����ƴ��
            QVariantMap imgjson;

            /* ԭͼԶ�� Url ��QQ ��Ϣ�� */
            QString origPicLnkUrl = qutil::toQString(picParam.GetUrl());

            /* ��ͼԶ�� Url ��QQ ��Ϣ�� */
            QString thumbnailPicLnkUrl = qutil::toQString(picParam.GetThumbnailUrl());

            /* ��ͼ���� Url ��QM ��Ϣ�������� UUID ʱ�� */
            bool isThumbnailUUIDDefined = false;
            STRINGUTF8 thumbnailPicUrl;
            if (!picParam.GetThumbnailUUID().empty() && picParam.GetThumbnailUUID() != "null")
            {
                isThumbnailUUIDDefined = true;
                thumbnailPicUrl = AppInfo::getPicturesPath()
                    + picParam.GetThumbnailUUID().substr(0, picParam.GetThumbnailUUID().find('_'));
            }

            /* �Ƿ�Ϊ GIF ��̬ͼƬ */
            bool isGifPic = qutil::toQString(picParam.GetExtName()).toUpper().contains("GIF");

            /* ���ɼ�ͼ��Ϣ */
            if (!thumbnailPicLnkUrl.isEmpty() && QUrl(thumbnailPicLnkUrl).isValid())
            {
                if (picParam.GetThumbnailWidth() == 0 || picParam.GetThumbnailHeight() == 0)
                {
                    imgjson["imagebriefwidth"] = g_imagewidth;
                    imgjson["imagebriefheight"] = g_imageheight;
                }
                else if (picParam.GetThumbnailWidth() <= g_imagewidth
                    && picParam.GetThumbnailHeight() <= g_imageheight)
                {
                    imgjson["imagebriefwidth"] = picParam.GetThumbnailWidth();
                    imgjson["imagebriefheight"] = picParam.GetThumbnailHeight();
                }
                else
                {
                    QSize preferredThumbnailPicSize
                        = QSize(picParam.GetThumbnailWidth(), picParam.GetThumbnailHeight())
                        .scaled(g_imagewidth, g_imageheight, Qt::KeepAspectRatio);
                    imgjson["imagebriefwidth"] = preferredThumbnailPicSize.width();
                    imgjson["imagebriefheight"] = preferredThumbnailPicSize.height();
                }

                thumbnailPicUrl = qutil::toString(thumbnailPicLnkUrl);
                imgjson["imagebrief"] = qutil::toBase64(qutil::toQString(thumbnailPicUrl));
            }
            else if (!thumbnailPicUrl.empty())
            {
                if (!QFile::exists(qutil::toQString(thumbnailPicUrl))
                    || QPixmap(qutil::toQString(thumbnailPicUrl)).isNull())
                {
                    SyncUrlRequest::instance()->addFile(picParam.GetThumbnailUUID(), thumbnailPicUrl);
                    thumbnailPicUrl = ControllerManager::instance()->getGlobalInstance()->GetPicURL(picParam.GetThumbnailUUID());
                    imgjson["imagebriefwidth"] = picParam.GetThumbnailWidth() == 0 ? g_imagewidth : picParam.GetThumbnailWidth();
                    imgjson["imagebriefheight"] = picParam.GetThumbnailHeight() == 0 ? g_imageheight : picParam.GetThumbnailHeight();
                }
                else
                {
                    imgjson["imagebriefwidth"] = picParam.GetThumbnailWidth() == 0 ? QPixmap(qutil::toQString(thumbnailPicUrl)).width() : picParam.GetThumbnailWidth();
                    imgjson["imagebriefheight"] = picParam.GetThumbnailHeight() == 0 ? QPixmap(qutil::toQString(thumbnailPicUrl)).height() : picParam.GetThumbnailHeight();
                    thumbnailPicUrl = std::string("file:///") + thumbnailPicUrl;
                }

                imgjson["imagebrief"] = qutil::toBase64(qutil::toQString(thumbnailPicUrl));
            }
            else
            {
                QString picname = qutil::getPicpath(picParam.GetContent());
                QPixmap briefPix(picname);
                imgjson["imagebriefwidth"] = briefPix.width();
                imgjson["imagebriefheight"] = briefPix.height();
                thumbnailPicUrl = qutil::toString(QString("%1%2").arg("file:///").arg(picname));

                imgjson["imagebrief"] = qutil::toBase64(qutil::toQString(thumbnailPicUrl));
            }

            /* GIF ԭͼ�������ֱ����ʾԭͼ */
            auto setGifAsBriefImageIfExists = [&imgjson](
                const QString &gifImagePath)
            {
                QImage origImage(gifImagePath);
                if (!origImage.isNull())
                {
                    imgjson["imagebrief"] = imgjson["imagepath"];

                    QSize imageSize(origImage.width(), origImage.height());
                    if (imageSize.width() > g_imagewidth || imageSize.height() > g_imageheight)
                    {
                        imageSize.scale(g_imagewidth, g_imageheight, Qt::KeepAspectRatio);
                    }
                    imgjson["imagebriefwidth"] = imageSize.width();
                    imgjson["imagebriefheight"] = imageSize.height();
                }
            };

            /* ����ԭͼ��Ϣ */
            if (!origPicLnkUrl.isEmpty() && QUrl(origPicLnkUrl).isValid())
            {
                QString origImagePath = qutil::toQString(picParam.GetFilePath());

                imgjson["imagepath"] = qutil::toBase64(origImagePath);
                imgjson["imageLnkPath"] = qutil::toBase64(origPicLnkUrl);

                if (origImagePath != qutil::toQString(thumbnailPicUrl))
                {
                    if (QFile(origImagePath).exists())
                    {
                        if (QString(qutil::getImageFileFormat(origImagePath)) == "GIF")
                        {
                            setGifAsBriefImageIfExists(origImagePath);
                        }
                    }
                    else
                    {
                        HttpDownloadReplyPtr reply_p = HttpTransfer::instance()->downloadImageAsyn(
                            origPicLnkUrl, origImagePath);

                        if (reply_p != nullptr)
                        {
                            ID currentId = this->m_Chattoid;
                            connect(&(*reply_p), &HttpDownloadReply::finished, this,
                                [this, reply_p, currentId, thumbnailPicUrl]()
                            {
                                m_updateGifBriefImage(
                                    currentId,
                                    qutil::toQString(thumbnailPicUrl),
                                    reply_p->localPath(),
                                    reply_p->isDownloadOkay());
                            });
                        }
                    }
                }
            }
            else if (!isThumbnailUUIDDefined
                && QFile::exists(qutil::toQString(picParam.GetFilePath())))
            {
                QString origImagePath = qutil::toQString(picParam.GetFilePath());

                /* ��� File Path ����ʱ������ʹ�� File Path �еĶ��� */
                imgjson["imagepath"] = qutil::toBase64(origImagePath);

                if (isGifPic)
                {
                    setGifAsBriefImageIfExists(origImagePath);
                }
            }
            else if (!picParam.GetUUID().empty())
            {
                QString origImagePath;
                origImagePath.append(qutil::toQString(AppInfo::getPicturesPath()));
                origImagePath.append(qutil::toQString(picParam.GetUUID()));
                QString ext = qutil::toQString(picParam.GetExtName());
                if (ext.startsWith("."))
                {
                    origImagePath.append(qutil::toQString(picParam.GetExtName()));
                }
                else
                {
                    origImagePath.append(".");
                    origImagePath.append(qutil::toQString(picParam.GetExtName()));
                }
                imgjson["imagepath"] = qutil::toBase64(origImagePath);

                if (isGifPic)
                {
                    if (QFile(origImagePath).exists())
                    {
                        setGifAsBriefImageIfExists(origImagePath);
                    }
                    else
                    {
                        ControllerManager::instance()->getMessageInstance()->RefreshPicInMsg(
                            curId, val.GetMsgId());
                    }
                }
            }
            else
            {
                /* ͼƬ�Ƚ�С����ͼ��Ϊԭͼ */
                imgjson["imagepath"] = qutil::toBase64(qutil::toQString(thumbnailPicUrl));
            }

            LOG_INFO << "image brief:" << qutil::toString(qutil::fromBase64(imgjson["imagebrief"].toByteArray()));
            LOG_INFO << "image path:" << qutil::toString(qutil::fromBase64(imgjson["imagepath"].toByteArray()));
            LOG_INFO << "image linked path:" << qutil::toString(qutil::fromBase64(imgjson["imageLnkPath"].toByteArray()));
            imgjson["chatid"] = curId.GetID_C_Str();
            imgjson["msgid"] = val.GetMsgId();
            msgcontent = imgjson;
        }
        break;
	case QMClient::MSG_Body_Type_File:       // �ļ�����
		{
			CMessage fmsg;
			CFileSendParam fp;
			ControllerManager::instance()->getMessageInstance()->FileSendParamParse(val.GetBodyList().bodylist(nIndex).msg(), fp);
			
			if (m_viewstyle == VIEW_CURPAGE)
			{
				QVariantMap fjson;
				fjson["msgid"] = val.GetMsgId()==0?qutil::toQString(val.GetUUID()):QString::number(val.GetMsgId());
				fjson["filename"] = qutil::toBase64(qutil::toQString(fp.GetFileName()));
				fjson["shortfilename"] = qutil::toBase64(qutil::cutShowText(qutil::toQString(fp.GetFileName()), g_filenameshortlength, 14, Qt::ElideMiddle));
				
				QString strpath;
				if (fp.GetFilePath().empty())
				{
					 strpath=qutil::toQString(AppInfo::getFileRecvPath() + "\\" + "recFile");
				}
				else{
					 strpath=qutil::toQString(fp.GetFilePath());
				}
				QFileInfo finfo(strpath);
				fjson["filepath"] = qutil::toBase64(finfo.filePath());
				fjson["filesize"] = fp.GetTotalSize();
				fjson["filedonesize"] = fp.GetDoneSize();
				if (val.GetFrom()==ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
				{// ���͵��ļ���Ϣ
					fjson["fileStatus"] = getFileSendStatus(fp.GetSendStatus());
				} 
				else
				{// ���յ��ļ���Ϣ
					fjson["fileStatus"] = getFileRecvStatus(fp.GetRecvStatus());
				}
				fjson["fileicon"] = qutil::toBase64(getFileIcon(finfo.filePath(), fp.GetFileIconBuffer()));
				msgcontent = fjson;
			}
			else if (m_viewstyle == VIEW_RECORD || m_viewstyle == VIEW_GRECORD)
			{
				if (val.GetFrom() == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
				{// �����ļ�
					switch (fp.GetSendStatus())
					{
					case FileSend_Status_Uploaded:                   // ���ϴ�
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("uploadsuccess");
							break;
						}
					case FileSend_Status_Reject:               // �Ѿܾ�
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("otherrecieverefuse");
							break;
						}
					case FileSend_Status_Cancel:               // ��ȡ��
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("sendcancel");
							break;
						}
					case FileSend_Status_Failed:               // ��ʧ��
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("transfail");
							break;
						}
					case FileSend_Status_Done:                 // �����:
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("sendsuccess");
							break;
						}
					default:
						msgcontent = tr("file") + qutil::toQString(fp.GetFileName());
						break;
					}
				}
				else
				{// �����ļ�
					switch (fp.GetRecvStatus())
					{
					case FileSend_Status_Reject:               // �Ѿܾ�
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("recievereject");
							break;
						}
					case FileSend_Status_Cancel:               // ��ȡ��
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("refuse");
							break;
						}
					case FileSend_Status_Failed:               // ��ʧ��
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("transfail");
							break;
						}
					case FileSend_Status_Done:                 // �����:
						{
							msgcontent = tr("file") + qutil::toQString(fp.GetFileName()) + " " + tr("recievesuccess");
							break;
						}
					default:
						msgcontent = tr("file") + qutil::toQString(fp.GetFileName());
						break;
					}
				}
			}

			break;
		}
	case QMClient::MSG_body_Type_RoomFile: // Ⱥ�ļ�
		{
			QMClient::RoomfileInfo rfinfo;
			if (rfinfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg()))
			{
				if (m_viewstyle == VIEW_CURPAGE)
				{
					QString strpath;
					if (rfinfo.filepath().empty())
					{
						strpath=qutil::toQString(AppInfo::getFileRecvPath() + "\\" + "recFile");
					}
					else{
						strpath=qutil::toQString(rfinfo.filepath());
					}
					QFileInfo finfo(strpath);
					QString filename = qutil::toQString(rfinfo.filename());
					QVariantMap fjson;
					fjson["msgid"] = val.GetMsgId()==0?qutil::toQString(val.GetUUID()):QString::number(val.GetMsgId());
					fjson["filename"] = qutil::toBase64(filename);
					fjson["shortfilename"] = qutil::toBase64(qutil::cutShowText(filename, g_filenameshortlength, 14, Qt::ElideMiddle));
					fjson["filesize"] = rfinfo.totalsize();
					fjson["filedonesize"] = rfinfo.donesize();
					fjson["fileStatus"] = getFileSendStatus(static_cast<EFileSendStatus>(rfinfo.statussend()));
					fjson["fileicon"] = qutil::toBase64(getFileIcon(finfo.filePath(), rfinfo.fileicon()));
					msgcontent = fjson;
				}
				else if (m_viewstyle == VIEW_RECORD || m_viewstyle == VIEW_GRECORD)
				{
					if (val.GetFrom() == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
					{// �����ļ�
						switch (rfinfo.statussend())
						{
						case FileSend_Status_Cancel:               // ��ȡ��
							{
								msgcontent = tr("file") + qutil::toQString(rfinfo.filename()) + " " + tr("cancelupload");
								break;
							}
						case FileSend_Status_Failed:               // ��ʧ��
							{
								msgcontent = tr("file") + qutil::toQString(rfinfo.filename()) + " " + tr("uploadfail");
								break;
							}
						case FileSend_Status_Done:                 // �����:
							{
								msgcontent = tr("file") + qutil::toQString(rfinfo.filename()) + " " + tr("uploadsuccess");
								break;
							}
						default:
							msgcontent = tr("file") + qutil::toQString(rfinfo.filename());
							break;
						}
					}
					else
					{// �����ļ�
						msgcontent = tr("file") + qutil::toQString(rfinfo.filename()) + " " + tr("uploadto") + tr("roomfiles");
					}
				}
			}
			break;
		}
	case QMClient::MSG_Body_Type_Shake:       // ����
	case QMClient::MSG_Body_Type_EnhancedShake:       // ����
		{
			if (val.GetFrom() == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
			{
				// ������Ϊ�Լ�
				msgcontent = tr("shakeother");
			} 
			else
			{
				// ������Ϊ�Է�
				msgcontent = tr("shakeme");
			}
			break;
		}
	case QMClient::MSG_Body_Type_RoomCard:    // Ⱥ��Ƭ
		{
			CRoomCardInfo roomcard;
			ControllerManager::instance()->getMessageInstance()->RoomCardInfoParse(val.GetBodyList().bodylist(nIndex).msg(), roomcard);
			if (m_viewstyle == VIEW_CURPAGE)
			{
				QVariantMap fjson;
				fjson["roomname"] = qutil::toBase64(qutil::toQString(roomcard.GetRoomName()));
				fjson["roomnum"] = qutil::toQString(roomcard.GetAlias());
				fjson["roomowner"] = qutil::toBase64(qutil::toQString(roomcard.GetOwnerName()));
				fjson["roommembercount"] = roomcard.GetTotalNum();
				fjson["roomid"] = roomcard.GetRoomId().GetID_C_Str();
				// true ��������Ⱥ �� false��������Ϣ
				fjson["status"] = !ControllerManager::instance()->getCrowdInstance()->IsUserInRoom(ControllerManager::instance()->getPersonInstance()->GetCurrentUserID(), roomcard.GetRoomId());
				msgcontent = fjson;
			}
			else if (m_viewstyle == VIEW_RECORD || m_viewstyle == VIEW_GRECORD)
			{
				msgcontent = tr("room card").arg(qutil::toQString(roomcard.GetRoomName()));
			}

			break;
		}
	case QMClient::MSG_Body_Type_QB_QuoteMoney:    // ��������
		{
			// ��ʾ�ֶΣ������ʲ������ޡ��������۸񡢱�ǩ����ע�����ԡ�
			QMClient::QuotationMoneyInfo qutoinfo;
			qutoinfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());

			// ��ȡ����״̬
			if (m_MMStatus > 0)
				killTimer(m_MMStatus);
			m_MMStatusList.push_back(CQuoteState(qutoinfo.quotationid(), val.GetTo()));
			m_MMStatus = startTimer(MMSTATUSTIMER);

			QVariantMap mp = qbprice::priceToJson(qutoinfo);
			mp["fromid"] = val.GetFrom().GetID_C_Str();
			mp["type"] = val.GetBodyList().bodylist(nIndex).type();
			mp["msgvalue"] = QByteArray(val.GetBodyList().bodylist(nIndex).msg().c_str(), val.GetBodyList().bodylist(nIndex).msg().size()).toBase64(); 
			msgcontent = mp;
			break;
		}
	case QMClient::MSG_Body_Type_QB_QuoteConditions: // ��������
		{
			break;
		}
	case QMClient::MSG_Body_Type_QB_QuoteBond:       // ծȯ����
		{
			QMClient::QuotationBondInfo bondmsg;
			bondmsg.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());

			// ��ȡ����״̬
			// ��ȡ����״̬
			if (m_QuoteBond > 0)
				killTimer(m_QuoteBond);
			m_QuoteBondList.push_back(CQuoteState(bondmsg.quotationid(), val.GetTo()));
			m_QuoteBond = startTimer(MMSTATUSTIMER);

			QVariantMap mp = qbprice::bondToJson(bondmsg);
			// �Ƿ���Ҫ����
			if (val.GetFrom().IsQmOrgId())
			{
				mp["subscribe"] = !ControllerManager::instance()->getPersonInstance()->IsSubscribeGoods(val.GetFrom(), bondmsg.companyid(), bondmsg.bondinfo().combbondkey());
			}
			else
			{
				mp["subscribe"] = false;
			}
			
			mp["fromid"] = val.GetFrom().GetID_C_Str();
			mp["type"] = val.GetBodyList().bodylist(nIndex).type();
			QVariantMap bondValue;
			QVariantMap SubscribeMap;
			SubscribeMap["bondkey"] = qutil::toQString(bondmsg.bondinfo().combbondkey());
			SubscribeMap["companyid"] = qutil::toQString(bondmsg.companyid());
			bondValue["Subscribevalue"] = qutil::toBase64(json::toString(SubscribeMap));
			bondValue["fromid"] = val.GetFrom().GetID_C_Str();
			mp["bonvalue"] = bondValue;
			mp["msgvalue"] = QByteArray(val.GetBodyList().bodylist(nIndex).msg().c_str(), val.GetBodyList().bodylist(nIndex).msg().size()).toBase64(); 
			msgcontent = mp;
			break;
		}
	case QMClient::MSG_Body_Type_FinancialNews:       // ������Ѷ-�ƾ�ͷ�� 
		{
			QMClient::FinancialNewsInfo financialNews;
			financialNews.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QVariantMap fNews;
			fNews["type"] = financialNews.type();
			switch (financialNews.type())
			{
			case QMClient::News_Txt:
				{
					QString content = qutil::toQString(financialNews.content());
					content.replace(QRegExp("(<[^>]*>)|(<[^>]*/[^>]*>)"), "");
					content = msgAnalyse::alertAnalyze(content);
					fNews["content"] = content;
					msgcontent = qutil::toBase64(json::toString(fNews));
					break;
				}
			default:
				break;
			}
			break;
		}
	case QMClient::MSG_Body_Type_OrganizationNotice:       // ����֪ͨ 
		{
			QMClient::OrganizationNoticeInfo OrgNotice;
			OrgNotice.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QVariantMap orgTxt;
			orgTxt["type"] = OrgNotice.type();
			switch (OrgNotice.type())
			{
			case QMClient::OrgType_Basic:
				{
					orgTxt["content"] = qutil::toQString(OrgNotice.content());
					msgcontent = qutil::toBase64(json::toString(orgTxt));
					break;
				}
			case QMClient::OrgType_Subscribe:
				{
					QMClient::QuotationBondBrief sub;
					sub.ParseFromString(OrgNotice.content());
					QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
					bondMsg.replace("{bondkey}", qutil::toQString(sub.combbondkey()));
					bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondtype()));
					bondMsg.replace("{bondval}", qutil::toQString(sub.bondshowname()));
					orgTxt["content"] = tr("no price of this code at current, you can subscribe it.").arg(bondMsg);
					// �Ƿ���Ҫ����
					if (val.GetFrom().IsQmOrgId())
					{
						orgTxt["subscribe"] = !ControllerManager::instance()->getPersonInstance()->IsSubscribeGoods(val.GetFrom(), sub.companyid(), sub.combbondkey());
					}
					else
					{
						orgTxt["subscribe"] = false;
					}
					QVariantMap bondValue;
					QVariantMap SubscribeMap;
					SubscribeMap["bondkey"] = qutil::toQString(sub.combbondkey());
					SubscribeMap["companyid"] = qutil::toQString(sub.companyid());
					bondValue["Subscribevalue"] = qutil::toBase64(json::toString(SubscribeMap));
					bondValue["fromid"] = val.GetFrom().GetID_C_Str();
					orgTxt["bondvalue"] = bondValue;
					msgcontent = qutil::toBase64(json::toString(orgTxt));
					break;
				}
			case QMClient::OrgType_Price: // �����ı�
				{
					QMClient::QuotationBondBrief sub;
					sub.ParseFromString(OrgNotice.content());
					QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
					bondMsg.replace("{bondkey}", qutil::toQString(sub.combbondkey()));
					bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondtype()));
					bondMsg.replace("{bondval}", qutil::toQString(sub.bondshowname()));
					orgTxt["content"] = QString("%1 %2/%3 %4/%5").arg(bondMsg) \
																 .arg(qutil::toQString(sub.bid())) \
																 .arg(qutil::toQString(sub.ofr())) \
																 .arg(qutil::toQString(sub.volbid())) \
																 .arg(qutil::toQString(sub.volofr()));
					msgcontent = qutil::toBase64(json::toString(orgTxt));
					break;
				}
			case QMClient::OrgType_NoPrice: // ���ޱ����ı�
				{
					QMClient::QuotationBondBriefList sub;
					sub.ParseFromString(OrgNotice.content());
					QStringList bondpriTxt;
					for (int i = 0; i < sub.bondbriefs_size(); i++)
					{
						QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
						bondMsg.replace("{bondkey}", qutil::toQString(sub.bondbriefs(i).combbondkey()));
						bondMsg.replace("{linkColor}", qutil::getBondColor(sub.bondbriefs(i).bondtype()));
						bondMsg.replace("{bondval}", qutil::toQString(sub.bondbriefs(i).bondshowname()));
						bondpriTxt.append(bondMsg);
					}
					orgTxt["content"] = tr("no price of this code at current, you can subscribe it.").arg(bondpriTxt.join(tr("chinese dunhao")));
					msgcontent = qutil::toBase64(json::toString(orgTxt));
					break;
				}
			case QMClient::OrgType_PriceRange: // һ������ȯ����䶯����Ϣ
				{
					QMClient::BondReminderInfo reminderinfo;
					reminderinfo.ParseFromString(OrgNotice.content());
					QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
					bondMsg.replace("{bondkey}", qutil::toQString(reminderinfo.bondbrief().combbondkey()));
					bondMsg.replace("{linkColor}", qutil::getBondColor(reminderinfo.bondbrief().bondtype()));
					bondMsg.replace("{bondval}", qutil::toQString(reminderinfo.bondbrief().bondshowname()));
					orgTxt["content"] =  tr("bond reminder").arg(qutil::toQString(reminderinfo.distributors()))
						.arg(bondMsg)
						.arg(qutil::toQString(reminderinfo.remindertxt()));
					msgcontent = qutil::toBase64(json::toString(orgTxt));

					break;
				}
			default:
				break;
			}
			break;
		}
	case QMClient::MSG_Body_Type_NEWBONDPUSH: // ��ȯ����
		{
			QMClient::BondPushInfo bondpushinfo;
			bondpushinfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString msg;
			msg.append(qutil::toQString(bondpushinfo.title()));
			msg.append("\n");
			for (int bp = 0; bp < bondpushinfo.bondinfos_size(); bp++)
			{
				QString bondurl = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
				QString bondstr;
				bondstr.append(bondurl.isEmpty()? \
					qutil::toQString(bondpushinfo.bondinfos(bp).bondshowname()): \
					qbprice::bondInfo(bondpushinfo.bondinfos(bp)));
				bondstr.append("  ");
				bondstr.append(qutil::toQString(bondpushinfo.bondinfos(bp).term()));
				bondstr.append("  ");
				bondstr.append(qutil::toQString(bondpushinfo.bondinfos(bp).biddingrange()));
				bondstr.append("  ");
				bondstr.append(qutil::toQString(bondpushinfo.bondinfos(bp).issuerrating()));
				bondstr.append("  ");
				bondstr.append(qutil::toQString(bondpushinfo.bondinfos(bp).tenderclosetime()));
				msg.append(bondstr);
				msg.append("\n");
			}
			msg.append(qutil::toQString(bondpushinfo.content()));

			msgcontent = qutil::toBase64(msg);

			break;
		}
	case QMClient::MSG_Body_Type_BONDTXT: // ������Ϣ
		{
			QMClient::BondTxtMsg bondtxt;
			bondtxt.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString msg;
			msg.append(qutil::toQString(bondtxt.title()));
			msg.append("\n");
			msg.append(msgAnalyse::stockAnalyze(qutil::encodeHtml(qutil::toQString(bondtxt.content()))));
			msgcontent = qutil::toBase64(msg);

			break;
		}
	case QMClient::MSG_Body_Type_News: // ����/���� ������
		{
			QMClient::NewsShareBriefInfo newsBrief;
			newsBrief.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QVariantMap briefMap;
			briefMap["weburl"] = QString::fromStdString(curId.GetID()) + ";" + QString::number(val.GetMsgId()) + ";"+ qutil::toQString(newsBrief.weburl());
			QString contentmsg = qutil::toQString(newsBrief.content());
			contentmsg.replace(QRegExp("(<[^>]*>)|(<[^>]*/[^>]*>)"), "");
			briefMap["briefcontent"] = qutil::cutShowText(contentmsg, 150*3, 15, Qt::ElideRight);
			briefMap["content"] = contentmsg;

			QString picPath;
			if (newsBrief.picuuid().empty()) {
				if (newsBrief.newspic().empty()) {	
					CUserInfo uinfo;
					ControllerManager::instance()->getUserInstance()->GetUserInfo(curId, uinfo);
					picPath = qutil::getPicpath(uinfo.GetAvatar());
				} else {
					picPath ="file:///" + qutil::getPicpath(newsBrief.newspic());
				}
			} else {
				string strpath=ControllerManager::instance()->getGlobalInstance()->GetPicURL(newsBrief.picuuid());
				picPath=qutil::toQString(strpath);
			}
			
			briefMap["newsPic"] = picPath;
			briefMap["paramtype"]=newsBrief.paramtype();
			briefMap["linkType"]=newsBrief.linktype();
			msgcontent = qutil::toBase64(json::toString(briefMap));
			break;
		}
	case QMClient::MSG_Body_Type_Bond: // ծȯ --> BondInfo 
		{
			QMClient::BondInfo binfo;
			binfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
			bondMsg.replace("{bondkey}", qutil::toQString(binfo.combbondkey()));
			bondMsg.replace("{linkColor}", qutil::getBondColor(binfo.bondtype()));
			bondMsg.replace("{bondval}", qutil::toQString(binfo.bondshowname()));
			msgcontent = qutil::toBase64(bondMsg);
			break;
		}
	case QMClient::MSG_Body_Type_SessionInfo: // �Ự���� --> SessionInfo
		{
			QMClient::SessionInfo sessioninfo;
			sessioninfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			CProtoComp::SessionInfo(sessioninfo);

			QVariantMap sessionMap;
			sessionMap["sessionid"] = sessioninfo.strid().c_str();
			sessionMap["sessionname"] = qutil::toQString(sessioninfo.showname());
			sessionMap["click"] = false;
			if (!sessioninfo.strid().empty())
			{
				if (ID::MakeQmID(sessioninfo.strid()).IsQmUserId())
				{
					CUserInfo uinfo;
					if (ControllerManager::instance()->getUserInstance()->GetUserInfo(ID::MakeQmID(sessioninfo.strid()), uinfo))
					{
						sessionMap["sessionname"] = qutil::toQString(uinfo.GetShowName());
						sessionMap["click"] = true;
						if (ID::MakeQmID(sessioninfo.strid()) == ControllerManager::instance()->getPersonInstance()->GetCurrentUserID())
						{
							sessionMap["click"] = false;
						}
					}
					else
						LOG_TRACE << "QMClient::MSG_Body_Type_SessionInfo - valid userid, but cannot get its info.";
				}
				else
					LOG_TRACE << "QMClient::MSG_Body_Type_SessionInfo - invalid userid.";
			}
			else
				LOG_TRACE << "QMClient::MSG_Body_Type_SessionInfo - no id.";

			msgcontent = qutil::toBase64(json::toString(sessionMap));
			
			break;
		}
	case QMClient::MSG_Body_Type_Link: // ���� --> LinkInfo
		{
			QMClient::LinkInfo linkinfo;
			linkinfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString link = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_WEBURL));
			link.replace("{weburl}", qutil::toQString(linkinfo.linkurl()));
			link.replace("{weburlcontent}", qutil::toQString(linkinfo.linkcontent()));
			msgcontent = qutil::toBase64(link);

			break;
		}
	case QMClient::MSG_Body_Type_Purchase: // һ���깺������Ϣ --> PurchaseInfo
		{
			QMClient::PurchaseInfo purchaseInfo;
			purchaseInfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QVariantMap purchaseMap;
			purchaseMap["purchaseid"] = QString("%1_%2_%3").arg("QuoteBondPurchase").arg(qutil::toQString(purchaseInfo.purchaseid())).arg(purchaseInfo.modifytime());
			purchaseMap["displayPre"] = qutil::toQString(purchaseInfo.displaypre());
			purchaseMap["displayMain"] = qutil::toQString(purchaseInfo.displaymain());
			purchaseMap["msgid"] = val.GetMsgId();
			QString errMsg = qutil::toQString(ControllerManager::instance()->getQbInstance()->GetPurchaseErrorStatusMsg(purchaseInfo.result()));
			purchaseMap["result"] = (!errMsg.isEmpty()) ? -1 : purchaseInfo.result();
			purchaseMap["errorStr"] = errMsg;
			msgcontent = qutil::toBase64(json::toString(purchaseMap));
			if (purchaseInfo.result() != 0 && //��ѯ��
				purchaseInfo.result() != 2 && //��ȷ��
				purchaseInfo.result() != 3)   //���޸�
			{//-2,-1,1��������ȡ
				if (ControllerManager::instance()->getPersonInstance()->GetCurrentUserID() == val.GetTo())
				{
					if (m_Purchase > 0)
						killTimer(m_Purchase);
					CPurchaseUkey pKey;
					pKey.SetFromId(val.GetFrom());
					pKey.SetToId(val.GetTo());
					pKey.SetMsgid(val.GetMsgId());
					pKey.SetModifyTime(purchaseInfo.modifytime());
					pKey.SetPurchaseID(purchaseInfo.purchaseid());
					m_Purchaselst.push_back(pKey);
					m_Purchase = startTimer(PURCHASETIMER);
				}
			}
			break;
		}
	case QMClient::MSG_Body_Type_ContactInfoSnap: // Ⱥ������ --> ContactInfoSnap
		{
			QMClient::ContactInfoSnap contacts;
			contacts.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			CProtoComp::ContactInfoSnap(contacts);

			QVariantMap mapMass;
			mapMass["ntype"] = contacts.ntype();
			mapMass["bSend"] = contacts.bsend();
			mapMass["msgid"] = val.GetMsgId();
			getMassUsers(val.GetBodyList().bodylist(nIndex).msg(), mapMass);
			
			msgcontent = qutil::toBase64(json::toString(mapMass));
			break;
		}
	case QMClient::MSG_Body_Type_ShareBond: //ծȯ���� --> ShareBondInfo
		{
			QMClient::ShareBondInfo sbond;
			sbond.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());

			QVariantMap mapSBond;
			mapSBond["bondkey"] = qutil::toQString(sbond.combbondkey());
			mapSBond["BondKeyTxt"] = qutil::toQString(sbond.bondcode());
			mapSBond["BondNameTxt"] = qutil::toQString(sbond.bondshortname());
			mapSBond["MemoTxt"] = qutil::toQString(sbond.memo());
			msgcontent = qutil::toBase64(json::toString(mapSBond));
			break;
		}
	case QMClient::MSG_Body_Type_MarginalGuidance: // �߼�ָ�� --> MarginalGuidance
		{
			QMClient::MarginalGuidance mginfo;
			mginfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
			bondMsg.replace("{bondkey}", qutil::toQString(mginfo.bondinfo().combbondkey()));
			bondMsg.replace("{linkColor}", qutil::getBondColor(mginfo.bondinfo().bondtype()));
			bondMsg.replace("{bondval}", qutil::toQString(mginfo.bondinfo().bondshowname()));
			msgcontent = qutil::toBase64(bondMsg+qutil::encodeHtml(qutil::toQString(mginfo.remark())));
			break;
		}
	case QMClient::MSG_Body_Type_BondsDelay: // ծȯ�Ƴٷ��� --> BondsDelay
		{
			QMClient::BondsDelay binfo;
			binfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString bondMsg = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_CREDITORRIGHT));
			bondMsg.replace("{bondkey}", qutil::toQString(binfo.bondinfo().combbondkey()));
			bondMsg.replace("{linkColor}", qutil::getBondColor(binfo.bondinfo().bondtype()));
			bondMsg.replace("{bondval}", qutil::toQString(binfo.bondinfo().bondshowname()));
			msgcontent = qutil::toBase64(tr("bond delay").arg(bondMsg));
			break;
		}
	case QMClient::MSG_Body_Type_Quoted_Alert: // �������� --> QuotedAlertInfo
		{
			QMClient::QuotedAlertInfo ainfo;
			ainfo.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QVariantMap mapSBond;
			mapSBond["alertid"] = ainfo.userid();
			QString nstring = qutil::toQString(ainfo.username());
			if (!ainfo.companyname().empty())
			{
				nstring.append("(");
				nstring.append(qutil::toQString(ainfo.companyname()));
				nstring.append(")");
			}
			else
			{
				CCompanyInfo cpinfo;
				ControllerManager::instance()->getUserInstance()->GetCompanyInfo(ainfo.companyid(), cpinfo);
				if (!cpinfo.GetCompanyShortName().empty())
				{
					nstring.append("(");
					nstring.append(qutil::toQString(cpinfo.GetCompanyShortName()));
					nstring.append(")");
				}
			}
			mapSBond["alertperson"] = nstring;
			mapSBond["actioncontent"] = tr("send a bond");
			mapSBond["bondcontent"] = qutil::encodeHtml(qutil::toQString(ainfo.content()));
			msgcontent = qutil::toBase64(json::toString(mapSBond));
			break;
		}
    case QMClient::MSG_Body_Type_QB_HelpCenterLink: // QB������Ϣ���� --> QBHelpCenterLink
        {
            QMClient::QBHelpCenterLink qbHelpCenterLink;
            qbHelpCenterLink.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
            QVariantMap mapQbHelpCenterLink;
            mapQbHelpCenterLink["text"] = QString::fromStdString(qbHelpCenterLink.content());
            mapQbHelpCenterLink["weburl"] = qutil::toQString(qbHelpCenterLink.url());
            msgcontent = qutil::toBase64(json::toString(mapQbHelpCenterLink));
        }
        break;
	case QMClient::MSG_Body_Type_MWNotice:
		{
			QMClient::MWNoticeMessage noticeMsg;
			noticeMsg.ParseFromString(val.GetBodyList().bodylist(nIndex).msg());
			QString templ = qutil::toQString(ControllerManager::instance()->getResourcesInstance()->GetLables(LABLE_MWNOTICE));
			templ.replace("{title}", qutil::toQString(noticeMsg.title()));
			templ.replace("{bondkey}", qutil::toQString(noticeMsg.bondkey()));
			templ.replace("{linkColor}", qutil::getBondColor(QMClient::Bond_Type_Others));
			templ.replace("{bondval}", qutil::toQString(noticeMsg.bondname()));
			templ.replace("{bondContent}", qutil::toQString(noticeMsg.bondcontent()));
			msgcontent = qutil::toBase64(templ);
		}
		break;
	default:
		break;
	}
	return msgcontent;
}

// ��ʼ���ı�
void webview::initTextSrc()
{
	QVariantMap textmap;
	textmap["senderror"] = tr("senderror");// ��Ϣ����ʧ��  
	textmap["retry"] = tr("retry");// ����
	textmap["shieldstranger"] = tr("shieldstranger");// ��������Ta�ĺ��ѣ���ҪTa����Ϊ���Ѻ�����
	textmap["addfriendyanzheng"] = tr("addfriend yanzheng");// ��������֤
	textmap["otherrecieverefuse"] = tr("otherrecieverefuse");// �Է��ܾ����� 
	textmap["cancel"] = tr("cancelalready");// ��ȡ�� 
	textmap["transfail"] = tr("transfail");// ����ʧ�� 
	textmap["uploadsuccess"] = tr("uploadsuccess");// �ϴ���� 
	textmap["sendsuccess"] = tr("sendsuccess");// ������� 
	textmap["othercancel"] = tr("othercancel");// �Է�ȡ������ 
	textmap["recievereject"] = tr("recievereject");// ȡ������ 
	textmap["recievefail"] = tr("recievefail");// ����ʧ�� 
	textmap["recievesuccess"] = tr("recievesuccess");// ������� 
	textmap["recieveto"] = tr("recieveto");// �ѽ����� 
	textmap["downloadfile"] = tr("downloadfile");// �����ļ��� 
	textmap["saveas"] = tr("saveas");// �����Ϊ 
	textmap["targetfile"] = tr("targetfile");// Ŀ���ļ���
	textmap["searchmore"] = tr("searchmore");//�鿴����
	textmap["searchmoreopenrecord"] = tr("searchmoreopenrecord");//�鿴�������ʷ��Ϣ
	textmap["newmsgfollow"] = tr("newmsgfollow");//����������Ϣ
	textmap["invite"] = tr("invite");//������
	textmap["addingroup"] = tr("addingroup");//������Ⱥ�ġ�
	textmap["youaddgroup"] = tr("youaddgroup"); // ���Ѽ���Ⱥ��
	textmap["meingroup"] = tr("meingroup");//�Ѿ������˱�Ⱥ����������к��ɡ�
	textmap["exitgroup"] = tr("exitgroup");//�˳���Ⱥ�ġ�
	textmap["you"] = tr("you");//��
	textmap["people"] = tr(" people");//��
	textmap["receive"] = tr("receive"); // ����
	textmap["saveas"] = tr("save as"); // ���Ϊ
	textmap["refuse"] = tr("reject"); // �ܾ�����
	textmap["offlinesend"] = tr("offlinesend"); // ���߷���
	textmap["refusesend"] = tr("refusesend"); // ȡ������
	textmap["publishprice"] = tr("publish price"); // ��������
	textmap["replyprice"] = tr("reply price"); // �ظ�����
	textmap["revokeprice"] = tr("revoke price"); // ��������
	textmap["applygroup"] = tr("apply group"); // ������Ⱥ
	textmap["sendmessage"] = tr("send message"); // ������Ϣ
	textmap["roomcode"] = tr("room code");//Ⱥ��
	textmap["roomowner"] = tr("room owner");//Ⱥ�� 
	textmap["total"] = tr("total ");//��
	textmap["month"] = tr("month");//��
	textmap["day"] = tr("day");//��
	textmap["Monday"] = tr("Monday");//����һ
	textmap["Tuesday"] = tr("Tuesday");//���ڶ�
	textmap["Wednesday"] = tr("Wednesday");//������
	textmap["Thursday"] = tr("Thursday");//������
	textmap["Friday"] = tr("Friday");//������
	textmap["Saturday"] = tr("Saturday");//������
	textmap["Sunday"] = tr("Sunday");//������
	textmap["Subscribe"] = tr("Subscribe");//����
	textmap["SubscribeExtendTxt"] = tr("SubscribeExtendTxt");// ����ڼ۸�䶯ʱ�յ�֪ͨ
	textmap["SubscribeSuccess"] = tr("SubscribeSuccess");// �Ѷ���
	textmap["source"] = tr("source"); // ��Դ
	textmap["bond_chu"]=tr("bond_chu");//��
	textmap["bond_shou"]=tr("bond_shou");//��
	textmap["bond_shang"]=tr("bond_shang");//��
	textmap["bond_shen"] =tr("bond_shen");//��
	textmap["Confirm"] = tr("Confirm"); // ȷ��
	textmap["Confirming"] = tr("Confirming"); // ȷ����
	textmap["Confirmed"] = tr("Confirmed"); // ��ȷ��
	textmap["ConfirmFailed"] = tr("ConfirmFailed"); // ȷ��ʧ��
	textmap["context"] = tr("context");//����������
	textmap["mass_group_send_message"] = tr("mass group send message"); // ��������������Ϣ��
	textmap["you_send_to"] = tr("you send to"); // ��������
	textmap["error_send"] = tr("error send"); // ����ʧ��
	textmap["message_resend"] = tr("message rsend"); // ���·���
	textmap["message_resended"] = tr("message rsended"); // �����·���
	textmap["left_parenthesis"] = tr("left parenthesis"); // ��
	textmap["error_send_that_need_check_to_be_friend"] = tr("error send that need check to be friend"); // ��������֤���ѣ����޷��յ�������Ϣ��
	textmap["check_resend"] = tr("check rsend"); // ���·�����֤
	textmap["check_resended"] = tr("check rsended"); // �����·�����֤
	textmap["dowloadingimg"] = qutil::toBase64("file:///"+qutil::skin("js/loading.gif"));
	textmap["uploadto"] = tr("uploadto"); // �ϴ���
	textmap["roomfiles"] = tr("roomfiles"); // Ⱥ�ļ�
	textmap["cancelupload"] = tr("cancelupload"); // ȡ���ϴ�
	textmap["uploadfail"] = tr("uploadfail"); // �ϴ�ʧ��
	QString content = QString("chatMessageManager.initStringResource('%1')").arg(json::toString(textmap));
	runjs(content);
}

void webview::initLables()
{
	std::map<LABLE_e, std::string> mapLables;
	// QB �ʽ�ȯ����
	mapLables[LABLE_CREDITORRIGHT] = "<a href='QBQuote_{bondkey}' bondId='{bondkey}' class='bondlink_style' onclick='bondopt(this);'><font color='{linkColor}' >{bondval}</font></a>";
	// �����ֶ�
	mapLables[LABLE_ALERT] = "<span class='alertcss'>{highLight}</span>";
	// ��ҳ����
	mapLables[LABLE_WEBURL] = "<span class='weblink'><a value='{weburl}' class='link_style' onclick='openurl(this);'>{weburlcontent}</a></span>";
	// ���ı��鿴��������
	mapLables[LABLE_SEEMORE] = "<div  class='see_more_link' style='display:inline-block;' value='{msgid}' onclick='showCompleteMsg(this);'>{seemore}</div>";
	// Ⱥ����Ϣ�ϰ벿��ģ��
	mapLables[LABLE_GROUPMASSTITLE] = "<div class='group_send_tip'>{tip}</div><span class='receiver_list' value='{receiverids}' onclick='showGroupMassReceivers(this);'>{receiverList}</span>";
	// @
	mapLables[LABLE_ALTED] = "<span class='altedcss'>{altedContent}</span>";
	// ���й���
	mapLables[LABLE_MWNOTICE] = "<span>{title}</span><a href='QBQuote_{bondkey}' bondId='{bondkey}' class='bondlink_style' onclick='bondopt(this);'><font color='{linkColor}' >{bondval}</font></a><span>{bondContent}</span>";

	ControllerManager::instance()->getResourcesInstance()->SetLable(mapLables);
}

// ��ʼ���������
void webview::initErrorCode()
{
	QVariantMap errorCode;
	errorCode["ERR_CLIENT_REJECT_STRANGER_MSG"] = ERR_CLIENT_REJECT_STRANGER_MSG;
	QString content = QString("chatMessageManager.initErrorCode('%1')").arg(json::toString(errorCode));
	runjs(content);
}

// ��ȡ�ļ�icon
QString webview::getFileIcon(const QString &filep, const std::string &ficon)
{
	QFileInfo tmpFile;
	if (!ficon.empty())
	{
		QPixmap pix;
		QByteArray myByteArray;
		QBuffer buffer( &myByteArray );
		buffer.setData(ficon.c_str(), ficon.size());
		pix.loadFromData(myByteArray, qutil::getImageFormat(ficon));
		pix = pix.scaled(QSize(36,36), Qt::KeepAspectRatio);
        QString fileicon = qutil::toQString(AppInfo::getFileIconPath()) + qutil::dmd5(ficon) + qutil::getImageFileSuffix(IMAGEPNG);
		tmpFile.setFile(fileicon);
		pix.save(tmpFile.filePath(), IMAGEPNG);
	}
	else
	{
		QFileInfo fileInfo(filep);
		QFileIconProvider fileIcon;
		QIcon icon;
		if (fileInfo.exists())
		{
			icon = fileIcon.icon(fileInfo);
			QPixmap pixmap = icon.pixmap(36,36);
			QString fileicon = qutil::toQString(AppInfo::getFileIconPath()) + qutil::fmd5(fileInfo.filePath()) + qutil::getImageFileSuffix(IMAGEPNG);
			tmpFile.setFile(fileicon);
			pixmap.save(tmpFile.filePath(), IMAGEPNG);
		}
		else
		{
			QString fileicon = qutil::toQString(AppInfo::getFileIconPath()) + qutil::fmd5(fileInfo.filePath()) + qutil::getImageFileSuffix(IMAGEPNG);
			tmpFile.setFile(fileicon);
		}
	}

	if (!tmpFile.exists() || tmpFile.suffix().isEmpty()) {
		QPixmap iconPixmap = QPixmap(qutil::skin("fileicon/icon_unknown_36x36.png"));
		iconPixmap = iconPixmap.scaled(QSize(36, 36), Qt::KeepAspectRatio);
		iconPixmap.save(tmpFile.filePath(), IMAGEPNG);
	}
	return "file:///" + tmpFile.filePath();
}

// ��ȡ��html������ͼƬ��Դ��·��
QString webview::getImagepFromHtml(const QString &valp)
{
	QString s = valp;
	if (valp.startsWith("qrc:/"))
	{
		return s.replace("qrc:/", ":/");
	}
	if (valp.startsWith("file:"))
	{
		return s.replace(QRegExp("file:/+"), "");
	}
	if (valp.startsWith("http:/"))
	{
		return s;
	}
	return "";
}

//// ���ֽض�չʾ
//QString webview::cutShowText(const QString &val, int length/*�������س���*/, int fontsize/*�������ش�С*/, Qt::TextElideMode qmode)
//{
//	QFont font;
//	font.setFamily("Microsoft YaHei");
//	font.setPixelSize(fontsize);
//	QFontMetrics fm(font);
//	int pointsize = fm.width(QStringLiteral("..."));
//	int realsize = fm.width(val);
//	QString str = val;
//	if (realsize > length)
//	{
//		int i = 0;
//		while (fm.width(fm.elidedText(val, qmode, length+length*i/8))<length)
//		{
//			str = fm.elidedText(val, qmode, length+length*i/8);
//			i++;
//		}
//	}
//	return str;
//}

// ��ȡ��Ϣ���� ��������Ⱥ����֪ͨ�ȴ����ͣ�
QString webview::getMessageType(EMSGType t)
{
	QString tp;
	switch (t) 
	{
	case MSG_Type_Invalid:                       // ��Ч�Ự
		tp = "MSG_Type_Invalid";
		break;
	case MSG_Type_Chat:                          // ����
		tp = "MSG_Type_Chat";
		break;
	case MSG_Type_Group_Normal:                  // Ⱥ��
		tp= "MSG_Type_Group_Normal";
		break;
	case MSG_Type_Group_Broadcast:               // Ⱥ��
		tp= "MSG_Type_Group_Broadcast";
		break;
	case MSG_Type_Group_Speaker:                 // SpeakerȺ��
		tp= "MSG_Type_Group_Speaker";
		break;
	case MSG_Type_Group_CustomerService:         // �ͷ�Ⱥ
		tp= "MSG_Type_Group_CustomerService";
		break;
	case MSG_Type_Broadcast:                     // �㲥��Ϣ 
		tp= "MSG_Type_Broadcast";
		break;
	default:
		tp = "MSG_Type_Invalid";
		break;
	}
	return tp;
}

// ��ȡ�ļ�״̬
QString webview::getFileSendStatus(EFileSendStatus status)
{
	QString filestatus;
	switch (status) 
	{
	case FileSend_Status_UnTreated:                // δ����
		filestatus = "FileSend_Status_UnTreated";
		break;
	case FileSend_Status_Uploading:                  // �ϴ���
		filestatus = "FileSend_Status_Uploading";
		break;
	case FileSend_Status_Uploaded:                   // ���ϴ�
		filestatus= "FileSend_Status_Uploaded";
		break;
	case FileSend_Status_Reject:                  // �Ѿܾ�
		filestatus= "FileSend_Status_Reject";
		break;
	case FileSend_Status_Cancel:                   // ��ȡ��
		filestatus= "FileSend_Status_Cancel";
		break;
	case FileSend_Status_Failed:                   // ��ʧ��
		filestatus= "FileSend_Status_Failed";
		break;
	case FileSend_Status_Done:                     // ����� 
		filestatus= "FileSend_Status_Done";
		break;
	default:
		filestatus = "FileSend_Status_Failed";
		break;
	}
	return filestatus;
}

QString webview::getFileRecvStatus(EFileRecvStatus status)
{
	QString filestatus;
	switch (status) 
	{
	case FileRecv_Status_UnTreated:                // δ����
		filestatus = "FileRecv_Status_UnTreated";
		break;
	case FileRecv_Status_DownLoading:                  // ������
		filestatus = "FileRecv_Status_DownLoading";
		break;
	case FileRecv_Status_Reject:                  // �Ѿܾ�
		filestatus= "FileRecv_Status_Reject";
		break;
	case FileRecv_Status_Cancel:                   // ��ȡ��
		filestatus= "FileRecv_Status_Cancel";
		break;
	case FileRecv_Status_Failed:                   // ��ʧ��
		filestatus= "FileRecv_Status_Failed";
		break;
	case FileRecv_Status_Done:                     // ����� 
		filestatus= "FileRecv_Status_Done";
		break;
	default:
		filestatus = "FileRecv_Status_Failed";
		break;
	}
	return filestatus;
}

// ��ȡ��ǰҳ�������
QString webview::getPageType(VIEWSTYLE_e eType)
{
	QString sType = "";
	switch (eType)
	{
	case VIEW_DEFAULT:
		sType = "VIEW_DEFAULT";
		break;
	case VIEW_CURPAGE:
		sType = "VIEW_CURPAGE";
		break;
	case VIEW_RECORD:
		sType = "VIEW_RECORD";
		break;
	case VIEW_GRECORD:
		sType = "VIEW_GRECORD";
		break;
	case VIEW_INTERACTION:
		sType = "VIEW_INTERACTION";
		break;
	default:
		sType = "VIEW_DEFAULT";
		break;
	}
	return sType;
}

// ��ȡMENU�˵�������
RightClickMenuType webview::getMenuType()
{
	RightClickMenuType menutype = MENU_CHAT;
	if (m_viewstyle == VIEW_CURPAGE)
	{
		menutype = MENU_CHAT;
	}
	else if (m_viewstyle == VIEW_RECORD)
	{
		menutype = MENU_HISTORYCHAT;
	}
	return menutype;
}

// ����չʾ����
void webview::setShowPercent(UINT32 sp)
{
	CSystemConfig sysconfig = ControllerManager::instance()->getPersonInstance()->GetSystemConfig();
	sysconfig.SetShowPercent(sp);
	ControllerManager::instance()->getPersonInstance()->SetSystemConfig(sysconfig);
}

// from sumscope
void webview::fromSumscope()
{
	QClipboard *clipboard = QApplication::clipboard();
	QString oldData = clipboard->mimeData()->html();
	// ȥ����ʽ
	oldData.replace(QRegExp("(<(?![bB][rR])(?![iI][mM][gG])[^>]*>)|(<(?![bB][rR])(?![iI][mM][gG])[^>]*/[^>]*>)"), "");
	QString oldDataTxt = clipboard->mimeData()->text();
	QMimeData *pmime = new QMimeData;
	if (m_Chattoid.IsQmOrgId())
	{
		pmime->setData("text/html", QString(oldData+tr("fromSumscope")).toUtf8());
		pmime->setData("text/plain", QString(oldDataTxt+tr("fromSumscope")).toUtf8());
	}
	else
	{
		pmime->setData("text/html", QString(oldData).toUtf8());
		pmime->setData("text/plain", QString(oldDataTxt).toUtf8());
	}
	clipboard->setMimeData(pmime);
}

// Ⱥ���� ��Ϣ����Ա��Ϣչʾ
void webview::getMassUsers(const std::string &val, QVariantMap &contactsMap)
{
	QString strSend = tr("mass group send message");

	CContactInfoSnap conInfo;
	ControllerManager::instance()->getMessageInstance()->ContactInfoSnapParse(val, conInfo);
	// չʾ���������3��
	QString strMemsShow;
	QStringList namelst;
	for (int i = 0; i < qMin(3, (int)conInfo.GetNameList().size()); i++)
	{
		namelst.append(qutil::toQString(conInfo.GetNameList()[i]));
	}
	strMemsShow = namelst.join(", ");
	// ��֯��Ա����
	if (!conInfo.GetNameList().empty() && conInfo.GetNameList().size() > 3)
	{
		strMemsShow.append(tr("as Count mems").arg(conInfo.GetIdList().size()));
	}
	QVariantMap idlink;
	QList<QVariant> lstids;
	for (size_t i = 0; i < conInfo.GetIdList().size(); i++)
	{
		lstids.append(conInfo.GetIdList()[i].GetID_C_Str());
	}
	idlink["ids"] = lstids;
	QList<QVariant> lstnames;
	for (size_t i = 0; i < conInfo.GetNameList().size(); i++)
	{
		lstnames.append(conInfo.GetNameList()[i].c_str());
	}
	idlink["names"] = lstnames;
	QList<QVariant> lstcompany;
	for (size_t i = 0; i < conInfo.GetNameList().size(); i++)
	{
		lstcompany.append(conInfo.GetCompanyNameList()[i].c_str());
	}
	idlink["companys"] = lstcompany;

	contactsMap["tip"] = strSend;
	contactsMap["receiverList"] = strMemsShow;
	contactsMap["receiverids"] = json::toString(idlink);
}

// test ����Сת
//QVariantMap webview::testRobotMsg(const CMessage & msg, bool b)
//{
//	auto basicMessage = [this] () ->QMClient::MessageBodyList {
//		QMClient::MessageBodyList msglist;
//		static int iiiiii = 16;
//		switch (iiiiii) 
//		{
//		case 0: // ͼ����Ϣ
//			{
//				
//				break;
//			}
//		case 1: // �ʽ𱨼� ����
//			{
//				int nall = QDateTime::currentMSecsSinceEpoch()%5 +1;
//				for (int i = 0; i < nall; i++)
//				{
//					QMClient::MessageBody* msgbody = msglist.add_bodylist();
//					msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteMoney);
//					QMClient::QuotationMoneyInfo qutoinfo = qbprice::getTestData(1);
//					msgbody->set_msg(qutoinfo.SerializeAsString());
//				}
//				break;
//			}
//		case 2: // �ʽ𱨼� �ظ�
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteMoney);
//				QMClient::QuotationMoneyInfo qutoinfo = qbprice::getTestData(2);
//				msgbody->set_msg(qutoinfo.SerializeAsString());
//				break;
//			}
//		case 3: // �ʽ𱨼� ����
//			{
//				int nall = QDateTime::currentMSecsSinceEpoch()%5 +1;
//				for (int i = 0; i < nall; i++)
//				{
//					QMClient::MessageBody* msgbody = msglist.add_bodylist();
//					msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteMoney);
//					QMClient::QuotationMoneyInfo qutoinfo = qbprice::getTestData(3);
//					msgbody->set_msg(qutoinfo.SerializeAsString());
//				}
//				break;
//			}
//		case 4:
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_EnhancedTEXT);
//				QMClient::TxtContent txt;
//				
//				if (QDateTime::currentMSecsSinceEpoch()%1)
//				{
//					txt.set_type(QMClient::TxtType_Basic);
//					txt.set_content(qutil::toString(QStringLiteral("sadadasd��������")));
//					msgbody->set_msg(txt.SerializeAsString());
//				} 
//				else
//				{
//					txt.set_type(QMClient::TxtType_Organization);
//					QMClient::TxtOrganization orgTxt;
//					switch (QDateTime::currentMSecsSinceEpoch()%10)
//					{
//					case 0:case 2:case 4:case 6:case 8:
//						{
//							orgTxt.set_type(QMClient::OrgType_Subscribe);
//							QMClient::QuotationBondBrief sc;
//							sc.set_bondtype(1);
//							sc.set_combbondkey(STRINGUTF8("010609.SH"));
//							sc.set_companyid(STRINGUTF8("sumscope"));
//							sc.set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//							orgTxt.set_content(sc.SerializeAsString());
//
//							break;
//						}
//					default:
//						{
//							orgTxt.set_type(QMClient::OrgType_Basic);
//							orgTxt.set_content(qutil::toString(QStringLiteral("���޴�ȯ��")));
//						}
//						break;
//					}
//					txt.set_content(orgTxt.SerializeAsString());
//				}
//				msgbody->set_msg(txt.SerializeAsString());
//				break;
//			}
//		case 5: // ������Ѷ-�ƾ�ͷ��
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_FinancialNews);
//				QMClient::FinancialNewsInfo oMsg;
//				oMsg.set_type(QMClient::News_Txt);
//				oMsg.set_content(qutil::toString(QStringLiteral("<i><font color= 'red'>O(��_��)O����~</font></i>")));
//				msgbody->set_msg(oMsg.SerializeAsString());
//				break;
//			}
//		case 6: // ����֪ͨ
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_OrganizationNotice);
//				QMClient::OrganizationNoticeInfo txt;
//
//				switch (QDateTime::currentMSecsSinceEpoch()%4)
//				{
//				case 0:
//					{
//						txt.set_type(QMClient::OrgType_Basic);
//						txt.set_content(qutil::toString(QStringLiteral("sadadasd��������")));
//						msgbody->set_msg(txt.SerializeAsString());
//						break;
//					}
//				case 1:
//					{
//						txt.set_type(QMClient::OrgType_Subscribe);
//						QMClient::QuotationBondBrief sc;
//						sc.set_bondtype(1);
//						sc.set_combbondkey(STRINGUTF8("010609.SH"));
//						sc.set_companyid(STRINGUTF8("sumscope"));
//						sc.set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//						txt.set_content(sc.SerializeAsString());
//						msgbody->set_msg(txt.SerializeAsString());
//						break;
//					}
//				case 2:
//					{
//						txt.set_type(QMClient::OrgType_Price);
//						QMClient::QuotationBondBrief sc;
//						sc.set_bondtype(1);
//						sc.set_combbondkey(STRINGUTF8("010609.SH"));
//						sc.set_companyid(STRINGUTF8("sumscope"));
//						sc.set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//						txt.set_content(sc.SerializeAsString());
//						msgbody->set_msg(txt.SerializeAsString());
//						break;
//					}
//				case 3:
//					{
//						txt.set_type(QMClient::OrgType_NoPrice);
//						QMClient::QuotationBondBriefList o;
//						QMClient::QuotationBondBrief* sc = o.add_bondbriefs();
//						sc->set_bondtype(1);
//						sc->set_combbondkey(STRINGUTF8("010609.SH"));
//						sc->set_companyid(STRINGUTF8("sumscope"));
//						sc->set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//						QMClient::QuotationBondBrief* sc1 = o.add_bondbriefs();
//						sc1->set_bondtype(1);
//						sc1->set_combbondkey(STRINGUTF8("0106099.SH"));
//						sc1->set_companyid(STRINGUTF8("sumscope9"));
//						sc1->set_bondshowname(qutil::toString(QStringLiteral("069��ծ(99) 11.269Y (0106099.SH)")));
//						txt.set_content(o.SerializeAsString());
//						msgbody->set_msg(txt.SerializeAsString());
//						break;
//					}
//				}
//				
//				break;
//			}
//		//case 6: // ���� ����
//		//	{
//
//		//		break;
//		//	}
//		case 7: // ծȯ ����
//			{
//				int nall = QDateTime::currentMSecsSinceEpoch()%5 +1;
//				for (int i = 0; i < nall; i++)
//				{
//					QMClient::MessageBody* msgbody = msglist.add_bodylist();
//					msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteBond);
//					QMClient::QuotationBondInfo qutoinfo = qbprice::getTestBondData(1);
//					msgbody->set_msg(qutoinfo.SerializeAsString());
//				}
//				break;
//			}
//		case 8: // ծȯ �ظ�
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteBond);
//				QMClient::QuotationBondInfo qutoinfo = qbprice::getTestBondData(2);
//				msgbody->set_msg(qutoinfo.SerializeAsString());
//				break;
//			}
//		case 9: // ծȯ ����
//			{
//				int nall = QDateTime::currentMSecsSinceEpoch()%5 +1;
//				for (int i = 0; i < nall; i++)
//				{
//					QMClient::MessageBody* msgbody = msglist.add_bodylist();
//					msgbody->set_type(QMClient::MSG_Body_Type_QB_QuoteBond);
//					QMClient::QuotationBondInfo qutoinfo = qbprice::getTestBondData(3);
//					msgbody->set_msg(qutoinfo.SerializeAsString());
//				}
//				break;
//			}
//		case 10: // ��������Ѷ�����
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_News);
//				QMClient::NewsShareBriefInfo oNews;
//				oNews.set_content(qutil::toString(QStringLiteral("�콫��������˹��Ҳ�����ȿ�����־������������Ҹ�������Ϊ��O(��_��)O����~O(��_��)O����~")));
//				oNews.set_weburl(qutil::toString("http://news.163.com/15/0818/15/B1AFI3RH0001124J.html"));
//				std::string buf;
//				qutil::setBufferFromPicpath("D:/sumscope/QM/pictures/969a4026ff71e4174f1eca9449b8cab6.png", buf);
//				oNews.set_newspic(buf);
//				msgbody->set_msg(oNews.SerializeAsString());
//				break;
//			}
//		case 11:
//			{
//				QMClient::MessageBody* txtmsgbody = msglist.add_bodylist();
//				txtmsgbody->set_type(QMClient::MSG_Body_Type_EnhancedTEXT);
//				QMClient::TxtContent txt;
//				txt.set_type(QMClient::TxtType_Basic);
//				txt.set_content(qutil::toString(QStringLiteral("sadadasd������<i>asdasd</i>����������ŷ���³��⳵���ǳ��ڳɳ��ֳ����г����г����г�������⳵����˹����������11111111111111111111111111111111111111111111111111111111111111111")));
//				txtmsgbody->set_msg(txt.SerializeAsString());
//
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_SessionInfo);
//				QMClient::SessionInfo sInfo;
//				//sInfo.set_struserid(QDateTime::currentMSecsSinceEpoch() % 2 == 0 ? ID::NULL_ID : ID::MakeQmID("10010116"));
//				sInfo.set_showname(qutil::toString(QStringLiteral("�������쪴�ʦ��ʱ����������������������������������������")));
//				msgbody->set_msg(sInfo.SerializeAsString());
//				break;
//			}
//		case 12:
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_Purchase);
//				QMClient::PurchaseInfo oinfo;
//				oinfo.set_purchaseid(std::string("123456"));
//				oinfo.set_modifytime(QDateTime::currentMSecsSinceEpoch());
//				oinfo.set_displaypre(qutil::toString(QStringLiteral("����ͨ��QB�깺��")));
//				oinfo.set_displaymain(qutil::toString(QStringLiteral("15����MTN001��4.50%/5000��")));
//				oinfo.set_result(1);
//				msgbody->set_msg(oinfo.SerializeAsString());
//			}
//		case 13:
//			{// Ⱥ������
//				iiiiii++;
//				QMClient::MessageBody* allbody = msglist.add_bodylist();
//				allbody->set_type(QMClient::MSG_Body_Type_ContactInfoSnap);
//				QMClient::ContactInfoSnap cinfo;
//				//cinfo.add_id("qm_10009907"); cinfo.add_id("qm_10009908");
//				//cinfo.add_id("qm_10009909"); cinfo.add_id("qm_10009910");
//				//cinfo.add_id("qm_10009911"); cinfo.add_id("qm_10009912");
//				cinfo.add_name(qutil::toString(QStringLiteral("����")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��ʥƽ")));
//				cinfo.add_name(qutil::toString(QStringLiteral("�쾲")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��ƽ��")));
//				cinfo.add_name(qutil::toString(QStringLiteral("������")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��˼��")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾1")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾2")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾3")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾4")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾5")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾6")));
//				cinfo.set_bsend(false);
//				cinfo.set_ntype(QMClient::ContactInfoSnap::e_success);
//				allbody->set_msg(cinfo.SerializeAsString());
//
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_EnhancedTEXT);
//				QMClient::TxtContent txt;
//				txt.set_content(qutil::toString(QStringLiteral("������������<i> o $$ o <i>�۰��Ǵ��˵�ط�����ط������Ǻ󽻸��ûظ��ظ��ûؼҷŸϽ��ָ��滮����͵緹���緹���緹���緹�����������ձ��������ͣ�fmsmgkf�緹���緹���緹����������Ҳ�Ź��볡vbfg���۵��˴�vbfgchrthfncvbdxgertrnfgbcbvedryyjghncbcbccbcvbcbrtyfgb")));
//				msgbody->set_msg(txt.SerializeAsString());
//				break;
//			}
//		case 14:
//			{// Ⱥ�����ָ��´���
//				iiiiii--;
//				QMClient::MessageBody* allbody = msglist.add_bodylist();
//				allbody->set_type(QMClient::MSG_Body_Type_ContactInfoSnap);
//				QMClient::ContactInfoSnap cinfo;
//				//cinfo.add_id("qm_10009907"); cinfo.add_id("qm_10009908");
//				//cinfo.add_id("qm_10009909"); cinfo.add_id("qm_10009910");
//				//cinfo.add_id("qm_10009911"); cinfo.add_id("qm_10009912");
//				cinfo.add_name(qutil::toString(QStringLiteral("����")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��ʥƽ")));
//				cinfo.add_name(qutil::toString(QStringLiteral("�쾲")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��ƽ��")));
//				cinfo.add_name(qutil::toString(QStringLiteral("������")));
//				cinfo.add_name(qutil::toString(QStringLiteral("��˼��")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾1")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾2")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾3")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾4")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾5")));
//				cinfo.add_company(qutil::toString(QStringLiteral("��˾6")));
//				cinfo.set_bsend(false);
//				cinfo.set_ntype(QMClient::ContactInfoSnap::e_success);
//				allbody->set_msg(cinfo.SerializeAsString());
//
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_EnhancedTEXT);
//				QMClient::TxtContent txt;
//				txt.set_content(qutil::toString(QStringLiteral("������������<i> o $$ o <i>�۰��Ǵ��˵�ط�����ط������Ǻ󽻸��ûظ��ظ��ûؼҷŸϽ��ָ��滮����͵緹���緹���緹���緹�����������ձ��������ͣ�fmsmgkf�緹���緹���緹����������Ҳ�Ź��볡vbfg���۵��˴�vbfgchrthfncvbdxgertrnfgbcbvedryyjghncbcbccbcvbcbrtyfgb")));
//				msgbody->set_msg(txt.SerializeAsString());
//
//				QMClient::MessageBody* errorbody = msglist.add_bodylist();
//				errorbody->set_type(QMClient::MSG_Body_Type_ContactInfoSnap);
//				QMClient::ContactInfoSnap errorcinfo;
//				//errorcinfo.add_strid("qm_10009907"); errorcinfo.add_strid("qm_10009908"); errorcinfo.add_strid("qm_10009909");
//				errorcinfo.add_name(qutil::toString(QStringLiteral("����")));
//				errorcinfo.add_name(qutil::toString(QStringLiteral("��ʥƽ")));
//				errorcinfo.add_name(qutil::toString(QStringLiteral("�쾲")));
//				errorcinfo.add_company(qutil::toString(QStringLiteral("��˾1")));
//				errorcinfo.add_company(qutil::toString(QStringLiteral("��˾2")));
//				errorcinfo.add_company(qutil::toString(QStringLiteral("��˾3")));
//				errorcinfo.set_bsend(false);
//				errorcinfo.set_ntype(QMClient::ContactInfoSnap::e_sendfail);
//				errorbody->set_msg(errorcinfo.SerializeAsString());
//
//				QMClient::MessageBody *checkbody = msglist.add_bodylist();
//				checkbody->set_type(QMClient::MSG_Body_Type_ContactInfoSnap);
//				QMClient::ContactInfoSnap checkcinfo;
//				//checkcinfo.add_strid("qm_10009910"); checkcinfo.add_strid("qm_10009911"); checkcinfo.add_strid("qm_10009912");
//				checkcinfo.add_name(qutil::toString(QStringLiteral("��ƽ��")));
//				checkcinfo.add_name(qutil::toString(QStringLiteral("������")));
//				checkcinfo.add_name(qutil::toString(QStringLiteral("��˼��")));
//				checkcinfo.add_company(qutil::toString(QStringLiteral("��˾4")));
//				checkcinfo.add_company(qutil::toString(QStringLiteral("��˾5")));
//				checkcinfo.add_company(qutil::toString(QStringLiteral("��˾6")));
//				checkcinfo.set_bsend(false);
//				checkcinfo.set_ntype(QMClient::ContactInfoSnap::e_addcheck);
//				checkbody->set_msg(checkcinfo.SerializeAsString());
//
//
//				break;
//			}
//		case 15:
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_ShareBond);
//				QMClient::ShareBondInfo oinfo;
//				oinfo.set_combbondkey(std::string("010609.SH"));
//				oinfo.set_bondcode(std::string("010609.SH"));
//				oinfo.set_bondshortname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//				oinfo.set_memo(std::string("4.50% 4.50%"));
//				msgbody->set_msg(oinfo.SerializeAsString());
//				msgbody->set_basiccontent(std::string("010609.SH 4.50% 4.50%"));
//				break;
//			}
//		case 16:
//			{
//				QMClient::MessageBody* msgbody = msglist.add_bodylist();
//				msgbody->set_type(QMClient::MSG_Body_Type_MarginalGuidance);
//				QMClient::MarginalGuidance oinfo;
//				oinfo.mutable_bondinfo()->set_combbondkey(std::string("010609.SH"));
//				oinfo.mutable_bondinfo()->set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//				oinfo.set_remark(qutil::toString(QStringLiteral("������Ǯ��׬Ǯ�ˡ�")));
//				msgbody->set_msg(oinfo.SerializeAsString());
//
//				QMClient::MessageBody* msgbody1 = msglist.add_bodylist();
//				msgbody1->set_type(QMClient::MSG_Body_Type_BondsDelay);
//				QMClient::BondsDelay oinfo1;
//				oinfo1.mutable_bondinfo()->set_combbondkey(std::string("010609.SH"));
//				oinfo1.mutable_bondinfo()->set_bondshowname(qutil::toString(QStringLiteral("06��ծ(9) 11.26Y (010609.SH)")));
//				msgbody1->set_msg(oinfo1.SerializeAsString());
//
//				QMClient::MessageBody* msgbody2 = msglist.add_bodylist();
//				msgbody2->set_type(QMClient::MSG_Body_Type_Quoted_Alert);
//				QMClient::QuotedAlertInfo oinfo2;
//				oinfo2.set_userid(10009911);
//				oinfo2.set_username(qutil::toString(QStringLiteral("��ɺɺ")));
//				oinfo2.set_companyname(qutil::toString(QStringLiteral("ɽ����ͨ")));
//				oinfo2.set_content(qutil::toString(QStringLiteral("15����MTN001��4.50%/5000��")));
//				msgbody2->set_msg(oinfo2.SerializeAsString());
//				break;
//			}
//		default:
//			break;
//		}
//		return msglist;
//	};
//
//#if 0
//	QMClient::MessageBodyList oldmsg = basicMessage();
//	QMClient::MessageBodyList newmsg;
//	newmsg.set_bodylisttype(QMClient::MSG_Robot);
//	*newmsg.mutable_bodylist() = oldmsg.bodylist();
//	QMClient::RobotMessageInfo robotmsg;
//	robotmsg.set_id(ViewController::instance()->getChatOtherid(msg));
//	robotmsg.set_name(qutil::toString(QStringLiteral("�۹���")));
//	robotmsg.set_orgname(qutil::toString(QStringLiteral("ɭ��")));
//	robotmsg.set_sayextend(qutil::toString(QStringLiteral("˵����")));
//	robotmsg.set_autoextend(qutil::toString(QStringLiteral("�Զ��ظ���")));
//	*robotmsg.mutable_bodylist() = basicMessage().bodylist();
//	newmsg.set_extendcontent(robotmsg.SerializeAsString());
//	CMessage tmpmsg = msg;
//	tmpmsg.SetBodyList(newmsg);
//	tmpmsg.SetFrom(util::RobotId());
//	return analyzeMessagetoVmap(tmpmsg, b);
//#else
//	CMessage tmpmsg = msg;
//	QMClient::MessageBodyList newmsg = basicMessage();
//	newmsg.set_bodylisttype(QMClient::MSG_Basic);
//	tmpmsg.SetBodyList(newmsg);
//	tmpmsg.SetFrom(ID::QmMassAssistantID());
//	tmpmsg.SetTo(ControllerManager::instance()->getPersonInstance()->GetCurrentUserID());
//	tmpmsg.SetMsgId(111);
//	return analyzeMessagetoVmap(tmpmsg, b);
//#endif
//}

void webview::notScrollBottom(const std::function<void(bool ok)> &callback)
{
	runjs("windowManager.notScrollBottom()", [callback](const QVariant &val) {
		callback(val.toBool());
	});
}

void webview::slotUpHistory()
{
	emit s_upToHistory();
}

void webview::slotDownHistory()
{
	emit s_downToHistory();
}

void webview::slotUpdateSetting()
{
	const CSystemConfig &systemConfig=ControllerManager::instance()->getPersonInstance()->GetSystemConfig();
	QVariantMap vm;
	vm["quoteshowset"] = 1;
	vm["CardModel"] = systemConfig.GetCardModel() ? 1 : 0;
	vm["upPage"] = systemConfig.GetUpPage() ? 1 : 0;
	vm["downPage"] = systemConfig.GetDownPage() ? 1 : 0;
	QString str = QString("chatMessageManager.updateSetting('%1')").arg(json::toString(vm));
	runjs(str);
}

//////////////////////////////////////////////////////////////////////////
// ��ʾ�����ѽ���
//void webview::slotFloatHandle(const QString &val)
//{
//	QVariant res;
//	QVariantMap mp = json::toMap(QByteArray::fromBase64(val.toUtf8()));
//	if (mp["type"].toString() == "xxxxxx") {
//		// ��Ϣ��ת
//		//emit ViewSignalManager::instance()->sigMsgGoto(0,0);
//	} else if (mp["type"].toString() == "yyyyyy") {
//	}
//}

void webview::ShowSystemTips( const QVariantMap &msg )
{
	QString str = QString("TipsManager.showSystemTips('%1')").arg(QString(qutil::toBase64(json::toString(msg))));
	runjs(str);
}

bool webview::isInit() const
{
	return m_isWebLoadFinished && m_isWebChannelInit;
}
