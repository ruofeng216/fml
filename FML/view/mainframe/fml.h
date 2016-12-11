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
	
private slots:
	void funcclick();

private:
	void showDetialFunc(bool bShow = true);

private:
    Ui::FMLClass ui;
	bool m_showFunc;
};
