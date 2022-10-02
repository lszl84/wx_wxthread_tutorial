#include <vector>
#include <wx/wx.h>

class VisualGrid : public wxWindow
{
public:
    VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int xSquaresCount, std::vector<float> &v, wxCriticalSection &cs);

private:
    void OnPaint(wxPaintEvent &evt);

    int xSquaresCount;

    wxCriticalSection &valuesCs;
    std::vector<float> &values;
};