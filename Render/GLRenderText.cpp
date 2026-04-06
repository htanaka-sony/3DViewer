#include "GLRenderText.h"

#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QWidget>

RENDER_NAMESPACE_BEGIN

GLRenderText::GLRenderText(QWidget* view, SceneView* scene_view) : m_view(view), m_scene_view(scene_view) {}

GLRenderText::~GLRenderText()
{
    for (auto& [font, glyphTextures] : m_fontGlyphTextures) {
        for (auto& [ch, glyph] : glyphTextures) {
            delete glyph.texture;
        }
    }
}

void GLRenderText::createGlyphTexture(const FontInfo& font, std::map<wchar_t, GlyphTexture>& glyphTextures,
                                      wchar_t character_)
{
    if (glyphTextures.find(character_) != glyphTextures.end()) {
        return;
    }

    QChar character(character_);

    QFont set_font(QString::fromStdWString(font.m_font_name), 64);
    if (font.m_font_bold) {
        set_font.setBold(true);
    }

    QFontMetrics metrics(set_font);
    int          width  = metrics.horizontalAdvance(character);
    int          height = metrics.height();

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setFont(set_font);
    painter.setPen(Qt::white);
    painter.drawText(0, metrics.ascent(), QString(character));

    QOpenGLTexture* texture = new QOpenGLTexture(image.mirrored());
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    glyphTextures[character_] = {texture, width, height};
}

void GLRenderText::renderText(float x, float y, float z, const std::wstring& text, const std::wstring& font_name,
                              const Point4f& font_color, float font_size, TextQuad& quad, bool font_bold, float angle,
                              TextAlignment alignment)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    float dpiScale = m_view->devicePixelRatio();
    glViewport(0, 0, m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight());
    glOrtho(0, m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight(), 0, -1, 1);

    font_size = font_size * dpiScale;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    FontInfo font;
    font.m_font_name = font_name;
    font.m_font_bold = font_bold;

    auto& glyphTextures = m_fontGlyphTextures[font];

    // 描画開始位置を調整
    float cursorX = x;
    float cursorY = y;
    float cursorZ = z;

    // 回転を追加
    Matrix4x4f rotateMatrix;
    if (angle != 0.0) {
        glTranslatef(cursorX, cursorY, cursorZ);
        glRotatef(-angle, 0.0f, 0.0f, 1.0f);
        glTranslatef(-cursorX, -cursorY, -cursorZ);

        rotateMatrix.translate(cursorX, cursorY, 0);
        rotateMatrix.rotateDegree(-angle, 0, 0, 1);
        rotateMatrix.translate(-cursorX, -cursorY, 0);
    }

    /// テキスト全体の幅と高さを計算
    std::vector<float> lineWidths;
    std::vector<float> lineHeights;
    float              lineWidth  = 0.0f;
    float              lineHeight = 0.0f;

    for (const wchar_t& character : text) {
        if (character == L'\n') {
            lineWidths.emplace_back(lineWidth);
            lineHeights.emplace_back(lineHeight);
            lineWidth  = 0.0f;
            lineHeight = 0.0f;
            continue;
        }
        createGlyphTexture(font, glyphTextures, character);
        const GlyphTexture& glyph = glyphTextures[character];
        lineWidth += glyph.width * font_size / 64.0;
        lineHeight = qMax(lineHeight, glyph.height * font_size / 64.0);
    }
    /// 最終行
    lineWidths.emplace_back(lineWidth);
    lineHeights.emplace_back(lineHeight);

    float textWidth  = 0.0f;
    float textHeight = 0.0f;
    for (size_t i = 0; i < lineWidths.size(); ++i) {
        textWidth = qMax(textWidth, lineWidths[i]);
        textHeight += lineHeights[i];
    }

    /// 基準位置に応じてオフセットを計算
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    switch (alignment) {
        case TextAlignment::BottomLeft:
            offsetX = 0.0f;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopLeft:
            offsetX = 0.0f;
            offsetY = -textHeight;
            break;
        case TextAlignment::CenterCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = -textHeight / 2.0f;
            break;
        case TextAlignment::BottomRight:
            offsetX = -textWidth;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopRight:
            offsetX = -textWidth;
            offsetY = -textHeight;
            break;
        case TextAlignment::BottomCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = -textHeight;
            break;
        case TextAlignment::CenterLeft:
            offsetX = 0.0f;
            offsetY = -textHeight / 2.0f;
            break;
        case TextAlignment::CenterRight:
            offsetX = -textWidth;
            offsetY = -textHeight / 2.0f;
            break;
    }

    float baseX = x + offsetX;
    float baseY = y - offsetY;

    baseY -= (textHeight - lineHeights[0]);

    cursorX = baseX;
    cursorY = baseY;

    /// テキストを描画
    int  lineIdx = 0;
    bool first   = true;
    for (const wchar_t& character : text) {
        if (character == L'\n') {
            cursorY += lineHeights[lineIdx];
            cursorX = baseX;
            ++lineIdx;
            continue;
        }

        const GlyphTexture& glyph = glyphTextures[character];
        glyph.texture->bind();

        glColor4f(font_color.x(), font_color.y(), font_color.z(), font_color.w());

        glBegin(GL_QUADS);
        glTexCoord3f(0.0f, 1.0f, cursorZ);
        glVertex3f(cursorX, cursorY - glyph.height * font_size / 64.0, cursorZ);
        glTexCoord3f(1.0f, 1.0f, cursorZ);
        glVertex3f(cursorX + glyph.width * font_size / 64.0, cursorY - glyph.height * font_size / 64.0, cursorZ);
        glTexCoord3f(1.0f, 0.0f, cursorZ);
        glVertex3f(cursorX + glyph.width * font_size / 64.0, cursorY, cursorZ);
        glTexCoord3f(0.0f, 0.0f, cursorZ);
        glVertex3f(cursorX, cursorY, cursorZ);
        glEnd();

        if (first) {
            quad.m_pos[0].set(cursorX, cursorY - glyph.height * font_size / 64.0, cursorZ);
            quad.m_pos[3].set(cursorX, cursorY, cursorZ);
            first = false;
        }
        quad.m_pos[1].set(cursorX + glyph.width * font_size / 64.0, cursorY - glyph.height * font_size / 64.0, cursorZ);
        quad.m_pos[2].set(cursorX + glyph.width * font_size / 64.0, cursorY, cursorZ);

        cursorX += (glyph.width * font_size / 64.0);

        glyph.texture->release();
    }

    if (angle != 0.0) {
        for (int i = 0; i < 4; ++i) {
            quad.m_pos[i] = rotateMatrix * quad.m_pos[i];
        }
    }

    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glDisable(GL_BLEND);
}

void GLRenderText::calcTextQuad(float x, float y, float z, const std::wstring& text, const std::wstring& font_name,
                                float font_size, TextQuad& quad, bool font_bold, float angle, TextAlignment alignment)
{
    float dpiScale = m_view->devicePixelRatio();
    font_size      = font_size * dpiScale;

    FontInfo font;
    font.m_font_name = font_name;
    font.m_font_bold = font_bold;

    auto& glyphTextures = m_fontGlyphTextures[font];

    /// テキスト全体の幅と高さを計算
    std::vector<float> lineWidths;
    std::vector<float> lineHeights;
    float              lineWidth  = 0.0f;
    float              lineHeight = 0.0f;

    for (const wchar_t& character : text) {
        if (character == L'\n') {
            lineWidths.emplace_back(lineWidth);
            lineHeights.emplace_back(lineHeight);
            lineWidth  = 0.0f;
            lineHeight = 0.0f;
            continue;
        }
        createGlyphTexture(font, glyphTextures, character);
        const GlyphTexture& glyph = glyphTextures[character];
        lineWidth += glyph.width * font_size / 64.0;
        lineHeight = qMax(lineHeight, glyph.height * font_size / 64.0);
    }
    /// 最終行
    lineWidths.emplace_back(lineWidth);
    lineHeights.emplace_back(lineHeight);

    float textWidth  = 0.0f;
    float textHeight = 0.0f;
    for (size_t i = 0; i < lineWidths.size(); ++i) {
        textWidth = qMax(textWidth, lineWidths[i]);
        textHeight += lineHeights[i];
    }

    /// 基準位置に応じてオフセットを計算
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    switch (alignment) {
        case TextAlignment::BottomLeft:
            offsetX = 0.0f;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopLeft:
            offsetX = 0.0f;
            offsetY = -textHeight;
            break;
        case TextAlignment::CenterCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = -textHeight / 2.0f;
            break;
        case TextAlignment::BottomRight:
            offsetX = -textWidth;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopRight:
            offsetX = -textWidth;
            offsetY = -textHeight;
            break;
        case TextAlignment::BottomCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = 0.0f;
            break;
        case TextAlignment::TopCenter:
            offsetX = -textWidth / 2.0f;
            offsetY = -textHeight;
            break;
        case TextAlignment::CenterLeft:
            offsetX = 0.0f;
            offsetY = -textHeight / 2.0f;
            break;
        case TextAlignment::CenterRight:
            offsetX = -textWidth;
            offsetY = -textHeight / 2.0f;
            break;
    }

    float baseX = x + offsetX;
    float baseY = y - offsetY;

    baseY -= (textHeight - lineHeights[0]);

    float cursorX = baseX;
    float cursorY = baseY;

    // バウンディングボックス（回転前）
    quad.m_pos[0].set(cursorX, cursorY - textHeight, z);                // 左上
    quad.m_pos[1].set(cursorX + textWidth, cursorY - textHeight, z);    // 右上
    quad.m_pos[2].set(cursorX + textWidth, cursorY, z);                 // 右下
    quad.m_pos[3].set(cursorX, cursorY, z);                             // 左下

    // 回転がある場合
    if (angle != 0.0f) {
        Matrix4x4f rotateMatrix;
        rotateMatrix.translate(x, y, 0);
        rotateMatrix.rotateDegree(-angle, 0, 0, 1);
        rotateMatrix.translate(-x, -y, 0);
        for (int i = 0; i < 4; ++i) {
            quad.m_pos[i] = rotateMatrix * quad.m_pos[i];
        }
    }
}

RENDER_NAMESPACE_END
