package.path = package.path .. ";../data/scripts/?.lua"

local scriptingCore = require("scriptingCore")

function helloFromFirst()
	print ("helloFromFirst")
end

function register()
	regTable = scriptingCore.getRegisterTable();
	table.insert(regTable["anim"], helloFromFirst);
end

register();

scriptingCore.logCallbacks();

