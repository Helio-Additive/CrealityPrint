#include "HelioActivationDialog.hpp"

#include "I18N.hpp"
#include "MsgDialog.hpp"
#include "GUI_App.hpp"
#include "BitmapCache.hpp"

#include "../Utils/HelioDragon.hpp"

#include <wx/clipbrd.h>

namespace Slic3r { namespace GUI {

static StateColor green_btn_bg()
{
    return StateColor(std::pair<wxColour, int>(wxColour(61, 203, 115), StateColor::Hovered),
                      std::pair<wxColour, int>(wxColour(0, 174, 66),  StateColor::Normal));
}

static wxColour dark_bg()
{
    return wxColour(16, 16, 16);
}

HelioActivationDialog::HelioActivationDialog(wxWindow* parent)
    : DPIDialog(parent ? parent : wxGetApp().GetTopWindow(),
                wxID_ANY,
                wxString("Helio Additive"),
                wxDefaultPosition,
                wxDefaultSize,
                wxCAPTION | wxCLOSE_BOX)
{
    build_ui();
    
    // Check if PAT already exists
    const std::string existing_pat = Slic3r::HelioQuery::get_helio_pat();
    if (!existing_pat.empty()) {
        // PAT exists - show success screen directly (for re-activation after uninstall)
        show_page(Page::Success);
    } else {
        // No PAT - show intro for first-time activation
        show_page(Page::Intro);
    }

    CentreOnParent();
    wxGetApp().UpdateDlgDarkUI(this);
}

void HelioActivationDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    // Keep it simple: this dialog is built with scalable widgets/bitmaps, so just relayout.
    // (Matches behavior of other lightweight DPIDialog-derived dialogs in this codebase.)
    Layout();
    Fit();
    CentreOnParent();
}

void HelioActivationDialog::build_ui()
{
    SetBackgroundColour(*wxWHITE);

    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Top header (dark) with icon + title
    auto* header = new wxPanel(this);
    header->SetBackgroundColour(dark_bg());
    header->SetMinSize(wxSize(-1, FromDIP(70)));
    header->SetMaxSize(wxSize(-1, FromDIP(70)));

    auto* header_h = new wxBoxSizer(wxHORIZONTAL);
    auto* header_v = new wxBoxSizer(wxVERTICAL);
    auto* header_content = new wxBoxSizer(wxHORIZONTAL);

    auto* helio_icon = new wxStaticBitmap(header, wxID_ANY, create_scaled_bitmap("helio_icon", header, 32),
                                          wxDefaultPosition, wxSize(FromDIP(32), FromDIP(32)), 0);
    auto* helio_title = new Label(header, Label::Body_16, L("HELIO ADDITIVE"));
    wxFont bold = helio_title->GetFont();
    bold.SetWeight(wxFONTWEIGHT_BOLD);
    helio_title->SetFont(bold);
    helio_title->SetForegroundColour(wxColour("#FEFEFF"));

    header_content->Add(helio_icon, 0, wxLEFT | wxALIGN_CENTER, FromDIP(45));
    header_content->Add(helio_title, 0, wxLEFT | wxALIGN_CENTER, FromDIP(8));
    header_v->Add(header_content, 0, wxALIGN_CENTER, 0);
    header_h->Add(header_v, 0, wxALIGN_CENTER, 0);
    header->SetSizer(header_h);
    header->Layout();

    // Panels
    m_intro_panel = new wxPanel(this);
    m_success_panel = new wxPanel(this);

    // Intro content (approximate layout from screenshot)
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        m_intro_panel->SetBackgroundColour(*wxWHITE);

        auto* title = new Label(m_intro_panel, Label::Head_14, _L("Know Your Print Will Work — Before You Hit Print"));
        wxFont tfont = title->GetFont();
        tfont.SetWeight(wxFONTWEIGHT_BOLD);
        title->SetFont(tfont);

        auto* subtitle = new Label(m_intro_panel, Label::Body_14,
                                   _L("Physics-based analysis and optimization that makes prints faster, stronger, and far more reliable."));
        subtitle->Wrap(FromDIP(680));

        // Feature cards row
        auto* cards = new wxBoxSizer(wxHORIZONTAL);
        auto mk_card = [&](const wxString& icon_name, const wxString& h, const wxString& b) -> wxPanel* {
            auto* card = new wxPanel(m_intro_panel);
            card->SetBackgroundColour(wxColour("#F7F7F8"));
            card->SetMinSize(wxSize(FromDIP(330), FromDIP(140)));

            auto* cs = new wxBoxSizer(wxVERTICAL);
            auto* icon = new wxStaticBitmap(card, wxID_ANY, create_scaled_bitmap(into_u8(icon_name), card, 44));
            auto* head = new Label(card, Label::Body_15, h);
            wxFont hf = head->GetFont();
            hf.SetWeight(wxFONTWEIGHT_BOLD);
            head->SetFont(hf);
            auto* body = new Label(card, Label::Body_14, b);
            body->Wrap(FromDIP(300));

            cs->AddStretchSpacer(1);
            cs->Add(icon, 0, wxALIGN_CENTER_HORIZONTAL, 0);
            cs->Add(0, FromDIP(8));
            cs->Add(head, 0, wxALIGN_CENTER_HORIZONTAL, 0);
            cs->Add(0, FromDIP(6));
            cs->Add(body, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL, FromDIP(16));
            cs->AddStretchSpacer(1);
            card->SetSizer(cs);
            card->Layout();
            return card;
        };

        // Icons: reuse existing helio assets; if you later add specific icons, change these names.
        auto* card1 = mk_card("helio_icon", _L("Fewer Failed Prints"), _L("Catch issues before printing and improve first-try success."));
        auto* card2 = mk_card("helio_icon", _L("Faster Prints, Better Quality"), _L("Automatically tune speed and extrusion without manual tweaking."));
        cards->Add(card1, 1, wxEXPAND | wxRIGHT, FromDIP(16));
        cards->Add(card2, 1, wxEXPAND, 0);

        // Error + retry (hidden unless needed)
        m_error_label = new Label(m_intro_panel, Label::Body_14, wxEmptyString);
        m_error_label->SetForegroundColour(wxColour("#FC8800"));
        m_error_label->Hide();

        m_btn_retry = new Button(m_intro_panel, _L("Retry"));
        m_btn_retry->SetBackgroundColor(green_btn_bg());
        m_btn_retry->SetBorderColor(*wxWHITE);
        m_btn_retry->SetTextColor(wxColour(255, 255, 254));
        m_btn_retry->SetCornerRadius(FromDIP(12));
        m_btn_retry->Hide();
        m_btn_retry->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { on_retry_clicked(); });

        // Buttons row
        auto* btn_row = new wxBoxSizer(wxHORIZONTAL);
        m_btn_enable = new Button(m_intro_panel, _L("Enable Helio Additive"));
        m_btn_enable->SetBackgroundColor(green_btn_bg());
        m_btn_enable->SetBorderColor(*wxWHITE);
        m_btn_enable->SetTextColor(wxColour(255, 255, 254));
        m_btn_enable->SetCornerRadius(FromDIP(12));
        m_btn_enable->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { on_enable_clicked(); });

        m_btn_uninstall = new Button(m_intro_panel, _L("Cancel"));
        m_btn_uninstall->SetCornerRadius(FromDIP(12));
        m_btn_uninstall->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { EndModal(wxID_CANCEL); });

        btn_row->AddStretchSpacer(1);
        btn_row->Add(m_btn_enable, 0, 0, 0);
        btn_row->Add(m_btn_uninstall, 0, wxLEFT, FromDIP(12));
        btn_row->AddStretchSpacer(1);

        s->Add(0, FromDIP(18));
        s->Add(title, 0, wxLEFT | wxRIGHT, FromDIP(24));
        s->Add(0, FromDIP(10));
        s->Add(subtitle, 0, wxLEFT | wxRIGHT, FromDIP(24));
        s->Add(0, FromDIP(18));
        s->Add(cards, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(24));
        s->Add(0, FromDIP(18));
        s->Add(m_error_label, 0, wxLEFT | wxRIGHT, FromDIP(24));
        s->Add(0, FromDIP(8));
        s->Add(m_btn_retry, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL, FromDIP(24));
        s->Add(0, FromDIP(18));
        s->Add(btn_row, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
        s->Add(0, FromDIP(18));

        m_intro_panel->SetSizer(s);
        m_intro_panel->Layout();
        m_intro_panel->Fit();
    }

    // Success content (approximate layout from screenshot)
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        m_success_panel->SetBackgroundColour(wxColour(10, 10, 10));

        auto* check_icon = new wxStaticBitmap(m_success_panel, wxID_ANY, create_scaled_bitmap("helio_activation_success_icon", m_success_panel, 72));
        auto* title = new Label(m_success_panel, Label::Head_14, _L("Activation Successful!"));
        wxFont tf = title->GetFont();
        tf.SetWeight(wxFONTWEIGHT_BOLD);
        title->SetFont(tf);
        title->SetForegroundColour(*wxWHITE);

        auto* subtitle = new Label(m_success_panel, Label::Body_14,
                                   _L("Helio Additive is now active. You have unlocked free optimizations!"));
        subtitle->SetForegroundColour(*wxWHITE);
        subtitle->Wrap(FromDIP(680));

        auto* btn_row = new wxBoxSizer(wxHORIZONTAL);
        m_btn_run_first = new Button(m_success_panel, _L("Run Your First Optimization"));
        m_btn_run_first->SetBackgroundColor(green_btn_bg());
        m_btn_run_first->SetBorderColor(*wxWHITE);
        m_btn_run_first->SetTextColor(wxColour(255, 255, 254));
        m_btn_run_first->SetCornerRadius(FromDIP(12));
        m_btn_run_first->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { on_run_first_clicked(); });

        m_btn_copy_pat = new Button(m_success_panel, _L("Copy PAT"));
        m_btn_copy_pat->SetCornerRadius(FromDIP(12));
        m_btn_copy_pat->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { on_copy_pat_clicked(); });

        btn_row->Add(m_btn_run_first, 0, 0, 0);
        btn_row->Add(m_btn_copy_pat, 0, wxLEFT, FromDIP(16));

        // Links row
        auto* links = new wxBoxSizer(wxHORIZONTAL);
        auto* l1 = new LinkLabel(m_success_panel, _L("Helio Additive"), "https://www.helioadditive.com/");
        auto* l2 = new LinkLabel(m_success_panel, _L("Privacy Policy"), "https://www.helioadditive.com/en-us/policies/privacy");
        auto* l3 = new LinkLabel(m_success_panel, _L("Terms of Use"), "https://www.helioadditive.com/en-us/policies/terms");
        l1->SetFont(Label::Body_13);
        l2->SetFont(Label::Body_13);
        l3->SetFont(Label::Body_13);
        l1->SeLinkLabelFColour(wxColour(0, 119, 250));
        l2->SeLinkLabelFColour(wxColour(0, 119, 250));
        l3->SeLinkLabelFColour(wxColour(0, 119, 250));

        links->Add(l1, 0, 0, 0);
        links->AddStretchSpacer(1);
        links->Add(l2, 0, 0, 0);
        links->AddStretchSpacer(1);
        links->Add(l3, 0, 0, 0);

        s->AddStretchSpacer(1);
        s->Add(check_icon, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        s->Add(0, FromDIP(18));
        s->Add(title, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        s->Add(0, FromDIP(10));
        s->Add(subtitle, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, FromDIP(24));
        s->Add(0, FromDIP(22));
        s->Add(btn_row, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        s->Add(0, FromDIP(26));
        s->Add(links, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(48));
        s->AddStretchSpacer(1);

        m_success_panel->SetSizer(s);
        m_success_panel->Layout();
        m_success_panel->Fit();
    }

    // Compose dialog
    main_sizer->Add(header, 0, wxEXPAND, 0);
    main_sizer->Add(m_intro_panel, 1, wxEXPAND, 0);
    main_sizer->Add(m_success_panel, 1, wxEXPAND, 0);

    SetSizer(main_sizer);
    Layout();
    Fit();
}

void HelioActivationDialog::show_page(Page page)
{
    m_page = page;
    if (m_intro_panel)  m_intro_panel->Show(page == Page::Intro);
    if (m_success_panel) m_success_panel->Show(page == Page::Success);
    Layout();
    Fit();
    CentreOnParent();
}

void HelioActivationDialog::set_error(const wxString& msg)
{
    if (!m_error_label || !m_btn_retry)
        return;

    if (msg.empty()) {
        m_error_label->Hide();
        m_btn_retry->Hide();
        return;
    }

    m_error_label->SetLabel(msg);
    m_error_label->Wrap(FromDIP(680));
    m_error_label->Show();
    m_btn_retry->Show();
    Layout();
    Fit();
}

void HelioActivationDialog::on_enable_clicked()
{
    set_error(wxEmptyString);

    // If PAT already exists, just enable and show success.
    if (!Slic3r::HelioQuery::get_helio_pat().empty()) {
        wxGetApp().app_config->set_bool("helio_enable", true);
        wxGetApp().request_helio_supported_data();
        show_page(Page::Success);
        return;
    }

    request_pat_async();
}

void HelioActivationDialog::on_retry_clicked()
{
    set_error(wxEmptyString);
    request_pat_async();
}

void HelioActivationDialog::on_uninstall_clicked()
{
    // Deactivate (keep PAT as requested).
    wxGetApp().app_config->set_bool("helio_enable", false);
    EndModal(wxID_CANCEL);
}

void HelioActivationDialog::on_copy_pat_clicked()
{
    const std::string pat = Slic3r::HelioQuery::get_helio_pat();
    if (pat.empty()) {
        MessageDialog dlg(this, _L("No PAT available to copy."), _L("Helio Additive"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }

    if (wxTheClipboard->Open()) {
        wxTheClipboard->Clear();
        wxTheClipboard->SetData(new wxTextDataObject(from_u8(pat)));
        wxTheClipboard->Close();
    }

    MessageDialog dlg(this, _L("Copy successful!"), _L("Copy"), wxOK | wxYES_DEFAULT);
    dlg.ShowModal();
}

void HelioActivationDialog::on_run_first_clicked()
{
    m_run_first_optimization = true;
    EndModal(wxID_OK);
}

void HelioActivationDialog::request_pat_async()
{
    // UI lock while requesting
    if (m_btn_enable)    m_btn_enable->Disable();
    if (m_btn_retry)     m_btn_retry->Disable();
    if (m_btn_uninstall) m_btn_uninstall->Disable();

    wxGetApp().request_helio_pat([this](std::string pat) {
        wxTheApp->CallAfter([this, pat]() {
            if (m_btn_enable)    m_btn_enable->Enable();
            if (m_btn_retry)     m_btn_retry->Enable();
            if (m_btn_uninstall) m_btn_uninstall->Enable();

            if (pat == "not_enough") {
                set_error(_L("Failed to obtain PAT. The quantity limit has been reached. Please retry later."));
                return;
            }
            if (pat == "error" || pat.empty()) {
                set_error(_L("Failed to obtain Helio PAT. Click Retry to try again."));
                return;
            }

            Slic3r::HelioQuery::set_helio_pat(pat);
            wxGetApp().app_config->set_bool("helio_enable", true);
            wxGetApp().request_helio_supported_data();
            show_page(Page::Success);
        });
    });
}

}} // namespace Slic3r::GUI

