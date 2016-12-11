#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>

class TitleWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TitleWidget(QWidget *parent=NULL);
	~TitleWidget();

	void init(const QPixmap &logoPixmap, const QString &title, bool isCenter = false);
private:
	QLabel *m_logo;
	QLabel *m_title;
};

