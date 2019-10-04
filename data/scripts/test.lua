package.path = package.path .. ";../data/scripts/?.lua"

local scriptingCore = require("scriptingCore")

function hello()
	print ("hello " .. tostring(20))
end

function register()
	regTable = scriptingCore.getRegisterTable();
	table.insert(regTable["anim"], hello);
end

register();
