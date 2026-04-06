#ifndef RENDER_GLRENDERTEXT_H
#define RENDER_GLRENDERTEXT_H

#include "RenderGlobal.h"

#include <QFont>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QString>

#include "Math/Point4f.h"
#include "Scene/Dimension.h"
#include "Scene/SceneView.h"
using namespace Core;

RENDER_NAMESPACE_BEGIN

class GLRenderText {
public:
    explicit GLRenderText(QWidget* view, SceneView* scene_view);
    ~GLRenderText();

    void renderText(float x, float y, float z, const std::wstring& text, const std::wstring& font_name,
                    const Point4f& font_color, float font_size, TextQuad& quad, bool font_bold = false,
                    float angle = 0.0f, TextAlignment alignment = TextAlignment::CenterLeft);

    /// サイズを取得するだけ
    void calcTextQuad(float x, float y, float z, const std::wstring& text, const std::wstring& font_name,
                      float font_size, TextQuad& quad, bool font_bold, float angle, TextAlignment alignment);

private:
    struct GlyphTexture {
        QOpenGLTexture* texture;
        int             width;
        int             height;
    };

    /// フォントの特定
    struct FontInfo {
        std::wstring m_font_name;
        bool         m_font_bold = false;

        FontInfo() {}
        FontInfo(QFont& font) { m_font_name = font.family().toStdWString(); }

        bool operator<(const FontInfo& other) const
        {
            if (m_font_name < other.m_font_name) {
                return true;
            }
            else if (m_font_name > other.m_font_name) {
                return false;
            }
            if (m_font_bold < other.m_font_bold) {
                return true;
            }
            return false;
        }
    };

    void createGlyphTexture(const FontInfo& font, std::map<wchar_t, GlyphTexture>& glyphTextures, wchar_t character);

    QWidget*                                            m_view;
    SceneView*                                          m_scene_view;
    std::map<FontInfo, std::map<wchar_t, GlyphTexture>> m_fontGlyphTextures;
};

RENDER_NAMESPACE_END

#endif    // RENDER_GLRENDERTEXT_H
