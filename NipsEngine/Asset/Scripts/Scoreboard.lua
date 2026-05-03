-- Scoreboard.lua
-- Shows a score using Asset/Texture/UI/Number.png.
-- Number.png layout is 5 columns x 2 rows:
--   Row 0: 0 1 2 3 4
--   Row 1: 5 6 7 8 9

local SHEET_COLS = 5
local SHEET_ROWS = 2
local SHEET_PATH = "Asset/Texture/UI/Number.png"
local SCORE_BG_PATH = "Asset/Texture/UI/ScoreBoardBG.png"


local ScorePanel = nil
local DigitImages = {}
local MAX_DIGITS = 6

local function DigitUV(digit)
    local d = math.max(0, math.min(9, math.floor(digit)))
    local col = d % SHEET_COLS
    local row = math.floor(d / SHEET_COLS)

    local minX = col / SHEET_COLS
    local minY = row / SHEET_ROWS
    local maxX = (col + 1) / SHEET_COLS
    local maxY = (row + 1) / SHEET_ROWS

    return minX, minY, maxX, maxY
end

local function SetScore(score)
    local n = math.max(0, math.floor(score or 0))

    for i = 1, MAX_DIGITS do
        local img = DigitImages[i]
        if img then
            local digit = n % 10
            img:SetUV(DigitUV(digit))

            if n == 0 and i > 1 then
                img:SetVisible(false)
            else
                img:SetVisible(true)
            end

            n = math.floor(n / 10)
        end
    end
end

function ShowScoreboard(dummyScore)
    if ScorePanel then return end

    ScorePanel = UIManager.CreateImage(nil, 0.5, 0.5, 0.5, 0.4, SCORE_BG_PATH, "FullRelative")
    ScorePanel:SetColor(1.0, 1.0, 1.0, 1.0)

    local title = UIManager.CreateText(ScorePanel, 0.5, 0.15, 300, 40, "RECORD", 36.0, "RelativePos")
    title:SetColor(1.0, 1.0, 0.0, 1.0)

    local digitW = 0.09
    local digitH = 0.25
    local startX = 0.5 + (MAX_DIGITS - 1) * 0.5 * digitW

    DigitImages = {}
    for i = 1, MAX_DIGITS do
        local x = startX - (i - 1) * digitW
        local img = UIManager.CreateImage(ScorePanel, x, 0.55, digitW, digitH, SHEET_PATH, "ParentRelative")
        img:SetUV(DigitUV(0))
        DigitImages[i] = img
    end

    local closeBtn = UIManager.CreateText(ScorePanel, 0.5, 0.88, 120, 28, "Close", 28.0, "RelativePos")
    closeBtn:SetColor(0.8, 0.8, 0.8, 1.0)
    closeBtn:SetInteractable(true)
    closeBtn:OnHoverEnter(function() closeBtn:SetColor(1.0, 1.0, 0.0, 1.0) end)
    closeBtn:OnHoverExit(function() closeBtn:SetColor(0.8, 0.8, 0.8, 1.0) end)
    closeBtn:OnClick(function()
        HideScoreboard()
    end)

    SetScore(dummyScore or 0)
end

function HideScoreboard()
    if ScorePanel then
        UIManager.DestroyElement(ScorePanel)
        ScorePanel = nil
        DigitImages = {}
    end
end
