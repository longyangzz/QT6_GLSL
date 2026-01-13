#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_BCGP.h"


//Qt
#include "QMdiSubWindow"

//DCGUI
#include "DCGui/AuxMainWindow.h"

class QMdiArea;
#include <QWidget>
class BCGP : public DCGui::AuxMainWindow
{
    Q_OBJECT

public:
    BCGP(QWidget *parent = nullptr);
    ~BCGP();

    void Init();

private:
    Ui::BCGPClass ui;

    QMdiArea* m_pMdiArea = nullptr;
};

