-- Native loader only; all game/camera state belongs to JishsFirstPersonMod.dll.
local directory = mod and type(mod.directory) == "string" and mod.directory ~= ""
  and mod.directory or "scripts/Custom/"
local dll = directory:gsub("[/\\]+$", "") .. "/JishsFirstPersonMod.dll"
local function log(message, level)
  if mod and type(mod.log) == "function" then mod.log(message, level) else print(message) end
end
if not package or type(package.loadlib) ~= "function" then
  log("JishsFirstPersonMod: package.loadlib unavailable", "ERROR")
  return
end
local ok, linked, err = pcall(package.loadlib, dll, "*")
if not ok or not linked then
  local fn, message = package.loadlib(dll, "luaopen_jishsfirstpersonmod")
  if type(fn) ~= "function" then
    log("JishsFirstPersonMod: DLL load failed: " .. tostring(message or err or linked), "ERROR")
    return
  end
  local started, failure = pcall(fn)
  if not started then log("JishsFirstPersonMod: startup entry failed: " .. tostring(failure), "ERROR"); return end
end
-- Optional console/script controls; they queue work onto the native engine tick.
JishsFirstPersonMod = JishsFirstPersonMod or {}
local function entry(symbol)
  return function()
    local fn, message = package.loadlib(dll, symbol)
    if type(fn) ~= "function" then error(message or symbol .. " unavailable") end
    return fn()
  end
end
JishsFirstPersonMod.toggle = entry("luaopen_jishsfirstpersonmod_toggle")
JishsFirstPersonMod.reload = entry("luaopen_jishsfirstpersonmod_reload")
log("JishsFirstPersonMod DLL loaded; see %LOCALAPPDATA%/JishsFirstPersonMod/JishsFirstPersonMod.log for native initialisation/hook status")
return JishsFirstPersonMod

