#ifndef POINTCLOUDWIDGET_H
#define POINTCLOUDWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <limits>

class PointCloudWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit PointCloudWidget(QWidget* parent = nullptr);
    ~PointCloudWidget() override;

    void loadPointCloud(const QString& filename);
    void setRenderMode(int mode); // 0: elevation, 1: RGB
    void resetView();
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void updateCamera();

    QOpenGLShaderProgram m_program;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    std::vector<float> m_points; // [x, y, z, r, g, b]
    float m_minZ = 0.0f, m_maxZ = 1.0f;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;

    QPoint m_lastMousePos;
    float m_yaw = -90.0f;
    float m_pitch = -20.0f;
    float m_distance = 15.0f;

    int m_renderMode = 0; // 0: elevation, 1: RGB

    QVector3D m_bboxMin;   // 真实坐标系下的最小点
    QVector3D m_bboxMax;   // 真实坐标系下的最大点
    QVector3D m_center;    // 包围盒中心
    float m_sceneRadius;   // 场景半径（用于设置相机距离）
};

#endif // POINTCLOUDWIDGET_H
