local move_speed = 0.5
local turn_speed = 1.0
local look_speed = 0.001
local min_pitch = -1.4
local max_pitch = 1.4
local orbit_yaw = nil
local orbit_pitch = nil
local orbit_radius = nil

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

    local camera_offset = camera:GetRelativeLocation()
    if orbit_radius == nil or orbit_yaw == nil or orbit_pitch == nil then
        orbit_radius = math.sqrt(
            camera_offset.X * camera_offset.X +
            camera_offset.Y * camera_offset.Y +
            camera_offset.Z * camera_offset.Z
        )
        if orbit_radius < 0.0001 then
            orbit_radius = 1.0
        end

        orbit_yaw = math.atan(camera_offset.Y, camera_offset.X)
        local flat_length = math.sqrt(camera_offset.X * camera_offset.X + camera_offset.Y * camera_offset.Y)
        orbit_pitch = math.atan(camera_offset.Z, flat_length)
    end

    orbit_yaw = orbit_yaw + delta_x * look_speed
    orbit_pitch = math.max(min_pitch, math.min(max_pitch, orbit_pitch + delta_y * look_speed))

    local cos_pitch = math.cos(orbit_pitch)
    local sin_pitch = math.sin(orbit_pitch)
    local cos_yaw = math.cos(orbit_yaw)
    local sin_yaw = math.sin(orbit_yaw)

    local next_x = orbit_radius * cos_pitch * cos_yaw
    local next_y = orbit_radius * cos_pitch * sin_yaw
    local next_z = orbit_radius * sin_pitch

    camera:SetRelativeLocation(next_x, next_y, next_z)

    local pivot = root:GetWorldLocation()
    camera:LookAt(pivot.X, pivot.Y, pivot.Z)
end

function OnMouseClick(button, is_pressed, mouse_x, mouse_y)
end
