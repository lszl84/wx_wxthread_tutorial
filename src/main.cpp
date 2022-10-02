#include <wx/wx.h>
#include <chrono>
#include <atomic>
#include <mutex>

#include <random>

#include "visualgrid.h"
#include "sortingthread.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame, public SortingThreadCallback
{
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    virtual ~MyFrame() { delete refreshTimer; }

private:
    wxButton *button;

    wxGauge *progressBar;
    VisualGrid *grid;

    bool processing{false};
    std::atomic<bool> quitRequested{false};

    std::vector<float> sharedData;
    std::mutex dataMutex;

    SortingThread *backgroundThread;
    void DoBackgroundWork() override;

    void OnButtonClick(wxCommandEvent &e);
    void OnClose(wxCloseEvent &e);

    void BackgroundTask();
    void RandomizeSharedData();

    wxTimer *refreshTimer;
    static constexpr int RefreshTimerId = 6612;
};

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

    grid = new VisualGrid(this, wxID_ANY, wxDefaultPosition, this->FromDIP(wxSize(1000, 800)), 250, sharedData, dataMutex);

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
}

void MyFrame::OnButtonClick(wxCommandEvent &e)
{
    if (!this->processing)
    {
        this->processing = true;

        this->backgroundThread = new SortingThread(this);

        if (this->backgroundThread->Run() != wxTHREAD_NO_ERROR)
        {
            this->SetStatusText("Could not create thread.");
            this->Layout();

            delete this->backgroundThread;

            this->processing = false;
        }
    }
}

void MyFrame::OnClose(wxCloseEvent &e)
{
    this->refreshTimer->Stop();
    if (this->processing)
    {
        e.Veto();

        this->quitRequested = true;
    }
    else
    {
        this->Destroy();
    }
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

void MyFrame::DoBackgroundWork()
{
    this->BackgroundTask();
}

void MyFrame::BackgroundTask()
{
    int n = sharedData.size();
    wxGetApp().CallAfter([this, n]()
                         {
                                     this->SetStatusText(wxString::Format("Sorting the array of %d elements...", n));
                                     this->Layout(); });

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < n - 1; i++)
    {
        wxGetApp().CallAfter([this, n, i]()
                             { this->progressBar->SetValue(i * this->progressBar->GetRange() / (n - 2)); });

        if (this->quitRequested)
        {
            wxGetApp().CallAfter([this]()
                                 {
                                             this->processing = false;
                                             this->quitRequested = false;
                                             this->Destroy(); });
            return;
        }

        std::lock_guard g(dataMutex);
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

    wxGetApp().CallAfter([this, diff]()
                         {
                                     auto frontValue = sharedData.front();
                                     this->SetStatusText(wxString::Format("The first number is: %f. Processing time: %.2f [ms]", frontValue, std::chrono::duration<double, std::milli>(diff).count()));
                                     this->Layout();

                                     this->processing = false; });
}