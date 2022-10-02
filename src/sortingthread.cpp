#include "sortingthread.h"

wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_UPDATED, wxThreadEvent);

wxThread::ExitCode SortingThread::Entry()
{
    int n = sharedData.size();

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < n - 1; i++)
    {
        wxThreadEvent *e = new wxThreadEvent(wxEVT_SORTINGTHREAD_UPDATED);
        e->SetPayload<double>(static_cast<double>(i) / static_cast<double>(n - 2));
        wxQueueEvent(threadEventHandler, e);

        if (wxThread::This()->TestDestroy())
        {
            wxThreadEvent *e = new wxThreadEvent(wxEVT_SORTINGTHREAD_COMPLETED);
            e->SetString("Processing aborted.");
            wxQueueEvent(threadEventHandler, e);
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
    wxQueueEvent(threadEventHandler, e);

    return nullptr;
}