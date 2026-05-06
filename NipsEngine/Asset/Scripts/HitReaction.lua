local function ResetVignette()
    Wait(0.18)
    Camera.SetVignette(0.0, 0.75)
end

function OnHit(self, other, hitInfo)
    HitFeel.HitStop(0.08)
    HitFeel.Slomo(0.35, 0.25)

    Camera.Shake(4.0, 20.0, 0.18)

    if Camera.FOVKick ~= nil then
        Camera.FOVKick(8.0, 0.2)
    end

    Camera.SetVignette(0.6, 0.75)
    StartCoroutine(ResetVignette)
end
