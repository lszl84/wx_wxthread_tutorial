#include <wx/wx.h>

class SortingThreadCallback
{
public:
    virtual void DoBackgroundWork() = 0;
};

class SortingThread : public wxThread
{
public:
    SortingThread(SortingThreadCallback *callback) : callback(callback) {}
    virtual ~SortingThread() {}

protected:
    virtual ExitCode Entry();

private:
    SortingThreadCallback *callback;
};