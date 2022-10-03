#include <wx/wx.h>
#include <atomic>
#include <chrono>
#include <random>

#include "visualgrid.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

wxDECLARE_EVENT(wxEVT_SORTINGTHREAD_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SORTINGTHREAD_CANCELLED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SORTINGTHREAD_UPDATED, wxThreadEvent);

class MyFrame : public wxFrame, public wxThreadHelper
{
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    virtual ~MyFrame() { delete refreshTimer; }

private:
    wxButton *button;

    wxGauge *progressBar;
    VisualGrid *grid;

    bool processing{false};

    std::vector<float> sharedData;
    wxCriticalSection dataCs;

    virtual wxThread::ExitCode Entry();

    void OnThreadUpdate(wxThreadEvent &);
    void OnThreadCompletion(wxThreadEvent &);
    void OnThreadCancel(wxThreadEvent &);

    void OnButtonClick(wxCommandEvent &e);
    void OnClose(wxCloseEvent &e);

    void RandomizeSharedData();

    wxTimer *refreshTimer;
    static constexpr int RefreshTimerId = 6612;
};

wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_CANCELLED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_UPDATED, wxThreadEvent);

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame("Hello World", wxPoint(300, 95), wxDefaultSize);
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size), sharedData(50000)
{
    RandomizeSharedData();
    this->CreateStatusBar(1);

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    wxSizer *panelSizer = new wxBoxSizer(wxHORIZONTAL);

    button = new wxButton(panel, wxID_ANY, "Start");
    button->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);

    progressBar = new wxGauge(panel, wxID_ANY, 1000, wxDefaultPosition, this->FromDIP(wxSize(320, 20)));

    panelSizer->Add(button, 0, wxALIGN_CENTER | wxRIGHT, this->FromDIP(5));
    panelSizer->Add(progressBar, 0, wxALIGN_CENTER);

    grid = new VisualGrid(this, wxID_ANY, wxDefaultPosition, this->FromDIP(wxSize(1000, 800)), 250, sharedData, dataCs);

    panel->SetSizer(panelSizer);

    sizer->Add(panel, 0, wxEXPAND | wxALL, this->FromDIP(5));
    sizer->Add(grid, 0, wxCENTER | wxLEFT | wxRIGHT | wxBOTTOM, this->FromDIP(5));

    this->SetSizerAndFit(sizer);

    this->Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

    this->SetStatusText("Click Start.", 0);

    this->refreshTimer = new wxTimer(this, RefreshTimerId);
    this->Bind(
        wxEVT_TIMER, [this](wxTimerEvent &)
        { this->grid->Refresh(); },
        RefreshTimerId);

    this->refreshTimer->Start(150);

    this->Bind(wxEVT_SORTINGTHREAD_UPDATED, &MyFrame::OnThreadUpdate, this);
    this->Bind(wxEVT_SORTINGTHREAD_COMPLETED, &MyFrame::OnThreadCompletion, this);
    this->Bind(wxEVT_SORTINGTHREAD_CANCELLED, &MyFrame::OnThreadCancel, this);
}

void MyFrame::OnButtonClick(wxCommandEvent &e)
{
    if (!this->processing)
    {
        this->processing = true;

        if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR ||
            GetThread()->Run() != wxTHREAD_NO_ERROR)
        {
            this->SetStatusText("Could not create thread.");

            this->processing = false;
        }
        else
        {
            this->SetStatusText(wxString::Format("Sorting the array of %zu elements...", this->sharedData.size()));
        }

        this->Layout();
    }
}

wxThread::ExitCode MyFrame::Entry()
{
    int n = sharedData.size();

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < n - 1; i++)
    {
        wxThreadEvent *e = new wxThreadEvent(wxEVT_SORTINGTHREAD_UPDATED);
        e->SetPayload<double>(static_cast<double>(i) / static_cast<double>(n - 2));
        wxQueueEvent(this, e);

        if (wxThread::This()->TestDestroy())
        {
            wxThreadEvent *e = new wxThreadEvent(wxEVT_SORTINGTHREAD_CANCELLED);
            e->SetString("Processing aborted.");
            wxQueueEvent(this, e);
            return nullptr;
        }

        wxCriticalSectionLocker lock(dataCs);
        for (int j = 0; j < n - i - 1; j++)
        {
            if (sharedData[j] > sharedData[j + 1])
            {
                std::swap(sharedData[j], sharedData[j + 1]);
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;

    auto frontValue = sharedData.front();

    wxThreadEvent *e = new wxThreadEvent(wxEVT_SORTINGTHREAD_COMPLETED);
    e->SetString(wxString::Format("The first number is: %f. Processing time: %.2f [ms]", frontValue, std::chrono::duration<double, std::milli>(diff).count()));
    wxQueueEvent(this, e);

    return nullptr;
}

void MyFrame::OnClose(wxCloseEvent &e)
{
    this->refreshTimer->Stop();

    if (GetThread() && GetThread()->IsRunning())
    {
        GetThread()->Delete();
    }

    this->Destroy();
}

void MyFrame::RandomizeSharedData()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(0, 1);

    for (int i = 0; i < sharedData.size(); i++)
    {
        sharedData[i] = distr(gen);
    }
}

void MyFrame::OnThreadUpdate(wxThreadEvent &e)
{
    double progressFraction = e.GetPayload<double>();
    this->progressBar->SetValue(progressFraction * this->progressBar->GetRange());
}

void MyFrame::OnThreadCompletion(wxThreadEvent &e)
{
    this->SetStatusText(e.GetString());
    this->Layout();

    GetThread()->Wait();

    this->processing = false;
}

void MyFrame::OnThreadCancel(wxThreadEvent &e)
{
    this->SetStatusText(e.GetString());
    this->Layout();

    this->processing = false;
}