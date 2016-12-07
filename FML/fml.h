#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_fml.h"

class FML : public QMainWindow
{
    Q_OBJECT

public:
    FML(QWidget *parent = Q_NULLPTR);

private:
    Ui::FMLClass ui;
};
