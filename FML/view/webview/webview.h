#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineView>
#include <QHash>
#include "qutil.h"
#include "right_click_menu.h"

// 网页展示方式
class CMessage;
typedef enum VIEWSTYLE_e  
{
	// 默认普通网页展示
	VIEW_DEFAULT = 0,
	//当前消息展示
	VIEW_CURPAGE,
	// 本地消息记录展示 单体
	VIEW_RECORD,
	// 消息记录 全局查找页面
	VIEW_GRECORD,
	// 回复报价等 内嵌本地交互页面
	VIEW_INTERACTION,
	//系统消息提示
	VIEW_SYS_TIPS,
	// 群文件
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

	// 包装runJavaScript，方便管理
	void runjs(const QString &js, const JsResponseCb &cb = nullptr);

private:
	// 展示方式
	VIEWSTYLE_e m_viewstyle;
	// 加载成功同步标志
	bool m_isWebLoadFinished;
	// web channel初始化成功标志
	bool m_isWebChannelInit;
	
	// 当前展示人员的id
	ID m_Chattoid;

	// 打开资料对象的id
	ID m_openuserinfo;
	QPoint m_infop;
	// 单击-双击的事件控制
	int m_clicktimer;

	QString m_hightlight;

	// 一级申购查询
	int m_Purchase;
	CPurchaseUkeyList m_Purchaselst;

	// 查询报价状态
	int m_MMStatus;
	CQuoteStateList m_MMStatusList;
	int m_QuoteBond;
	CQuoteStateList m_QuoteBondList;
	WebViewChannel *m_channel;
	WebPage *m_page;
	QList<QPair<QString, JsResponseCb>> m_loadFinishRunList;

// 与js通信的回调函数
public slots: 
	// 打开链接
	void slotopenurl(const QString &val);
	// 获取消息模板
	QString slotgetHtmlContent(const QString &val);
	// 查看更多
	void slotsearchmore();
	// 查看更多，打开消息记录
	void slotopenRecordtoSearchMore();
	// 接收文件
	void slotfileaccept(const QString &caller, const QString &msgid);
	// 另存文件
	void slotfileSave(const QString &caller, const QString &msgid);
	// 拒绝接收文件
	void slotfilereject(const QString &caller, const QString &msgid);
	// 在线转离线发送
	void slotOfflineSend(const QString &caller, const QString &msgid);
	// 双击打开图片
	void slotimageDbclickShow(const QString &val);
	// 重新发送
	void slotresendmsg(const QString &val);
	// 打开资料
	void slotOpenInfoWnd(const QString &val);
	// 打开与此人的聊天窗口
	void slotOpenChatto(const QString &val);
	// 打开文件夹
	void slotOpenFilepath(const QString &val);
	// 打开文件
	void slotOpenFile(const QString &val);
	// 历史消息记录翻页 0:上；1:下
	void slothistoryPage(const QString &val);

	// 更新大文本消息内容
	void slotUpdateBigTxtMsgShow(const QString &val);

	// 群发成员展示
	void slotShowGroupMassReceivers(const QString &val);

	// 开始拖动
	void slotStartDrag(const QString &type, const QString &val);

	// 点击债券代码  QB 联系报价方
	void slotCreditoright(const QString &val);

	// 点击群名片
	void slotRoomcard(const QString &caller, const QString &val);

	// 点击债券代码，交互
	void slotBondOpt(const QString &val);

	// 订阅
	void slotSubscribe(const QString &caller, const QString &val);

	// 特定div 保存为图片
	void slotGrabPixmap(const QString &val);

	void slotUpHistory();
	void slotDownHistory();
	void slotUpdateSetting();

	//////////////////////////////////////////////////////////////////////////
	// 提示窗提醒交互
	//void slotFloatHandle(const QString &val);
protected slots:
	// 下载成功 bResult: 0 success, or fail
	virtual void slotdownloadresult(const std::string fuuid, const std::string localPath, int nResult);
	// 下载进度
	virtual void slotdownloadprogress(const std::string fuuid, qint64 donesize, qint64 totalsize);
    // 图片(GIF)下载完成，更新为动图
    void updateGifBriefImage(const CPicDownLoadNty &val);
    // 图片(GIF)下载完成，更新为动图
    void m_updateGifBriefImage(
        const ID &chatId,
        const QString &thumbnailPicUrl,
        const QString &origImageLocalPath,
        bool isDownloadOkay);

private slots:
	// 加载成功
	void finish(bool bsuccess);
	// 加载进度
	void slotLoadProgress(int progress);
	// 链接响应
	void hyperlinkClicked(const QUrl &);
	// 更新图片
	void slotWebviewImageReplaceNty(const QString &briefSrc, const QString &oldSrc);
	// 右键菜单
	void slotContextMenu(const QVariant &val);
	// finishe初始化
	virtual void finishInit();
	// 处理web调用
	virtual void slotHandle(const QString &caller, const QString &val);

public:
	// 加载网页
	void loadUrl(const QString &url = "");

	// 设置用户id
	void setUserid(const ID& id);
	ID getUserid();

	// 添加消息
	void appendMessage(const CMessage &val /* 消息主体 */);
	// 从上面插入消息
	void prependMessage(const CMessageList &vallst);
	// 从下面插入消息 切换时批量插入
	void appendMessage(/*const CMessageList &oldvallst,*//*旧消息队列*/
					   const CMessageList &vallst, /*新消息队列*/
					   //0:无查看；1：查看更多；2：查看更多打开消息记录
					   int nScrollPos = -1);
	// 添加消息
	void appendHistoryMessage(const CMessageList &val /* 消息主体 */,
							  const QString &hilight = ""/* 高亮字段 */,
							  const ULONG64& locateId = 0/* 定位id*/);
	// 清除页面
	void clearhtml();
	// 删除页面前n条消息
	void delHtmlmsgFromMsgid(const CMessage &val);
	// 设置滚动条位置
	void setScrollPosition(int nIndex);
	// 滚动到底部
	void ScrollToButtom();
	// 获取滚动条位置
	int getScrollPosition();
	// 显示查看更多
	void showSearchMore();
	// 显示查看更多，打开右侧聊天记录
	void openRecordtoSearchMore(bool bshow = true);
	// 显示分割线 底端  --- 以上为新消息
	void showSpilitline(const QString &locateId = "");
	// 更新当前页面的消息id
	void updateCurPageMsgid(ULONG64 msgid, const STRINGUTF8 &uuid, int errorCode);

	// 文件传输进度通知
	void UpdateFileSendProgress(const CFileSendProgress &fp);
	// 文件消息状态变更通知
	void FileSendStatusChangeJS(const CFileSendStatusChange &fnty, bool bSend = true);

	// 群文件上传成功
	void RoomFileUploadNotify(const CRoomFileUploadNotify &p);
	// 群文件上传进度
	void RoomFileSendProgress(const CRoomFileSendProgress &p);


	// 系统提示
	void systemtips(Systip_e etype, const QVariantMap &valtip);

	// 加载回复报价等页面
	void loadInteractionPage(const QVariantMap &val);

	// 更新大文本消息
	void updateBigTxtmsg(const CMessage &val);

	//判断滚动条是否在底部
	void notScrollBottom(const std::function<void(bool ok)> &callback);

	// 系统设置 - 模板展示
	void SetSettingMgr();

	// 定位消息
	void LocateMessage(const QVariant& msgid);

	// 批量查询申购的状态返回
	// 确认申购返回申购的状态
	// 申购状态改变推送
	void PurchaseStatus(const CPurchaseUkey &purchaseKey, int nStatus);

	// 更新消息展示
	void UpdateMessage(const CMessage &msg);

	//显示系统提示
	void ShowSystemTips(const QVariantMap &msg);

	bool isInit() const;

protected:
	 void keyPressEvent(QKeyEvent * ev);
	 void timerEvent(QTimerEvent *event); 
	 void contextMenuEvent(QContextMenuEvent *event);
	 void dragEnterEvent(QDragEnterEvent *event);
	 void dropEvent(QDropEvent * event);

signals:
	// 查看更多
	void s_searchmore();
	// 查看更多，打开右侧聊天记录
	void s_openRecordtoSearchMore();
	// 重新发送
	void s_resendmsg(const QString &val);
	// 清除网页数据
	void s_ClearHtml();
	// 设置滚动条位置
	void s_setScrollPos();
	// 历史消息记录向上向下翻页
	void s_historyPage(bool val);
	// 接收文件路径回推
	void s_FileSavepath(ULONG64 msgid, const QString &filepath, bool bOperate);
	// 拒绝/取消文件操作
	void s_FileRejectCancel(ULONG64 msgid, bool bOperate);
	// 操作延时
	void s_OperateTimeout();
	// 获取大文本消息
	void s_getBigTxtmsg(const QString &val);
	//按照比例输入框需要同比修改
	void s_ChangeInputFontWeight(double val);
	// 一级申购确认中
	void s_PurchaseConfirm(ULONG64 msgid);

	//更新消息
	void s_UpdateMessage(const CMessage &msg);
	// 查看上下文
	void s_OpenContext(const ID& userid, ULONG64 msgid);

	void s_downToHistory();
	void s_upToHistory();
	void s_init();

private:
	// 初始化文本
	void initTextSrc();

	// 初始化模板
	void initLables();

	// 初始化错误代码
	void initErrorCode();

	// 消息解析
	QString analyzeMessage(const CMessage &val, const QString &hilight = "", bool btxtfull = false);
	QVariantMap analyzeMessagetoVmap(const CMessage &val, bool btxtfull = false,const QString highlight="");
	QVariant analyzeMessageBodyList(const CMessage &val, QVariantMap &msgmap, bool btxtfull = false);
	QVariant analyzeMessageContent(const CMessage &val, int nIndex = 0);

	// 获取文件icon
	QString getFileIcon(const QString &filep, const std::string &ficon = "");

	// 获取从html过来的图片资源的路径
	QString getImagepFromHtml(const QString &valp);

	// 文字截断展示
	//QString cutShowText(const QString &val, int length = 0/*文字像素长度*/, int fontsize = 12/*字体像素大小*/, Qt::TextElideMode qmode = Qt::ElideRight/*截断方式*/ );
	
	// 获取消息类型 （单发，群发，通知等大类型）
	QString getMessageType(EMSGType t);

	// 获取文件状态
	QString getFileSendStatus(EFileSendStatus status);
	QString getFileRecvStatus(EFileRecvStatus status);

	// 获取当前页面的类型
	QString getPageType(VIEWSTYLE_e eType);

	// 获取MENU菜单的类型
	RightClickMenuType getMenuType();

	// 设置展示比例
	void setShowPercent(UINT32 sp);

	// from sumscope
	void fromSumscope();

	// 群发等 消息中人员信息展示
	void getMassUsers(const std::string &val, QVariantMap &contactsMap);

	// test 机器小转
	//QVariantMap testRobotMsg(const CMessage & msg, bool b);
};

#endif // WEBVIEW_H
