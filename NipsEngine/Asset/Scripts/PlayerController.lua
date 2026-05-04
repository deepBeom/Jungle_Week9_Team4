local move_speed = 0.5
local turn_speed = 1.0
local look_speed = 0.01
local min_pitch = -1.4
local max_pitch = 1.4

local function get_components()
    if Pawn == nil then
        return nil, nil, nil
    end

    return Pawn:GetRootComponent(), Pawn:GetCharacterComponent(), Pawn:GetCameraComponent()
end

function OnKeyDown(key)
    local root, mesh, camera = get_components()
    if root == nil or mesh == nil then
        return
    end

    local forward = mesh:GetForwardVector()
    forward.Z = 0.0

    local forwardLength = math.sqrt(forward.X * forward.X + forward.Y * forward.Y)
    if forwardLength <= 0.0001 then
        return
    end

    forward.X = forward.X / forwardLength
    forward.Y = forward.Y / forwardLength

    if key == "W" then root:AddWorldOffset(forward.X * move_speed, forward.Y * move_speed, forward.Z * move_speed) end
    if key == "S" then root:AddWorldOffset(-forward.X * move_speed, -forward.Y * move_speed, -forward.Z * move_speed) end
    if key == "A" then mesh:Rotate(-turn_speed, 0.0) end
    if key == "D" then mesh:Rotate(turn_speed, 0.0) end
end

function OnKeyUp(key)
end

function OnMouseMove(delta_x, delta_y, mouse_x, mouse_y)

    local root, mesh, camera = get_components()
    if root == nil or camera == nil then
        return
    end

    -- TODO: 카메라의 Orbit 회전 추가
end

function OnMouseClick(button, is_pressed, mouse_x, mouse_y)
end
