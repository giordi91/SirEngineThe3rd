package.path = package.path .. ";../data/scripts/?.lua"

local scriptingCore = require("scriptingCore")

function spinCameraFunc()
	rotateMainCameraY(0.00001);
end

function register()
	regTable = scriptingCore.getRegisterTable();
	table.insert(regTable["anim"], spinCameraFunc);
end

register();
