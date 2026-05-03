-- LogoHud.lua

local ShowHud
local MakeHoverLabel

local NUMBER_COLS = 5
local NUMBER_ROWS = 2
local NUMBER_SHEET = "Asset/Texture/UI/Number.png"
local HUD_BG_SHEET = "Asset/Texture/UI/BG.png"
local SCORE_BG_PATH = "Asset/Texture/UI/ScoreBoardBG.png"
local ENDING_SHEET = "Asset/Texture/UI/Ending.png"

local MONEY_SHEET = "Asset/Texture/UI/Money.png"
local MONEY_ICON_SIZE = 96
local MONEY_FONT_SIZE = 56
local MONEY_TEXT_W = 180
local MONEY_GAP = 10
local SHIP_WHEEL_SHEET = "Asset/Texture/UI/ShipWheel.png"
local SHIP_WHEEL_SIZE = 480
local SHIP_WHEEL_SCREEN_X = 0.5
local SHIP_WHEEL_SCREEN_Y = 1.0
local SHIP_WHEEL_ROT_SPEED = 90.0
local SHIP_WHEEL_RELEASE_KICK = 16.0
local SHIP_WHEEL_RELEASE_DURATION = 0.45

local HEART_SHEET = "Asset/Texture/UI/Heart.png"
local HEART_COLS = 5
local HEART_COUNT = 3
local HEART_ANIM_FPS = 12.0
local HEART_FRAME_W = 17
local HEART_FRAME_H = 17
local HEART_DRAW_SCALE = 3.0
local HEART_DRAW_W = HEART_FRAME_W * HEART_DRAW_SCALE
local HEART_DRAW_H = HEART_FRAME_H * HEART_DRAW_SCALE
local HEART_SPACING = 8
local HEART_ROW_W = HEART_COUNT * HEART_DRAW_W + (HEART_COUNT - 1) * HEART_SPACING
local MONEY_ROW_W = MONEY_ICON_SIZE + MONEY_GAP + MONEY_TEXT_W
local PROGRESS_BG_SHEET = "Asset/Texture/UI/WeightPBBG.png"
local PROGRESS_FILL_SHEET = "Asset/Texture/UI/WeightPB.png"
local PROGRESS_ANCHOR_SHEET = "Asset/Texture/UI/Anchor.png"
local PROGRESS_W = MONEY_ROW_W * 2
local PROGRESS_H = 96
local PROGRESS_ANCHOR_SIZE = 84
local PROGRESS_ANCHOR_OVERLAP = 0
local PROGRESS_GAP_TOP = 10
local PROGRESS_GAP_BOTTOM = 8
local PROGRESS_ANIM_DURATION = 0.35
local DEFAULT_WEIGHT_CAPACITY = 30.0
local INTRO_DURATION = 3.0
local INTRO_START_OFFSET_X = -45.0
local INTRO_START_OFFSET_Y = 0.0
local INTRO_START_OFFSET_Z = -8.0
local MENU_CAMERA_X = -55.763822
local MENU_CAMERA_Y = 0.0
local MENU_CAMERA_Z = 20.132818
local MENU_CAMERA_ROT_X = 0.0
local MENU_CAMERA_ROT_Y = 33.690063
local MENU_CAMERA_ROT_Z = 0.0
local MONEY_Y = HEART_DRAW_H + PROGRESS_GAP_TOP
local PROGRESS_Y = MONEY_Y + MONEY_ICON_SIZE + PROGRESS_GAP_BOTTOM
local INGAME_HUD_W = math.max(HEART_ROW_W, PROGRESS_W, MONEY_ROW_W)
local INGAME_HUD_H = PROGRESS_Y + PROGRESS_H

local MenuPanel = nil
local ScorePanel = nil
local InGamePanel = nil
local GameOverPanel = nil
local ProgressBg = nil
local ProgressFill = nil
local ProgressAnchor = nil
local MoneyText = nil
local ProgressValue = 0.0
local ProgressStartValue = 0.0
local ProgressTargetValue = 0.0
local ProgressAnimTime = PROGRESS_ANIM_DURATION
local ShipWheel = nil
local ShipWheelAngle = 0.0
local ShipWheelLastDir = 0
local ShipWheelKickStartAngle = 0.0
local ShipWheelKickTargetAngle = 0.0
local ShipWheelKickTime = SHIP_WHEEL_RELEASE_DURATION
local HeartImages = {}
local bHeartAnimPlaying = false
local HeartAnimDir = 1
local HeartAnimIndex = 0
local HeartAnimTime = 0.0
local HeartAnimFrame = 0
local HeartHealth = HEART_COUNT
local BoatActor = nil
local BoatHomePosition = nil
local BoatHomeRotation = nil
local bIntroPlaying = false
local IntroTime = 0.0

local function DigitUV(d)
    local digit = math.max(0, math.min(9, math.floor(d)))
    local col = digit % NUMBER_COLS
    local row = math.floor(digit / NUMBER_COLS)

    return col / NUMBER_COLS, row / NUMBER_ROWS,
           (col + 1) / NUMBER_COLS, (row + 1) / NUMBER_ROWS
end

local function HeartUV(index)
    local col = math.max(0, math.min(HEART_COLS - 1, index or 0))
    return col / HEART_COLS, 0.0, (col + 1) / HEART_COLS, 1.0
end

local function Clamp01(value)
    return math.max(0.0, math.min(1.0, value or 0.0))
end

local function GetHudHealth()
    if GetDriftSalvageHealth then
        return GetDriftSalvageHealth()
    end

    return HEART_COUNT
end

local function GetHudMoney()
    if GetDriftSalvageMoney then
        return GetDriftSalvageMoney()
    end

    return 0
end

local function GetHudWeight()
    if GetDriftSalvageWeight then
        return GetDriftSalvageWeight()
    end

    return 0.0
end

local function GetHudWeightCapacity()
    if GetDriftSalvageWeightCapacity then
        return math.max(0.001, GetDriftSalvageWeightCapacity())
    end

    return DEFAULT_WEIGHT_CAPACITY
end

local function EaseSmoothStep(t)
    t = Clamp01(t)
    return t * t * (3.0 - 2.0 * t)
end

local function Lerp(a, b, t)
    return a + (b - a) * t
end

local function ResolveBoatActor()
    if BoatActor and BoatActor:IsValid() then
        return BoatActor
    end

    if FindActorByTag then
        BoatActor = FindActorByTag("Boat")
    end

    if BoatActor and BoatActor:IsValid() and not BoatHomePosition then
        BoatHomePosition = BoatActor:GetPosition()
        BoatHomeRotation = BoatActor:GetRotation()
    end

    return BoatActor
end

local function SetMenuCamera()
    if SetGameplayCameraFollowEnabled then
        SetGameplayCameraFollowEnabled(false)
    end

    if SetGameplayCameraTransformValues then
        SetGameplayCameraTransformValues(
            MENU_CAMERA_X,
            MENU_CAMERA_Y,
            MENU_CAMERA_Z,
            MENU_CAMERA_ROT_X,
            MENU_CAMERA_ROT_Y,
            MENU_CAMERA_ROT_Z)
    end
end

local function SetBoatAtHome()
    local boat = ResolveBoatActor()
    if not boat then return end

    if BoatHomePosition then
        boat:SetPosition(BoatHomePosition.X, BoatHomePosition.Y, BoatHomePosition.Z)
    end

    if BoatHomeRotation then
        boat:SetRotation(BoatHomeRotation.X, BoatHomeRotation.Y, BoatHomeRotation.Z)
    end
end

local function GetBoatIntroStartPosition()
    if not BoatHomePosition then
        return nil, nil, nil
    end

    return
        BoatHomePosition.X + INTRO_START_OFFSET_X,
        BoatHomePosition.Y + INTRO_START_OFFSET_Y,
        BoatHomePosition.Z + INTRO_START_OFFSET_Z
end

local function SetBoatAtIntroStart()
    local boat = ResolveBoatActor()
    local startX, startY, startZ = GetBoatIntroStartPosition()
    if not boat or not startX then return end

    boat:SetPosition(startX, startY, startZ)

    if BoatHomeRotation then
        boat:SetRotation(BoatHomeRotation.X, BoatHomeRotation.Y, BoatHomeRotation.Z)
    end
end

local function PrepareMenuPresentation()
    SetGameplayInputEnabled(false)
    SetMenuCamera()
    bIntroPlaying = false
    IntroTime = 0.0
    SetBoatAtHome()
    SetBoatAtIntroStart()
end

local function GetHeartAnimFrame(time, direction)
    local duration = HEART_COLS / HEART_ANIM_FPS
    local ratio = EaseSmoothStep(time / duration)
    local frame = math.floor(ratio * (HEART_COLS - 1) + 0.5)

    if direction < 0 then
        return HEART_COLS - 1 - frame
    end

    return frame
end

local function ApplyHeartFrame(index, frame)
    local heart = HeartImages[index]
    if not heart then return end

    local u1, v1, u2, v2 = HeartUV(frame)
    heart:SetUV(u1, v1, u2, v2)
end

local function ApplyHeartHealth(health)
    local clamped = math.max(0, math.min(HEART_COUNT, math.floor((health or 0) + 0.5)))
    HeartHealth = clamped
    bHeartAnimPlaying = false
    HeartAnimIndex = 0

    for i = 1, HEART_COUNT do
        ApplyHeartFrame(i, i <= clamped and 0 or HEART_COLS - 1)
    end
end

local function SetProgressTarget(value)
    ProgressStartValue = ProgressValue
    ProgressTargetValue = Clamp01(value)
    ProgressAnimTime = 0.0
end

local function ApplyProgressValue(value)
    ProgressValue = Clamp01(value)

    if ProgressFill then
        ProgressFill:SetSize(PROGRESS_W * ProgressValue, PROGRESS_H)
        ProgressFill:SetUV(0.0, 0.0, ProgressValue, 1.0)
        ProgressFill:SetVisible(ProgressValue > 0.0)
    end
end

local function UpdateProgressBar(deltaTime)
    if not ProgressFill or ProgressAnimTime >= PROGRESS_ANIM_DURATION then return end

    ProgressAnimTime = math.min(PROGRESS_ANIM_DURATION, ProgressAnimTime + (deltaTime or 0.0))
    local ratio = EaseSmoothStep(ProgressAnimTime / PROGRESS_ANIM_DURATION)

    ApplyProgressValue(ProgressStartValue + (ProgressTargetValue - ProgressStartValue) * ratio)
end

local function SyncGameplayHud()
    if not InGamePanel then return end

    if MoneyText then
        MoneyText:SetText(tostring(GetHudMoney()) .. "$")
    end

    local targetWeightRatio = Clamp01(GetHudWeight() / GetHudWeightCapacity())
    if math.abs(targetWeightRatio - ProgressTargetValue) > 0.001 then
        SetProgressTarget(targetWeightRatio)
    end

    local health = GetHudHealth()
    if health ~= HeartHealth then
        ApplyHeartHealth(health)
    end
end

local function UpdateShipWheel(deltaTime)
    if not ShipWheel or not Input then return end
    if IsGameplayInputEnabled and not IsGameplayInputEnabled() then return end

    local dt = deltaTime or 0.0
    local dir = 0
    if Input.GetKey("Left") or Input.GetKey("A") then
        dir = dir - 1
    end
    if Input.GetKey("Right") or Input.GetKey("D") then
        dir = dir + 1
    end

    if dir ~= 0 then
        ShipWheelAngle = (ShipWheelAngle + dir * SHIP_WHEEL_ROT_SPEED * dt) % 360.0
        ShipWheelLastDir = dir
        ShipWheelKickTime = SHIP_WHEEL_RELEASE_DURATION
    elseif ShipWheelLastDir ~= 0 and ShipWheelKickTime >= SHIP_WHEEL_RELEASE_DURATION then
        ShipWheelKickStartAngle = ShipWheelAngle
        ShipWheelKickTargetAngle = ShipWheelAngle - ShipWheelLastDir * SHIP_WHEEL_RELEASE_KICK
        ShipWheelKickTime = 0.0
        ShipWheelLastDir = 0
    elseif ShipWheelKickTime < SHIP_WHEEL_RELEASE_DURATION then
        ShipWheelKickTime = math.min(SHIP_WHEEL_RELEASE_DURATION, ShipWheelKickTime + dt)
        local ratio = EaseSmoothStep(ShipWheelKickTime / SHIP_WHEEL_RELEASE_DURATION)
        ShipWheelAngle = (ShipWheelKickStartAngle + (ShipWheelKickTargetAngle - ShipWheelKickStartAngle) * ratio) % 360.0
    end

    ShipWheel:SetRotation(ShipWheelAngle)
end

local function HideInGameHud()
    SetGameplayInputEnabled(false)

    if InGamePanel then
        UIManager.DestroyElement(InGamePanel)
        InGamePanel = nil
        ProgressBg = nil
        ProgressFill = nil
        ProgressAnchor = nil
        MoneyText = nil
        ProgressValue = 0.0
        ProgressStartValue = 0.0
        ProgressTargetValue = 0.0
        ProgressAnimTime = PROGRESS_ANIM_DURATION
        HeartImages = {}
        bHeartAnimPlaying = false
        HeartAnimDir = 1
        HeartAnimIndex = 0
        HeartAnimTime = 0.0
        HeartAnimFrame = 0
        HeartHealth = HEART_COUNT
    end

    if ShipWheel then
        UIManager.DestroyElement(ShipWheel)
        ShipWheel = nil
        ShipWheelAngle = 0.0
        ShipWheelLastDir = 0
        ShipWheelKickTime = SHIP_WHEEL_RELEASE_DURATION
    end
end

local function BeginBoatIntro()
    SetGameplayInputEnabled(false)
    SetMenuCamera()

    if ResetDriftSalvageStats then
        ResetDriftSalvageStats()
    end

    local boat = ResolveBoatActor()
    if boat and BoatHomePosition then
        local startX, startY, startZ = GetBoatIntroStartPosition()
        if startX then
            boat:SetPosition(startX, startY, startZ)
        end

        if BoatHomeRotation then
            boat:SetRotation(BoatHomeRotation.X, BoatHomeRotation.Y, BoatHomeRotation.Z)
        end
    end

    bIntroPlaying = true
    IntroTime = 0.0
end

local function UpdateBoatIntro(deltaTime)
    if not bIntroPlaying then return end

    local boat = ResolveBoatActor()
    if not boat or not BoatHomePosition then
        bIntroPlaying = false
        if SetGameplayCameraFollowEnabled then
            SetGameplayCameraFollowEnabled(true)
        end
        SetGameplayInputEnabled(true)
        return
    end

    IntroTime = math.min(INTRO_DURATION, IntroTime + (deltaTime or 0.0))
    local ratio = EaseSmoothStep(IntroTime / INTRO_DURATION)
    local startX, startY, startZ = GetBoatIntroStartPosition()
    if not startX then
        bIntroPlaying = false
        return
    end

    boat:SetPosition(
        Lerp(startX, BoatHomePosition.X, ratio),
        Lerp(startY, BoatHomePosition.Y, ratio),
        Lerp(startZ, BoatHomePosition.Z, ratio))
    SetMenuCamera()

    if IntroTime >= INTRO_DURATION then
        bIntroPlaying = false
        SetBoatAtHome()
        if SetGameplayCameraFollowEnabled then
            SetGameplayCameraFollowEnabled(true)
        end
        SetGameplayInputEnabled(true)
    end
end

local function ShowInGameHud()
    if InGamePanel then return end

    ProgressValue = Clamp01(GetHudWeight() / GetHudWeightCapacity())
    ProgressStartValue = ProgressValue
    ProgressTargetValue = ProgressValue
    ProgressAnimTime = PROGRESS_ANIM_DURATION

    InGamePanel = UIManager.CreateImage(nil, 20, 20, INGAME_HUD_W, INGAME_HUD_H, nil, nil)
    InGamePanel:SetColor(0.0, 0.0, 0.0, 0.0)

    HeartImages = {}
    local u1, v1, u2, v2 = HeartUV(0)

    for i = 1, HEART_COUNT do
        local x = (i - 1) * (HEART_DRAW_W + HEART_SPACING)
        local heart = UIManager.CreateImage(InGamePanel, x, 0, HEART_DRAW_W, HEART_DRAW_H, HEART_SHEET, nil)
        heart:SetUV(u1, v1, u2, v2)
        heart:SetColor(1.6, 1.6, 1.6, 1.0)
        heart:SetVisible(true)
        HeartImages[i] = heart
    end

    ProgressBg = UIManager.CreateImage(InGamePanel, 0, PROGRESS_Y, PROGRESS_W, PROGRESS_H, PROGRESS_BG_SHEET, nil)
    ProgressBg:SetColor(1.0, 1.0, 1.0, 1.0)

    ProgressFill = UIManager.CreateImage(InGamePanel, 0, PROGRESS_Y, PROGRESS_W, PROGRESS_H, PROGRESS_FILL_SHEET, nil)
    ProgressFill:SetColor(1.0, 1.0, 1.0, 1.0)
    ApplyProgressValue(ProgressValue)

    local anchorX = -PROGRESS_ANCHOR_OVERLAP
    local anchorY = PROGRESS_Y + (PROGRESS_H - PROGRESS_ANCHOR_SIZE) * 0.5
    ProgressAnchor = UIManager.CreateImage(InGamePanel, anchorX, anchorY, PROGRESS_ANCHOR_SIZE, PROGRESS_ANCHOR_SIZE, PROGRESS_ANCHOR_SHEET, nil)
    ProgressAnchor:SetColor(1.0, 1.0, 1.0, 1.0)

    local moneyIcon = UIManager.CreateImage(InGamePanel, 0, MONEY_Y, MONEY_ICON_SIZE, MONEY_ICON_SIZE, MONEY_SHEET, nil)
    moneyIcon:SetColor(1.25, 1.25, 1.25, 1.0)

    local moneyTextY = MONEY_Y + (MONEY_ICON_SIZE - MONEY_FONT_SIZE) * 0.5
    MoneyText = UIManager.CreateText(InGamePanel, MONEY_ICON_SIZE + MONEY_GAP, moneyTextY, MONEY_TEXT_W, MONEY_FONT_SIZE, tostring(GetHudMoney()) .. "$", MONEY_FONT_SIZE, nil)
    MoneyText:SetColor(1.0, 0.95, 0.25, 1.0)

    ShipWheel = UIManager.CreateImage(nil, SHIP_WHEEL_SCREEN_X, SHIP_WHEEL_SCREEN_Y, SHIP_WHEEL_SIZE, SHIP_WHEEL_SIZE, SHIP_WHEEL_SHEET, "RelativePos")
    ShipWheel:SetColor(1.0, 1.0, 1.0, 1.0)
    ShipWheel:SetRotation(ShipWheelAngle)

    bHeartAnimPlaying = false
    HeartAnimDir = 1
    HeartAnimIndex = 0
    HeartAnimTime = 0.0
    HeartAnimFrame = 0
    ApplyHeartHealth(GetHudHealth())
    SyncGameplayHud()
end

local function HideScoreboard()
    if ScorePanel then
        UIManager.DestroyElement(ScorePanel)
        ScorePanel = nil
    end
end

local function HideGameOver()
    if GameOverPanel then
        UIManager.DestroyElement(GameOverPanel)
        GameOverPanel = nil
    end
end

local function ShowScoreboard(bestScore)
    if ScorePanel then return end

    ScorePanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.6, 0.5, SCORE_BG_PATH, "FullRelative")
    ScorePanel:SetColor(1.0, 1.0, 1.0, 1.0)

    local title = UIManager.CreateText(ScorePanel, -0.005, -0.195, 300, 50, "BEST SCORE", 32.0, "RelativePos")
    title:SetColor(0.0, 0.0, 0.0, 1.0)

    local digitList = {}
    local n = math.max(0, math.floor(bestScore or 0))
    if n == 0 then
        digitList = { 0 }
    else
        while n > 0 do
            table.insert(digitList, 1, n % 10)
            n = math.floor(n / 10)
        end
    end

    local digitW = 0.08
    local digitH = 0.25
    local spacing = 0.085
    local startX = -((#digitList - 1) * spacing) * 0.5

    for i, digit in ipairs(digitList) do
        local x = startX + (i - 1) * spacing
        local img = UIManager.CreateImage(ScorePanel, x, 0.0, digitW, digitH, NUMBER_SHEET, "ParentRelative")
        img:SetUV(DigitUV(digit))
    end

    local closeBtn = UIManager.CreateText(ScorePanel, 0.0, 0.17, 120, 28, "CLOSE", 28.0, "RelativePos")
    closeBtn:SetColor(0, 0, 0, 1.0)
    closeBtn:SetInteractable(true)
    closeBtn:OnHoverEnter(function() closeBtn:SetColor(1.0, 1.0, 0.0, 1.0) end)
    closeBtn:OnHoverExit(function() closeBtn:SetColor(0.0, 0.0, 0.0, 1.0) end)
    closeBtn:OnClick(function()
        HideScoreboard()
        ShowHud()
    end)
end

local function ShowGameOver()
    if GameOverPanel then return end

    SetGameplayInputEnabled(false)
    local finalScore = GetHudMoney()
    HideInGameHud()

    GameOverPanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.55, 0.55, nil, "FullRelative")
    GameOverPanel:SetColor(0.0, 0.0, 0.0, 0.0)

    local gameOverBg = UIManager.CreateImage(GameOverPanel, 0.0, 0.0, 0.75, 1.5, HUD_BG_SHEET, "ParentRelative")
    gameOverBg:SetColor(1.0, 1.0, 1.0, 1.0)

    local endingImage = UIManager.CreateImage(GameOverPanel, 0.0, -0.34, 0.46, 0.42, ENDING_SHEET, "ParentRelative")
    endingImage:SetColor(1.0, 1.0, 1.0, 1.0)

    local scoreText = UIManager.CreateText(GameOverPanel, -0.03, 0.0, 400, 72, "SCORE : " .. tostring(finalScore), 48.0, "RelativePos")
    scoreText:SetColor(0.0, 0.0, 0.0, 1.0)

    local restartLabel = UIManager.CreateText(GameOverPanel, -0.01, 0.15, 280, 48, "RESTART", 48.0, "RelativePos")
    restartLabel:SetColor(0.0, 0.0, 0.0, 1.0)
    restartLabel:SetInteractable(true)
    restartLabel:OnHoverEnter(function() restartLabel:SetColor(1.0, 1.0, 0.0, 1.0) end)
    restartLabel:OnHoverExit(function() restartLabel:SetColor(0.0, 0.0, 0.0, 1.0) end)
    restartLabel:OnClick(function()
        RequestGameRestart()
    end)

    local titleLabel = UIManager.CreateText(GameOverPanel, 0.01, 0.27, 280, 48, "TITLE", 48.0, "RelativePos")
    titleLabel:SetColor(0.0, 0.0, 0.0, 1.0)
    titleLabel:SetInteractable(true)
    titleLabel:OnHoverEnter(function() titleLabel:SetColor(1.0, 1.0, 0.0, 1.0) end)
    titleLabel:OnHoverExit(function() titleLabel:SetColor(0.0, 0.0, 0.0, 1.0) end)
    titleLabel:OnClick(function()
        RequestGameRestart()
    end)
end

MakeHoverLabel = function(parent, x, y, w, h, text, fontSize, mode)
    local label = UIManager.CreateText(parent, x, y, w, h, text, fontSize, mode)
    label:SetInteractable(true)
    label:OnHoverEnter(function() label:SetColor(1.0, 1.0, 0.0, 1.0) end)
    label:OnHoverExit(function() label:SetColor(0.0, 0.0, 0.0, 1.0) end)
    return label
end

local function HideHud()
    if MenuPanel then
        UIManager.DestroyElement(MenuPanel)
        MenuPanel = nil
    end
end

ShowHud = function()
    if MenuPanel then return end

    HideInGameHud()
    HideGameOver()

    MenuPanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.4, 0.5, nil, "FullRelative")
    MenuPanel:SetColor(0.0, 0.0, 0.0, 0.0)

    local menuBg = UIManager.CreateImage(MenuPanel, 0.0, 0.0, 1.0, 1.5, HUD_BG_SHEET, "ParentRelative")
    menuBg:SetColor(1.0, 1.0, 1.0, 1.0)

    UIManager.CreateImage(MenuPanel, 0.0, -0.3, 0.8, 0.3, "Asset/Texture/UI/Logo.png", "ParentRelative")
    UIManager.CreateImage(MenuPanel, -0.25, 0.1, 0.15, 0.25, "Asset/Texture/UI/Icon_boat.png", "ParentRelative")

    local StartLabel = MakeHoverLabel(MenuPanel, 0.025, 0.03, 256, 28, "START", 48.0, "RelativePos")
    StartLabel:OnClick(function()
        HideHud()
        ShowInGameHud()
        local ok, err = pcall(BeginBoatIntro)
        if not ok then
            if Log then
                Log("BeginBoatIntro failed: " .. tostring(err))
            end
            SetGameplayInputEnabled(true)
        end
    end)

    UIManager.CreateImage(MenuPanel, -0.25, 0.4, 0.15, 0.25, "Asset/Texture/UI/Icon_book.png", "ParentRelative")

    local RecordLabel = MakeHoverLabel(MenuPanel, 0.025, 0.18, 256, 28, "RECORD", 48.0, "RelativePos")
    RecordLabel:OnClick(function()
        HideHud()
        ShowScoreboard(1557)
    end)

    local ok, err = pcall(PrepareMenuPresentation)
    if not ok and Log then
        Log("PrepareMenuPresentation failed: " .. tostring(err))
    end
end

function OnStart(self)
    ShowHud()
end

function OnUpdate(self, deltaTime)
    if InGamePanel and Input and Input.GetKeyDown("E") then
        ShowGameOver()
        return
    end

    UpdateBoatIntro(deltaTime)
    SyncGameplayHud()
    UpdateProgressBar(deltaTime)
    UpdateShipWheel(deltaTime)

    if bHeartAnimPlaying then
        HeartAnimTime = HeartAnimTime + (deltaTime or 0.0)
        local duration = HEART_COLS / HEART_ANIM_FPS
        local nextFrame = GetHeartAnimFrame(HeartAnimTime, HeartAnimDir)

        if HeartAnimTime >= duration then
            bHeartAnimPlaying = false

            if HeartAnimDir > 0 then
                HeartAnimFrame = HEART_COLS - 1
                HeartHealth = math.max(0, HeartHealth - 1)
            else
                HeartAnimFrame = 0
                HeartHealth = math.min(HEART_COUNT, HeartHealth + 1)
            end

            ApplyHeartFrame(HeartAnimIndex, HeartAnimFrame)
            HeartAnimIndex = 0
        elseif nextFrame ~= HeartAnimFrame then
            HeartAnimFrame = nextFrame
            ApplyHeartFrame(HeartAnimIndex, HeartAnimFrame)
        end
    end

    if InGamePanel and not bIntroPlaying and GetHudHealth() <= 0 then
        ShowGameOver()
    end
end

function OnDestroy(self)
    SetGameplayInputEnabled(false)
    SetGameplayCameraFollowEnabled(false)
    HideHud()
    HideScoreboard()
    HideGameOver()
    HideInGameHud()
end
