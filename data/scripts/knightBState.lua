package.path = package.path .. ";../data/scripts/?.lua"

--local scriptingCore = require("scriptingCore")


-- transition functions
function idleToWalk()
	print("lets go walking!");
	return true;
end

function walkToIdle()
	print("lets go idle");
	return true;
end


local stateMachine = {

-- states
states = {
	idle = { animation= "knightBIdle"},
	walk = { animation= "knightBWalk"}
        },
-- transitions
transitions = {
				-- idle transitions
				idle = {{targetState="walk", delayInSeconds = 1, logic = idleToWalk}},
				-- walk transitions
				walk = {{targetState="idle", delayInSeconds = 1, logic = walkToIdle }},
			  },
}

function evaluate(currentState)
	-- lets get the current state
	state = stateMachine.states[currentState];
	
	-- lets get the transitions and start to iterate over them
	transitions = stateMachine.transitions[currentState];
	for state,transition in pairs(transitions) do
		result = transition.logic(); 
		print(result);
		if result == true then
			-- the transition was taken!! we need to perform the transition
			return transition.targetState
			--currentState = transition.targetState;
			-- call some c++ code for doing the transition
		end
	end
	return currentState 
end


--return evaluate(currentState);
