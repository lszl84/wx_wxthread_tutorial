#include "sortingthread.h"

wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SORTINGTHREAD_UPDATED, wxThreadEvent);

wxThread::ExitCode SortingThread::Entry()
{
    this->callback->DoBackgroundWork();
    return nullptr;
}