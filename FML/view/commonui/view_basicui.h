#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QPixmap>
// �߽�
#define PADDING 2
#define PADDING2 15

namespace Ui {
	class basic;
}
class TitleWidget;
class basicui : public QWidget
{
	Q_OBJECT

public:
	enum TitleStyle 
	{
		TS_NONE = 0,		// �ޱ���
		TS_CLOSE = 1<<0,	// �ر�
		TS_MAX =1<<1 ,		// ���,˫��������󻯣�����ʾ��󻯰�ť��
		TS_MIN = 1<<2,		// ��С��
		TS_FUNC = 1<<3,		// ����
		TS_LEFT =1<<4,		// ���������
		TS_CENTER= 1<<5,	// �������
		TS_LOGO =1<<6,		// LOGO
	};

	explicit basicui(QWidget *parent, QWidget *contentWidget, const QString &wndid, 
		const QString &title, int titlestyle = TS_LOGO|TS_MAX|TS_CLOSE|TS_MIN|TS_LEFT);

	virtual ~basicui();

	QString id() const;									// Ψһ��ʾ���ڵ�ID
	void setTitleStyle(int titlestyle);					// ���ñ�����ʽ
	void setCloseIsHide(bool isHide);					// ����رհ�ť�������ش���
	void setForbidMove(bool forbidMove);				// ��ֹ�����ƶ�
	void setBackground(const QPixmap &bg);
	QWidget *getContentWidget() const;
	void setSize(QSize size);
	void setFrameHighlight(bool isHighlight);			// ���ñ�������
	void forceEnableStretch();							// ǿ���������죬����UI��ťӰ��

signals:
	void sigClose(const QString &id="", bool isExit=true);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void closeEvent(QCloseEvent *event);
	bool nativeEvent(const QByteArray & eventType, void * message, long * result);

	void setMaxRestoreVisible();

public Q_SLOTS:
	bool close();
	void min();
	virtual void max();

protected:
	// ������ڴ��������ϣ��£����ң����ϣ����£����£����ϣ��м�
	enum Direction { UP=0, DOWN, LEFT, RIGHT, LEFTTOP, LEFTBOTTOM, RIGHTBOTTOM, RIGHTTOP, MIDDLE };
	// ���Ŀǰ��λ��ת����Ӧ������������
	void transRegion(const QPoint &cursorGlobalPoint);

	bool m_blpressdown; //��갴��
	
private:
	Ui::basic *m_ui;
	QString m_wndid;
	QPoint m_movepoint; //�ƶ��ľ��� 
	Direction m_dir;    //�����������
	int m_titlestyle;   //���⹦�ܰ�ť����
	bool m_closeIsHide;	//����رհ�ť���ش���
	bool m_forbidMove;  //��ֹ�����ƶ�
	QWidget *m_contentWidget; // ����������
	QPixmap m_bgImage; //����ͼƬ
	QString m_title; //��������
};
