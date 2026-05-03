local move_speed = 1.0
local look_speed = 1.0

function OnKeyDown(key)
    if key == "W" then MoveForward(move_speed) end
    if key == "S" then MoveForward(-move_speed) end
    if key == "D" then MoveRight(move_speed) end
    if key == "A" then MoveRight(-move_speed) end
    if key == "E" then MoveUp(move_speed) end
    if key == "Q" then MoveUp(-move_speed) end
end

function OnMouseMove(delta_x, delta_y)
    if IsMouseCaptured() then
        AddYaw(delta_x * look_speed)
        AddPitch(-delta_y * look_speed)
    end
end

function OnMouseClick(button, pressed, local_x, local_y)
    if button == "RightMouseButton" then
        SetMouseCaptured(pressed)
    end
end
