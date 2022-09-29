#include <vector>
#include <mutex>
#include <wx/wx.h>

class VisualGrid : public wxWindow
{
public:
    VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int xSquaresCount, std::vector<float> &v, std::mutex &m);

private:
    void OnPaint(wxPaintEvent &evt);

    int xSquaresCount;

    std::mutex &valuesMutex;
    std::vector<float> &values;
};