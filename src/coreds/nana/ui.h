#pragma once

#include <vector>
#include <forward_list>
#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/textbox.hpp>

namespace ui {

/* msvc2015 fails to compile this -> C3805
enum class Msg
{
    SUCCESS,
    ERROR,
    WARNING
};
*/

enum class Msg
{
    $SUCCESS,
    $ERROR,
    $WARNING
};

struct MsgColors
{
    const nana::color success_fg;
    const nana::color success_bg;
    const nana::color error_fg;
    const nana::color error_bg;
    const nana::color warning_fg;
    const nana::color warning_bg;
    const nana::color close_fg;
    
    MsgColors(
            unsigned success_fg, unsigned success_bg,
            unsigned error_fg, unsigned error_bg,
            unsigned warning_fg, unsigned warning_bg,
            unsigned close_fg):
        success_fg(nana::color_rgb(success_fg)),
        success_bg(nana::color_rgb(success_bg)),
        error_fg(nana::color_rgb(error_fg)),
        error_bg(nana::color_rgb(error_bg)),
        warning_fg(nana::color_rgb(warning_fg)),
        warning_bg(nana::color_rgb(warning_bg)),
        close_fg(nana::color_rgb(close_fg))
    {
        
    }
    
    static const MsgColors DEFAULT;
};

const MsgColors MsgColors::DEFAULT {
    0x52A954, 0xDEFCD5,
    0xA95252, 0xF1D7D7,
    0x96904D, 0xF6F3D5,
    0x777777
};

// not in the widget api
inline void visible(nana::widget& w, bool on)
{
    nana::API::show_window(w.handle(), on);
}

inline void border_top(nana::paint::graphics& graph, const nana::color& color)
{
    /*
    nana::rectangle r(graph.size());
    graph.line_begin(r.x, r.y);
    graph.line_to({ r.right() - 1, r.y }, color);
    */
    graph.line_begin(0, 0);
    graph.line_to({ static_cast<int>(graph.width()), 0 }, color);
}

inline void border_bottom(nana::paint::graphics& graph, const nana::color& color)
{
    auto y = static_cast<int>(graph.height()) - 1;
    graph.line_begin(0, y);
    graph.line_to({ static_cast<int>(graph.width()), y }, color);
}

/*inline void border_bottom_n(nana::paint::graphics& graph, const nana::color& color, int n)
{
    auto x = static_cast<int>(graph.width());
    auto y = static_cast<int>(graph.height()) - 1;
    do
    {
        graph.line_begin(0, y);
        graph.line_to({ x, y }, color);
        y--;
    }
    while (0 < --n);
}*/

inline void border_left(nana::paint::graphics& graph, const nana::color& color)
{
    graph.line_begin(0, 0);
    graph.line_to({ 0, static_cast<int>(graph.height()) - 1 }, color);
}

inline void border_right(nana::paint::graphics& graph, const nana::color& color)
{
    auto x = static_cast<int>(graph.width()) - 1;
    graph.line_begin(x, 0);
    graph.line_to({ x, static_cast<int>(graph.height()) - 1 }, color);
}

/*inline void draw_border_top(nana::window w, const nana::color& color)
{
    nana::drawing dw(w);
    dw.draw([&color](nana::paint::graphics& graph) {
        border_top(graph, color);
    });
}*/

enum class WindowFlags : uint8_t
{
    TASKBAR = 1,
    FLOATING = 2,
    NO_ACTIVATE = 4,
    MINIMIZE = 8,
    MAXIMIZE = 16,
    SIZABLE = 32,
    DECORATION = 64,
    
    DEFAULT = DECORATION | TASKBAR | MINIMIZE | MAXIMIZE | SIZABLE,
    STATIC = DECORATION | TASKBAR | MINIMIZE
};

inline uint8_t operator| (uint8_t a, WindowFlags b)
{
    return a | static_cast<uint8_t>(b);
}
inline uint8_t operator& (uint8_t a, WindowFlags b)
{
    return a & static_cast<uint8_t>(b);
}

struct RootForm;
RootForm* root{ nullptr };

struct RootForm : nana::form
{
private:
    bool closed{ false };
public:
    RootForm(nana::rectangle rect,
            uint8_t flags = uint8_t(WindowFlags::DEFAULT),
            const nana::color& bg = nana::colors::white): nana::form(rect,
        nana::appearance(
            0 != (flags & WindowFlags::DECORATION),
            0 != (flags & WindowFlags::TASKBAR),
            0 != (flags & WindowFlags::FLOATING),
            0 != (flags & WindowFlags::NO_ACTIVATE),
            0 != (flags & WindowFlags::MINIMIZE),
            0 != (flags & WindowFlags::MAXIMIZE),
            0 != (flags & WindowFlags::SIZABLE)
        )
    )
    {
        root = this;
        bgcolor(bg);
        events().unload([this](const nana::arg_unload& arg) {
            closed = true;
        });
    }
    bool isClosed()
    {
        return closed;
    }
};

struct SubForm : nana::form
{
    const bool modal;
    SubForm(nana::rectangle rect,
            const std::string& title = "",
            bool modal = true,
            uint8_t flags = uint8_t(WindowFlags::DECORATION),
            const nana::color& bg = nana::colors::white): nana::form(*ui::root, rect,
        nana::appearance(
            0 != (flags & WindowFlags::DECORATION),
            0 != (flags & WindowFlags::TASKBAR),
            0 != (flags & WindowFlags::FLOATING),
            0 != (flags & WindowFlags::NO_ACTIVATE),
            0 != (flags & WindowFlags::MINIMIZE),
            0 != (flags & WindowFlags::MAXIMIZE),
            0 != (flags & WindowFlags::SIZABLE)
        )
    ), modal(modal)
    {
        bgcolor(bg);
        if (!title.empty())
            caption(title);
        events().unload([this](const nana::arg_unload& arg) {
            if (ui::root->isClosed())
                return;
            
            arg.cancel = true;
            if (this->modal)
            {
                nana::API::window_enabled(*root, true);
                root->focus();
            }
            hide();
            onClose();
        });
    }
protected:
    virtual void onClose() {}
    int resizeY(int y)
    {
        auto sz = size();
        sz.height += y;
        size(sz);
        return sz.height;
    }
    void popTo(nana::point pos)
    {
        nana::API::move_window(*this, pos);
        show();
        if (modal)
            nana::API::window_enabled(*root, false);
    }
    void popTo(nana::window target, int y)
    {
        auto pos = nana::API::window_position(target);
        pos.y += y;
        popTo(pos);
    }
};

struct Place : nana::place
{
    Place(nana::widget& owner, const char* layout) : nana::place(owner)
    {
        div(layout);
    }
};

struct Icon : nana::picture
{
    Icon(nana::widget& owner, nana::paint::image icon, bool cursor_hand = false) : nana::picture(owner)
    {
        load(icon);
        transparent(true);
        
        if (!cursor_hand)
            return;
        
        events().mouse_move([this](const nana::arg_mouse& arg) {
            cursor(nana::cursor::hand);
        });
    }
    Icon(nana::widget& owner, const char* icon, bool cursor_hand = false):
        Icon(owner, nana::paint::image(icon), cursor_hand)
    {
        
    }
};

struct DeferredIcon : nana::picture
{
    DeferredIcon(nana::paint::image icon, bool cursor_hand = false) : nana::picture()
    {
        load(icon);
        //transparent(true);
        
        if (!cursor_hand)
            return;
        
        events().mouse_move([this](const nana::arg_mouse& arg) {
            cursor(nana::cursor::hand);
        });
    }
protected:
    void _m_complete_creation() override
    {
        transparent(true);
    }
};

struct Panel : nana::panel<false>
{
    nana::place place{ *this };
    
    Panel(nana::widget& owner, const char* layout) : nana::panel<false>(owner)
    {
        place.div(layout);
    }
};

struct DeferredPanel : nana::panel<false>
{
    const char* const layout;
    nana::place place;
    
    DeferredPanel(const char* layout) : nana::panel<false>(), layout(layout)
    {
        
    }
protected:
    void _m_complete_creation() override
    {
        place.bind(*this);
        place.div(layout);
    }
};

struct ToggleIcon : Panel
{
    Icon on_;
    Icon off_;
    
    ToggleIcon(nana::widget& owner, nana::paint::image icon_on, nana::paint::image icon_off, bool cursor_hand = true):
        Panel(owner, "<on_><off_>"),
        on_(*this, icon_on, cursor_hand),
        off_(*this, icon_off, cursor_hand)
    {
        place["on_"] << on_;
        place["off_"] << off_;
        place.collocate();
        place.field_display("off_", false);
    }
    
    void update(bool on)
    {
        place.field_display("on_", on);
        place.field_display("off_", !on);
        place.collocate();
    }
};

struct DeferredToggleIcon : DeferredPanel
{
    DeferredIcon on_;
    DeferredIcon off_;
    
    DeferredToggleIcon(nana::paint::image icon_on, nana::paint::image icon_off, bool cursor_hand = true):
        DeferredPanel("<on_><off_>"),
        on_(icon_on, cursor_hand),
        off_(icon_off, cursor_hand)
    {
        
    }
    
    void update(bool on)
    {
        place.field_display("on_", on);
        place.field_display("off_", !on);
        place.collocate();
    }
protected:
    void _m_complete_creation() override
    {
        DeferredPanel::_m_complete_creation();
        
        on_.create(*this);
        off_.create(*this);
        
        place["on_"] << on_;
        place["off_"] << off_;
        
        place.collocate();
        place.field_display("off_", false);
    }
};

struct BgPanel : nana::panel<true>
{
    nana::place place{ *this };
    
    BgPanel(nana::widget& owner, const char* layout, unsigned bg = 0, unsigned fg = 0) : nana::panel<true>(owner)
    {
        place.div(layout);
        if (bg)
            bgcolor(nana::color_rgb(bg));
        if (fg)
            fgcolor(nana::color_rgb(fg));
    }
};

struct DeferredBgPanel : nana::panel<true>
{
    const char* const layout;
    nana::place place;
    
    DeferredBgPanel(const char* layout) : nana::panel<true>(), layout(layout)
    {
        
    }
protected:
    void _m_complete_creation() override
    {
        place.bind(*this);
        place.div(layout);
    }
};

struct MsgPanel : BgPanel
{
    const MsgColors colors;
    nana::label msg_{ *this, "" };
    nana::label close_{ *this, "<bold target=\"0\"> x </>" };
    
    MsgPanel(nana::widget& owner, const MsgColors& colors) : BgPanel(owner,
        "margin=[1,2,1,3]"
        "<msg_ margin=[1,0,0,0]>"
        "<close_ weight=18>"
    ),  colors(colors)
    {
        auto listener = [this](nana::label::command cmd, const std::string& target) {
            if (nana::label::command::click == cmd)
                hide();
        };
        
        place["msg_"] << msg_
                .text_align(nana::align::left)
                .transparent(true);
        
        place["close_"] << close_
                .text_align(nana::align::right)
                .add_format_listener(listener)
                .format(true)
                .transparent(true);
        
        close_.fgcolor(colors.close_fg);
        
        place.collocate();
        
        // initially hidden
        hide();
    }
    
    void update(const std::string& msg, Msg type = ui::Msg::$ERROR)
    {
        msg_.caption(msg);
        
        switch (type)
        {
            case ui::Msg::$SUCCESS:
                msg_.fgcolor(colors.success_fg);
                bgcolor(colors.success_bg);
                break;
            case ui::Msg::$ERROR:
                msg_.fgcolor(colors.error_fg);
                bgcolor(colors.error_bg);
                break;
            case ui::Msg::$WARNING:
                msg_.fgcolor(colors.warning_fg);
                bgcolor(colors.warning_bg);
                break;
        }
        
        show();
    }
};

namespace fonts {

const nana::paint::font r8("", 8); // max ph: 16
const nana::paint::font r9("", 9); // max ph: 18
const nana::paint::font r10("", 10); // max ph: 22
const nana::paint::font r11("", 11); // max ph: 24
const nana::paint::font r12("", 12); // max ph: 27
const nana::paint::font r14("", 14); // max ph: 32
const nana::paint::font r16("", 16); // max ph: 36
const nana::paint::font r18("", 18); // max ph: 41
const nana::paint::font r20("", 20); // max ph: 46
const nana::paint::font r22("", 22); // max ph: 51
const nana::paint::font r24("", 24); // max ph: 56

} // fonts

namespace w$ {

const int
    h8 = 16,
    h9 = 18,
    h10 = 22,
    h11 = 24,
    h12 = 27,
    h14 = 32,
    h16 = 36,
    h18 = 41,
    h20 = 46,
    h22 = 51,
    h24 = 56;

#ifdef WIN32
const char* const input8 = "margin=[0,1,1,1]<_>";
const char* const input9 = input8;
const char* const input10 = input8;
const char* const input11 = input8;
const char* const input12 = input8;
const char* const input14 = input8;
const char* const input16 = input8;
const char* const input18 = input8;
const char* const input20 = input8;
const char* const input22 = input8;
const char* const input24 = input8;
#else
const char* const input8 = "margin=[1,1,1,1]<_>";
const char* const input9 = input8;
const char* const input10 = "margin=[2,1,1,1]<_>";
const char* const input11 = input10;
const char* const input12 = "margin=[3,1,1,1]<_>";
const char* const input14 = "margin=[4,1,1,1]<_>";
const char* const input16 = "margin=[5,1,1,1]<_>";
const char* const input18 = "margin=[6,1,1,1]<_>";
const char* const input20 = "margin=[7,1,1,1]<_>";
const char* const input22 = "margin=[8,1,1,1]<_>";
const char* const input24 = "margin=[9,1,1,1]<_>";
#endif

struct Input : BgPanel
{
    static const char* $layout(int size, int* flex_height)
    {
        switch (size)
        {
            case 8: if (flex_height) *flex_height += h8; return input8;
            case 9: if (flex_height) *flex_height += h9; return input9;
            case 10: if (flex_height) *flex_height += h10; return input10;
            case 11: if (flex_height) *flex_height += h11; return input11;
            case 12: if (flex_height) *flex_height += h12; return input12;
            case 14: if (flex_height) *flex_height += h14; return input14;
            case 16: if (flex_height) *flex_height += h16; return input16;
            case 18: if (flex_height) *flex_height += h18; return input18;
            case 20: if (flex_height) *flex_height += h20; return input20;
            case 22: if (flex_height) *flex_height += h22; return input22;
            case 24: if (flex_height) *flex_height += h24; return input24;
            default: if (flex_height) *flex_height += h10; return input10;
        }
    }
    
    nana::textbox $;
    
    Input(nana::widget& owner, int* flex_height,
            const std::string& placeholder,
            const nana::paint::font& font,
            const nana::color* bottom_color = nullptr,
            bool multi_lines = false,
            bool borderless = true):
        BgPanel(owner, $layout((int)font.size(), flex_height)),
        $(*this)
    {
        place["_"] << $;
        
        $.multi_lines(multi_lines);
        $.borderless(borderless);
        if (bottom_color)
        {
            nana::API::effects_edge_nimbus($, nana::effects::edge_nimbus::none);
            nana::drawing dw(*this);
            dw.draw([bottom_color](nana::paint::graphics& graph) {
                border_bottom(graph, *bottom_color);
            });
        }
        
        $.typeface(font);
        if (!placeholder.empty())
            $.tip_string(placeholder);
        
        place.collocate();
    }
    nana::textbox& bg(const nana::color& color)
    {
        $.bgcolor(color);
        bgcolor(color);
        return $;
    }
};

#ifdef WIN32
const char* const label8 = "margin=[0,5]<_>";
const char* const label9 = label8;
const char* const label10 = label8;
const char* const label11 = label8;
const char* const label12 = label8;
const char* const label14 = label8;
const char* const label16 = label8;
const char* const label18 = label8;
const char* const label20 = label8;
const char* const label22 = label8;
const char* const label24 = label8;
#else
const char* const label8 = "margin=[1,5,0,5]<_>";
const char* const label9 = label8;
const char* const label10 = "margin=[2,5,0,5]<_>";
const char* const label11 = label10;
const char* const label12 = "margin=[3,5,0,5]<_>";
const char* const label14 = "margin=[4,5,0,5]<_>";
const char* const label16 = "margin=[5,5,0,5]<_>";
const char* const label18 = "margin=[6,5,0,5]<_>";
const char* const label20 = "margin=[7,5,0,5]<_>";
const char* const label22 = "margin=[8,5,0,5]<_>";
const char* const label24 = "margin=[9,5,0,5]<_>";
#endif

struct Label : BgPanel
{
    static const char* $layout(int size, int* flex_height)
    {
        switch (size)
        {
            case 8: if (flex_height) *flex_height += h8; return label8;
            case 9: if (flex_height) *flex_height += h9; return label9;
            case 10: if (flex_height) *flex_height += h10; return label10;
            case 11: if (flex_height) *flex_height += h11; return label11;
            case 12: if (flex_height) *flex_height += h12; return label12;
            case 14: if (flex_height) *flex_height += h14; return label14;
            case 16: if (flex_height) *flex_height += h16; return label16;
            case 18: if (flex_height) *flex_height += h18; return label18;
            case 20: if (flex_height) *flex_height += h20; return label20;
            case 22: if (flex_height) *flex_height += h22; return label22;
            case 24: if (flex_height) *flex_height += h24; return label24;
            default: if (flex_height) *flex_height += h10; return label10;
        }
    }
    
    nana::label $;
    
    Label(nana::widget& owner, int* flex_height,
            const std::string& text,
            const nana::paint::font& font,
            bool format = false):
        BgPanel(owner, $layout((int)font.size(), flex_height)),
        $(*this)
    {
        place["_"] << $;
        
        $.typeface(font);
        if (format)
            $.format(true);
        if (!text.empty())
            $.caption(text);
        
        place.collocate();
    }
    nana::label& bg(const nana::color& color)
    {
        $.bgcolor(color);
        bgcolor(color);
        return $;
    }
};

struct DeferredLabel : DeferredBgPanel
{
    const nana::paint::font& font;
    nana::label $;
    
    DeferredLabel(int* flex_height, const nana::paint::font& font):
        DeferredBgPanel(Label::$layout((int)font.size(), flex_height)),
        font(font)
    {
        
    }
    nana::label& bg(const nana::color& color)
    {
        $.bgcolor(color);
        bgcolor(color);
        return $;
    }
protected:
    void _m_complete_creation() override
    {
        DeferredBgPanel::_m_complete_creation();
        
        $.create(handle());
        $.typeface(font);
        
        place["_"] << $;
        place.collocate();
    }
};

} // w$

template <typename T, typename W>
struct List : Panel
{
private:
    std::forward_list<W> items;
    std::vector<W*> array;
    nana::color selected_bg;
    int selected_idx{ -1 };
    
public:    
    List(nana::widget& owner, const char* layout = nullptr, unsigned selected_bg = 0xF3F3F3):
        Panel(owner, layout ? layout : "margin=[5,0] <items_ vert>"),
        selected_bg(nana::color_rgb(selected_bg))
    {
        
    }
    
    void collocate(int pageSize = 10)
    {
        for (int i = 0; i < pageSize; i++)
        {
            items.emplace_front(*this);
            place["items_"] << items.front();
            array.push_back(&items.front());
        }
        
        place.collocate();
    }
    
    int size()
    {
        return array.size();
    }
    
    void populate(int idx, T* pojo)
    {
        array[idx]->update(pojo);
    }
    
    bool trySelect(int idx)
    {
        int prev_idx = selected_idx;
        if (idx == prev_idx)
            return false;
        
        if (idx == -1)
        {
            // deselect
            selected_idx = idx;
            array[prev_idx]->bgcolor(nana::colors::white);
            return true;
        }
        
        if (idx < 0 || idx >= array.size())
            return false;
        
        selected_idx = idx;
        
        if (prev_idx != -1)
            array[prev_idx]->bgcolor(nana::colors::white);
        
        array[idx]->bgcolor(selected_bg);
        return true;
    }
};
    
} // ui
