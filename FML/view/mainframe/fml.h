#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_fml.h"
#include "util/datatype.h"
#include "viewsignalmanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>

class FML : public QMainWindow
{
    Q_OBJECT

public:
    FML(QWidget *parent = Q_NULLPTR);
	void login(const CMyBasePtr val);
private:
    Ui::FMLClass ui;
};

class Worker : public QThread
{
	Q_OBJECT
public:
	Worker(QObject *p = NULL) :QThread(p){
	}
	~Worker(){}
	void run()
	{
		qDebug() << "Worker::onTimeout get called from?: " << QThread::currentThreadId();
		emit VIEWSIGNAL->callBackUI(CMyBasePtr(new CLogin("aaa", "123456")));
		quit();
	}
};