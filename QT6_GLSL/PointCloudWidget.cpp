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
    updateBoundingBoxGeometry();
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
    m_logDistance = log(m_distance);
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
    //initScreenAxis();
    initScreenAxisOrtho();

    initBoundingBoxGeometry();
}

void PointCloudWidget::resizeGL(int w, int h)
{
    // 设置视口
    glViewport(0, 0, w, h);
    m_glHeight = h;
    m_glWidth = w;

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

    // 2. 渲染坐标轴（半透明，无深度写入）
    glDepthMask(GL_FALSE);
    //renderScreenAxis();
    renderScreenAxisOrtho();
    glDepthMask(GL_TRUE);

    // 3. 渲染边界盒（最后渲染，确保在最前面）
    renderBoundingBox();
    
    
}

void PointCloudWidget::initScreenAxis()
{
    if (m_axisInitialized) return;

    // 1. 创建着色器
    m_axisShader = new QOpenGLShaderProgram(this);

    // 使用视图矩阵的旋转部分，但固定位置
    m_axisShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 uRotation;      // 只包含旋转的矩阵\n"
        "uniform vec2 uCorner;        // 角落位置 (0-1)\n"
        "uniform float uSize;         // 大小 (0-1)\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "   // 应用旋转\n"
        "   vec4 rotated = uRotation * vec4(aPos, 1.0);\n"
        "   \n"
        "   // 映射到屏幕角落\n"
        "   vec2 screenPos = uCorner + rotated.xy * uSize;\n"
        "   \n"
        "   // 转换到NDC (-1到1)\n"
        "   vec2 ndc = screenPos * 2.0 - 1.0;\n"
        "   ndc.y = -ndc.y;  // 翻转Y轴\n"
        "   \n"
        "   gl_Position = vec4(ndc, 0.999, 1.0);\n"
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

    if (!m_axisShader->link()) return;

    // 2. 准备顶点数据（单位长度的坐标轴）
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };

    std::vector<Vertex> vertices = {
        // X轴（红）
        {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        // Y轴（绿）
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        // Z轴（蓝）
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}
    };

    m_axisVbo.create();
    m_axisVbo.bind();
    m_axisVbo.allocate(vertices.data(),
        static_cast<int>(vertices.size() * sizeof(Vertex)));
    m_axisVbo.release();

    m_axisInitialized = true;
}

void PointCloudWidget::renderScreenAxis()
{
    if (!m_axisInitialized) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_axisShader->bind();

    // 从视图矩阵提取旋转（移除平移）
    QMatrix4x4 rotation = m_view;
    rotation.setColumn(3, QVector4D(0, 0, 0, 1));

    // 左下角位置和大小
    QVector2D corner(0.05f, 0.75f);  // 左下角，稍微靠上一点
    float size = 0.15f;              // 占屏幕的15%

    m_axisShader->setUniformValue("uRotation", rotation);
    m_axisShader->setUniformValue("uCorner", corner);
    m_axisShader->setUniformValue("uSize", size);

    // 渲染
    glLineWidth(2.5f);
    m_axisVbo.bind();  // 重用现有的坐标轴VBO
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(float) * 6, nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(float) * 6,
        reinterpret_cast<void*>(sizeof(float) * 3));

    glDrawArrays(GL_LINES, 0, 6);

    // 清理
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    m_axisVbo.release();
    m_axisShader->release();

    // 恢复状态
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // 可选：添加标签
    renderCornerAxisLabels(rotation);
}

void PointCloudWidget::initScreenAxisOrtho()
{
    if (m_axisInitialized) return;

    // 1. 创建着色器（使用正交投影矩阵）
    m_axisShader = new QOpenGLShaderProgram(this);

    m_axisShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 uProjection;\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "   gl_Position = uProjection * vec4(aPos, 1.0);\n"
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

    if (!m_axisShader->link()) {
        qWarning() << "Screen axis shader link failed:" << m_axisShader->log();
        return;
    }

    // --- 2. 构建顶点数据：轴线 + 字母（Z/X/Y）---
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };
    std::vector<Vertex> vertices;

    const float L = 30.0f; // 轴长度
    const float W = 5.0f;  // 字母大小

    // ---- Z 轴（蓝色）----
    // 轴线
    vertices.push_back({ 0, 0, 0, 0, 0, 1 });
    vertices.push_back({ 0, 0, L, 0, 0, 1 });
    

    // ---- X 轴（红色）----
    // 轴线
    vertices.push_back({ 0, 0, 0, 1, 0, 0 });
    vertices.push_back({ L, 0, 0, 1, 0, 0 });
    

    // ---- Y 轴（绿色）----
    // 轴线
    vertices.push_back({ 0, 0, 0, 0, 1, 0 });
    vertices.push_back({ 0, L, 0, 0, 1, 0 });
    

    // 3. 创建VBO
    m_axisVbo.create();
    m_axisVbo.bind();
    m_axisVbo.allocate(vertices.data(),
        static_cast<int>(vertices.size() * sizeof(vertices)));
    m_axisVbo.release();

    m_axisInitialized = true;
}

void PointCloudWidget::renderScreenAxisOrtho()
{
    if (!m_axisInitialized) return;

    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);
    glEnable(GL_LINE_SMOOTH);

    // 创建正交投影矩阵（覆盖整个屏幕）
    // 屏幕左下角偏移（像素）
    float margin = 50.0f;
    float posX = -static_cast<float>(m_glWidth) / 2.0f + margin;
    float posY = -static_cast<float>(m_glHeight) / 2.0f + margin;

    // 构建正交投影（像素空间）
    QMatrix4x4 ortho;
    ortho.setToIdentity();
    ortho.ortho(
        -static_cast<float>(m_glWidth) / 2.0f,
        static_cast<float>(m_glWidth) / 2.0f,
        -static_cast<float>(m_glHeight) / 2.0f,
        static_cast<float>(m_glHeight) / 2.0f,
        -1000.0f, 1000.0f
    );

    QMatrix4x4 viewMatrix = m_view; // 假设这是你的视图矩阵
    QMatrix3x3 rotationMatrix = viewMatrix.normalMatrix(); // 获取用于法线变换的3x3子矩阵，实际上就是旋转部分
    // 创建坐标轴的模型矩阵
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity(); // 初始化为单位矩阵

    // 将提取的旋转应用到模型矩阵上
    // 注意，这里可能需要根据实际情况调整旋转的方向或顺序
    modelMatrix.rotate(QQuaternion::fromRotationMatrix(rotationMatrix));
    //平移到posX posY
    QMatrix4x4 model;
    model.translate(posX, posY, 0.0f);
    QMatrix4x4 mvp = ortho * model * modelMatrix;

    // 渲染
    m_axisShader->bind();
    m_axisShader->setUniformValue("uProjection", mvp);

    m_axisVbo.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(float) * 6, nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(float) * 6,
        reinterpret_cast<void*>(sizeof(float) * 3));

    glDrawArrays(GL_LINES, 0, 6);

    // 清理
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    m_axisVbo.release();
    m_axisShader->release();
}

void PointCloudWidget::renderCornerAxisLabels(const QMatrix4x4& rotation)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算标签位置（屏幕坐标）
    int axisSize = width() * 0.15f;
    int startX = width() * 0.05f;
    int startY = height() * 0.75f;

    // 应用相同的旋转到单位向量
    QVector3D xDir = rotation.map( QVector3D(1, 0, 0));
    QVector3D yDir = rotation.map(QVector3D(0, 1, 0));
    QVector3D zDir = rotation.map(QVector3D(0, 0, 1));

    // 绘制标签
    painter.setPen(Qt::red);
    painter.drawText(startX + xDir.x() * axisSize + 10,
        startY - xDir.y() * axisSize, "X");

    painter.setPen(Qt::green);
    painter.drawText(startX + yDir.x() * axisSize + 10,
        startY - yDir.y() * axisSize, "Y");

    painter.setPen(Qt::blue);
    painter.drawText(startX + zDir.x() * axisSize + 10,
        startY - zDir.y() * axisSize, "Z");

    painter.end();
}

void PointCloudWidget::updateBoundingBoxGeometry()
{
    if (m_points.empty()) return;

    // 使用世界坐标系的实际顶点创建边界盒
    float minX = m_bboxMin.x();
    float minY = m_bboxMin.y();
    float minZ = m_bboxMin.z();
    float maxX = m_bboxMax.x();
    float maxY = m_bboxMax.y();
    float maxZ = m_bboxMax.z();

    // 边界盒的8个顶点（世界坐标）
    m_boxVertices = {
        // 底面4个顶点
        minX, minY, minZ,  // 0: 左前下
        maxX, minY, minZ,  // 1: 右前下
        maxX, maxY, minZ,  // 2: 右后下
        minX, maxY, minZ,  // 3: 左后下

        // 顶面4个顶点
        minX, minY, maxZ,  // 4: 左前上
        maxX, minY, maxZ,  // 5: 右前上
        maxX, maxY, maxZ,  // 6: 右后上
        minX, maxY, maxZ   // 7: 左后上
    };

    // 12条边的索引（24个索引）
    m_boxIndices = {
        // 底面矩形
        0, 1,  // 底面前边
        1, 2,  // 底面右边
        2, 3,  // 底面后边
        3, 0,  // 底面左边

        // 顶面矩形
        4, 5,  // 顶面前边
        5, 6,  // 顶面右边
        6, 7,  // 顶面后边
        7, 4,  // 顶面左边

        // 垂直边
        0, 4,  // 左前垂直边
        1, 5,  // 右前垂直边
        2, 6,  // 右后垂直边
        3, 7   // 左后垂直边
    };

    // 如果VBO已经创建，更新数据
    if (m_boxInitialized) {
        makeCurrent();

        // 更新顶点数据
        m_boxVbo.bind();
        m_boxVbo.allocate(m_boxVertices.data(),
            static_cast<int>(m_boxVertices.size() * sizeof(float)));
        m_boxVbo.release();

        // 如果有EBO，更新索引数据
        doneCurrent();
    }
}

void PointCloudWidget::initBoundingBoxGeometry()
{
    if (m_boxInitialized) return;

    // 1. 创建着色器
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
        "uniform float uAlpha;\n"
        "void main() {\n"
        "   FragColor = vec4(uColor, uAlpha);\n"
        "}"
    );

    if (!m_boxShader->link()) {
        qWarning() << "Box shader link failed:" << m_boxShader->log();
        return;
    }

    // 2. 初始化为空数据，稍后由 updateBoundingBoxGeometry 填充
    m_boxVbo.create();
    m_boxVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    m_boxInitialized = true;
}


void PointCloudWidget::renderBoundingBox()
{
    if (!m_boxInitialized || m_boxVertices.empty() || m_points.empty()) {
        qDebug() << "Cannot render bounding box: not initialized or empty";
        return;
    }

    qDebug() << "Rendering bounding box with" << m_boxVertices.size() / 3 << "vertices";

    // 保存当前状态
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

    // 禁用深度写入，确保边界盒始终可见
    glDepthMask(GL_FALSE);

    // 启用混合，使边界盒半透明
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 设置线条渲染
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(3.0f); // 更粗的线

    m_boxShader->bind();

    // 设置MVP矩阵
    QMatrix4x4 mvp = m_projection * m_view;
    m_boxShader->setUniformValue("uMVP", mvp);
    m_boxShader->setUniformValue("uColor", QVector3D(1.0f, 0.5f, 0.0f)); // 橙色
    m_boxShader->setUniformValue("uAlpha", 0.8f); // 80%不透明度

    // 绑定顶点数据
    m_boxVbo.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // 绘制所有边
    glDrawElements(GL_LINES,
        static_cast<GLsizei>(m_boxIndices.size()),
        GL_UNSIGNED_INT,
        m_boxIndices.data());

    // 清理
    glDisableVertexAttribArray(0);
    m_boxVbo.release();
    m_boxShader->release();

    // 恢复渲染状态
    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
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

// 在类中增加一个 float m_logDistance;
// 初始化：m_logDistance = log(m_distance);

void PointCloudWidget::wheelEvent(QWheelEvent* event)
{
    const float zoomSpeed = 0.1f;
    if (event->angleDelta().y() > 0) {
        m_logDistance -= zoomSpeed; // 放大
    }
    else {
        m_logDistance += zoomSpeed; // 缩小
    }
    m_distance = std::exp(m_logDistance);
    m_distance = std::max(0.01f, m_distance); // 仅限制最小值
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
