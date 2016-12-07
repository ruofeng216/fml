#pragma once

#include <QObject>


class QControllerManager : public QObject
{
	Q_OBJECT

public:
	QControllerManager(QObject *parent);
	~QControllerManager();
};
