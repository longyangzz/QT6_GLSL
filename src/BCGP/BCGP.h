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

    GLSLViewer* CurrentDCViewer();
private slots:
    //! 导入数据，创建新窗口
    void ImportData();

    //! 导入数据到指定的窗口中
    //void ImportDataToView();

    //! 改变当前窗体（MDI通知主窗口）
    void ChangedCurrentViewer(QMdiSubWindow* subWindow);
private:
    Ui::BCGPClass ui;

    MdiArea* m_pMdiArea = nullptr;
};

