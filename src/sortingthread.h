#include <wx/wx.h>

class SortingThreadCallback
{
public:
    virtual void DoBackgroundWork() = 0;
    virtual void OnThreadDestruction() = 0;
};

class SortingThread : public wxThread
{
public:
    SortingThread(SortingThreadCallback *callback) : callback(callback) {}
    virtual ~SortingThread() { this->callback->OnThreadDestruction(); }

protected:
    virtual ExitCode Entry();

private:
    SortingThreadCallback *callback;
};