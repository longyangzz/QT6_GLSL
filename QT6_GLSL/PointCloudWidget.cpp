#include "PointCloudWidget.h"
#include <QDir>
#include <QDebug>

PointCloudWidget::PointCloudWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_renderMode(0)
    , m_distance(1.0f)
    , m_sceneRadius(0.0f)
{
    setFocusPolicy(Qt::StrongFocus);
}

PointCloudWidget::~PointCloudWidget()
{
    makeCurrent();
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    doneCurrent();
}

void PointCloudWidget::loadPointCloud(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file:" << filename;
        return;
    }

    // 临时存储原始点（x, y, z）和颜色
    std::vector<float> originalPoints; // [x, y, z]
    std::vector<float> colors;         // [r, g, b]

    m_bboxMin = QVector3D(std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());
    m_bboxMax = QVector3D(std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest());

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 3) continue;

        bool ok;
        float x = parts[0].toFloat(&ok); if (!ok) continue;
        float y = parts[1].toFloat(&ok); if (!ok) continue;
        float z = parts[2].toFloat(&ok); if (!ok) continue;

        // 更新包围盒（使用原始坐标！）
        m_bboxMin.setX(qMin(m_bboxMin.x(), x));
        m_bboxMin.setY(qMin(m_bboxMin.y(), y));
        m_bboxMin.setZ(qMin(m_bboxMin.z(), z));

        m_bboxMax.setX(qMax(m_bboxMax.x(), x));
        m_bboxMax.setY(qMax(m_bboxMax.y(), y));
        m_bboxMax.setZ(qMax(m_bboxMax.z(), z));

        originalPoints.insert(originalPoints.end(), { x, y, z });

        float r = 1.0f, g = 1.0f, b = 1.0f;
        if (parts.size() >= 6) {
            r = qBound(0.0f, parts[3].toFloat() / 255.0f, 1.0f);
            g = qBound(0.0f, parts[4].toFloat() / 255.0f, 1.0f);
            b = qBound(0.0f, parts[5].toFloat() / 255.0f, 1.0f);
        }
        colors.insert(colors.end(), { r, g, b });
    }

    if (originalPoints.empty()) {
        qWarning() << "No valid points loaded.";
        return;
    }

    // === 计算场景中心和半径 ===
    m_center = (m_bboxMin + m_bboxMax) * 0.5f;
    QVector3D diagonal = m_bboxMax - m_bboxMin;
    m_sceneRadius = 0.5f * diagonal.length();

    // 避免除零
    if (m_sceneRadius < 1e-6f) m_sceneRadius = 1.0f;

    // === 构建最终的 m_points: [x, y, z_norm, r, g, b] ===
    // 注意：z_norm 是归一化的 z（用于高程色），但 x,y,z 渲染仍用原始值！
    // 所以我们只替换 z 分量用于 color 通道，顶点位置仍用原始 xyz
    m_points.clear();
    float zRange = m_bboxMax.z() - m_bboxMin.z();
    if (zRange < 1e-6f) zRange = 1.0f;

    for (size_t i = 0; i < originalPoints.size(); i += 3) {
        float x = originalPoints[i];
        float y = originalPoints[i + 1];
        float z = originalPoints[i + 2];

        float r = colors[i];
        float g = colors[i + 1];
        float b = colors[i + 2];

        // 归一化 z 用于高程色（存入 color 的 r/g/b 不影响，我们用 aPos.z）
        // 但注意：顶点位置仍然是原始 x,y,z！
        float zNorm = (z - m_bboxMin.z()) / zRange;

        // 我们仍然上传 [x, y, z, r, g, b] 到 GPU
        // —— 顶点着色器中 aPos.z 就是原始 z，但我们希望高程色用归一化值？
        // 方案：要么传 zNorm 作为额外 attribute，要么在 CPU 归一化 z 后上传
        // 这里为了简单，**上传归一化后的 z 作为顶点 z**（仅影响高程色，不影响几何？）
        // ❌ 错误！会影响几何位置！

        // ✅ 正确做法：顶点位置保持原始 xyz，额外传一个 normalizedZ 作为 color 或 attribute
        // 但为简化，我们改用：在顶点着色器中用 uniform 传 minZ/maxZ 来归一化
        // → 更好！无需修改顶点数据

        // 因此，这里我们**不修改 z**，保持原始几何
        m_points.insert(m_points.end(), { x, y, z, r, g, b });
    }

    // === 重新初始化包围盒相关的 uniform（可选）===
    // 我们将在顶点着色器中用 uniform 传递 minZ/maxZ

    // === 上传到 GPU ===
    if (isValid()) {
        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float), m_points.data(), GL_STATIC_DRAW);
        doneCurrent();
    }

    // === 自动重置视图 ===
    resetView();

    update();
}

// PointCloudWidget.cpp
void PointCloudWidget::resetView()
{
    if (m_sceneRadius <= 0) return;

    // 相机看向中心点
    m_yaw = -90.0f;   // 从 +X 方向看（常见于点云）
    m_pitch = -20.0f; // 稍微俯视

    // 相机距离：根据场景大小自动调整
    m_distance = m_sceneRadius * 2.5f; // 可调系数（2~3 倍半径通常合适）

    updateCamera();
}



void PointCloudWidget::setRenderMode(int mode)
{
    m_renderMode = mode;
    update();
}

void PointCloudWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 编译着色器（从文件）
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shaders/pointcloud.vert")) {
        qCritical() << "Failed to compile vertex shader";
        return;
    }
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaders/pointcloud.frag")) {
        qCritical() << "Failed to compile fragment shader";
        return;
    }
    if (!m_program.link()) {
        qCritical() << "Failed to link shader program";
        return;
    }

    // Create VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    if (!m_points.empty()) {
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float), m_points.data(), GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PointCloudWidget::resizeGL(int w, int h)
{
    m_projection.setToIdentity();
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    m_projection.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void PointCloudWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_points.empty()) return;

    m_program.bind();
    m_program.setUniformValue("uProjection", m_projection);
    m_program.setUniformValue("uView", m_view);
    m_program.setUniformValue("uRenderMode", m_renderMode);
    m_program.setUniformValue("uMinZ", m_bboxMin.z());
    m_program.setUniformValue("uMaxZ", m_bboxMax.z());

    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_points.size() / 6));
    glBindVertexArray(0);

    m_program.release();
}

void PointCloudWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
}

void PointCloudWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        float dx = event->pos().x() - m_lastMousePos.x();
        float dy = event->pos().y() - m_lastMousePos.y();
        m_yaw += dx * 0.3f;
        m_pitch = qBound(-89.0f, m_pitch - dy * 0.3f, 89.0f);
        updateCamera();
        update();
    }
    m_lastMousePos = event->pos();
}

void PointCloudWidget::wheelEvent(QWheelEvent* event)
{
    m_distance *= (event->angleDelta().y() > 0) ? 0.9f : 1.1f;
    m_distance = qBound(1.0f, m_distance, 100.0f);
    updateCamera();
    update();
}

void PointCloudWidget::updateCamera()
{
    // 相机位置：从中心点出发，沿球坐标方向后退 m_distance
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    float x = m_center.x() + m_distance * cos(pitchRad) * cos(yawRad);
    float y = m_center.y() + m_distance * sin(pitchRad);
    float z = m_center.z() + m_distance * cos(pitchRad) * sin(yawRad);

    QVector3D cameraPos(x, y, z);
    QVector3D target = m_center;
    QVector3D up(0, 1, 0);

    m_view.setToIdentity();
    m_view.lookAt(cameraPos, target, up);
}
