#ifndef slic3r_GUI_HelioActivationDialog_hpp_
#define slic3r_GUI_HelioActivationDialog_hpp_

#include "GUI_Utils.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/LinkLabel.hpp"

#include <wx/wx.h>

namespace Slic3r { namespace GUI {

// Modal dialog used to enable Helio Additive and obtain/store a PAT.
// Shows an intro screen + activation success screen (with Copy PAT).
class HelioActivationDialog : public DPIDialog
{
public:
    explicit HelioActivationDialog(wxWindow* parent = nullptr);
    ~HelioActivationDialog() override = default;

    void on_dpi_changed(const wxRect& suggested_rect) override;

    // If true when the dialog ends, caller may want to continue into Helio flow.
    bool should_run_first_optimization() const { return m_run_first_optimization; }

private:
    enum class Page {
        Intro,
        Success
    };

private:
    void build_ui();
    void show_page(Page page);
    void set_error(const wxString& msg);

    void on_enable_clicked();
    void on_retry_clicked();
    void on_uninstall_clicked();
    void on_copy_pat_clicked();
    void on_run_first_clicked();

    void request_pat_async();

private:
    Page m_page { Page::Intro };

    wxPanel* m_intro_panel { nullptr };
    wxPanel* m_success_panel { nullptr };

    // Intro widgets
    Button* m_btn_enable { nullptr };
    Button* m_btn_uninstall { nullptr };
    Button* m_btn_retry { nullptr };
    Label*  m_error_label { nullptr };

    // Success widgets
    Button* m_btn_run_first { nullptr };
    Button* m_btn_copy_pat { nullptr };

    bool m_run_first_optimization { false };
};

}} // namespace Slic3r::GUI

#endif // slic3r_GUI_HelioActivationDialog_hpp_

