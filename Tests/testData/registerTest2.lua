package.path = package.path .. ";../data/scripts/?.lua"

local scriptingCore = require("scriptingCore")

function helloFromSecond()
	print ("helloFromSecond")
end

function register()
	regTable = scriptingCore.getRegisterTable();
	table.insert(regTable["anim"], helloFromSecond);
end

register();
helloFromC();
scriptingCore.logCallbacks();

