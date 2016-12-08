#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineView>
#include <QHash>
#include "qutil.h"
#include "right_click_menu.h"

// ��ҳչʾ��ʽ
class CMessage;
typedef enum VIEWSTYLE_e  
{
	// Ĭ����ͨ��ҳչʾ
	VIEW_DEFAULT = 0,
	//��ǰ��Ϣչʾ
	VIEW_CURPAGE,
	// ������Ϣ��¼չʾ ����
	VIEW_RECORD,
	// ��Ϣ��¼ ȫ�ֲ���ҳ��
	VIEW_GRECORD,
	// �ظ����۵� ��Ƕ���ؽ���ҳ��
	VIEW_INTERACTION,
	//ϵͳ��Ϣ��ʾ
	VIEW_SYS_TIPS,
	// Ⱥ�ļ�
	VIEW_ROOMFILE
}VIEWSTYLE_e;

class QWebChannel;
class WebViewChannel;
class WebPage;
class webview : public QWebEngineView
{
	Q_OBJECT
public:
	typedef std::function<void(const QVariant &)> JsResponseCb;
	explicit webview(QWidget *parent = 0, VIEWSTYLE_e eStyle = VIEW_DEFAULT);
	~webview();

	WebViewChannel* channel();

	// ��װrunJavaScript���������
	void runjs(const QString &js, const JsResponseCb &cb = nullptr);

private:
	// չʾ��ʽ
	VIEWSTYLE_e m_viewstyle;
	// ���سɹ�ͬ����־
	bool m_isWebLoadFinished;
	// web channel��ʼ���ɹ���־
	bool m_isWebChannelInit;
	
	// ��ǰչʾ��Ա��id
	ID m_Chattoid;

	// �����϶����id
	ID m_openuserinfo;
	QPoint m_infop;
	// ����-˫�����¼�����
	int m_clicktimer;

	QString m_hightlight;

	// һ���깺��ѯ
	int m_Purchase;
	CPurchaseUkeyList m_Purchaselst;

	// ��ѯ����״̬
	int m_MMStatus;
	CQuoteStateList m_MMStatusList;
	int m_QuoteBond;
	CQuoteStateList m_QuoteBondList;
	WebViewChannel *m_channel;
	WebPage *m_page;
	QList<QPair<QString, JsResponseCb>> m_loadFinishRunList;

// ��jsͨ�ŵĻص�����
public slots: 
	// ������
	void slotopenurl(const QString &val);
	// ��ȡ��Ϣģ��
	QString slotgetHtmlContent(const QString &val);
	// �鿴����
	void slotsearchmore();
	// �鿴���࣬����Ϣ��¼
	void slotopenRecordtoSearchMore();
	// �����ļ�
	void slotfileaccept(const QString &caller, const QString &msgid);
	// ����ļ�
	void slotfileSave(const QString &caller, const QString &msgid);
	// �ܾ������ļ�
	void slotfilereject(const QString &caller, const QString &msgid);
	// ����ת���߷���
	void slotOfflineSend(const QString &caller, const QString &msgid);
	// ˫����ͼƬ
	void slotimageDbclickShow(const QString &val);
	// ���·���
	void slotresendmsg(const QString &val);
	// ������
	void slotOpenInfoWnd(const QString &val);
	// ������˵����촰��
	void slotOpenChatto(const QString &val);
	// ���ļ���
	void slotOpenFilepath(const QString &val);
	// ���ļ�
	void slotOpenFile(const QString &val);
	// ��ʷ��Ϣ��¼��ҳ 0:�ϣ�1:��
	void slothistoryPage(const QString &val);

	// ���´��ı���Ϣ����
	void slotUpdateBigTxtMsgShow(const QString &val);

	// Ⱥ����Աչʾ
	void slotShowGroupMassReceivers(const QString &val);

	// ��ʼ�϶�
	void slotStartDrag(const QString &type, const QString &val);

	// ���ծȯ����  QB ��ϵ���۷�
	void slotCreditoright(const QString &val);

	// ���Ⱥ��Ƭ
	void slotRoomcard(const QString &caller, const QString &val);

	// ���ծȯ���룬����
	void slotBondOpt(const QString &val);

	// ����
	void slotSubscribe(const QString &caller, const QString &val);

	// �ض�div ����ΪͼƬ
	void slotGrabPixmap(const QString &val);

	void slotUpHistory();
	void slotDownHistory();
	void slotUpdateSetting();

	//////////////////////////////////////////////////////////////////////////
	// ��ʾ�����ѽ���
	//void slotFloatHandle(const QString &val);
protected slots:
	// ���سɹ� bResult: 0 success, or fail
	virtual void slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult);
	// ���ؽ���
	virtual void slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize);
    // ͼƬ(GIF)������ɣ�����Ϊ��ͼ
    void updateGifBriefImage(const CPicDownLoadNty &val);
    // ͼƬ(GIF)������ɣ�����Ϊ��ͼ
    void m_updateGifBriefImage(
        const ID &chatId,
        const QString &thumbnailPicUrl,
        const QString &origImageLocalPath,
        bool isDownloadOkay);

private slots:
	// ���سɹ�
	void finish(bool bsuccess);
	// ���ؽ���
	void slotLoadProgress(int progress);
	// ������Ӧ
	void hyperlinkClicked(const QUrl &);
	// ����ͼƬ
	void slotWebviewImageReplaceNty(const QString &briefSrc, const QString &oldSrc);
	// �Ҽ��˵�
	void slotContextMenu(const QVariant &val);
	// finishe��ʼ��
	virtual void finishInit();
	// ����web����
	virtual void slotHandle(const QString &caller, const QString &val);

public:
	// ������ҳ
	void loadUrl(const QString &url = "");

	// �����û�id
	void setUserid(const ID& id);
	ID getUserid();

	// �����Ϣ
	void appendMessage(const CMessage &val /* ��Ϣ���� */);
	// �����������Ϣ
	void prependMessage(const CMessageList &vallst);
	// �����������Ϣ �л�ʱ��������
	void appendMessage(/*const CMessageList &oldvallst,*//*����Ϣ����*/
					   const CMessageList &vallst, /*����Ϣ����*/
					   //0:�޲鿴��1���鿴���ࣻ2���鿴�������Ϣ��¼
					   int nScrollPos = -1);
	// �����Ϣ
	void appendHistoryMessage(const CMessageList &val /* ��Ϣ���� */,
							  const QString &hilight = ""/* �����ֶ� */,
							  const ULONG64& locateId = 0/* ��λid*/);
	// ���ҳ��
	void clearhtml();
	// ɾ��ҳ��ǰn����Ϣ
	void delHtmlmsgFromMsgid(const CMessage &val);
	// ���ù�����λ��
	void setScrollPosition(int nIndex);
	// �������ײ�
	void ScrollToButtom();
	// ��ȡ������λ��
	int getScrollPosition();
	// ��ʾ�鿴����
	void showSearchMore();
	// ��ʾ�鿴���࣬���Ҳ������¼
	void openRecordtoSearchMore(bool bshow = true);
	// ��ʾ�ָ��� �׶�  --- ����Ϊ����Ϣ
	void showSpilitline(const QString &locateId = "");
	// ���µ�ǰҳ�����Ϣid
	void updateCurPageMsgid(ULONG64 msgid, const STRINGUTF8 &uuid, int errorCode);

	// �ļ��������֪ͨ
	void UpdateFileSendProgress(const CFileSendProgress &fp);
	// �ļ���Ϣ״̬���֪ͨ
	void FileSendStatusChangeJS(const CFileSendStatusChange &fnty, bool bSend = true);

	// Ⱥ�ļ��ϴ��ɹ�
	void RoomFileUploadNotify(const CRoomFileUploadNotify &p);
	// Ⱥ�ļ��ϴ�����
	void RoomFileSendProgress(const CRoomFileSendProgress &p);


	// ϵͳ��ʾ
	void systemtips(Systip_e etype, const QVariantMap &valtip);

	// ���ػظ����۵�ҳ��
	void loadInteractionPage(const QVariantMap &val);

	// ���´��ı���Ϣ
	void updateBigTxtmsg(const CMessage &val);

	//�жϹ������Ƿ��ڵײ�
	void notScrollBottom(const std::function<void(bool ok)> &callback);

	// ϵͳ���� - ģ��չʾ
	void SetSettingMgr();

	// ��λ��Ϣ
	void LocateMessage(const QVariant& msgid);

	// ������ѯ�깺��״̬����
	// ȷ���깺�����깺��״̬
	// �깺״̬�ı�����
	void PurchaseStatus(const CPurchaseUkey &purchaseKey, int nStatus);

	// ������Ϣչʾ
	void UpdateMessage(const CMessage &msg);

	//��ʾϵͳ��ʾ
	void ShowSystemTips(const QVariantMap &msg);

	bool isInit() const;

protected:
	 void keyPressEvent(QKeyEvent * ev);
	 void timerEvent(QTimerEvent *event); 
	 void contextMenuEvent(QContextMenuEvent *event);
	 void dragEnterEvent(QDragEnterEvent *event);
	 void dropEvent(QDropEvent * event);

signals:
	// �鿴����
	void s_searchmore();
	// �鿴���࣬���Ҳ������¼
	void s_openRecordtoSearchMore();
	// ���·���
	void s_resendmsg(const QString &val);
	// �����ҳ����
	void s_ClearHtml();
	// ���ù�����λ��
	void s_setScrollPos();
	// ��ʷ��Ϣ��¼�������·�ҳ
	void s_historyPage(bool val);
	// �����ļ�·������
	void s_FileSavepath(ULONG64 msgid, const QString &filepath, bool bOperate);
	// �ܾ�/ȡ���ļ�����
	void s_FileRejectCancel(ULONG64 msgid, bool bOperate);
	// ������ʱ
	void s_OperateTimeout();
	// ��ȡ���ı���Ϣ
	void s_getBigTxtmsg(const QString &val);
	//���ձ����������Ҫͬ���޸�
	void s_ChangeInputFontWeight(double val);
	// һ���깺ȷ����
	void s_PurchaseConfirm(ULONG64 msgid);

	//������Ϣ
	void s_UpdateMessage(const CMessage &msg);
	// �鿴������
	void s_OpenContext(const ID& userid, ULONG64 msgid);

	void s_downToHistory();
	void s_upToHistory();
	void s_init();

private:
	// ��ʼ���ı�
	void initTextSrc();

	// ��ʼ��ģ��
	void initLables();

	// ��ʼ���������
	void initErrorCode();

	// ��Ϣ����
	QString analyzeMessage(const CMessage &val, const QString &hilight = "", bool btxtfull = false);
	QVariantMap analyzeMessagetoVmap(const CMessage &val, bool btxtfull = false,const QString highlight="");
	QVariant analyzeMessageBodyList(const CMessage &val, QVariantMap &msgmap, bool btxtfull = false);
	QVariant analyzeMessageContent(const CMessage &val, int nIndex = 0);

	// ��ȡ�ļ�icon
	QString getFileIcon(const QString &filep, const std::string &ficon = "");

	// ��ȡ��html������ͼƬ��Դ��·��
	QString getImagepFromHtml(const QString &valp);

	// ���ֽض�չʾ
	//QString cutShowText(const QString &val, int length = 0/*�������س���*/, int fontsize = 12/*�������ش�С*/, Qt::TextElideMode qmode = Qt::ElideRight/*�ضϷ�ʽ*/ );
	
	// ��ȡ��Ϣ���� ��������Ⱥ����֪ͨ�ȴ����ͣ�
	QString getMessageType(EMSGType t);

	// ��ȡ�ļ�״̬
	QString getFileSendStatus(EFileSendStatus status);
	QString getFileRecvStatus(EFileRecvStatus status);

	// ��ȡ��ǰҳ�������
	QString getPageType(VIEWSTYLE_e eType);

	// ��ȡMENU�˵�������
	RightClickMenuType getMenuType();

	// ����չʾ����
	void setShowPercent(UINT32 sp);

	// from sumscope
	void fromSumscope();

	// Ⱥ���� ��Ϣ����Ա��Ϣչʾ
	void getMassUsers(const std::string &val, QVariantMap &contactsMap);

	// test ����Сת
	//QVariantMap testRobotMsg(const CMessage & msg, bool b);
};

#endif // WEBVIEW_H
