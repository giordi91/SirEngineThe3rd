package.path = package.path .. ";../data/scripts/?.lua"

local scriptingCore = require("scriptingCore")

function runAnim()
	regTable = scriptingCore.getRegisterTable();
	for k,v in pairs(regTable["anim"]) do
	   v()
	end
end

runAnim();

