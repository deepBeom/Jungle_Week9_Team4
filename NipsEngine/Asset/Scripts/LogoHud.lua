-- LogoHud.lua

local ShowHud

local NUMBER_COLS = 5
local NUMBER_ROWS = 2
local NUMBER_SHEET = "Asset/Texture/UI/Number.png"
local SCORE_BG_PATH = "Asset/Texture/UI/ScoreBoardBG.png"

local HEART_SHEET = "Asset/Texture/UI/Heart.png"
local HEART_COLS = 6
local HEART_COUNT = 5

local MenuPanel = nil
local ScorePanel = nil
local InGamePanel = nil
local HeartImages = {}
local bHeartsVisible = false

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

local function SetHeartsVisible(visible)
    bHeartsVisible = visible

    for _, heart in ipairs(HeartImages) do
        heart:SetVisible(visible)
    end
end

local function HideInGameHud()
    if InGamePanel then
        UIManager.DestroyElement(InGamePanel)
        InGamePanel = nil
        HeartImages = {}
        bHeartsVisible = false
    end
end

local function ShowInGameHud()
    if InGamePanel then return end

    InGamePanel = UIManager.CreateImage(nil, 20, 20, 220, 36, nil, nil)
    InGamePanel:SetColor(0.0, 0.0, 0.0, 0.0)

    HeartImages = {}
    local u1, v1, u2, v2 = HeartUV(0)

    for i = 1, HEART_COUNT do
        local x = (i - 1) * 34
        local heart = UIManager.CreateImage(InGamePanel, x, 0, 32, 32, HEART_SHEET, nil)
        heart:SetUV(u1, v1, u2, v2)
        heart:SetVisible(false)
        HeartImages[i] = heart
    end
end

local function HideScoreboard()
    if ScorePanel then
        UIManager.DestroyElement(ScorePanel)
        ScorePanel = nil
    end
end

local function ShowScoreboard(bestScore)
    if ScorePanel then return end

    ScorePanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.6, 0.5, SCORE_BG_PATH, "FullRelative")
    ScorePanel:SetColor(1.0, 1.0, 1.0, 1.0)

    local title = UIManager.CreateText(ScorePanel, -0.005, -0.195, 300, 50, "BEST SCORE", 32.0, "RelativePos")
    title:SetColor(1.0, 1.0, 0.0, 1.0)

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

    local closeBtn = UIManager.CreateText(ScorePanel, 0.0, 0.17, 120, 28, "Close", 28.0, "RelativePos")
    closeBtn:SetColor(0, 0, 0, 1.0)
    closeBtn:SetInteractable(true)
    closeBtn:OnHoverEnter(function() closeBtn:SetColor(1.0, 1.0, 0.0, 1.0) end)
    closeBtn:OnHoverExit(function() closeBtn:SetColor(0.0, 0.0, 0.0, 1.0) end)
    closeBtn:OnClick(function()
        HideScoreboard()
        ShowHud()
    end)
end

local function MakeHoverLabel(parent, x, y, w, h, text, fontSize, mode)
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

    MenuPanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.4, 0.5, nil, "FullRelative")

    UIManager.CreateImage(MenuPanel, 0.0, -0.3, 0.8, 0.3, "Asset/Texture/UI/Logo.png", "ParentRelative")
    UIManager.CreateImage(MenuPanel, -0.25, 0.1, 0.15, 0.25, "Asset/Texture/UI/Icon_boat.png", "ParentRelative")

    local StartLabel = MakeHoverLabel(MenuPanel, 0.025, 0.03, 256, 28, "Start", 48.0, "RelativePos")
    StartLabel:OnClick(function()
        HideHud()
        ShowInGameHud()
    end)

    UIManager.CreateImage(MenuPanel, -0.25, 0.4, 0.15, 0.25, "Asset/Texture/UI/Icon_book.png", "ParentRelative")

    local RecordLabel = MakeHoverLabel(MenuPanel, 0.025, 0.18, 256, 28, "Record", 48.0, "RelativePos")
    RecordLabel:OnClick(function()
        HideHud()
        ShowScoreboard(1557)
    end)
end

function OnStart(self)
    ShowHud()
end

function OnUpdate(self, deltaTime)
    if InGamePanel and Input and Input.GetKeyDown("H") then
        SetHeartsVisible(not bHeartsVisible)
    end
end

function OnDestroy(self)
    HideHud()
    HideScoreboard()
    HideInGameHud()
end
