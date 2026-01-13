#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_BCGP.h"


//Qt
#include "QMdiSubWindow"

//DCGUI
#include "DCGui/AuxMainWindow.h"
#include "GLSLViewer/GLSLViewer.h"
class MdiArea;
class GLSLViewer;
#include <QWidget>
class BCGP : public DCGui::AuxMainWindow
{
    Q_OBJECT

public:
    BCGP(QWidget *parent = nullptr);
    ~BCGP();

    void Init();

    static BCGP* Instance();

    static void UnInitialize();

    //!--------------------------数据文件加载----------------------------------
    int LoadFile(const QString& fileName, GLSLViewer* viewer);
    int AddFile(GLSLViewer* viewer, const QString& fileName);

    //! 打开工程
    void OpenProject(QString proName);
    QWidget* ActiveMdiChild();
private:
    Ui::BCGPClass ui;

    MdiArea* m_pMdiArea = nullptr;
};

