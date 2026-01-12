#include <QApplication>
#include "PointCloudWidget.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    PointCloudWidget widget;
    widget.resize(1024, 768);
    widget.setWindowTitle("LiDAR Point Cloud Viewer - Qt6 + OpenGL");
    widget.show();

    // 可选：自动加载示例数据（需在 build 目录有 data/sample.txt）
    widget.loadPointCloud(QString::fromLocal8Bit("D:\\data-example\\point-inter\\矿体数据补洞.txt"));

    return app.exec();
}
