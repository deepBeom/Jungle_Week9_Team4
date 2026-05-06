-- DriftSalvageHud.lua

local ShowHud

local SCRIPT_DIR = "Asset/Scripts/"
local HUD_BG_SHEET = "Asset/Texture/UI/BG.png"
local SCORE_BG_PATH = "Asset/Texture/UI/ScoreBoardBG.png"
local ENDING_SHEET = "Asset/Texture/UI/Ending.png"
local SCORE_MANAGER_PATH = SCRIPT_DIR .. "ScoreManager.lua"
local DEFAULT_WEIGHT_CAPACITY = 150.0

local MenuPanel = nil
local ScorePanel = nil
local GameOverPanel = nil
local BoatActor = nil
local BoatHomePosition = nil
local BoatHomeRotation = nil
local ScoreManager = nil
local GameplayHud = nil

local function LoadScript(path)
    local paths = {
        path,
        "NipsEngine/" .. path,
    }

    local lastError = nil
    for _, candidate in ipairs(paths) do
        local ok, result = pcall(dofile, candidate)
        if ok and result then
            return result
        end
        lastError = result
    end

    error("Lua script load failed: " .. tostring(path) .. " (" .. tostring(lastError) .. ")")
end

local function LoadScoreManager()
    if ScoreManager then
        return ScoreManager
    end

    local ok, managerOrError = pcall(LoadScript, SCORE_MANAGER_PATH)
    if ok and managerOrError then
        ScoreManager = managerOrError
        return ScoreManager
    end

    if Log then
        Log("ScoreManager load failed: " .. tostring(managerOrError))
    end

    ScoreManager = {
        GetRecords = function()
            return { 0, 0, 0 }
        end,
        RecordScore = function(score)
            local records = { 0, math.max(0, math.floor(score or 0)), 0 }
            table.sort(records, function(a, b) return a > b end)
            return records
        end,
    }
    return ScoreManager
end

local function Clamp01(value)
    return math.max(0.0, math.min(1.0, value or 0.0))
end

local function EaseSmoothStep(t)
    t = Clamp01(t)
    return t * t * (3.0 - 2.0 * t)
end

local function EnterUIMode()
    if SetUIMode then
        SetUIMode(true)
    end
end

local function EnterGameplayMode()
    if SetUIMode then
        SetUIMode(false)
    end
end

local function GetHudHealth()
    if GetDriftSalvageHealth then
        return GetDriftSalvageHealth()
    end

    return 5
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

local function GetActorYawDegrees(actor)
    if not actor then
        return 0.0
    end

    if actor.GetForwardVector then
        local forward = actor:GetForwardVector()
        if forward and (math.abs(forward.X or 0.0) > 0.0001 or math.abs(forward.Y or 0.0) > 0.0001) then
            return math.deg(math.atan(forward.Y or 0.0, forward.X or 1.0))
        end
    end

    local rot = actor:GetRotation()
    if not rot then
        return 0.0
    end

    return rot.Z or rot.Y or 0.0
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

local function PrepareMenuPresentation()
    EnterUIMode()
    SetBoatAtHome()
end

local function CreateCenteredText(parent, x, y, text, fontSize)
    local value = tostring(text or "")
    local size = fontSize or 16.0
    return UIManager.CreateText(parent, x, y, math.max(size, #value * size), size, value, size, "Centered")
end

local function MakeHoverLabel(parent, x, y, w, h, text, fontSize, mode)
    local label = UIManager.CreateText(parent, x, y, w, h, text, fontSize, mode)
    label:SetInteractable(true)
    label:OnHoverEnter(function() label:SetColor(1.0, 1.0, 0.0, 1.0) end)
    label:OnHoverExit(function() label:SetColor(0.0, 0.0, 0.0, 1.0) end)
    return label
end

local function GetGameplayHud()
    if GameplayHud then
        return GameplayHud
    end

    local MinimapModule = LoadScript(SCRIPT_DIR .. "DriftSalvageMinimap.lua")
    local GameplayHudModule = LoadScript(SCRIPT_DIR .. "DriftSalvageGameplayHud.lua")
    local minimap = MinimapModule.Create({
        ResolveBoatActor = ResolveBoatActor,
        GetActorYawDegrees = GetActorYawDegrees,
    })

    GameplayHud = GameplayHudModule.Create({
        Minimap = minimap,
        Clamp01 = Clamp01,
        EaseSmoothStep = EaseSmoothStep,
        EnterUIMode = EnterUIMode,
        GetMoney = GetHudMoney,
        GetWeight = GetHudWeight,
        GetWeightCapacity = GetHudWeightCapacity,
        GetHealth = GetHudHealth,
    })

    return GameplayHud
end

local function HideHud()
    if MenuPanel then
        UIManager.DestroyElement(MenuPanel)
        MenuPanel = nil
    end
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

local function HideInGameHud()
    if GameplayHud then
        GameplayHud:Hide()
    end
end

local function ShowInGameHud()
    GetGameplayHud():Show()
end

local function ShowScoreboard(records)
    if ScorePanel then return end

    ScorePanel = UIManager.CreateImage(nil, 0.5, 0.5, 448, 558, SCORE_BG_PATH, "RelativePos")
    ScorePanel:SetColor(1.0, 1.0, 1.0, 1.0)

    local title = CreateCenteredText(ScorePanel, 0, -186, "RECORD", 36.0)
    title:SetColor(0.0, 0.0, 0.0, 1.0)

    if type(records) ~= "table" then
        records = LoadScoreManager().GetRecords()
    end

    local rowY = { -100, -5, 90 }
    for rank = 1, 3 do
        local scoreText = CreateCenteredText(ScorePanel, 20, rowY[rank], records[rank] or 0, 60.0)
        scoreText:SetColor(0.0, 0.0, 0.0, 1.0)
    end

    local closeBtn = CreateCenteredText(ScorePanel, 0, 226, "CLOSE", 30.0)
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

    EnterUIMode()
    local finalScore = GetHudMoney()
    LoadScoreManager().RecordScore(finalScore)
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
        _G.DriftSalvageHudNextStartMode = "Gameplay"
        RequestGameRestart()
    end)

    local titleLabel = UIManager.CreateText(GameOverPanel, 0.01, 0.27, 280, 48, "TITLE", 48.0, "RelativePos")
    titleLabel:SetColor(0.0, 0.0, 0.0, 1.0)
    titleLabel:SetInteractable(true)
    titleLabel:OnHoverEnter(function() titleLabel:SetColor(1.0, 1.0, 0.0, 1.0) end)
    titleLabel:OnHoverExit(function() titleLabel:SetColor(0.0, 0.0, 0.0, 1.0) end)
    titleLabel:OnClick(function()
        _G.DriftSalvageHudNextStartMode = "Title"
        RequestGameRestart()
    end)
end

local function StartGameplay()
    HideHud()
    ShowInGameHud()
    if ResetDriftSalvageStats then
        ResetDriftSalvageStats()
    end
    SetBoatAtHome()
    EnterGameplayMode()
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

    local startLabel = MakeHoverLabel(MenuPanel, 0.025, 0.03, 256, 28, "START", 48.0, "RelativePos")
    startLabel:OnClick(function()
        StartGameplay()
    end)

    UIManager.CreateImage(MenuPanel, -0.25, 0.4, 0.15, 0.25, "Asset/Texture/UI/Icon_book.png", "ParentRelative")

    local recordLabel = MakeHoverLabel(MenuPanel, 0.025, 0.18, 256, 28, "RECORD", 48.0, "RelativePos")
    recordLabel:OnClick(function()
        HideHud()
        ShowScoreboard(LoadScoreManager().GetRecords())
    end)

    local ok, err = pcall(PrepareMenuPresentation)
    if not ok and Log then
        Log("PrepareMenuPresentation failed: " .. tostring(err))
    end
end

function OnStart(self)
    local nextStartMode = _G.DriftSalvageHudNextStartMode
    _G.DriftSalvageHudNextStartMode = nil

    if nextStartMode == "Gameplay" then
        StartGameplay()
        return
    end

    ShowHud()
end

function OnUpdate(self, deltaTime)
    local hud = GameplayHud
    if not hud then
        return
    end

    if hud:IsVisible() and ConsumeDriftSalvageGameOverRequest and ConsumeDriftSalvageGameOverRequest() then
        ShowGameOver()
        return
    end

    if hud:IsVisible() and Input and Input.GetKeyDown("E") then
        ShowGameOver()
        return
    end

    hud:Update(deltaTime)

    if hud:IsVisible() and GetHudHealth() <= 0 and not hud:IsHeartAnimating() then
        ShowGameOver()
    end
end

function OnDestroy(self)
    EnterUIMode()
    HideHud()
    HideScoreboard()
    HideGameOver()
    HideInGameHud()
end
