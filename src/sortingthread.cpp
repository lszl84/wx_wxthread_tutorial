#include "sortingthread.h"

wxThread::ExitCode SortingThread::Entry()
{
    this->callback->DoBackgroundWork();
    return nullptr;
}