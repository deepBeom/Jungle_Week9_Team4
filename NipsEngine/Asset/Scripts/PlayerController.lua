local base_mass = 1.0                    -- 배의 최소 질량입니다. 실제 질량은 base_mass + cargo weight 입니다.

local look_speed = 0.001
local min_pitch = -1.4
local max_pitch = 1.4

local boat_forward_accel = 120.0
local boat_reverse_accel = 70.0
local boat_brake_accel = 150.0
local boat_linear_drag = 45.0
local boat_turn_accel = 220.0
local boat_turn_drag = 260.0
local boat_max_forward_speed = 120.0
local boat_max_reverse_speed = 45.0
local boat_max_yaw_speed = 110.0
local boat_min_steer_authority = 0.30
local boat_speed_epsilon = 0.001

local orbit_yaw = nil
local orbit_pitch = nil
local orbit_radius = nil

local function clamp(value, min_value, max_value)
    if value < min_value then
        return min_value
    end

    if value > max_value then
        return max_value
    end

    return value
end

local function get_cargo_weight()
    if GetDriftSalvageWeight then
        return math.max(0.0, GetDriftSalvageWeight())
    end

    return 0.0
end

local function get_effective_mass()
    return math.max(1.0, base_mass + get_cargo_weight())
end

local function get_components()
    if Pawn == nil then
        return nil, nil, nil
    end

    return Pawn:GetRootComponent(), Pawn:GetCharacterComponent(), Pawn:GetCameraComponent()
end

local function is_input_locked()
    if Pawn == nil or IsActorPushed == nil then
        return false
    end

    return IsActorPushed(Pawn)
end

local function get_input_axis(negative_key, positive_key)
    local value = 0.0
    if Input.GetKey(negative_key) then
        value = value - 1.0
    end
    if Input.GetKey(positive_key) then
        value = value + 1.0
    end
    return value
end

function OnUpdate(delta_time)
    if is_input_locked() then
        return
    end

    local root, mesh, camera = get_components()
    if root == nil or mesh == nil or Pawn == nil then
        return
    end

    -- local dt = clamp(delta_time or 0.0, 0.0001, 0.05)
    local throttle_input = get_input_axis("S", "W")
    local steer_input = get_input_axis("A", "D")
    local mass = get_effective_mass()

    Pawn:UpdateBoatMovement(
        delta_time,
        throttle_input,
        steer_input,
        mass,
        boat_forward_accel,
        boat_reverse_accel,
        boat_brake_accel,
        boat_linear_drag,
        boat_turn_accel,
        boat_turn_drag,
        boat_max_forward_speed,
        boat_max_reverse_speed,
        boat_max_yaw_speed,
        boat_min_steer_authority,
        boat_speed_epsilon
    )
end

function OnKeyDown(key)
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
