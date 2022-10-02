#include <wx/wx.h>
#include <vector>

wxDECLARE_EVENT(wxEVT_SORTINGTHREAD_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SORTINGTHREAD_UPDATED, wxThreadEvent);

class SortingThreadCallback
{
public:
    virtual void OnThreadDestruction() = 0;
};

class SortingThread : public wxThread
{
public:
    SortingThread(SortingThreadCallback *callback, wxEvtHandler *handler, std::vector<float> &data, wxCriticalSection &cs) : wxThread(wxTHREAD_JOINABLE), callback(callback), threadEventHandler(handler), sharedData(data), dataCs(cs) {}
    virtual ~SortingThread() { this->callback->OnThreadDestruction(); }

protected:
    virtual ExitCode Entry();

private:
    SortingThreadCallback *callback;
    wxEvtHandler *threadEventHandler;
    std::vector<float> &sharedData;
    wxCriticalSection &dataCs;
};