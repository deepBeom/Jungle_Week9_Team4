#include "Engine/Input/Controller/ViewportInputController.h"

void IViewportInputController::SetViewportDim(float X, float Y, float Width, float Height)
{
    ViewportX = X;
    ViewportY = Y;
    ViewportWidth = Width;
    ViewportHeight = Height;
}