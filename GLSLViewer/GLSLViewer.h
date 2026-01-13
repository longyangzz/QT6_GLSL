#pragma once

#include "glslviewer_global.h"

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
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class GLSLVIEWER_EXPORT GLSLViewer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit GLSLViewer(QWidget* parent = nullptr);
    ~GLSLViewer() override;

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

    void paintEvent(QPaintEvent* event) override;
private:
    void updateCamera();

    int m_glWidth;
    int m_glHeight;

    //点云数据
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;

    void initPointCloud();
    void renderPointCloud();

    //相机场景相关
    std::vector<float> m_points; // [x, y, z, r, g, b]
    float m_minZ = 0.0f, m_maxZ = 1.0f;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;

    QPoint m_lastMousePos;
    float m_yaw = -90.0f;
    float m_pitch = -20.0f;
    float m_distance = 15.0f;
    float m_logDistance;

    int m_renderMode = 0; // 0: elevation, 1: RGB

    QVector3D m_bboxMin;   // 真实坐标系下的最小点
    QVector3D m_bboxMax;   // 真实坐标系下的最大点
    QVector3D m_center;    // 包围盒中心
    QVector3D m_bboxSize;
    float m_sceneRadius = 0.0f;   // 场景半径（用于设置相机距离）

    bool m_showColorBar = false; // 是否显示颜色条

    // 坐标轴相关
    QOpenGLShaderProgram* m_axisShader = nullptr;
    QOpenGLBuffer m_axisVbo;
    QOpenGLVertexArrayObject m_axisVao;
    bool m_axisInitialized = false;
    // 可选：缓存轴长（像素单位）
    float m_axisLength = 40.0f; // 像素

    //绘制方法一
    void initScreenAxis();
    void renderScreenAxis();
    void renderCornerAxisLabels(const QMatrix4x4& rotation);

    //绘制方法二
    void initScreenAxisOrtho();
    void renderScreenAxisOrtho();

    // 边界盒相关
    QOpenGLShaderProgram* m_boxShader;
    QOpenGLBuffer m_boxVbo;
    QOpenGLVertexArrayObject m_boxVao;
    bool m_boxInitialized;
    std::vector<float> m_boxVertices; // 存储边界盒顶点数据
    std::vector<unsigned int> m_boxIndices; // 存储边界盒索引数据

    void initBoundingBoxGeometry();
    void renderBoundingBox();
    void updateBoundingBoxGeometry();
};