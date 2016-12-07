#pragma once

#include <QObject>
#include "util\datatype.h"

#define VIEWSIGNAL ViewSignalManager::instance()

class ViewSignalManager : public QObject
{
	Q_OBJECT

public:
	ViewSignalManager(QObject *parent = NULL);
	~ViewSignalManager();

	static ViewSignalManager* instance();

signals:
	void login(const CMyBasePtr val);
};
