#include "visualgrid.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

VisualGrid::VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int xSquaresCount, std::vector<float> &v, std::mutex &m) : wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE), xSquaresCount(xSquaresCount), values(v), valuesMutex(m)
{
    this->SetBackgroundStyle(wxBG_STYLE_PAINT);
    this->Bind(wxEVT_PAINT, &VisualGrid::OnPaint, this);
}

void VisualGrid::OnPaint(wxPaintEvent &evt)
{
    wxAutoBufferedPaintDC dc(this);

    wxGraphicsContext *gc = wxGraphicsContext::CreateFromUnknownDC(dc);
    if (gc)
    {
        float squareWidth = static_cast<float>(this->GetSize().GetWidth()) / xSquaresCount;

        std::lock_guard g(valuesMutex);
        for (int i = 0; i < values.size(); i++)
        {
            int colorIntensity = values[i] * 255.0;

            gc->SetBrush(wxBrush(wxColor(0, colorIntensity, colorIntensity)));
            gc->DrawRectangle(i % xSquaresCount * squareWidth,
                              i / xSquaresCount * squareWidth,
                              squareWidth - 1.0,
                              squareWidth - 1.0);
        }

        delete gc;
    }
}
