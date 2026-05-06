-- Tag one ACineCameraActor as "CineCamera_Lighthouse".

local bEndingStarted = false
local ActiveText = nil

local function ClearText()
    if ActiveText ~= nil then
        UIManager.DestroyElement(ActiveText)
        ActiveText = nil
    end
end

local function ShowText(text, duration, y)
    ClearText()

    ActiveText = UIManager.CreateText(nil, 0.26, y or 0.18, 1200, 60, text, 50, "RelativePos")
    ActiveText:SetColor(1.0, 1.0, 1.0, 1.0)

    if duration ~= nil and duration > 0.0 then
        Wait(duration)
        ClearText()
    end
end

local function IsPlayerActor(actor)
    if actor == nil or not actor.IsValid or not actor:IsValid() then
        return false
    end

    return actor:GetTag() == "Boat" or actor:GetTag() == "Player"
end

local function EndingSequence()
    if Input and Input.SetPlayerControlEnabled then
        Input.SetPlayerControlEnabled(false)
    end

    Camera.SetLetterBox(0.16, 1.0)
    Camera.SetViewTargetWithBlend("CineCamera_Lighthouse", 2.0)
    HitFeel.Slomo(0.4, 1.5)
    Wait(2.0)

    ShowText("Mission Complete", 2.0, 0.16)
    ShowText("Team 7 - Member A / Member B / Member C", 3.0, 0.22)

    Camera.FadeOut(2.0)
    Wait(2.0)

    ClearText()
end

function OnBeginOverlap(self, other)
    if bEndingStarted or not IsPlayerActor(other) then
        return
    end

    bEndingStarted = true
    StartCoroutine(EndingSequence)
end

function OnDestroy(self)
    ClearText()
end
