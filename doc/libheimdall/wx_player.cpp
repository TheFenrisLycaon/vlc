// g++ wx_player.cpp `wx-config --libs` `wx-config --cxxflags` `pkg-config --cflags gtk+-2.0 libheimdall` `pkg-config --libs gtk+-2.0 libheimdall` -o wx_player

/* License WTFPL http://sam.zoy.org/wtfpl/ */
/* Written by Vincent Schüßler */

#include <wx/wx.h>
#include <wx/filename.h>
#include <heimdall/heimdall.h>
#include <climits>

#ifdef __WXGTK__
    #include <gdk/gdkx.h>
    #include <gtk/gtk.h>
    #include <wx/gtk/win_gtk.h>
    #define GET_XID(window) GDK_WINDOW_XWINDOW(GTK_PIZZA(window->m_wxwindow)->bin_window)
#endif

#define myID_PLAYPAUSE wxID_HIGHEST+1
#define myID_STOP wxID_HIGHEST+2
#define myID_TIMELINE wxID_HIGHEST+3
#define myID_VOLUME wxID_HIGHEST+4

#define TIMELINE_MAX (INT_MAX-9)
#define VOLUME_MAX 100

DECLARE_EVENT_TYPE(heimdallEVT_END, -1)
DECLARE_EVENT_TYPE(heimdallEVT_POS, -1)
DEFINE_EVENT_TYPE(heimdallEVT_END)
DEFINE_EVENT_TYPE(heimdallEVT_POS)

void OnPositionChanged_Heimdall(void *opaque, libheimdall_time_t time, double pos );
void OnEndReached_Heimdall(void *opaque, libheimdall_state_t state);

class MainWindow : public wxFrame {
    public:
        MainWindow(const wxString& title);
        ~MainWindow();

    private:
        void initHeimdall();

        void OnOpen(wxCommandEvent& event);
        void OnPlayPause(wxCommandEvent& event);
        void OnStop(wxCommandEvent& event);
        void OnPositionChanged_USR(wxCommandEvent& event);
        void OnPositionChanged_Heimdall(wxCommandEvent& event);
        void OnEndReached_Heimdall(wxCommandEvent& event);
        void OnVolumeChanged(wxCommandEvent& event);
        void OnVolumeClicked(wxMouseEvent& event);
        void OnTimelineClicked(wxMouseEvent& event);

        void play();
        void pause();
        void stop();
        void setTimeline(float value);
        void connectTimeline();

        wxButton *playpause_button;
        wxButton *stop_button;
        wxSlider *timeline;
        wxSlider *volume_slider;
        wxWindow *player_widget;

        libheimdall_media_player_t *media_player;
        libheimdall_instance_t *heimdall_inst;
};

MainWindow *mainWindow;

MainWindow::MainWindow(const wxString& title) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition) {
    // setup menubar
    wxMenuBar *menubar;
    wxMenu *file;
    menubar = new wxMenuBar;
    file = new wxMenu;
    file->Append(wxID_OPEN, wxT("&Open"));
    menubar->Append(file, wxT("&File"));
    SetMenuBar(menubar);
    Connect(wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnOpen));

    // setup vbox
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vbox);

    //setup player widget
    player_widget = new wxWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
    player_widget->SetBackgroundColour(wxColour(wxT("black")));
    vbox->Add(player_widget, 1, wxEXPAND | wxALIGN_TOP);

    //setup timeline slider
    timeline = new wxSlider(this, myID_TIMELINE, 0, 0, TIMELINE_MAX);
    timeline->Enable(false);
    vbox->Add(timeline, 0, wxEXPAND);
    connectTimeline();
    timeline->Connect(myID_TIMELINE, wxEVT_LEFT_UP, wxMouseEventHandler(MainWindow::OnTimelineClicked));

    //setup control panel
    wxPanel *controlPanel = new wxPanel(this, wxID_ANY);

    //setup hbox
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    controlPanel->SetSizer(hbox);
    vbox->Add(controlPanel, 0, wxEXPAND);

    //setup controls
    playpause_button = new wxButton(controlPanel, myID_PLAYPAUSE, wxT("Play"));
    stop_button = new wxButton(controlPanel, myID_STOP, wxT("Stop"));
    volume_slider = new wxSlider(controlPanel, myID_VOLUME, VOLUME_MAX, 0, VOLUME_MAX, wxDefaultPosition, wxSize(100, -1));
    playpause_button->Enable(false);
    stop_button->Enable(false);
    hbox->Add(playpause_button);
    hbox->Add(stop_button);
    hbox->AddStretchSpacer();
    hbox->Add(volume_slider);
    Connect(myID_PLAYPAUSE, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainWindow::OnPlayPause));
    Connect(myID_STOP, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainWindow::OnStop));
    Connect(myID_VOLUME, wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(MainWindow::OnVolumeChanged));
    volume_slider->Connect(myID_VOLUME, wxEVT_LEFT_UP, wxMouseEventHandler(MainWindow::OnVolumeClicked));

    static const struct libheimdall_media_player_cbs cbs = {
        .version = 0,
        .on_state_changed = ::OnEndReached_Heimdall,
        .on_position_changed = ::OnPositionChanged_Heimdall,
    };
    //setup heimdall
    heimdall_inst = libheimdall_new(0, NULL);
    media_player = libheimdall_media_player_new(heimdall_inst, &cbs, NULL);
    Connect(wxID_ANY, heimdallEVT_END, wxCommandEventHandler(MainWindow::OnEndReached_Heimdall));
    Connect(wxID_ANY, heimdallEVT_POS, wxCommandEventHandler(MainWindow::OnPositionChanged_Heimdall));

    Show(true);
    initHeimdall();
}

MainWindow::~MainWindow() {
    libheimdall_media_player_release(media_player);
    libheimdall_release(heimdall_inst);
}

void MainWindow::initHeimdall() {
    #ifdef __WXGTK__
        libheimdall_media_player_set_xwindow(media_player, GET_XID(this->player_widget));
    #else
        LONG style = GetWindowLong( player_widget->GetHandle(), GWL_STYLE );
        if( !(style & WS_CLIPCHILDREN) )
            SetWindowLong( player_widget->GetHandle(), GWL_STYLE, style | WS_CLIPCHILDREN );
        libheimdall_media_player_set_hwnd(media_player, this->player_widget->GetHandle());
    #endif
}

void MainWindow::OnOpen(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, wxT("Choose File"));

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    else {
        libheimdall_media_t *media;
        wxFileName filename = wxFileName::FileName(openFileDialog.GetPath());
        filename.MakeRelativeTo();
        media = libheimdall_media_new_path(filename.GetFullPath().mb_str());
        libheimdall_media_player_set_media(media_player, media);
        play();
        libheimdall_media_release(media);
    }
}

void MainWindow::OnPlayPause(wxCommandEvent& event) {
    if(libheimdall_media_player_is_playing(media_player) == 1) {
        pause();
    }
    else {
        play();
    }
}

void MainWindow::OnStop(wxCommandEvent& event) {
    stop();
}

void MainWindow::OnPositionChanged_USR(wxCommandEvent& event) {
    libheimdall_media_player_set_position(media_player, (float) event.GetInt() / (float) TIMELINE_MAX, false);
}

void MainWindow::OnPositionChanged_Heimdall(wxCommandEvent& event) {
    float factor = libheimdall_media_player_get_position(media_player);
    setTimeline(factor);
}

void MainWindow::OnEndReached_Heimdall(wxCommandEvent& event) {
    stop();
}

void MainWindow::OnVolumeChanged(wxCommandEvent& event) {
    libheimdall_audio_set_volume(media_player, volume_slider->GetValue());
}

void MainWindow::OnVolumeClicked(wxMouseEvent& event) {
    wxSize size = mainWindow->volume_slider->GetSize();
    float position = (float) event.GetX() / (float) size.GetWidth();
    mainWindow->volume_slider->SetValue(position*VOLUME_MAX);
    libheimdall_audio_set_volume(mainWindow->media_player, position*VOLUME_MAX);
    event.Skip();
}

void MainWindow::OnTimelineClicked(wxMouseEvent& event) {
    wxSize size = mainWindow->timeline->GetSize();
    float position = (float) event.GetX() / (float) size.GetWidth();
    libheimdall_media_player_set_position(mainWindow->media_player, position, false);
    mainWindow->setTimeline(position);
    event.Skip();
}

void MainWindow::play() {
    libheimdall_media_player_play(media_player);
    playpause_button->SetLabel(wxT("Pause"));
    playpause_button->Enable(true);
    stop_button->Enable(true);
    timeline->Enable(true);
}

void MainWindow::pause() {
    libheimdall_media_player_pause(media_player);
    playpause_button->SetLabel(wxT("Play"));
}

void MainWindow::stop() {
    pause();
    libheimdall_media_player_stop_async(media_player);
    stop_button->Enable(false);
    setTimeline(0.0);
    timeline->Enable(false);
}

void MainWindow::setTimeline(float value) {
    if(value < 0.0) value = 0.0;
    if(value > 1.0) value = 1.0;
    Disconnect(myID_TIMELINE);
    timeline->SetValue((int) (value * TIMELINE_MAX));
    connectTimeline();
}

void MainWindow::connectTimeline() {
    Connect(myID_TIMELINE, wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(MainWindow::OnPositionChanged_USR));
}

class MyApp : public wxApp {
    public:
        virtual bool OnInit();
};

void OnPositionChanged_Heimdall(void *opaque, libheimdall_time_t time, double pos ) {
    wxCommandEvent evt(heimdallEVT_POS, wxID_ANY);
    mainWindow->GetEventHandler()->AddPendingEvent(evt);
}

void OnEndReached_Heimdall(void *opaque, libheimdall_state_t state) {
    if (state == libheimdall_Stopped || state == libheimdall_Error)
    {
        wxCommandEvent evt(heimdallEVT_END, wxID_ANY);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

bool MyApp::OnInit() {
    mainWindow = new MainWindow(wxT("wxWidgets libHeimdall demo"));
    return true;
}

IMPLEMENT_APP(MyApp)
