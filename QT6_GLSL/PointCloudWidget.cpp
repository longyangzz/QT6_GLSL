#include "PointCloudWidget.h"
#include <QDir>
#include <QDebug>
#include <QPainter>
#include <QLinearGradient>
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
    m_bboxSize = m_bboxMax - m_bboxMin;
    m_sceneRadius = 0.5f * m_bboxSize.length();

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

    m_showColorBar = (m_renderMode == 0); // 高程色模式才显示
}

void PointCloudWidget::paintEvent(QPaintEvent* event)
{
    // 先调用 QOpenGLWidget 的 paintEvent（会触发 paintGL）
    QOpenGLWidget::paintEvent(event);

    // 如果不是高程色模式，不画颜色条
    if (!m_showColorBar) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 颜色条位置：右侧，宽 20px，高 80% 窗口
    int barWidth = 20;
    int barHeight = static_cast<int>(height() * 0.8f);
    int barX = width() - barWidth - 10;
    int barY = (height() - barHeight) / 2;

    // 创建垂直线性渐变（从 minZ 到 maxZ）
    QLinearGradient gradient(barX, barY + barHeight, barX, barY); // 从下到上

    // 定义与 GLSL 中相同的颜色映射
    gradient.setColorAt(0.0, QColor(0, 0, 128));       // 深蓝
    gradient.setColorAt(0.25, QColor(0, 128, 0));      // 绿
    gradient.setColorAt(0.5, QColor(204, 204, 0));     // 黄
    gradient.setColorAt(0.75, QColor(255, 255, 255));  // 白
    gradient.setColorAt(1.0, QColor(255, 255, 255));   // 白（保持）

    // 画颜色条
    painter.fillRect(barX, barY, barWidth, barHeight, gradient);

    // 可选：画边框
    painter.setPen(Qt::white);
    painter.drawRect(barX, barY, barWidth - 1, barHeight - 1);

    // 可选：标注 min/max Z 值
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    QString minText = QString::number(m_bboxMin.z(), 'f', 2);
    QString maxText = QString::number(m_bboxMax.z(), 'f', 2);
    painter.drawText(barX - 50, barY + barHeight, minText);
    painter.drawText(barX - 50, barY, maxText);

    painter.end();
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
    m_showColorBar = (mode == 0); // 仅高程色显示
    update(); // 触发重绘（包括 paintEvent）
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

    // 初始化坐标轴
    initAxis();

    initBoundingBoxGeometry();
}

void PointCloudWidget::resizeGL(int w, int h)
{
    // 设置视口
    glViewport(0, 0, w, h);

    m_projection.setToIdentity();
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    m_projection.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void PointCloudWidget::paintGL()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glEnable(GL_DEPTH_TEST);   // 必须开启深度测试
    //glDisable(GL_BLEND);       // 避免混合干扰
    //glDisable(GL_CULL_FACE);   // 坐标轴无背面
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

    // 渲染坐标轴（放在最后，确保在前面）
    glDepthMask(GL_FALSE); // 不写深度缓冲
    renderAxis();
    glDepthMask(GL_TRUE);  // 恢复

    // 渲染边界盒（在点云之后，确保线框可见）
    renderBoundingBox();
    
}


void PointCloudWidget::renderAxis()
{
    if (!m_axisInitialized) return;

    // 就放在包围盒的最小角（世界坐标固定）
    QVector3D origin = m_center;

    // 向内偏移一点避免遮挡
    origin += 0.02f * m_bboxSize;

    // 长度 = 包围盒最大边的 10%
    float L = 0.1f * std::max({ m_bboxSize.x(), m_bboxSize.y(), m_bboxSize.z() });

    QMatrix4x4 model;
    model.translate(origin);
    model.scale(L);

    QMatrix4x4 mvp = m_projection * m_view * model;

    // 使用着色器
    m_axisShader->bind();
    m_axisShader->setUniformValue("uMVP", mvp);

    // 绑定 VBO
    m_axisVbo.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, reinterpret_cast<void*>(sizeof(float) * 3));

    // 绘制 3 条线（6 个顶点）
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 6);

    // 解绑
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    m_axisVbo.release();
    m_axisShader->release();
}


void PointCloudWidget::initAxis()
{
    if (m_axisInitialized) return;

    // 1. 创建着色器
    m_axisShader = new QOpenGLShaderProgram(this);
    m_axisShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 uMVP;\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "   gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "   vColor = aColor;\n"
        "}"
    );
    m_axisShader->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(vColor, 1.0);\n"
        "}"
    );
    m_axisShader->link();

    // 2. 准备顶点数据：每条线两个点 + RGB 颜色
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };
    std::vector<Vertex> vertices = {
        // X 轴：红
        {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        // Y 轴：绿
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        // Z 轴：蓝
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}
    };

    // 3. 创建 VBO
    m_axisVbo.create();
    m_axisVbo.bind();
    m_axisVbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(Vertex)));
    m_axisVbo.release();

    m_axisInitialized = true;
}

void PointCloudWidget::initBoundingBoxGeometry()
{
    if (m_boxInitialized) return;

    // --- 1. 创建着色器（不依赖任何数据）---
    m_boxShader = new QOpenGLShaderProgram(this);
    m_boxShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "   gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}"
    );
    m_boxShader->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 uColor;\n"
        "void main() {\n"
        "   FragColor = vec4(uColor, 1.0);\n"
        "}"
    );

    if (!m_boxShader->link()) {
        qWarning() << "Box shader link failed:" << m_boxShader->log();
        return;
    }

    // --- 2. 创建单位立方体 VBO（固定几何）---
    std::vector<QVector3D> unitCube = {
        {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
        {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}
    };
    std::vector<GLuint> indices = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    m_boxVbo.create();
    m_boxVbo.bind();
    m_boxVbo.allocate(unitCube.data(), static_cast<int>(unitCube.size() * sizeof(QVector3D)));
    m_boxVbo.release();

    m_boxEbo.create();
    m_boxEbo.bind();
    m_boxEbo.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(GLuint)));
    m_boxEbo.release();

    m_boxInitialized = true;
}


void PointCloudWidget::renderBoundingBox()
{
    if (!m_boxInitialized || m_points.empty()) return;

    // 动态构建模型矩阵：将 [0,1]^3 映射到 [min, max]
    QMatrix4x4 model;
    model.translate(m_bboxMin);                    // 移动到最小角
    model.scale(m_bboxMax - m_bboxMin);            // 缩放到实际尺寸

    QMatrix4x4 mvp = m_projection * m_view * model;

    // 渲染
    m_boxShader->bind();
    m_boxShader->setUniformValue("uMVP", mvp);
    m_boxShader->setUniformValue("uColor", QVector3D(1.0f, 1.0f, 0.0f)); // 黄色

    m_boxVbo.bind();
    m_boxEbo.bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    //glDepthMask(GL_FALSE); // 避免被点云遮挡
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    //glDepthMask(GL_TRUE);

    glDisableVertexAttribArray(0);
    m_boxVbo.release();
    m_boxEbo.release();
    m_boxShader->release();
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
