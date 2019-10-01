
local scriptingCore = {}

function scriptingCore.getRegisterTable()
	return scriptingCore["callbacks"];
end

function scriptingCore.logCallbacks()
	callbacks = scriptingCore["callbacks"];
	for k,v in pairs(callbacks["anim"]) do
		print (k,v)
	end
end

function init()
	--registering callbacks entry in the map
	scriptingCore["callbacks"] = {};
	scriptingCore["callbacks"]["anim"] = {};

end


init()

return scriptingCore